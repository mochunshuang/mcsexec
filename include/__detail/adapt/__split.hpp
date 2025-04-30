#pragma once

#include <atomic>
#include <cassert>
#include <type_traits>
#include <utility>
#include <optional>
#include <variant>

#include "./__as_tuple.hpp"

#include "../snd/__transform_sender.hpp"
#include "../snd/__make_sender.hpp"

#include "../snd/general/__get_domain_early.hpp"
#include "../snd/general/__impls_for.hpp"
#include "../snd/__completion_signatures_of_t.hpp"

#include "../queries/__env_of_t.hpp"
#include "../queries/__stop_token_of_t.hpp"

#include "../conn/__connect_result_t.hpp"

#include "../tfxcmplsigs/__unique_variadic_template.hpp"

#include "../pipeable/__sender_adaptor.hpp"

#include "../__stoptoken/__stop_callback_of_t.hpp"

#include "../tool/SimpleAtomicOperation.hpp"

#include "../diagnostics/__check_type.hpp"

namespace mcs::execution
{
    namespace adapt::__split
    {

        // Note: split的rcvr定义的 Token是 [inplace_stop_token]
        // Note: 模板+具体类型 => 生成确定的类型
        template <typename Token, typename CallbackFn>
        using stop_callback_of_t =
            typename stoptoken::stop_callback_of_t<Token, CallbackFn>;

        template <typename Sndr>
        struct shared_state;

        struct local_state_base
        {
            virtual void notify() noexcept = 0; // exposition only
            virtual ~local_state_base() = default;

            local_state_base() = default;
            local_state_base(local_state_base &&) = delete;
            local_state_base(const local_state_base &) = delete;
            local_state_base &operator=(local_state_base &&) = delete;
            local_state_base &operator=(const local_state_base &) = delete;

            std::atomic<local_state_base *> next{nullptr}; // NOLINT for intrusive_list
        };

        template <class Sndr, class Rcvr>
        struct local_state : local_state_base
        { // exposition only

            struct forward_stop_request
            {
                stoptoken::single_inplace_stop_source &stop_source; // NOLINT
                void operator()() noexcept
                {
                    stop_source.request_stop();
                }
            };
            using onstopcallback = // Note: Rcvr + get_stop_token_t => token_type
                stop_callback_of_t<queries::stop_token_of_t<queries::env_of_t<Rcvr>>,
                                   forward_stop_request>;

            void register_stop_callback( // exposition only // NOLINT
                queries::stop_token_of_t<queries::env_of_t<Rcvr>> st) noexcept
            {
                on_stop.emplace(std::move(st), forward_stop_request{sh_state->stop_src});
            }

            ~local_state() noexcept override
            {
                sh_state->dec_ref();
            }
            void notify() noexcept override
            {
                on_stop.reset();
                std::visit(
                    [this](const auto &tupl) noexcept -> void {
                        std::apply(
                            [this](auto tag, const auto &...args) noexcept -> void {
                                tag(std::move(*rcvr), args...);
                            },
                            tupl);
                    },
                    sh_state->result);
            }

            /**
             * @brief Construct a new local state object
             * Note: 原先签名 local-state(Sndr&& sndr, Rcvr& rcvr) noexcept
             * Note: 应该是是错误的。原先是 内部确定 sh_state类型，现在外部
             * Note: 原因是 shared_state<child_sndr> conflict shared_state<Sndr>
             * Note: child_sndr 得是 new __split::shared_state{}传递的 Sndr
             * @param sh_state: 指向堆内存
             * @param rcvr
             */
            local_state(shared_state<Sndr> *sh_state, Rcvr &rcvr) noexcept
                : on_stop{}, sh_state{sh_state}, rcvr{std::addressof(rcvr)}
            {
                this->sh_state->inc_ref();
            }

            local_state(local_state &&) = delete;
            local_state(const local_state &) = delete;
            local_state &operator=(local_state &&) = delete;
            local_state &operator=(const local_state &) = delete;

            std::optional<onstopcallback> on_stop; // exposition only // NOLINT

            __split::shared_state<Sndr> *sh_state; // exposition only // NOLINT
            Rcvr *rcvr;                            // exposition only // NOLINT
        };

        // split-receiver
        template <class Sndr>
        struct split_receiver
        {
            using receiver_concept = receiver_t;

            template <class Tag, class... Args>
            void complete(Tag /*unused*/, Args &&...args) noexcept
            {
                using tuple_t = decayed_tuple<Tag, Args...>;
                try
                {
                    sh_state->result.template emplace<tuple_t>(
                        Tag(), std::forward<Args>(args)...);
                }
                catch (...)
                {
                    using tuple_t = std::tuple<set_error_t, std::exception_ptr>;
                    sh_state->result.template emplace<tuple_t>(recv::set_error,
                                                               std::current_exception());
                }
                sh_state->notify(); // Note: shared_state<Sndr>
            }

            template <class... Args>
            void set_value(Args &&...args) && noexcept // NOLINT
            {
                this->complete(execution::set_value, std::forward<Args>(args)...);
            }

            template <class Error>
            void set_error(Error &&err) && noexcept // NOLINT
            {
                this->complete(execution::set_error, std::forward<Error>(err));
            }

            void set_stopped() && noexcept // NOLINT
            {
                this->complete(execution::set_stopped);
            }

            struct env
            {
                shared_state<Sndr> *sh_state; // exposition only // NOLINT

                [[nodiscard]] single_inplace_stop_token query(
                    queries::get_stop_token_t /*unused*/) const noexcept
                {
                    return sh_state->stop_src.get_token();
                }
            };

            env get_env() const noexcept // NOLINT
            {
                return env{sh_state};
            }

            // Note: INIT by fun: sndr::connect(sndr,recr{init})
            shared_state<Sndr> *sh_state; // exposition only // NOLINT
        };

        namespace __detail
        {
            template <typename Sigs>
            struct shared_state_variant_type;

            template <typename... Sig>
            struct shared_state_variant_type<cmplsigs::completion_signatures<Sig...>>
            {
                using type = typename tfxcmplsigs::unique_variadic_template<
                    std::variant<std::tuple<set_stopped_t>,
                                 std::tuple<set_error_t, std::exception_ptr>,
                                 typename adapt::__detail::as_tuple<Sig>::type...>>::type;
            };

            /**
             * @brief 通知集合： start 的时候 push_back
             *
             */
            struct state_list_type // NOLINT
            {
                std::atomic<local_state_base *> head{nullptr}; // NOLINT
                std::atomic_flag list_lock{};                  // NOLINT
                state_list_type() = default;

                // 定义 move 赋值操作符
                state_list_type(state_list_type &&other) noexcept = delete;
                // Note: 仅仅用于自身局部变量的赋值操作
                state_list_type &operator=(state_list_type &&other) noexcept
                {
                    if (this != &other)
                    {

                        // head = other.head;
                        // other.head = nullptr;

                        // 使用原子操作等价上面操作
                        // memory_order_relaxed	宽松操作：没有同步或定序约束。
                        // memory_order_acq_rel:
                        // 所有释放同一原子变量的线程的写入可见于修改之前，而且修改可见于其他获得同一原子变量的线程。
                        head.store(
                            other.head.exchange(nullptr, std::memory_order_acq_rel),
                            std::memory_order_relaxed);
                    }
                    return *this;
                }

                /**
                 * @brief
                 *
                 * @param new_node
                 * @return true
                 * @return If new_node is the first item added.
                 */
                bool push_front(local_state_base *new_node) noexcept // NOLINT
                {
                    assert(new_node != nullptr);

                    /**
                     * @brief
                        成功时（memory_order_release）：释放-获得定序 且 释放-消费定序

                        失败时（memory_order_relaxed）：宽松定序
                        不需要对 current_head 的值进行任何顺序保证，从而提高性能。
                     *
                     */
                    while (true)
                    {
                        // memory_order_acquire: 释放-获得定序
                        // 其他线程的所有释放同一原子变量的写入，能为当前线程所见
                        auto *current_head = head.load(std::memory_order_acquire);
                        new_node->next.store(current_head, std::memory_order_relaxed);

                        if (head.compare_exchange_weak(current_head, new_node,
                                                       std::memory_order_release,
                                                       std::memory_order_relaxed))
                        {
                            return (current_head == nullptr);
                        }
                    }
                }
                [[nodiscard]] auto empty() const
                {
                    // return head == nullptr;
                    return head.load(std::memory_order_acquire) == nullptr;
                }
            };
        }; // namespace __detail

        // Note: 手动围护引用计数; ref_count 默认为 1
        template <class Sndr>
        struct shared_state
        {
            /**
             * Then variant-type denotes the type variant<tuple<set_stopped_t>,
             * tuple<set_error_t, exception_ptr>, as-tuple<Sigs>...>, but with duplicate
             * types removed.
             */
            using variant_type = typename __detail::shared_state_variant_type<
                typename snd::completion_signatures_of_t<Sndr>>::type; // exposition
                                                                       // only

            using state_list_type = __detail::state_list_type; // exposition only

            explicit shared_state(Sndr &&sndr)
                : op_state(conn::connect(std::forward<Sndr>(sndr), split_receiver{this}))
            {
                // Postcondition: waiting_states is empty, and completed is false
                assert(waiting_states.empty());
                assert(not completed.load(std::memory_order_acquire));
            }

            // Effects: Calls inc-ref(). If stop_src.stop_requested() is true, calls
            // notify(); otherwise, calls start(op_state).
            void start_op() noexcept // exposition only // NOLINT
            {
                this->inc_ref();
                if (stop_src.stop_requested())
                    this->notify();
                else
                    opstate::start(op_state);
            }
            /**
             * @brief Effects: Atomically does the following:
             *  Sets completed to true, and
             *  Exchanges waiting_states with an empty list, storing the old value in a
             *  local prior_states.
             *
             *  then: for each pointer p in prior_states, calls p->notify(). Finally,
             *  calls dec-ref()
             */
            void notify() noexcept // exposition only // NOLINT
            {
                // Atomically does the following:
                // 1. Sets completed to true,
                // 2. Exchanges waiting_states with an empty list,
                //      storing the old value in a local prior_states.
                // Note: repeatedly std::move(waiting_states) no effect
                // Note: std::move only change waiting_states.head
                // Note: waiting_states.list_lock is live after std::move
                tool::SimpleAtomicOperation atomicOpration{waiting_states.list_lock};
                state_list_type prior_states;
                atomicOpration([&] { completed.store(true, std::memory_order_release); },
                               [&] { prior_states = std::move(waiting_states); });

                // 3. Then, for each pointer p in prior_states, calls p->notify().
                //    Finally, calls dec-ref().
                /**
                 * @brief memory_order_relaxed
                        不保证操作的顺序性，可能导致线程读取到不一致的数据。
                        例如，一个线程可能读取到 head 的新值，但 head->next 仍然是旧值。
                        memory_order_acquire: 释放-获得定序
                 *
                 */
                auto *head = prior_states.head.load(std::memory_order_acquire);
                while (head != nullptr)
                {
                    head->notify();
                    head = head->next.load(std::memory_order_acquire);
                }
                dec_ref();
            }
            // Effects: Increments ref_count
            void inc_ref() noexcept // exposition only // NOLINT
            {
                // ref_count++; 需要读旧值
                ++ref_count;
            }
            /**
             * @brief
                Effects: Decrements ref_count. If the new value of ref_count is 0,
                         calls delete this.

                Synchronization: If dec-ref() does not decrement the ref_count to 0 then
                synchronizes with the call to dec-ref() that decrements ref_count to 0
             *
             */
            void dec_ref() noexcept // exposition only // NOLINT
            {
                // Note: fetch_sub 返回的是减少之前的值
                //  使用 memory_order_release
                //  确保在减少引用计数之前的所有操作对其他线程可见
                if (ref_count.fetch_sub(1, std::memory_order_release) == 1)
                {
                    // 使用 memory_order_acquire
                    // 确保在销毁对象之前，所有其他线程的操作已完成
                    // Note: 防止指令重排序，保证操作的顺序性
                    std::atomic_thread_fence(std::memory_order_acquire);
                    delete this;
                }
            }
            stoptoken::single_inplace_stop_source stop_src{};            // NOLINT
            variant_type result{};                                       // NOLINT
            state_list_type waiting_states{};                            // NOLINT
            std::atomic<bool> completed{false};                          // NOLINT
            std::atomic<std::size_t> ref_count{1};                       // NOLINT
            conn::connect_result_t<Sndr, split_receiver<Sndr>> op_state; // NOLINT
        };

        template <typename T>
        shared_state(T &&) -> shared_state<::std::decay_t<T>>;

    }; // namespace adapt::__split

    namespace adapt
    {
        struct split_impl_tag
        {
        };

        struct split_env
        {
            [[nodiscard]] constexpr auto query( // NOLINT
                queries::get_stop_token_t const & /*unused*/) const noexcept
            {
                // https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p3409r1.html#orgb5a09bc
                return stoptoken::single_inplace_stop_token{};
            }
        };

        namespace __split
        {
            /**
             * @brief   管理共享状态对象的引用计数。
                        通过 sh_state 指针访问共享的 堆对象。
             *
             * @tparam State
             * @tparam Tag 算法命名空间
             */
            template <class State, class Tag>
            struct shared_wrapper
            {

                State *sh_state; // NOLINT
                Tag tag;         // NOLINT

                shared_wrapper(State *state, Tag tag) noexcept : sh_state(state), tag(tag)
                {
                    // Note: assert是宏，仅再调试时有效
                    assert(sh_state != nullptr);
                    assert(sh_state->ref_count.load() == 1);
                }

                // 复制时，调用 sh_state->inc-ref() 增加引用计数
                shared_wrapper(const shared_wrapper &other) noexcept
                    : sh_state(other.sh_state), tag(other.tag)
                {
                    if (sh_state)
                        sh_state->inc_ref();
                }

                shared_wrapper(shared_wrapper &&other) noexcept
                    : sh_state(other.sh_state), tag(other.tag)
                {
                    other.sh_state = nullptr;
                }

                // Note: change ref_count by shared_wrapper(...)
                // Note: operator=: copy-and-swap
                shared_wrapper &operator=(const shared_wrapper &other) noexcept
                {
                    shared_wrapper temp(other);
                    // 成员就是 一个指针，一个是空对象。一定是没问题的
                    std::swap(*this, temp);
                    // Note: 复制操作都 inc_ref; std::shared_ptr也是这样的
                    if (sh_state)
                        sh_state->inc_ref();

                    return *this;
                }
                // Note: operator=: move-and-swap
                shared_wrapper &operator=(shared_wrapper &&other) noexcept
                {
                    if (this != &other)
                    {
                        shared_wrapper temp(std::move(other));
                        std::swap(*this, temp);
                    }
                    return *this;
                }

                // The destructor has no effect if sh_state is null; otherwise, it
                // decrements the reference count by calling sh_state->dec-ref().
                ~shared_wrapper() noexcept
                {
                    if (sh_state != nullptr)
                    {
                        sh_state->dec_ref();
                    }
                }
            };
        }; // namespace __split

        struct split_t
        {
            template <snd::sender Sndr>
                requires(diagnostics::check_type<snd::__detail::basic_sender<
                             adapt::split_t, snd::empty_data, std::decay_t<Sndr>>>)
            auto operator()(Sndr &&sndr) const noexcept
            {
                auto dom = snd::general::get_domain_early(std::as_const(sndr));
                return snd::transform_sender(
                    dom, snd::make_sender(*this, {}, std::forward<Sndr>(sndr)));
            }

            auto operator()() const noexcept -> pipeable::sender_adaptor<split_t>
            {
                return {*this};
            }

            template <snd::sender Sndr>
            auto transform_sender(Sndr &&sndr) noexcept // NOLINT
                requires(snd::sender_for<decltype((sndr)), split_t>)
            {
                // Note: tag_change: split_t => split_impl_tag
                // Note: data_change: {} => shared_wrapper(shared_state{child},old_tag)
                auto &&[old_tag, _, child] = sndr;
                auto *sh_state =
                    new __split::shared_state{std::forward_like<decltype(sndr)>(child)};
                return snd::make_sender(split_impl_tag(),
                                        __split::shared_wrapper{sh_state, old_tag});
            }
        };
        inline constexpr split_t split{}; // NOLINT
    }; // namespace adapt

    template <>
    struct snd::general::impls_for<adapt::split_impl_tag> : snd::__detail::default_impls
    {
        // Note: 框架规定 get_state 在 basic_state中生成state时被调用
        // Note: 框架规定 state 被 start算法使用。下面也自定义了
        static constexpr auto get_state = // NOLINT
            []<class Sndr>(Sndr &&sndr, auto &rcvr) noexcept {
                // Note: get_state 默认实现 返回的是 tag,data,child...中的data
                // Note: 应该取出 shared_wrapper才对。因为 local_state 要表明了
                // Note: local_state 和  sh_state 模板参数类型都指向同一个  Sndr
                // Note: 而 shared_state 即 wrapper.sh_state 指向的是 split的 Sndr
                // Note: 因此返回值 local_state 也就是指向 split的 Sndr。堆内存
                // 构造函数 shared_wrapper.sh_state 映射 shared_state<Sndr>
                // 一定会解析出 split 绑定的 Sndr[也就是要split的sender]
                auto &&[tag, shared_wrapper] = sndr;
                return adapt::__split::local_state{
                    std::forward_like<Sndr>(shared_wrapper.sh_state), rcvr};
            };

        // Note: initialized with a callable object that has a function call operator
        static constexpr auto start = // NOLINT
            []<class Sndr, class Rcvr>(adapt::__split::local_state<Sndr, Rcvr> &state,
                                       Rcvr &rcvr) noexcept {
                /**
                 * @brief Effects: If state.sh_state->completed is true, calls
                 *  state.notify() and returns. Otherwise, see bellow
                 *
                 */
                if (state.sh_state->completed.load(std::memory_order_acquire))
                {
                    state.notify();
                    return;
                }

                /**
                 * @brief Otherwise, does the following in order:
                 *  1、Calls:
                 *  2、Then: 1,2
                 *  3、If c is true, calls state.notify() and returns.
                 *  4、Otherwise ...
                 */

                // 1、Calls: here
                state.register_stop_callback(
                    queries::get_stop_token(queries::get_env(rcvr)));

                // 2、Then: here
                // Then atomically does the following:
                // atomically does the following
                // 1. Reads the value c of state.sh_state->completed, and
                // 2. Inserts addressof(state) into waiting_states if c is false.
                bool c{false};
                bool first_item{false};
                tool::SimpleAtomicOperation atomicOpration{
                    state.sh_state->waiting_states.list_lock};
                atomicOpration([&] { c = state.sh_state->completed.load(); },
                               [&] {
                                   if (not c)
                                   {
                                       first_item =
                                           state.sh_state->waiting_states.push_front(
                                               std::addressof(state));
                                   };
                               });

                // 3、If c is true, calls state.notify() and returns.
                if (c)
                {
                    state.notify();
                    return;
                }

                // 4、if addressof(state) is the first item added
                if (first_item)
                {
                    state.sh_state->start_op();
                }
            };
    };

    template <typename Sndr, typename... Env>
    struct cmplsigs::completion_signatures_for_impl<
        snd::__detail::basic_sender<adapt::split_t, snd::empty_data, Sndr>, Env...>
    {
        using type = snd::completion_signatures_of_t<Sndr, Env...>;
    };

    template <typename Sndr, typename... Env>
    struct cmplsigs::completion_signatures_for_impl<
        snd::__detail::basic_sender<
            adapt::split_impl_tag,
            adapt::__split::shared_wrapper<adapt::__split::shared_state<Sndr>,
                                           adapt::split_t>>,
        Env...>
    {
        using type = snd::completion_signatures_of_t<Sndr, Env...>;
    };

    namespace diagnostics
    {
        template <typename Sndr> // NOLINTNEXTLINE
        inline constexpr bool check_type_impl<
            snd::__detail::basic_sender<adapt::split_t, snd::empty_data, Sndr>> =
            []() consteval {
                static_cast<void>(snd::get_completion_signatures<
                                  Sndr, decltype(snd::general::FWD_ENV(
                                            std::declval<adapt::split_env>()))>());
                return true;
            }();
    }; // namespace diagnostics

}; // namespace mcs::execution
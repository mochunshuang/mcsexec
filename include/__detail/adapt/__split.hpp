#pragma once

#include <atomic>
#include <cassert>
#include <mutex>
#include <type_traits>
#include <utility>

#include "./__as_tuple.hpp"

#include "../snd/__transform_sender.hpp"
#include "../snd/__make_sender.hpp"

#include "../snd/general/__get_domain_early.hpp"
#include "../snd/general/__impls_for.hpp"
#include "../snd/general/__on_stop_request.hpp"
#include "../snd/__completion_signatures_of_t.hpp"
#include "../snd/__detail/mate_type/__child_type.hpp"

#include "../snd/__sender_in.hpp"

#include "../queries/__env_of_t.hpp"
#include "../queries/__stop_token_of_t.hpp"

#include "../conn/__connect_result_t.hpp"

#include "../tfxcmplsigs/__unique_variadic_template.hpp"

#include "../pipeable/__sender_adaptor.hpp"

namespace mcs::execution
{
    namespace adapt::__split
    {

        // Note: split的rcvr定义的 Token是 [inplace_stop_token]
        // Note: 模板+具体类型 => 生成确定的类型
        template <typename Token, typename CallbackFn>
        using stop_callback_of_t = typename Token::template callback_type<CallbackFn>;

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

            local_state_base *next{}; // NOLINT for intrusive_slist
        };

        template <class Sndr, class Rcvr>
        struct local_state : local_state_base
        {                          // exposition only
            using onstopcallback = // Note: Rcvr + get_stop_token_t => token_type
                stop_callback_of_t<queries::stop_token_of_t<queries::env_of_t<Rcvr>>,
                                   snd::general::on_stop_request>;

            ~local_state() noexcept override;
            void notify() noexcept override;

            local_state(Sndr &&sndr, Rcvr &rcvr) noexcept;

            local_state(local_state &&) = delete;
            local_state(const local_state &) = delete;
            local_state &operator=(local_state &&) = delete;
            local_state &operator=(const local_state &) = delete;

            std::optional<onstopcallback> on_stop; // exposition only // NOLINT

            // using Childen = snd::__detail::mate_type::child_type<Sndr>;
            shared_state<Sndr> *sh_state; // exposition only // NOLINT
            Rcvr *rcvr;                   // exposition only // NOLINT
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

                [[nodiscard]] inplace_stop_token query(
                    queries::get_stop_token_t /*unused*/) const noexcept
                {
                    // Note: stop_src == stoptoken::inplace_stop_source
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
                local_state_base *head{nullptr}; // NOLINT
                local_state_base *tail{nullptr}; // NOLINT
                std::mutex mtx;                  // NOLINT

                explicit state_list_type(local_state_base *init) : head(init), tail(init)
                {
                }
                // NOLINTNEXTLINE
                explicit state_list_type(local_state_base *head, local_state_base *tail)
                    : head(head), tail(tail)
                {
                }

                void push_back(local_state_base *item) noexcept // NOLINT
                {
                    assert(item != nullptr);
                    // std::lock_guard<std::mutex> lock(mtx);  //Note: external assurances
                    if (tail != nullptr)
                    {
                        tail->next = item;
                        tail = item;
                    }
                    else
                    {
                        head = tail = item;
                    }
                    tail->next = nullptr;
                }

                [[nodiscard]] bool empty()
                {
                    std::lock_guard<std::mutex> lock(mtx);
                    return head == nullptr;
                }

                [[nodiscard]] auto move() // NOLINT
                {
                    // std::lock_guard<std::mutex> lock(mtx);  //Note: external assurances
                    auto *old_head = head;
                    auto *old_tail = tail;
                    head = nullptr;
                    tail = nullptr;
                    return state_list_type{old_head, old_tail};
                }
            };
        }; // namespace __detail

        // Note: 手动围护引用计数
        template <class Sndr>
        struct shared_state
        {

            using variant_type = typename __detail::shared_state_variant_type<
                typename snd::completion_signatures_of_t<Sndr>>::type; // exposition
                                                                       // only

            using state_list_type = __detail::state_list_type; // exposition only

            explicit shared_state(Sndr &&sndr);

            void start_op() noexcept; // exposition only // NOLINT
            void notify() noexcept;   // exposition only // NOLINT
            void inc_ref() noexcept;  // exposition only // NOLINT
            void dec_ref() noexcept;  // exposition only // NOLINT

            stoptoken::inplace_stop_source stop_src{};                   // NOLINT
            variant_type result{};                                       // NOLINT
            state_list_type waiting_states{nullptr};                     // NOLINT
            std::atomic<bool> completed{false};                          // NOLINT
            std::atomic<std::size_t> ref_count{1};                       // NOLINT
            conn::connect_result_t<Sndr, split_receiver<Sndr>> op_state; // NOLINT
        };

        template <typename T>
        shared_state(T &&) -> shared_state<::std::decay_t<T>>;

    }; // namespace adapt::__split

    //////////////////////// impl
    namespace adapt
    {
        /////////////////////////////////////////////////////////////////////////
        //// localstate
        template <class Sndr, class Rcvr>
        __split::local_state<Sndr, Rcvr>::local_state(Sndr &&sndr, Rcvr &rcvr) noexcept
        {
            // TODO(mcs): Sndr是同一个吗？ 明显是同一个啊。 这是构造函数
            // Note:  Sndr 是完整 split_impl_tag的 sndr，data 不是sndr
            // 自己怎么。 类型错误。 现在是两个类型不匹配
            auto &[_, data] = sndr;
            this->sh_state = data.sh_state; // TODO(mcs): 肯定不匹配。重新看设计
            this->sh_state->inc_ref();
            this->rcvr = std::addressof(rcvr);
        };

        template <class Sndr, class Rcvr>
        __split::local_state<Sndr, Rcvr>::~local_state() noexcept
        {
            sh_state->dec_ref();
        };

        template <class Sndr, class Rcvr>
        void __split::local_state<Sndr, Rcvr>::notify() noexcept
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
        };

        /////////////////////////////////////////////////////////////////////////
        //// shared_state
        template <class Sndr> // Note: only need init op_state, other is ready
        __split::shared_state<Sndr>::shared_state(Sndr &&sndr) // NOLINT
            : op_state(conn::connect(std::forward<Sndr>(sndr), split_receiver{this}))
        {
            // Postcondition:
            assert(waiting_states.empty());
            assert(not completed.load(std::memory_order_acquire));
        }

        template <class Sndr>
        void __split::shared_state<Sndr>::start_op() noexcept
        {
            this->inc_ref();
            if (stop_src.stop_requested())
                this->notify();
            else
                opstate::start(op_state);
        }

        template <class Sndr>
        void __split::shared_state<Sndr>::notify() noexcept
        {
            // Atomically does the following:
            // 1. Sets completed to true,
            // 2. Exchanges waiting_states with an empty list,
            //      storing the old value in a local prior_states.
            std::unique_lock<std::mutex> lock(waiting_states.mtx);
            completed.store(true, std::memory_order_release);
            state_list_type prior_states = waiting_states.move();
            lock.unlock();

            // 3. Then, for each pointer p in prior_states, calls p->notify().
            //    Finally, calls dec-ref().
            auto *p = prior_states.head;
            while (p != nullptr)
            {
                p->notify();

                auto *old = std::exchange(p, p->next);
                old->next = nullptr;
            }
            dec_ref();
        }

        template <class Sndr>
        void __split::shared_state<Sndr>::inc_ref() noexcept
        {
            ref_count++;
        }

        template <class Sndr>
        void __split::shared_state<Sndr>::dec_ref() noexcept
        {
            // memory_order_release: write sync
            // memory_order_acquire: read sync
            if (ref_count.fetch_sub(1, std::memory_order_release) == 1)
            {
                std::atomic_thread_fence(std::memory_order_acquire);
                delete this;
            }
        }

    }; // namespace adapt

    namespace adapt
    {
        struct split_impl_tag
        {
        };

        struct split_env
        {
            [[nodiscard]] constexpr auto query( // NOLINT
                queries::get_stop_token_t const & /*unused*/) const
            {
                return stoptoken::inplace_stop_token{};
            }
        };

        namespace __split
        {
            // manages the reference count of the shared-state pointer with sh_state
            template <class State, class Tag>
            struct shared_wrapper
            {

                State *sh_state; // NOLINT
                Tag tag;         // NOLINT

                shared_wrapper() noexcept : sh_state(nullptr) {}

                shared_wrapper(State *state, Tag tag) noexcept : sh_state(state), tag(tag)
                {
                    if (sh_state)
                    {
                        sh_state->inc_ref();
                    }
                }

                shared_wrapper(const shared_wrapper &other) noexcept
                    : sh_state(other.sh_state), tag(other.tag)
                {
                    if (sh_state)
                    {
                        sh_state->inc_ref(); // +1
                    }
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
                    this->swap(*this, temp);
                    return *this;
                }
                // Note: operator=: move-and-swap
                shared_wrapper &operator=(shared_wrapper &&other) noexcept
                {
                    if (this != &other)
                    {
                        shared_wrapper temp(std::move(other));
                        this->swap(*this, temp);
                    }
                    return *this;
                }

                ~shared_wrapper() noexcept
                {
                    // Note: destructor has no effect if sh_state is null
                    if (sh_state != nullptr)
                    {
                        sh_state->dec_ref();
                    }
                }

                friend void swap(shared_wrapper &l, shared_wrapper &r) noexcept
                {
                    std::swap(l.sh_state, r.sh_state);
                    std::swap(l.tag, r.tag);
                }
            };
        }; // namespace __split

        struct split_t
        {
            template <snd::sender Sndr>
                requires(snd::sender_in<Sndr, split_env>)
            auto operator()(Sndr &&sndr) const // noexcept
            {
                auto dom = snd::general::get_domain_early(std::as_const(sndr));
                return snd::transform_sender(
                    dom, snd::make_sender(*this, {}, std::forward<Sndr>(sndr)));
            }

            template <snd::sender Sndr>
            auto transform_sender(Sndr &&sndr) noexcept // NOLINT
                requires(snd::sender_for<decltype((sndr)), split_t>)
            {
                // TODO(mcs):
                // Note: tag_change: split_t => split_impl_tag
                // Note: dara_change: {} => shared_wrapper(shared_state{child},old_tag)
                // Note: sh_state: shared_state<child_sndr> conflict shared_state<Sndr>
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
        // Note: create [basic_state->state] when connect. 不是 inner_ops。Sndr外部的
        static constexpr auto get_state = // NOLINT
            []<class Sndr>(Sndr &&sndr, auto &rcvr) noexcept {
                // auto &&[tag, _, child] = sndr; // TODO(mcs):
                // Note: call: local_state(sndr,rcvr)
                return adapt::__split::local_state{std::forward_like<Sndr>(sndr), rcvr};
            };

        // Note: initialized with a callable object that has a function call operator
        static constexpr auto start = // NOLINT
            []<class Sndr, class Rcvr>(adapt::__split::local_state<Sndr, Rcvr> &state,
                                       Rcvr &rcvr) noexcept {
                if (state.sh_state->completed.load(std::memory_order_acquire))
                {
                    state.notify();
                    return;
                }

                state.on_stop.emplace(
                    queries::get_stop_token(queries::get_env(rcvr)),
                    snd::general::on_stop_request{state.sh_state->stop_src});

                // atomically does the following
                // 1. Reads the value c of state.sh_state->completed, and
                // 2. Inserts addressof(state) into waiting_states if c is false.
                std::unique_lock<std::mutex> lock(state.sh_state->waiting_states.mtx);
                bool c = state.sh_state->completed.load();
                if (not c)
                    state.sh_state->waiting_states.push_back(std::addressof(state));
                lock.unlock();

                if (c)
                {
                    state.notify(); // lock? need?
                    return;
                }

                // TODO(mcs): 可能有争议
                //  check is state first add
                if (state.sh_state->waiting_states.head != nullptr &&
                    state.sh_state->waiting_states.head == std::addressof(state))
                {
                    state.sh_state->start_op();
                }
            };
    };

    // template <typename Sndr, typename Env>
    // struct cmplsigs::completion_signatures_for_impl<
    //     snd::__detail::basic_sender<adapt::split_t, Sndr>, Env>
    // {
    //     using type = snd::completion_signatures_of_t<Sndr, Env>;
    // };

    template <typename Sndr, typename Env>
    struct cmplsigs::completion_signatures_for_impl<
        snd::__detail::basic_sender<
            adapt::split_impl_tag,
            adapt::__split::shared_wrapper<adapt::__split::shared_state<Sndr>,
                                           adapt::split_t>>,
        Env>
    {
        using type = snd::completion_signatures_of_t<Sndr, Env>;
    };

    namespace test
    {

        // Note:
        //  transform_sender => {split_impl_tag,shared_wrapper{shared_state{sndr},tag}}
        //    shared_state
        // using T = mcs::execution::adapt::__split::shared_state<
        //     mcs::execution::snd::__detail::basic_sender<
        //         mcs::execution::adapt::split_impl_tag,
        //         mcs::execution::adapt::__split::shared_wrapper<
        //             mcs::execution::adapt::__split::shared_state<
        //                 mcs::execution::snd::__detail::basic_sender<
        //                     mcs::execution::factories::__just_t<
        //                         mcs::execution::recv::set_value_t>,
        //                     mcs::execution::snd::__detail::product_type<int>>>,
        //             mcs::execution::adapt::split_t>>>;

        // using Sndr = mcs::execution::adapt::__split::shared_state<
        //     mcs::execution::snd::__detail::basic_sender<
        //         mcs::execution::factories::__just_t<mcs::execution::recv::set_value_t>,
        //         mcs::execution::snd::__detail::product_type<int>>>;
    } // namespace test

}; // namespace mcs::execution
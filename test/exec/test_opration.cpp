#include <iostream>
#include <stdexcept>
#include <type_traits>
#include "../test_base_head.hpp"

namespace test
{
    struct my_default_impls // exposition only
    {
        static constexpr auto get_attrs = // NOLINT
            [](const auto &, const auto &...child) noexcept -> decltype(auto) {
            if constexpr (sizeof...(child) == 1)
                return (::mcs::execution::snd::general::FWD_ENV(
                            ::mcs::execution::queries::get_env(child)),
                        ...);
            else
                return ::mcs::execution::empty_env();
        };

        static constexpr auto get_env = // NOLINT
            [](auto, auto &, const auto &rcvr) noexcept -> decltype(auto) {
            return ::mcs::execution::snd::general::FWD_ENV(
                ::mcs::execution::queries::get_env(rcvr));
        };

        static constexpr auto get_state = // NOLINT
            []<class Sndr, class Rcvr>(Sndr &&sndr,
                                       Rcvr & /*rcvr*/) noexcept -> decltype(auto) {
            return sndr.apply([](auto &, auto &data, auto &&...) noexcept(noexcept(
                                  std::forward_like<Sndr>(data))) -> decltype(auto) {
                return std::forward_like<Sndr>(data);
            });
        };

        // NOLINTNEXTLINE
        static constexpr auto start = [](auto &, auto &, auto &...ops) noexcept -> void {
            (::mcs::execution::opstate::start(ops), ...);
        };

        static constexpr auto complete = // NOLINT
            []<class Index, class Rcvr, class Tag, class... Args>(
                Index, auto & /*state*/, Rcvr &rcvr, Tag, Args &&...args) noexcept -> void
            requires ::mcs::execution::functional::callable<Tag, Rcvr, Args...>
        {
            // Mandates: Index::value == 0,the index is  tag
            static_assert(Index::value == 0,
                          "I don't know how to complete this operation.");
            Tag()(std::move(rcvr), std::forward<Args>(args)...);
        };
    };

    template <class Tag>
    struct my_impls_for : my_default_impls
    {
    };

    template <class Sndr, class Rcvr>
    struct basic_state // exposition only
    {
        basic_state(Sndr &&sndr,
                    Rcvr &&rcvr) noexcept(std::is_nothrow_move_constructible_v<Rcvr>)
            : rcvr(std::move(rcvr)),
              state(my_impls_for<::mcs::execution::tag_of_t<Sndr>>::get_state(
                  std::forward<Sndr>(sndr), this->rcvr))
        {
        }

        Rcvr rcvr; // exposition only // NOLINT
        ::mcs::execution::snd::__detail::mate_type::state_type<Sndr, Rcvr>
            state; // exposition only // NOLINT
    };

    template <class Sndr, class Rcvr>
    struct basic_operation : basic_state<Sndr, Rcvr> // exposition only
    {
        using operation_state_concept = ::mcs::execution::operation_state_t;
        using tag_t = ::mcs::execution::tag_of_t<Sndr>; // exposition only

        ::mcs::execution::snd::__detail::connect_all_result<Sndr, Rcvr>
            inner_ops; // exposition only // NOLINT

        basic_operation(Sndr &&sndr, Rcvr &&rcvr) noexcept // exposition only
            : basic_state<Sndr, Rcvr>(std::forward<Sndr>(sndr), std::move(rcvr)),
              inner_ops{::mcs::execution::snd::__detail::connect_all(
                  this, std::move(sndr),
                  ::mcs::execution::snd::__detail::mate_type::indices_for<Sndr>())}
        {
        }

        void start() & noexcept
        {
            inner_ops.apply(
                [&]<typename... Op>(Op &...ops) noexcept(noexcept(
                    my_impls_for<tag_t>::start(this->state, this->rcvr, ops...))) {
                    my_impls_for<tag_t>::start(this->state, this->rcvr, ops...);
                });
        }

        basic_operation(const basic_operation &) = delete;
        basic_operation(basic_operation &&) = delete;
        basic_operation &operator=(const basic_operation &) = delete;
        basic_operation &operator=(basic_operation &&) = delete;
        ~basic_operation() = default;
    };

    template <class Sndr, class Rcvr, class Index>
    struct basic_receiver // exposition only
    {
        using receiver_concept = ::mcs::execution::receiver_t;

        using tag_t = ::mcs::execution::tag_of_t<Sndr>; // exposition only
        using state_t =
            ::mcs::execution::snd::__detail::mate_type::state_type<Sndr,
                                                                   Rcvr>; // exposition
                                                                          // only

        // NOLINTNEXTLINE
        static constexpr const auto &complete = my_impls_for<tag_t>::complete;
        basic_state<Sndr, Rcvr> *parent_op; // exposition only // NOLINT

        template <class... Args>
            requires ::mcs::execution::functional::callable<decltype(complete), Index,
                                                            state_t &, Rcvr &,
                                                            ::mcs::execution::set_value_t,
                                                            Args...>
        void set_value(Args &&...args) && noexcept // NOLINT
        {
            complete(Index(), parent_op->state, parent_op->rcvr,
                     ::mcs::execution::set_value_t(), std::forward<Args>(args)...);
        }

        template <class Error>
            requires ::mcs::execution::functional::callable<decltype(complete), Index,
                                                            state_t &, Rcvr &,
                                                            ::mcs::execution::set_error_t,
                                                            Error>
        void set_error(Error &&err) && noexcept // NOLINT
        {
            complete(Index(), parent_op->state, parent_op->rcvr,
                     ::mcs::execution::set_error_t(), std::forward<Error>(err));
        }

        void set_stopped() && noexcept // NOLINT
            requires ::mcs::execution::functional::callable<
                decltype(complete), Index, state_t &, Rcvr &,
                ::mcs::execution::set_stopped_t>
        {
            complete(Index(), parent_op->state, parent_op->rcvr,
                     ::mcs::execution::set_stopped_t());
        }

        auto get_env() const noexcept // NOLINT
            -> ::mcs::execution::snd::__detail::mate_type::env_type<Index, Sndr,
                                                                    Rcvr> // NOLINT
        {
            return my_impls_for<tag_t>::get_env(Index(), parent_op->state,
                                                parent_op->rcvr);
        }
    };

    template <class Tag, class Data, class... Child>
    struct basic_sender
        : ::mcs::execution::snd::__detail::product_type<Tag, Data,
                                                        Child...> // exposition only
    {

        using sender_concept = ::mcs::execution::sender_t;
        using indices_for = std::index_sequence_for<Child...>; // exposition only

        decltype(auto) get_env() const noexcept // NOLINT
        {
            return this->apply(
                [](auto & /*tag*/, auto &data, auto &...child) -> decltype(auto) {
                    return my_impls_for<Tag>::get_attrs(data, child...);
                });
        }

        template <::mcs::execution::decays_to<basic_sender> Self,
                  ::mcs::execution::receiver Rcvr>
        auto connect(this Self &&self, Rcvr rcvr) noexcept -> basic_operation<Self, Rcvr>
        {
            return {std::forward<Self>(self), std::move(rcvr)};
        }

        template <::mcs::execution::decays_to<basic_sender> Self, class Env>
        auto get_completion_signatures(this Self && /*self*/, // NOLINT
                                       Env && /*env*/) noexcept
            -> ::mcs::execution::cmplsigs::completion_signatures_for<
                std::remove_cvref_t<Self>, Env>
        {
            return {};
        }
    };

    //================================

    template <typename T>
    struct my_tag;

    template <typename T>
    struct my_sender
    {
        int value{0}; // NOLINT

        using __tag_t = my_tag<T>;

        // No need get_completion_signatures funcation
        using completion_signatures = ex::completion_signatures<ex::set_value_t(T)>;

        // NOLINTNEXTLINE
        static constexpr auto process_previous_values = [](auto &&pre_value) -> T {
            if constexpr (std::is_same_v<std::remove_cvref_t<decltype(pre_value)>, T>)
                throw std::logic_error("");
            else
            {
                return pre_value + 1;
            }
        };
        using state_t = std::remove_cvref_t<decltype(process_previous_values)>;

        decltype(auto) get_env() const noexcept // NOLINT
        {
            return ::mcs::execution::empty_env{};
        }

        template <::mcs::execution::decays_to<my_sender> Self,
                  ::mcs::execution::receiver Rcvr>
        auto connect(this Self &&self, Rcvr rcvr) noexcept -> basic_operation<Self, Rcvr>
        {
            return {std::forward<Self>(self), std::move(rcvr)};
        }

        state_t get_state() noexcept
        {

            return process_previous_values;
        }
    };

    template <class Sndr, class Rcvr, class Index>
    struct my_basic_receiver // exposition only
    {
        using receiver_concept = ::mcs::execution::receiver_t;

        using tag_t = ::mcs::execution::tag_of_t<Sndr>; // exposition only
        using state_t =
            ::mcs::execution::snd::__detail::mate_type::state_type<Sndr,
                                                                   Rcvr>; // exposition
                                                                          // only

        // NOLINTNEXTLINE
        static constexpr const auto &complete = my_impls_for<tag_t>::complete;
        basic_state<Sndr, Rcvr> *parent_op; // exposition only // NOLINT

        template <class... Args>
            requires ::mcs::execution::functional::callable<decltype(complete), Index,
                                                            state_t &, Rcvr &,
                                                            ::mcs::execution::set_value_t,
                                                            Args...>
        void set_value(Args &&...args) && noexcept // NOLINT
        {
            complete(Index(), parent_op->state, parent_op->rcvr,
                     ::mcs::execution::set_value_t(), std::forward<Args>(args)...);
        }

        template <class Error>
            requires ::mcs::execution::functional::callable<decltype(complete), Index,
                                                            state_t &, Rcvr &,
                                                            ::mcs::execution::set_error_t,
                                                            Error>
        void set_error(Error &&err) && noexcept // NOLINT
        {
            complete(Index(), parent_op->state, parent_op->rcvr,
                     ::mcs::execution::set_error_t(), std::forward<Error>(err));
        }

        void set_stopped() && noexcept // NOLINT
            requires ::mcs::execution::functional::callable<
                decltype(complete), Index, state_t &, Rcvr &,
                ::mcs::execution::set_stopped_t>
        {
            complete(Index(), parent_op->state, parent_op->rcvr,
                     ::mcs::execution::set_stopped_t());
        }

        auto get_env() const noexcept // NOLINT
            -> ::mcs::execution::snd::__detail::mate_type::env_type<Index, Sndr,
                                                                    Rcvr> // NOLINT
        {
            return my_impls_for<tag_t>::get_env(Index(), parent_op->state,
                                                parent_op->rcvr);
        }
    };

    template <typename T>
    struct my_tag
    {
    };

    template <typename T>
    struct my_impls_for<my_tag<T>> : public my_default_impls
    {
        static constexpr auto get_state = // NOLINT
            []<class Sndr, class Rcvr>(Sndr &&sndr,
                                       Rcvr & /*rcvr*/) noexcept -> decltype(auto) {
            return sndr.get_state();
        };
        // 特化的实现
        static constexpr auto complete = // NOLINT
            []<class Fn, class Tag, class... Args>(auto, Fn &fn, auto &rcvr, Tag,
                                                   Args &&...args) noexcept -> void {
            // check one
            static_assert(std::is_same_v<std::remove_cvref_t<decltype(fn)>,
                                         typename my_sender<T>::state_t>);
            static_assert(std::is_same_v<std::invoke_result_t<Fn, Args...>, T>);

            if constexpr (std::same_as<Tag, my_tag<T>>)
            {
                try
                {
                    ::mcs::execution::recv::set_value(
                        std::move(rcvr),
                        std::invoke(std::move(fn), std::forward<Args>(args)...));
                }
                catch (...)
                {
                    ::mcs::execution::recv::set_error(std::move(rcvr),
                                                      std::current_exception());
                }
            }
            else
            {
                Tag()(std::move(rcvr), std::forward<Args>(args)...);
            }
        };

    }; // namespace test
}; // namespace test
/**
 * @brief 验证 如果 子操作，没有发布完成. 修改 reciver 和 opration 观察
 *  reciver 负责唤醒 p_opration 通过 complate函数通知
 *
 * @return int
 */

int main()
{
    std::cout << "main done\n";
    return 0;
}
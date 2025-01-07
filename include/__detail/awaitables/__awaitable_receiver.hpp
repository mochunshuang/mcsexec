#pragma once
#include "../__core_types.hpp"
#include "./__detail/__value_type.hpp"
#include "./__detail/__result_type.hpp"
#include "../consumers/__AS_EXCEPT_PTR.hpp"
#include "../queries/__forwar_dingquery_concept.hpp"

namespace mcs::execution
{
    namespace awaitables
    {
        template <class Promise>
        struct forwarding_environment
        {
            const std::coroutine_handle<Promise> &continuation; // NOLINT

            template <class Tag, class... Args>
                requires queries::forwardingquery<Tag>
            auto query(Tag tag, Args &&...args) const noexcept
            {
                return tag(queries::get_env(continuation.promise()),
                           std::forward<Args>(args)...);
            }
        };

        template <class Sndr, class Promise>
        struct awaitable_receiver
        {
            using receiver_concept = receiver_t;

            using value_type = __detail::value_type<Sndr, Promise>; // exposition only
            using result_type = __detail::result_type<value_type>;

            // NOLINTNEXTLINE
            std::variant<std::monostate, result_type, std::exception_ptr> *result_ptr;
            std::coroutine_handle<Promise> continuation; // NOLINT

            template <class... Vs>
            void set_value(Vs &&...vs) && noexcept // NOLINT
                requires ::std::constructible_from<result_type, decltype((vs))...>
            {
                try
                {
                    result_ptr->template emplace<1>(::std::forward<Vs>(vs)...);
                }
                catch (...)
                {
                    result_ptr->template emplace<2>(::std::current_exception());
                }
                continuation.resume();
            }

            template <class Err>
            void set_error(Err &&err) && noexcept // NOLINT
            {
                result_ptr->template emplace<2>(consumers::AS_EXCEPT_PTR(
                    ::std::forward<Err>(err))); // see [exec.general]
                continuation.resume();
            }

            void set_stopped() && noexcept // NOLINT
            {
                static_cast<::std::coroutine_handle<>>(
                    continuation.promise().unhandled_stopped())
                    .resume();
            }

            /**
             * Note: crcvr is (const *this)
             * For any expression tag whose type satisfies forwarding-query and for any
             * pack of subexpressions as, get_env(crcvr).query(tag, as...) is
             * expression-equivalent to
             * tag(get_env(as_const(crcvr.continuation.promise())), as...).
             */
            auto get_env() const noexcept // NOLINT
            {

                return forwarding_environment<Promise>{continuation};
            }
        };
    }; // namespace awaitables

}; // namespace mcs::execution
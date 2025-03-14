#pragma once
#include "../conn/__connect_result_t.hpp"
#include "./__awaitable_receiver.hpp"
#include "../opstate/__start.hpp"
#include <utility>

namespace mcs::execution::awaitables
{
    template <class Sndr, class Promise>
    class sender_awaitable // NOLINT
    {

        using awaitable_receiver = awaitables::awaitable_receiver<Sndr, Promise>;

        using value_type = awaitable_receiver::value_type;
        using result_type = awaitable_receiver::result_type;

        std::variant<std::monostate, result_type, std::exception_ptr> result{}; // NOLINT
        conn::connect_result_t<Sndr, awaitable_receiver> state;                 // NOLINT

      public:
        /**
         * Effects: Initializes state with connect(std::forward<Sndr>(sndr),
         * awaitable-receiver{addressof(result),
         * coroutine_handle<Promise>::from_promise(p)})
         */
        sender_awaitable(Sndr &&sndr, Promise &p)
            : state{conn::connect(
                  ::std::forward<Sndr>(sndr),
                  awaitable_receiver{
                      .result_ptr = ::std::addressof(result),
                      .continuation = ::std::coroutine_handle<Promise>::from_promise(p)})}
        {
        }

        static constexpr bool await_ready() noexcept // NOLINT
        {
            return false;
        }
        void await_suspend(std::coroutine_handle<Promise> /*unused*/) noexcept // NOLINT
        {
            opstate::start(state);
        }
        value_type await_resume() // NOLINT
        {
            if (result.index() == 2)
                std::rethrow_exception(std::get<2>(result));

            if constexpr (not std::is_void_v<value_type>)
                return std::forward<value_type>(std::get<1>(result));
        }
    };
}; // namespace mcs::execution::awaitables
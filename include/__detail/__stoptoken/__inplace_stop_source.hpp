#pragma once

#include <atomic>
#include <thread>
#include <mutex>

#include "./__inplace_callback_base.hpp"
#include "./__invocable_destructible.hpp"

namespace mcs::execution::stoptoken
{
    struct inplace_stop_token;

    template <__detail::invocable_destructible CallbackFn>
    class inplace_stop_callback;

    // Note: [inplace_stop_source] is the [sole owner] of its stop state
    // Note: [inplace_stop_token] and [inplace_stop_callback] does not owner
    // The class inplace_stop_source models stoppable-source.
    class inplace_stop_source // NOLINT
    {
      public:
        // [stopsource.inplace.cons], constructors, copy, and assignment
        // Effects: Initializes a new stop state inside *this.
        // Postconditions: stop_requested() is false
        inplace_stop_source() noexcept; // Note: std::thread::id no constexpr

        inplace_stop_source(inplace_stop_source &&) = delete;
        inplace_stop_source(const inplace_stop_source &) = delete;
        inplace_stop_source &operator=(inplace_stop_source &&) = delete;
        inplace_stop_source &operator=(const inplace_stop_source &) = delete;
        ~inplace_stop_source();

        //[stopsource.inplace.mem], stop handling
        // Returns: A new associated inplace_stop_token object.
        // The inplace_stop_token object’s stop-source member is equal to this.
        inplace_stop_token get_token() const noexcept; // NOLINT
        static constexpr bool stop_possible() noexcept // NOLINT
        {
            return true;
        }
        // Returns:
        // true if the stop state inside *this has received a stop request;
        // otherwise, false.
        bool stop_requested() const noexcept; // NOLINT
        // Effects: Executes a stop request operation ([stoptoken.concepts]).
        // Postconditions: stop_requested() is true.
        bool request_stop() noexcept; // NOLINT

      private:
        template <__detail::invocable_destructible CallbackFun>
        friend class ::mcs::execution::stoptoken::inplace_stop_callback;

        struct stop_state
        {
            std::thread::id id;                               // NOLINT
            std::mutex mtx;                                   // NOLINT
            inplace_callback_base *register_list{};           // NOLINT
            std::atomic<bool> stopped{};                      // NOLINT
            std::atomic<inplace_callback_base *> executing{}; // NOLINT
        };
        stop_state stop_state; // NOLINT

        auto registration(inplace_callback_base *callback_fn) -> void;
        auto deregistration(inplace_callback_base *callback_fn) -> void;
    };
}; // namespace mcs::execution::stoptoken
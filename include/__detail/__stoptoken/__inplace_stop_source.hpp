#pragma once

#include <atomic>
#include <thread>
#include <cassert>
#include <utility>

#include "./__inplace_callback_base.hpp"
#include "../tool/spin_wait.hpp"
#include "./__inplace_stop_token.hpp"

namespace mcs::execution::stoptoken
{

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
        inplace_stop_source() noexcept : state{} // Note: std::thread::id no constexpr
        {
            assert(not stop_requested());
        }
        //[stopsource.inplace.mem], stop handling
        // Returns: A new associated inplace_stop_token object.
        // The inplace_stop_token object’s stop-source member is equal to this.
        inplace_stop_token get_token() const noexcept // NOLINT
        {
            return inplace_stop_token(this);
        }
        static constexpr bool stop_possible() noexcept // NOLINT
        {
            return true;
        }
        // Returns:
        // true if the stop state inside *this has received a stop request;
        // otherwise, false.
        bool stop_requested() const noexcept // NOLINT
        {
            return (state.status.load(std::memory_order_acquire) &
                    stop_source_state::stop_requested_flag) != 0;
        }
        // Effects: Executes a stop request operation ([stoptoken.concepts]).
        // Postconditions: stop_requested() is true.
        bool request_stop() noexcept; // NOLINT

      private:
        template <__detail::invocable_destructible CallbackFun>
        friend class ::mcs::execution::stoptoken::inplace_stop_callback;

        struct stop_source_state
        {
            static constexpr std::uint8_t stop_requested_flag{1}; // NOLINT
            static constexpr std::uint8_t locked_flag{2};         // NOLINT
            mutable std::atomic<std::uint8_t> status{0};
            std::thread::id notifying_thread;
            inplace_callback_base *register_list{nullptr};
            std::atomic<inplace_callback_base *> executing{nullptr};
        };
        stop_source_state state; // NOLINT

        void add_stop_requested_flag() const noexcept // NOLINT
        {
            // state.status | state.stop_requested_flag
            state.status.fetch_or(stop_source_state::stop_requested_flag,
                                  std::memory_order_release);
        }

        auto lock_list() const noexcept -> void // NOLINT
        {
            constexpr tool::spin_wait spin_wait; // NOLINT
            auto old_status = state.status.load(std::memory_order_relaxed);
            do // NOLINT
            {
                // wait until you can obtain lock
                while ((old_status & stop_source_state::locked_flag) != 0)
                {
                    spin_wait.wait();
                    old_status = state.status.load(std::memory_order_relaxed);
                }
                // try again if failure
            } while (not state.status.compare_exchange_weak(
                old_status, old_status | stop_source_state::locked_flag,
                std::memory_order_release, std::memory_order_relaxed));
        }
        void unlock_list() const noexcept // NOLINT
        {
            // status &= ~locked_flag;
            state.status.fetch_and(~stop_source_state::locked_flag,
                                   std::memory_order_release);
        }
        auto registration(inplace_callback_base *new_head) noexcept -> void;
        auto deregistration(inplace_callback_base *delete_node) noexcept -> void;
    };

    /**
     * @brief As specified in [basic.life], the behavior of stop_requested() is undefined
     * unless the call strongly happens before the start of the destructor of the
     * associated inplace_stop_source, if any
     *
     * @return true
     * @return false
     */
    inline bool inplace_stop_token::stop_requested() const noexcept // NOLINT
    {
        // Note: must call before the start of the destructor
        return stop_source != nullptr && stop_source->stop_requested();
    }

    /**
     * @brief request_stop shall return true if a stop request was made, and false
     * otherwise.
     *
     * After a call to request_stop either a call to stop_possible shall return
     * false or a call to stop_requested shall return true.
     *
     * @return true : if call add_stop_requested_flag
     * @return false
     */
    inline bool inplace_stop_source::request_stop() noexcept
    {
        if (not stop_requested())
        {
            add_stop_requested_flag();

            if (state.register_list == nullptr)
                return true;

            state.notifying_thread = std::this_thread::get_id();

            while (state.register_list != nullptr)
            {
                lock_list();
                auto *exc_node =
                    std::exchange(state.register_list, state.register_list->next);
                unlock_list();

                state.executing.store(exc_node, std::memory_order_release);
                exc_node->invoke_callback(exc_node);
                state.executing.store(nullptr, std::memory_order_release);
            }
            return true;
        }
        // 2. Postconditions: stop_requested() is true.
        assert(stop_requested()); // Note: no necessary. allway is true
        return false;
    }

    inline auto inplace_stop_source::registration(
        inplace_callback_base *new_head) noexcept -> void
    {
        assert(new_head != nullptr);
        lock_list();
        new_head->next = std::exchange(state.register_list, new_head);
        unlock_list();
    }

    inline auto inplace_stop_source::deregistration(
        inplace_callback_base *delete_node) noexcept -> void
    {
        if (state.executing.load(std::memory_order_acquire) == delete_node)
        {
            // delete_node is removed by this_thread
            if (state.notifying_thread == std::this_thread::get_id())
                return;

            // wait util other thread done callback,
            constexpr tool::spin_wait spin_wait; // NOLINT
            while (state.executing.load(std::memory_order_relaxed) != nullptr)
                spin_wait.wait();
        }
        else
        {
            if (state.register_list == nullptr)
                return;

            lock_list();
            auto **indirect = &state.register_list;
            while (*indirect != nullptr)
            {
                if (*indirect == delete_node)
                {
                    *indirect = delete_node->next;
                    break;
                }
                indirect = &((*indirect)->next);
            }
            unlock_list();
        }
    }

}; // namespace mcs::execution::stoptoken
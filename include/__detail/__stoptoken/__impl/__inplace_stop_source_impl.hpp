#pragma once
#include <cassert>
#include <utility>

#include "../__inplace_stop_token.hpp"
#include "../__inplace_stop_source.hpp"

namespace mcs::execution::stoptoken
{

    inline inplace_stop_source::inplace_stop_source() noexcept : stop_state{}
    {
        // Effects: Initializes a new stop state inside *this. => stop_state{}
        // Postconditions: stop_requested() is false.
        assert(not stop_requested());
    }

    inline inplace_stop_token inplace_stop_source::get_token() const noexcept
    {
        // The inplace_stop_token object’s stop-source member is equal to this.
        return inplace_stop_token(this);
    }

    inline inplace_stop_source::~inplace_stop_source() = default;

    inline bool inplace_stop_source::stop_requested() const noexcept
    {
        // if the stop state inside *this has received a stop request; otherwise, false.
        return stop_state.stopped.load();
    }

    /**
     * @brief request_stop shall return true if a stop request was made, and false
     * otherwise.
     *
     * After a call to request_stop either a call to stop_possible shall return
     * false or a call to stop_requested shall return true.
     *
     * @return true
     * @return false
     */
    inline bool inplace_stop_source::request_stop() noexcept
    {
        // 1. Effects: Executes a stop request operation ([stoptoken.concepts]).
        using relock_helper = std::unique_ptr<std::unique_lock<std::mutex>,
                                              decltype([](auto p) { p->lock(); })>;

        // when stop_state == not stoped. set stop_state=true and invoke_callback
        if (not stop_state.stopped.exchange(true))
        {
            std::unique_lock guard(stop_state.mtx);
            for (auto *it = stop_state.register_list; it != nullptr;
                 it = stop_state.register_list)
            {
                stop_state.executing = it;

                stop_state.id = ::std::this_thread::get_id();
                stop_state.register_list = it->next;
                {
                    relock_helper r(&guard);
                    guard.unlock();

                    // No constraint is placed on the order in which the callback
                    // invocations are executed
                    it->invoke_callback(); // do impl_request_stop()
                } // lock

                stop_state.executing = nullptr; // Note: 逻辑耦合deregistration
            }
            return true;
        }

        // 2. Postconditions: stop_requested() is true.
        // assert(stop_requested()); //Note: no necessary. allway is true

        return false;
    }

    inline auto inplace_stop_source::registration(inplace_callback_base *callback_fn)
        -> void
    {
        std::lock_guard guard(stop_state.mtx);
        // add to head
        callback_fn->next = std::exchange(stop_state.register_list, callback_fn);
    }

    inline auto inplace_stop_source::deregistration(inplace_callback_base *callback_fn)
        -> void
    {
        // 拿到锁先检查：是否正在删除？是等待。否则存在移除。强保证invoke_callback()一定是正常的
        std::unique_lock guard(stop_state.mtx);

        // Note: 1: If callback_fn is concurrently executing
        if (stop_state.executing == callback_fn)
        {
            // Note:  executing on the current thread shall not block
            if (stop_state.id == std::this_thread::get_id())
            {
                return;
            }

            // Note: shall not block on the some other callback registereds
            guard.unlock();

            // Note: executing on another thread shall block unit returns  ([defns.block])
            while (stop_state.executing == callback_fn)
            {
            }
            return;
        }

        // Note: 2.shall be removed callback_fn from the register_list
        for (inplace_callback_base **it{&stop_state.register_list}; *it != nullptr;
             it = &(*it)->next)
        {
            if (*it == callback_fn)
            {
                *it = callback_fn->next;
                break;
            }
        }
    }

}; // namespace mcs::execution::stoptoken
#pragma once

#include <exception>

#include "./__suspend_complete.hpp"

#include "../../__recv.hpp"

namespace mcs::execution::conn::__detail
{
    template <class operation_state_task, class DS, class DR, class Sigs, class V>
    operation_state_task connect_awaitable(DS sndr, DR rcvr)
        requires receiver_of<DR, Sigs>
    {
        std::exception_ptr ep;
        try
        {
            if constexpr (std::same_as<V, void>)
            {
                co_await std::move(sndr);
                co_await suspend_complete(set_value, std::move(rcvr));
            }
            else
            {
                co_await suspend_complete(set_value, std::move(rcvr),
                                          co_await std::move(sndr));
            }
        }
        catch (...)
        {
            ep = std::current_exception();
        }
        co_await suspend_complete(set_error, std::move(rcvr), std::move(ep));
    }
}; // namespace mcs::execution::conn::__detail
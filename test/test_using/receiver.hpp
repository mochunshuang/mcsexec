#pragma once
#include <iostream>

#include "sender.hpp"

namespace mcs::execution
{
    namespace recr
    {
        struct receiver_t
        {
            void recr_hello() const // NOLINT
            {
                std::cout << "recr_hello call \n";
                sender.hello(this); // sender 不能再使用 receiver变量
            }
        };
    } // namespace recr

    using recr::receiver_t;
    constexpr inline receiver_t receiver; // NOLINT
}; // namespace mcs::execution
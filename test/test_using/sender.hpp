#pragma once

#include <iostream>

// 嵌套了，失败的.
// 只能使用  receiver_t. 不能互相指向。否则一定有 未完全定义的错误
#include "./__type.hpp"

namespace mcs::execution
{
    namespace snd
    {
        struct sender_t
        {
            void hello(const recr::receiver_t *p) const // NOLINT
            {

                using T = recr::receiver_t;
                const T *a = p;
                (void)a;
                std::cout << "sender_t: hello call,ptr:: " << a << "\n";

                // has incomplete type 'recr::receiver_t'
                // recr::receiver_t b;
                // (void)b;
            }
        };
    }; // namespace snd

    using snd::sender_t;
    constexpr inline sender_t sender; // NOLINT
}; // namespace mcs::execution

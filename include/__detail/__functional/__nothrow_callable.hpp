
#pragma once

#include "./__callable.hpp"

namespace mcs::execution::functional
{

    template <class Fn, class... Args>
    concept nothrow_callable = // exposition only
        callable<Fn, Args...> && requires(Fn &&fn, Args &&...args) {
            { std::forward<Fn>(fn)(std::forward<Args>(args)...) } noexcept;
        };

}; // namespace mcs::execution::functional

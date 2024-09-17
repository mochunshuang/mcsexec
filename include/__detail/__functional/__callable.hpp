#pragma once

#include <utility>

namespace mcs::execution::functional
{
    template <class Fn, class... Args>
    concept callable = // exposition only
        requires(Fn &&fn, Args &&...args) {
            std::forward<Fn>(fn)(std::forward<Args>(args)...);
        };

}; // namespace mcs::execution::functional
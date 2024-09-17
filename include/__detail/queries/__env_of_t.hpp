#pragma once

#include "./__get_env.hpp"

namespace mcs::execution::queries
{
    template <class T>
    using env_of_t = decltype(get_env(std::declval<T>()));

}; // namespace mcs::execution::queries
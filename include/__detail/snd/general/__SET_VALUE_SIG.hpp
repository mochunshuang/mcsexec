#pragma once

#include <type_traits>
#include "../../__core_types.hpp"

namespace mcs::execution::snd::general
{
    template <typename T>
    using SET_VALUE_SIG = typename std::conditional_t<std::is_same_v<T, void>,
                                                      set_value_t(), set_value_t(T)>;
};
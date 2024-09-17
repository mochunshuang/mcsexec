#pragma once

#include "./__get_stop_token.hpp"

namespace mcs::execution::queries
{
    template <class T>
    using stop_token_of_t =
        std::remove_cvref_t<decltype(get_stop_token(std::declval<T>()))>;

}; // namespace mcs::execution::queries
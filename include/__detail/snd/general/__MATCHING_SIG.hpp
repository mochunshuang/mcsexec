#pragma once
#include "./__NORMALIZE_SIG.hpp"
#include <concepts>
namespace mcs::execution::snd::general
{
    template <typename F1, typename F2>
    concept MATCHING_SIG = // NOLINT
        std::same_as<NORMALIZE_SIG<F1>, NORMALIZE_SIG<F2>>;

}; // namespace mcs::execution::snd::general
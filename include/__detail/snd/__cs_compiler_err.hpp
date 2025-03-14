#pragma once

#include "../cmplsigs/__completion_signatures.hpp"

namespace mcs::execution::snd
{
    template <class CS>
    concept cs_compiler_err = std::is_same_v<CS, cmplsigs::completion_signatures<>>;

}; // namespace mcs::execution::snd
#pragma once

#include "./__completion_signature.hpp"
#include <exception>

namespace mcs::execution::cmplsigs
{

    template <completion_signature... Sigs>
    struct completion_signatures;

    template <bool NoThrowing>
    inline constexpr auto eptr_completion_if = // NOLINT
        std::conditional_t<NoThrowing, completion_signatures<>,
                           completion_signatures<set_error_t(std::exception_ptr)>>();

}; // namespace mcs::execution::cmplsigs
#pragma once

#include "../__core_types.hpp"
#include "./__variant_or_empty.hpp"

#include "../snd/__completion_signatures_of_t.hpp"
#include "../snd/__sender_in.hpp"

#include "../cmplsigs/__gather_signatures.hpp"

namespace mcs::execution::cmplsigs
{
    template <class Sndr, class Env = ::mcs::execution::empty_env,
              template <class...> class Variant = variant_or_empty>
        requires snd::sender_in<Sndr, Env>
    using error_types_of_t =
        cmplsigs::gather_signatures<set_error_t,
                                    snd::completion_signatures_of_t<Sndr, Env>,
                                    std::type_identity_t, Variant>;

}; // namespace mcs::execution::cmplsigs
#pragma once

#include "../__core_types.hpp"
#include "./__gather_signatures.hpp"
#include "./__variant_or_empty.hpp"

#include "../snd/__sender_in.hpp"
#include "../snd/__completion_signatures_of_t.hpp"

namespace mcs::execution::cmplsigs
{

    template <class Sndr, class Env = ::mcs::execution::empty_env,
              template <class...> class Tuple = decayed_tuple,
              template <class...> class Variant = variant_or_empty>
        requires snd::sender_in<Sndr, Env>
    using value_types_of_t =
        gather_signatures<set_value_t, snd::completion_signatures_of_t<Sndr, Env>, Tuple,
                          Variant>;

}; // namespace mcs::execution::cmplsigs
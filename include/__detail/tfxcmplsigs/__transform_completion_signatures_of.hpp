#pragma once

#include "./__transform_completion_signatures.hpp"

#include "../snd/__sender.hpp"
#include "../snd/__sender_in.hpp"
#include "../snd/__completion_signatures_of_t.hpp"

namespace mcs::execution::tfxcmplsigs
{

    template <snd::sender Sndr, class Env = ::mcs::execution::empty_env,
              cmplsigs::valid_completion_signatures AdditionalSignatures =
                  cmplsigs::completion_signatures<>,
              template <class...> class SetValue = default_set_value,
              template <class> class SetError = default_set_error,
              cmplsigs::valid_completion_signatures SetStopped =
                  cmplsigs::completion_signatures<set_stopped_t()>>
        requires snd::sender_in<Sndr, Env>
    using transform_completion_signatures_of =
        transform_completion_signatures<snd::completion_signatures_of_t<Sndr, Env>,
                                        AdditionalSignatures, SetValue, SetError,
                                        SetStopped>;

    template <snd::sender Sndr, class Env = ::mcs::execution::empty_env,
              cmplsigs::valid_completion_signatures AdditionalSignatures =
                  cmplsigs::completion_signatures<>,
              template <class...> class SetValue = default_set_value,
              template <class> class SetError = default_set_error,
              cmplsigs::valid_completion_signatures SetStopped =
                  cmplsigs::completion_signatures<set_stopped_t()>>
        requires snd::sender_in<Sndr, Env>
    using transform_completion_signatures_of_test_ags = int;

}; // namespace mcs::execution::tfxcmplsigs
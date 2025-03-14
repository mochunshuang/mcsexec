#pragma once

#include "./cmplsigs/__completion_signature.hpp"
#include "./cmplsigs/__completion_signatures.hpp"
#include "./cmplsigs/__valid_completion_signatures.hpp"
#include "./cmplsigs/__error_types_of_t.hpp"
#include "./cmplsigs/__value_types_of_t.hpp"
#include "./cmplsigs/__sends_stopped.hpp"
#include "./cmplsigs/__single_sender_value_type.hpp"

#include "./tfxcmplsigs/__transform_completion_signatures.hpp"
#include "./tfxcmplsigs/__transform_completion_signatures_of.hpp"

namespace mcs::execution
{
    // [exec.utils], sender and receiver utilities
    // [exec.utils.cmplsigs]
    using cmplsigs::completion_signature;
    using cmplsigs::valid_completion_signatures;
    using cmplsigs::completion_signatures; // template
    using cmplsigs::value_types_of_t;
    using cmplsigs::error_types_of_t;
    using cmplsigs::sends_stopped;
    using cmplsigs::single_sender_value_type;

    // [exec.utils.tfxcmplsigs]
    using tfxcmplsigs::transform_completion_signatures;
    using tfxcmplsigs::transform_completion_signatures_of;

}; // namespace mcs::execution
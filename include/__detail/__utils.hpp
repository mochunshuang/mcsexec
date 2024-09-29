#pragma once

#include "./cmplsigs/__completion_signature.hpp"
#include "./cmplsigs/__completion_signatures.hpp"
#include "./cmplsigs/__valid_completion_signatures.hpp"
#include "./cmplsigs/__detail/__tpl_param_trnsfr.hpp"

#include "./tfxcmplsigs/__transform_completion_signatures.hpp"
#include "./tfxcmplsigs/__transform_completion_signatures_of.hpp"
#include "./tfxcmplsigs/__default_set_error.hpp"
#include "./tfxcmplsigs/__default_set_value.hpp"

namespace mcs::execution
{
    // [exec.utils], sender and receiver utilities
    // [exec.utils.cmplsigs]
    using cmplsigs::completion_signature;
    using cmplsigs::valid_completion_signatures;
    using cmplsigs::__detail::tpl_param_trnsfr_t;
    using cmplsigs::completion_signatures; // template

    // [exec.utils.tfxcmplsigs]
    using tfxcmplsigs::transform_completion_signatures;
    using tfxcmplsigs::transform_completion_signatures_of;
    using tfxcmplsigs::default_set_value;
    using tfxcmplsigs::default_set_error;

    using tfxcmplsigs::__detail::__completion_signatures_set;

}; // namespace mcs::execution
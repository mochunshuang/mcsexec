#pragma once

#include "./__filter_tuple.hpp"
#include "./__tpl_param_trnsfr.hpp"
#include "./__select_tag.hpp"
#include "../__completion_signatures.hpp"

namespace mcs::execution::cmplsigs::__detail
{
    template <typename Completion, typename Sigs>
    using filter_sigs_by_completion = tpl_param_trnsfr_t<
        completion_signatures,
        typename filter_tuple<select_tag<Completion>::template predicate,
                              tpl_param_trnsfr_t<std::tuple, Sigs>>::type>;

}; // namespace mcs::execution::cmplsigs::__detail
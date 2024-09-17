#pragma once

#include "./__valid_completion_signatures.hpp"

#include "./__detail/__gather_signatures_helper.hpp"
#include "./__detail/__select_tag.hpp"
#include "./__detail/__tpl_param_trnsfr.hpp"
#include "./__detail/__filter_tuple.hpp"

namespace mcs::execution::cmplsigs
{
    template <class Tag, valid_completion_signatures InputCompletions,
              template <class...> class Tuple, template <class...> class Variant>
    using gather_signatures = typename __detail::gather_signatures_helper<
        __detail::tpl_param_trnsfr_t<
            cmplsigs::completion_signatures,
            typename __detail::filter_tuple<
                __detail::select_tag<Tag>::template predicate,
                __detail::tpl_param_trnsfr_t<std::tuple, InputCompletions>>::type>,
        Tuple, Variant>::type;

}; // namespace mcs::execution::cmplsigs

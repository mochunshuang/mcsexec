#pragma once

#include "./__valid_completion_signatures.hpp"
#include "./__detail/__gather_signatures_helper.hpp"
#include "./__detail/__filter_sigs_by_completion.hpp"

namespace mcs::execution::cmplsigs
{
    template <class Tag, valid_completion_signatures InputCompletions,
              template <class...> class Tuple, template <class...> class Variant>
    using gather_signatures = typename __detail::gather_signatures_helper<
        typename __detail::filter_sigs_by_completion<Tag, InputCompletions>::type, Tuple,
        Variant>::type;

}; // namespace mcs::execution::cmplsigs

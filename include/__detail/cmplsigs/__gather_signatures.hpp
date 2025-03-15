#pragma once

#include "./__valid_completion_signatures.hpp"
#include "./__detail/__gather_signatures_helper.hpp"

namespace mcs::execution::cmplsigs
{
    template <class Tag, valid_completion_signatures InputCompletions,
              template <class...> class Tuple, template <class...> class Variant>
    using gather_signatures = typename __detail::gather_signatures_helper<
        decltype(InputCompletions::template filter_sigs<Tag>()), Tuple, Variant>::type;

}; // namespace mcs::execution::cmplsigs

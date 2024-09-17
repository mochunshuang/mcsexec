#pragma once

#include "./__get_completion_signatures.hpp"
#include "../__functional/__call_result_t.hpp"

namespace mcs::execution::snd
{

    template <typename S, typename E = ::mcs::execution::empty_env>
    using completion_signatures_of_t =
        functional::call_result_t<get_completion_signatures_t, S, E>;

}; // namespace mcs::execution::snd

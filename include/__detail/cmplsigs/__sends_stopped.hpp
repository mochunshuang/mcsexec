#pragma once

#include "../cmplsigs/__gather_signatures.hpp"

#include "../__core_types.hpp"
#include "../snd/__sender_in.hpp"
#include "../snd/__completion_signatures_of_t.hpp"

namespace mcs::execution::cmplsigs
{

    template <class Sndr, class Env = ::mcs::execution::empty_env>
        requires snd::sender_in<Sndr, Env>
    inline constexpr bool sends_stopped = // NOLINT
        not std::same_as<type_list<>,
                         cmplsigs::gather_signatures<
                             set_stopped_t, snd::completion_signatures_of_t<Sndr, Env>,
                             type_list, type_list>>;

}; // namespace mcs::execution::cmplsigs
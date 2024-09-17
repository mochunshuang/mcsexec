#pragma once

#include "./__receiver.hpp"

#include "../__functional/__callable.hpp"
#include "../cmplsigs/__completion_signatures.hpp"

namespace mcs::execution::recv
{

    template <class Signature, class Rcvr>
    concept valid_completion_for = // exposition only
        requires(Signature *sig) {
            []<class Tag, class... Args>(Tag (*)(Args...))
                requires functional::callable<Tag, std::remove_cvref_t<Rcvr>, Args...>
            {
            }(sig);
        };

    template <class Rcvr, class Completions>
    concept has_completions = // exposition only
        requires(Completions *completions) {
            []<valid_completion_for<Rcvr>... Sigs>(
                cmplsigs::completion_signatures<Sigs...> *) {
            }(completions);
        };

    template <class Rcvr, class Completions>
    concept receiver_of = receiver<Rcvr> && has_completions<Rcvr, Completions>;

}; // namespace mcs::execution::recv
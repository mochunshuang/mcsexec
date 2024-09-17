#pragma once

#include "../__core_types.hpp"
#include "../__core_concepts.hpp"

#include "../snd/__sender.hpp"
#include "../factories/__schedule.hpp"

namespace mcs::execution::sched
{
    //////////////////////////////////////////////////////////////////////////////
    // [exec.scheduler.concepts]
    template <class Sch>
    concept scheduler =
        std::derived_from<typename std::remove_cvref_t<Sch>::scheduler_concept,
                          scheduler_t> &&
        queryable<Sch> &&
        requires(Sch &&sch) {
            { factories::schedule(std::forward<Sch>(sch)) } -> snd::sender;
            {
                auto(queries::get_completion_scheduler<set_value_t>(
                    queries::get_env(factories::schedule(std::forward<Sch>(sch)))))
            } -> std::same_as<std::remove_cvref_t<Sch>>;
        } && std::equality_comparable<std::remove_cvref_t<Sch>> &&
        std::copy_constructible<std::remove_cvref_t<Sch>>;

}; // namespace mcs::execution::sched

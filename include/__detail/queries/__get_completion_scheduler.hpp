#pragma once

#include "./__forwarding_query.hpp"

#include "../__core_types.hpp"

namespace mcs::execution::queries
{

    template <class CPO>
    struct get_completion_scheduler_t;

    template <class Tag>
    concept completion_tag = // exposition only
        std::same_as<Tag, set_value_t> || std::same_as<Tag, set_error_t> ||
        std::same_as<Tag, set_stopped_t>;

    //  Note: the concept scheduler is dependent on this get_completion_scheduler,
    //  so no check query return
    //  get_completion_scheduler<completion-tag> obtains the completion scheduler
    //  associated with a completion tag from a sender’s attributes.
    template <completion_tag CompletionTag>
    struct get_completion_scheduler_t<CompletionTag> : forwarding_query_t
    {
        using __self_t = get_completion_scheduler_t<CompletionTag>;

        template <typename T>
            requires requires(T &&env) {
                { std::as_const(env).query(std::declval<__self_t>()) } noexcept;
            }
        constexpr auto operator()(T &&env) const noexcept -> auto
        {
            return std::as_const(env).query(*this);
        }
        // get_completion_scheduler<completion-tag>(get_env(sndr)) is well-formed and
        // denotes a scheduler sch.
        // If an asynchronous operation created by connecting sndr with a receiver rcvr
        // causes the evaluation of completion-fn(rcvr, args...),
        // the behavior is undefined unless the evaluation happens on an execution agent
        // that belongs to sch's associated execution resource.
    };

    // obtains the completion scheduler associated with a completion tag from a sender’s
    // attributes.
    template <class CPO>
    inline constexpr get_completion_scheduler_t<CPO> get_completion_scheduler{}; // NOLINT
}; // namespace mcs::execution::queries
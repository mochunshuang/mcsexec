#pragma once

#include <tuple>
namespace mcs::execution
{
    // [exec.sched], schedulers
    struct scheduler_t
    {
    };

    // [exec.recv]
    namespace recv
    {
        struct set_error_t;
        struct set_stopped_t;
        struct set_value_t;
    }; // namespace recv
    using recv::set_error_t;
    using recv::set_stopped_t;
    using recv::set_value_t;

    // [exec.snd], senders
    struct sender_t
    {
    };
    // [exec.recv], receivers
    struct receiver_t
    {
    };
    // [exec.opstate], operation states
    struct operation_state_t
    {
    };
    struct empty_env
    {
    };

    template <class... Ts>
    using decayed_tuple = std::tuple<std::decay_t<Ts>...>;

    template <class... Ts>
    struct type_list; // exposition only

    // [exec.queries], queries
    enum class forward_progress_guarantee
    {
        concurrent,     // NOLINT
        parallel,       // NOLINT
        weakly_parallel // NOLINT
    };

}; // namespace mcs::execution
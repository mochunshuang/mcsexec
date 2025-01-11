#pragma once

#include "__core_concepts.hpp"
#include <cstdint>
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

    namespace queries
    {
        // https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p3325r5.html
        template <queryable... Envs>
        struct env;
        template <>
        struct env<>
        {
        };
    }; // namespace queries
    // struct empty_env
    // {
    // };
    using empty_env = queries::env<>;

    template <class... Ts>
    using decayed_tuple = std::tuple<std::decay_t<Ts>...>;

    template <class... Ts>
    struct type_list; // exposition only

    // [exec.queries], queries
    enum class forward_progress_guarantee : std::uint8_t
    {
        concurrent,     // NOLINT
        parallel,       // NOLINT
        weakly_parallel // NOLINT
    };

}; // namespace mcs::execution
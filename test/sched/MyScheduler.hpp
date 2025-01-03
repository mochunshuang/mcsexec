#pragma once

#include "./../../include/execution.hpp"
#include <utility>
namespace ex = mcs::execution; // NOLINT
template <typename Receiver>
struct state
{
    using operation_state_concept = ex::operation_state_t;
    std::remove_cvref_t<Receiver> receiver; // NOLINT
    auto start() & noexcept -> void
    {
        ex::set_value(::std::move(this->receiver));
    }
};

struct MyScheduler
{
    using scheduler_concept = ex::scheduler_t;
    struct Sched_Sndr_Env
    {
        static auto query(
            const ex::get_completion_scheduler_t<ex::set_value_t> & /*unused*/) noexcept
            -> MyScheduler
        {
            return {};
        }
    };

    struct MySender
    {

        using sender_concept = ex::sender_t;

        // No need get_completion_signatures funcation
        using completion_signatures = ex::completion_signatures<ex::set_value_t()>;

        // NOLINTNEXTLINE: const noexcept
        [[nodiscard]] auto get_env() const noexcept -> ex::queryable auto
        {

            return Sched_Sndr_Env{};
        }

        template <ex::receiver Receiver>
        auto connect(Receiver &&receiver) -> state<Receiver>
        {
            return {std::forward<Receiver>(receiver)};
        }
    };

    auto operator==(const MyScheduler &) const -> bool = default;
    [[nodiscard]] auto schedule() const noexcept // NOLINT
    {
        return MySender{};
    };

    // static_assert(ex::snd::sender<MySender>);
};

static_assert(ex::snd::sender<MyScheduler::MySender>);
static_assert(ex::snd::sender_in<MyScheduler::MySender>);
static_assert(ex::sched::scheduler<MyScheduler>);
static_assert(std::equality_comparable<std::remove_cvref_t<MyScheduler>> &&
              std::copy_constructible<std::remove_cvref_t<MyScheduler>>);

// int main()
// {
//     [[maybe_unused]] MyScheduler sch;
//     [[maybe_unused]] MyScheduler::MySender sndr;

//     [[maybe_unused]] auto e = ex::queries::get_env(sndr);
//     ex::factories::schedule(sch);

//     return 0;
// }
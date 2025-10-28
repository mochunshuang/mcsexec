#include "../test_base_head.hpp"

struct receiver_any
{
    using receiver_concept = ex::receiver_t;
    int &value; // NOLINT

    void set_value(int v) && noexcept // NOLINT
    {
        this->value = v;
    }
    template <typename E> // NOLINTNEXTLINE
    auto set_error(E &&e) && noexcept -> void
    {
    }

    ex::empty_env get_env() const noexcept // NOLINT
    {
        return {};
    }
};
static_assert(ex::receiver<receiver_any>);

int main()
{
    mcs::execution::scope::__detail::inline_scheduler sched;
    auto sched_sender{ex::schedule(sched)};

    TEST("inline_scheduler is scheduler ") = [] {
        static_assert(ex::scheduler<decltype(sched)>);
    };

    TEST("schedule(sched) is sender ") = [] {
        static_assert(ex::sender<decltype(sched_sender)>);
    };

    TEST("inline_scheduler's env provide completion_scheduler that is itslef ") = [&] {
        auto env{ex::get_env(sched_sender)};
        assert(sched == ex::get_completion_scheduler<ex::set_value_t>(env));
    };

    TEST("schedule(sched) base test") = [&] {
        int value{};
        auto state{
            // NOLINTNEXTLINE
            ex::connect(std::move(sched_sender) | ex::then([]() noexcept { return 17; }),
                        receiver_any{value})};
        static_assert(operation_state<decltype(state)>);

        assert(value == 0);
        ex::opstate::start(state);
        assert(value == 17);
    };

    return 0;
}
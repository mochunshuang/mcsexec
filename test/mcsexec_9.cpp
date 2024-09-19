#include "../include/execution.hpp"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <thread>
#include <tuple>
#include <utility>

using namespace mcs::execution;

#include <iostream>

// 前向声明
template <typename Sigs, typename Rcvr, typename sched_t>
struct state_type;

// 定义 receiver_type
template <typename state_type>
struct receiver_type
{
    using receiver_concept = ::mcs::execution::receiver_t;
    state_type *state; // 使用指针

    void set_value() && noexcept
    {
        std::visit(
            [this]<class Tuple>(Tuple &result) noexcept -> void {
                if constexpr (not std::same_as<std::monostate, Tuple>)
                {
                    std::apply(
                        [&]<class Tag, typename... Args>(Tag &tag, Args &...args) {
                            tag(std::move(state->rcvr), std::move(args)...);
                        },
                        result);
                }
            },
            state->async_result);
    }

    template <class Error>
    void set_error(Error &&err) && noexcept
    {
        recv::set_error(std::move(state->rcvr), std::forward<Error>(err));
    }

    void set_stopped() && noexcept
    {
        recv::set_stopped(std::move(state->rcvr));
    }

    decltype(auto) get_env() const noexcept
    {
        auto &r = state->rcvr;
        // auto v = state->rcvr.get_env();
        // auto v = snd::general::FWD_ENV(queries::get_env(state->rcvr));
        return empty_env{};
    }
};

// 定义 state_type
template <typename Sigs, typename Rcvr, typename sched_t>
struct state_type
{
    Rcvr &rcvr;

    using variant_t = typename adapt::compute_variant_t<Sigs>::type;
    using receiver_t = receiver_type<state_type>; // 使用前向声明的类型
    using operation_t =
        conn::connect_result_t<sched::schedule_result_t<sched_t>, receiver_t>;

    // exposition only
    operation_t op_state;   // exposition only
    variant_t async_result; // exposition only

    explicit state_type(sched_t sch, Rcvr &rcvr) noexcept(
        noexcept(conn::connect(factories::schedule(sch), receiver_t{this})))
        : rcvr(rcvr), op_state(conn::connect(factories::schedule(sch), receiver_t{this}))
    {
    }
};

int main()
{
    static_thread_pool<3> cpu_ctx;
    scheduler auto sch1 = cpu_ctx.get_scheduler();
    {
        using Sigs = mcs::execution::cmplsigs::completion_signatures<
            mcs::execution::recv::set_value_t(),
            mcs::execution::recv::set_error_t(std::__exception_ptr::exception_ptr),
            mcs::execution::recv::set_stopped_t()>;
        using sched_t = decltype(auto(sch1));
    }

    std::cout << "hello world\n";
    return 0;
}

void base()
{
    struct state_type;

    using T = receiver_type<state_type>;
    struct state_type
    {
        using Rcvr = int;
        using variant_t = std::tuple<std::monostate, std::tuple<int>>;
        using operation_t = int;

        Rcvr &rcvr;             // exposition only // NOLINT
        variant_t async_result; // exposition only // NOLINT
        operation_t op_state;   // exposition only // NOLINT
    };
    T t;

    // noexcept Use of undeclared identifier 'A'
    []() noexcept(true) {
        struct A
        {
        };
        return A{};
    }();

    {
        struct B
        {
            /**
             * @brief error:
                    'auto' parameter not permitted in this context
             *
             * @param v
             */
            // Note: 局部的类，不能声明模板
            // void set_value(auto v) // NOLINT
            // {
            //     std::cout << v << '\n';
            // };
        };
        B b;
        // b.set_value(1);
    }
}
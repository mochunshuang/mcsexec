#include "../test_base_head.hpp"
#include <cassert>
#include <chrono>
#include <thread>

// 自定义查询标签
struct my_allocator_key
{
};
struct my_scheduler_key
{
};

// 自定义值类型
struct my_alloc
{
};
struct my_sched
{
};

// 自定义可查询对象
struct alloc_env
{
    constexpr my_alloc query(my_allocator_key) const noexcept // NOLINT
    {
        return my_alloc{};
    }
};

struct sched_env
{
    constexpr my_sched query(my_scheduler_key) const noexcept // NOLINT
    {
        return my_sched{};
    }
};

struct receiver_all
{
    using receiver_concept = mcs::execution::receiver_t;

    template <typename... A> // NOLINTNEXTLINE
    auto set_value(A &&...a) && noexcept -> void
    {
    }

    template <typename E> // NOLINTNEXTLINE
    auto set_error(E &&) && noexcept -> void
    {
    }

    void set_stopped() && noexcept // NOLINT
    {
    }

    constexpr auto get_env() const noexcept // NOLINT
    {
        return ex::empty_env{};
    }
};

int main()
{
    ex::static_thread_pool<1> pool;

    TEST("base operation") = [&] {
        bool called{false};
        bool called_fun{false};
        std::any any;
        test::channel c{test::channel::NO_CALL};

        ex::sender auto snd = ex::schedule(pool.get_scheduler());
        auto op =
            connect(std::move(snd),
                    test::any_receiver{.called = &called, .data = &any, .chanel = &c});

        EXPECT(not called);
        EXPECT(not called_fun);
        EXPECT(c == test::channel::NO_CALL);
        start(op);
        while (not called)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        EXPECT(called);
        EXPECT(not called_fun);
        EXPECT(c == test::channel::VALUE_CHANNEL);
    };

    TEST("base") = [] {
        // 创建可查询对象
        alloc_env alloc_env_obj;
        sched_env sched_env_obj;

        // 创建 env 对象
        auto e = mcs::execution::queries::env(alloc_env_obj, sched_env_obj);

        // 查询环境
        auto alloc = e.query(my_allocator_key{});
        auto sched = e.query(my_scheduler_key{});
        static_assert(std::is_same_v<decltype(sched), my_sched>);
        static_assert(std::is_same_v<decltype(alloc), my_alloc>);
    };

    TEST("base with prop") = [] {
        using namespace mcs::execution::queries; // NOLINT
        auto e = env{prop(my_allocator_key{}, my_alloc{}),
                     prop(my_scheduler_key{}, my_sched{})};

        // 查询环境
        auto alloc = e.query(my_allocator_key{});
        auto sched = e.query(my_scheduler_key{});
        static_assert(std::is_same_v<decltype(sched), my_sched>);
        static_assert(std::is_same_v<decltype(alloc), my_alloc>);
    };

    TEST("env from sndr") = [&] {
        {
            auto sndr = ex::just();
            auto e = ex::get_env(sndr);
            static_assert(std::is_same_v<decltype(e), ex::empty_env>);
            static_assert(std::is_same_v<decltype(e), ex::env<>>);

            // ex::queries::get_scheduler(e);//NOTE: 不存在
            // static_assert(
            //     !requires(const ex::empty_env &e) { ex::queries::get_scheduler(e); });
            static_assert(!std::is_invocable_v<decltype(ex::queries::get_scheduler),
                                               const ex::empty_env &>);

            // auto then = ex::just() | ex::then([] {});
            // auto then_e = ex::get_env(then);
            // auto then_sched = ex::queries::get_scheduler(then_e); //NOTE: 语法错误
        }
        {
            auto sndr = ex::schedule(pool.get_scheduler());
            auto e = ex::get_env(sndr);
            static_assert(
                std::is_same_v<decltype(e), ex::run_loop::scheduler::sender::env>);

            auto sched = ex::queries::get_scheduler(e);
            EXPECT(sched == pool.get_scheduler());

            auto then = sndr | ex::then([] {});
            auto then_e = ex::get_env(then);
            auto then_sched = ex::queries::get_scheduler(then_e);

            EXPECT(sched == then_sched); // NOTE 转发成功

            auto sndrrr = then | ex::then([] {}) | ex::then([] {}) | ex::then([] {}) |
                          ex::then([] {}) | ex::then([] {}) | ex::then([] {}) |
                          ex::then([] {}) | ex::then([] {}) | ex::then([] {});
            auto env = ex::get_env(sndrrr);
            EXPECT(sched == ex::queries::get_scheduler(env));

            static_assert(
                not std::is_same_v<decltype(env), ex::run_loop::scheduler::sender::env>);
            static_assert(std::is_same_v<
                          decltype(env),
                          ex::snd::general::FWD_ENV<
                              mcs::execution::ctx::run_loop::scheduler::sender::env>>);
        }
    };
    TEST("env from test::any_receiver") = [&] {
        auto then = ex::schedule(pool.get_scheduler()) | ex::then([] {});
        using Sndr = decltype(then);
        using Rcvr [[maybe_unused]] =
            ex::snd::__detail::basic_receiver<Sndr, test::any_receiver,
                                              std::integral_constant<std::size_t, 0>>;
        auto op = ex::conn::connect(then, test::any_receiver{});
        test::any_receiver r = op.rcvr;
        auto sc = ex::queries::get_scheduler(ex::get_env(r));

        static_assert(std::is_same_v<MyScheduler, decltype(sc)>);

        auto &idx0 = op.inner_ops.get<0>();
        auto e = idx0.rcvr.get_env();
        auto sch = ex::queries::get_scheduler(e);

        // NOTE: 找到的是 接收方的 env
        static_assert(std::is_same_v<MyScheduler, decltype(sch)>);

        EXPECT(sc == sch);

        // auto &idx1 = op.inner_ops.get<1>(); //编译错误。确实只有一个子操作
    };

    TEST("env from receiver_all") = [&] {
        auto then = ex::schedule(pool.get_scheduler()) | ex::then([] {});
        auto op = ex::conn::connect(then, receiver_all{});
        auto r = op.rcvr;
        // auto sc = ex::queries::get_scheduler(ex::get_env(r)); //NOTE: 语法错误
        static_assert(std::is_same_v<receiver_all, decltype(r)>);

        auto &idx0 = op.inner_ops.get<0>();
        auto &rcvr = idx0.rcvr; // basic_receiver
        using Tag = decltype(idx0.rcvr)::tag_t;
        static_assert(
            std::is_same_v<
                mcs::execution::adapt::__then_t<mcs::execution::recv::set_value_t>, Tag>);
        auto e [[maybe_unused]] = rcvr.get_env();
        // auto sch = ex::queries::get_scheduler(e); // NOTE: 没有转发过去

        // NOTE: 结论 从 revr 拿到的 调度信息。来自最顶层的 revr. 丢失
        // NOTE: 从 sndr 能找到。 recv 却不行
        {
            auto op [[maybe_unused]] =
                ex::conn::connect(ex::schedule(pool.get_scheduler()), receiver_all{});
            // auto sch = ex::queries::get_scheduler(ex::get_env(op.rcvr)); //NOTE:
            // 语法错误 auto &idx0 = op.inner_ops.get<0>(); //NOTE: 语法错误

            (void)op;
        }
    };
    TEST("env from ex::starts_on") = [&] {
        std::thread::id t_id;
        bool called{};
        auto task = ex::just() | ex::then([&]() {
                        called = true;
                        t_id = std::this_thread::get_id();
                    });
        auto sndr = ex::starts_on(pool.get_scheduler(), task);
        // NOTE: ex::starts_on 也不传递？
        // auto sched = ex::queries::get_scheduler(ex::get_env(sndr));
        // EXPECT(sched == pool.get_scheduler());

        auto op = ex::conn::connect(sndr, receiver_all{});

        using Tag = decltype(op.inner_ops.get<0>().rcvr)::tag_t;
        static_assert(
            std::is_same_v<
                mcs::execution::adapt::__let_t<mcs::execution::recv::set_value_t>, Tag>);
        // auto e = ex::get_env(rcvr);
        // auto sched = ex::queries::get_scheduler(ex::get_env(rcvr));

        ex::start(op);
        while (not called)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        EXPECT(t_id == pool[0].thread_id()); // NOTE: sndr 手动也可以。 要等等

        mcs::this_thread::sync_wait(sndr); // NOTE: 启动后才成功.....
        EXPECT(t_id == pool[0].thread_id());
    };

    TEST("env from ex::starts_on + row start") = [&] {
        auto &context = pool[0];
        std::thread::id t_id;
        auto [thread_id]{mcs::this_thread::sync_wait(
                             ex::schedule(context.get_scheduler()) |
                             ex::then([&] { return std::this_thread::get_id(); }))
                             .value_or(std::tuple{std::thread::id{}})};

        bool called{};
        auto task = ex::just() | ex::then([&]() {
                        called = true;
                        t_id = std::this_thread::get_id();
                    });
        auto sndr = ex::starts_on(context.get_scheduler(), task);
        auto op = ex::connect(sndr, receiver_all{});

        ex::start(op);

        while (not called)
            std::this_thread::sleep_for(std::chrono::milliseconds(1));

        assert(t_id == thread_id); // NOTE: 没有未定义行为
    };

    TEST("env from ex::starts_on + row start + ex::task<>") = [&] {
        auto &context = pool[0];
        std::thread::id t_id;
        auto main_id = std::this_thread::get_id();
        auto [thread_id]{mcs::this_thread::sync_wait(
                             ex::schedule(context.get_scheduler()) |
                             ex::then([&] { return std::this_thread::get_id(); }))
                             .value_or(std::tuple{std::thread::id{}})};

        bool called{};
        auto task = []() -> ex::task<> {
            co_return;
        }() | ex::then([&]() {
                                t_id = std::this_thread::get_id();
                                called = true;
                            });
        auto sndr = ex::starts_on(context.get_scheduler(), std::move(task));
        auto op = ex::connect(std::move(sndr), receiver_all{});

        ex::start(op);

        while (not called)
            std::this_thread::sleep_for(std::chrono::milliseconds(1));

        assert(t_id == thread_id);
    };

    TEST("get_scheduler from sndr") = [] {
        auto has_scheduler_type = [](const ex::sender auto &sndr) constexpr {
            return requires() { ex::queries::get_scheduler(ex::get_env(sndr)); };
        };
        auto env_has_scheduler_type = [](const auto &env) constexpr {
            return requires() { ex::queries::get_scheduler(env); };
        };
        auto sndr0 = ex::just();
        static_assert(std::is_same_v<decltype(ex::get_env(sndr0)), ex::env<>>);
        static_assert(std::is_same_v<ex::empty_env, ex::env<>>);
        // ex::queries::get_scheduler(ex::get_env(sndr0));
        static_assert(not has_scheduler_type(sndr0));

        ex::static_thread_pool<1> pool;
        auto sndr1 = ex::starts_on(pool[0].get_scheduler(), std::move(ex::just()));
        auto env = ex::get_env(sndr1); // NOTE: transform_env 才有用

        static_assert(not has_scheduler_type(sndr1));
        static_assert(
            std::same_as<ex::snd::tag_of_t<decltype(std::move(sndr1))>, ex::starts_on_t>);
        static_assert(ex::snd::sender_for<decltype(std::move(sndr1)), ex::starts_on_t>);
        // NOTE: ex::starts_on 是 constexpr 变量 要求 transform_env 被 const 修饰
        auto s_env = auto{ex::starts_on}.transform_env(std::move(sndr1), std::move(env));
        static_assert(env_has_scheduler_type(s_env));
    };

    return 0;
}
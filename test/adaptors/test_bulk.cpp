#include "../test_base_head.hpp"
#include <iostream>
#include <numeric>
#include <string>
#include <string_view>
#include <thread>
#include <tuple>
#include <utility>

template <class Shape, int N, int (&Counter)[N]> // NOLINT
void function(Shape i)                           // NOLINT
{
    Counter[i]++; // NOLINT
}

template <class Shape>
struct function_object_t
{
    int *Counter; // NOLINT

    // Note: const 必须添加。因为 bulk内部的 lambda, 没有 mutable
    void operator()(Shape i) const
    {
        Counter[i]++;
    }
};

int main()
{
    TEST("bulk returns a sender") = [] {
        auto snd = ex::bulk(ex::just(1), std::execution::par, 2, [](int, int) {});
        static_assert(ex::sender<decltype(snd)>);
    };

    TEST("bulk with environment returns a sender") = [] {
        auto snd = ex::bulk(ex::just(1), std::execution::par, 2, [](int, int) {});
        static_assert(ex::sender_in<decltype(snd), ex::empty_env>);
    };

    TEST("bulk can be piped") = [] {
        ex::sender auto snd [[maybe_unused]] =
            ex::just() | ex::bulk(std::execution::par, 2, [](int) {});
    };

    TEST("bulk keeps values_type from input sender") = [] {
        constexpr int n = 4; // NOLINT
        {
            auto pre_sndr = ex::just();
            auto snd = pre_sndr | ex::bulk(std::execution::par, n, [](int) {});
            using Pre_Sndr = decltype(pre_sndr);
            using Sndr = decltype(snd);
            static_assert(std::is_same_v<ex::cmplsigs::value_types_of_t<Pre_Sndr>,
                                         ex::cmplsigs::value_types_of_t<Sndr>>);
        }
        {
            auto pre_sndr = ex::just(1.0);
            auto snd = pre_sndr | ex::bulk(std::execution::par, n, [](int, double) {});
            using Pre_Sndr = decltype(pre_sndr);
            using Sndr = decltype(snd);
            static_assert(std::is_same_v<ex::cmplsigs::value_types_of_t<Pre_Sndr>,
                                         ex::cmplsigs::value_types_of_t<Sndr>>);
        }
        {
            auto pre_sndr = ex::just(1.0, std::string{});
            auto snd = pre_sndr | ex::bulk(std::execution::par, n,
                                           [](int, double, const std::string &) {});
            using Pre_Sndr = decltype(pre_sndr);
            using Sndr = decltype(snd);
            static_assert(std::is_same_v<ex::cmplsigs::value_types_of_t<Pre_Sndr>,
                                         ex::cmplsigs::value_types_of_t<Sndr>>);
        }
    };

    TEST("bulk keeps error_types from input sender") = [] {
        constexpr int n = 4; // NOLINT

        auto pre_sndr = ex::just_error(std::string{"error"});
        auto snd = pre_sndr | ex::bulk(std::execution::par, n, [](int) {});
        using Pre_Sndr = decltype(pre_sndr);
        using Sndr = decltype(snd);

        using E_pS = ex::cmplsigs::error_types_of_t<Pre_Sndr, ex::empty_env, std::tuple>;
        using E_S = ex::cmplsigs::error_types_of_t<Sndr, ex::empty_env, std::tuple>;
        using T = decltype(std::tuple_cat(std::declval<E_pS>()));
        // NOTE: bulk fwd error_types
        static_assert(std::is_same_v<E_S, T>);
        static_assert(std::is_same_v<E_S, E_pS>);
    };

    TEST("bulk can be used with a function") = [] {
        constexpr int n = 9;     // NOLINT
        static int counter[n]{}; // NOLINT

        bool called{false};
        test::channel c{test::channel::NO_CALL};
        std::fill_n(counter, n, 0);

        ex::sender auto snd =
            ex::just() | ex::bulk(std::execution::par, n, function<int, n, counter>);
        auto op = ex::connect(snd, test::void_receiver{.called = &called, .chanel = &c});
        start(op);

        // invokes f(i, args...) for every i of type Shape
        for (int i : counter)
        {
            EXPECT(i == 1);
        }
        EXPECT(called);
        EXPECT(c == test::channel::VALUE_CHANNEL);
    };

    TEST("bulk can be used with a function object") = [] {
        bool called{false};
        test::channel c{test::channel::NO_CALL};

        constexpr int n = 9; // NOLINT
        int counter[n]{0};   // NOLINT
        std::fill_n(counter, n, 0);

        function_object_t<int> fn{counter};
        ex::sender auto snd = ex::just() | ex::bulk(std::execution::par, n, fn);

        auto op = ex::connect(snd, test::void_receiver{.called = &called, .chanel = &c});
        start(op);

        for (int i : counter)
        {
            EXPECT(i == 1);
        }
        EXPECT(called);
        EXPECT(c == test::channel::VALUE_CHANNEL);
    };

    TEST("bulk can be used with a lambda") = [] {
        bool called{false};
        test::channel c{test::channel::NO_CALL};
        constexpr int n = 9; // NOLINT

        // int counter[n]{0};   // NOLINT
        std::array<int, n> counter{}; // NOLINTNEXTLINE
        ex::sender auto snd =
            ex::just() | ex::bulk(std::execution::par, n, [&](int i) { counter[i]++; });

        auto op = ex::connect(snd, test::void_receiver{.called = &called, .chanel = &c});
        start(op);

        for (int i : counter)
        {
            EXPECT(i == 1);
        }
        EXPECT(called);
        EXPECT(c == test::channel::VALUE_CHANNEL);
    };

    TEST("bulk forwards values") = [] {
        constexpr int n = 9;             // NOLINT
        constexpr int magic_number = 42; // NOLINT
        std::array<int, n> counter{};    // NOLINT

        auto snd = ex::just(magic_number) //
                   | ex::bulk(std::execution::par, n, [&](int i, int val) {
                         if (val == magic_number)
                         {
                             counter.at(i)++;
                         }
                     });

        bool called{false};
        test::channel c{test::channel::NO_CALL};
        std::any any;
        auto op = ex::connect(
            snd, test::any_receiver{.called = &called, .data = &any, .chanel = &c});
        start(op);

        for (int i : counter)
        {
            EXPECT(i == 1);
        }
        EXPECT(called);
        EXPECT(c == test::channel::VALUE_CHANNEL);

        auto [ret] = std::any_cast<std::tuple<int>>(any);
        EXPECT(ret == magic_number);
    };

    TEST("bulk forwards values that can be taken by reference") = [] {
        constexpr std::size_t n = 9; // NOLINT
        std::vector<int> vals(n, 0);
        std::vector<int> vals_expected(n);
        // iota 生成一个从起始值开始的连续序列。
        std::ranges::iota(vals_expected, 0);

        auto snd = ex::just(std::move(vals)) //
                   | ex::bulk(std::execution::par, n,
                              [&](std::size_t i, std::vector<int> &vals) {
                                  vals[i] = static_cast<int>(i);
                              });

        bool called{false};
        test::channel c{test::channel::NO_CALL};
        std::any any;
        auto op = ex::connect(
            snd, test::any_receiver{.called = &called, .data = &any, .chanel = &c});
        start(op);

        EXPECT(called);
        EXPECT(c == test::channel::VALUE_CHANNEL);

        auto &&[ret] = std::any_cast<std::tuple<std::vector<int>>>(any);
        for (int i = 0; i < n; ++i)
        {
            EXPECT(vals_expected[i] == ret[i]);
        }
    };

    TEST("bulk can throw, and set_error will be called") = [] {
        constexpr int n = 2; // NOLINT

        auto snd = ex::just() //
                   | ex::bulk(std::execution::par, n,
                              [](int) -> int { throw std::logic_error{"err"}; });
        bool called{false};
        test::channel c{test::channel::NO_CALL};
        std::any any;
        auto op = ex::connect(
            snd, test::any_receiver{.called = &called, .data = &any, .chanel = &c});
        start(op);

        EXPECT(called);
        EXPECT(c == test::channel::ERROR_CHANNEL);

        try
        {
            auto ret = std::any_cast<std::exception_ptr>(any);
            std::rethrow_exception(ret);
        }
        catch (const std::logic_error &e)
        {
            EXPECT(std::string_view(e.what()) == "err");
        }
    };

    TEST("bulk function is not called on error") = [] {
        constexpr int n = 2; // NOLINT
        int count{};

        auto snd = ex::just_error(std::string{"err"}) |
                   ex::bulk(std::execution::par, n, [&count](int, auto &) { count++; });

        bool called{false};
        test::channel c{test::channel::NO_CALL};
        std::any any;
        auto op = ex::connect(
            snd, test::any_receiver{.called = &called, .data = &any, .chanel = &c});
        start(op);

        EXPECT(called);
        EXPECT(count == 0);
        EXPECT(c == test::channel::ERROR_CHANNEL);

        auto ret = std::any_cast<std::string>(any);
        EXPECT(ret == std::string_view{"err"});
    };

    TEST("bulk function in not called on stop") = [] {
        constexpr int n = 2; // NOLINT
        int count{};

        auto snd = ex::just_stopped() |
                   ex::bulk(std::execution::par, n, [&count](int) { count++; });

        bool called{false};
        test::channel c{test::channel::NO_CALL};
        std::any any;
        auto op = ex::connect(
            snd, test::any_receiver{.called = &called, .data = &any, .chanel = &c});
        start(op);

        EXPECT(called);
        EXPECT(count == 0);
        EXPECT(c == test::channel::STOPDE_CHANNEL);
    };

    TEST("default bulk works with non-default constructible types") = [] {
        ex::sender auto s = ex::just(non_default_constructible{1}) |
                            ex::bulk(std::execution::par, 1, [](int, auto &) {});
        auto [ret] = mcs::this_thread::sync_wait(s).value();
        EXPECT(ret == non_default_constructible{1});
    };

    TEST("static thread pool works with non-default constructible types") = [] {
        ex::static_thread_pool<4> pool{};
        ex::scheduler auto sch = pool.get_scheduler();

        auto m_id = std::this_thread::get_id();
        ex::sender auto s = ex::just(non_default_constructible{1}) |
                            ex::continues_on(sch) |
                            ex::bulk(std::execution::par, 3, [&](int index, auto &) {
                                std::cout << "ex::bulk: index: " << index << '\n';

                                EXPECT(m_id != std::this_thread::get_id());
                            });
        {
            // Note: 增加是因为 p3481r1 impls-for<bulk_t>::complete
            // 原版设计有问题，改成bulk_chunked_t或bulk_unchunked_t 结构就行了
            using Sig = ex::cmplsigs::completion_signatures_for<
                decltype(ex::just(non_default_constructible{1}) | ex::continues_on(sch)),
                ex::empty_env>;
            static_assert(
                std::is_same_v<ex::cmplsigs::completion_signatures<ex::recv::set_value_t(
                                   non_default_constructible)>,
                               Sig>);
        }
        auto [ret] = mcs::this_thread::sync_wait(s).value();
        EXPECT(ret == non_default_constructible{1});
    };

    TEST("bulk_chunked") = [] {
        ex::static_thread_pool<4> pool{};
        ex::scheduler auto sch = pool.get_scheduler();

        auto m_id = std::this_thread::get_id();
        ex::sender auto s =
            ex::just(non_default_constructible{1}) | ex::continues_on(sch) |
            ex::bulk_chunked(std::execution::par, 3, [&](int begin, int end, auto &) {
                std::cout << "ex::bulk: begin,end: (" << begin << "," << end << ")\n";
                EXPECT(m_id != std::this_thread::get_id());
            });
        auto [ret] = mcs::this_thread::sync_wait(s).value();
        EXPECT(ret == non_default_constructible{1});
    };
    TEST("bulk_unchunked") = [] {
        ex::static_thread_pool<4> pool{};
        ex::scheduler auto sch = pool.get_scheduler();

        auto m_id = std::this_thread::get_id();
        ex::sender auto s =
            ex::just(non_default_constructible{1}) | ex::continues_on(sch) |
            ex::bulk_unchunked(std::execution::par, 3, [&](int index, auto &) {
                std::cout << "ex::bulk: index: " << index << '\n';
                EXPECT(m_id != std::this_thread::get_id());
            });

        auto [ret] = mcs::this_thread::sync_wait(s).value();
        EXPECT(ret == non_default_constructible{1});
    };
    // TODO(mcs) bulk_xxx 没有使用到std::execution::par。是给编译器厂商优化用的的吧
    // for 循环 比 std::for_each + par 强没想到吧

    return 0;
}
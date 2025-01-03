#include "../test_base_head.hpp"
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

    void operator()(Shape i)
    {
        Counter[i]++;
    }
};

int main()
{
    TEST("bulk returns a sender") = [] {
        auto snd = ex::bulk(ex::just(1), 2, [](int, int) {});
        static_assert(ex::sender<decltype(snd)>);
    };

    TEST("bulk with environment returns a sender") = [] {
        auto snd = ex::bulk(ex::just(1), 2, [](int, int) {});
        static_assert(ex::sender_in<decltype(snd), ex::empty_env>);
    };

    TEST("bulk can be piped") = [] {
        ex::sender auto snd [[maybe_unused]] = ex::just() | ex::bulk(2, [](int) {});
    };

    TEST("bulk keeps values_type from input sender") = [] {
        constexpr int n = 4; // NOLINT
        {
            auto pre_sndr = ex::just();
            auto snd = pre_sndr | ex::bulk(n, [](int) {});
            using Pre_Sndr = decltype(pre_sndr);
            using Sndr = decltype(snd);
            static_assert(std::is_same_v<ex::cmplsigs::value_types_of_t<Pre_Sndr>,
                                         ex::cmplsigs::value_types_of_t<Sndr>>);
        }
        {
            auto pre_sndr = ex::just(1.0);
            auto snd = pre_sndr | ex::bulk(n, [](int, double) {});
            using Pre_Sndr = decltype(pre_sndr);
            using Sndr = decltype(snd);
            static_assert(std::is_same_v<ex::cmplsigs::value_types_of_t<Pre_Sndr>,
                                         ex::cmplsigs::value_types_of_t<Sndr>>);
        }
        {
            auto pre_sndr = ex::just(1.0, std::string{});
            auto snd = pre_sndr | ex::bulk(n, [](int, double, const std::string &) {});
            using Pre_Sndr = decltype(pre_sndr);
            using Sndr = decltype(snd);
            static_assert(std::is_same_v<ex::cmplsigs::value_types_of_t<Pre_Sndr>,
                                         ex::cmplsigs::value_types_of_t<Sndr>>);
        }
    };

    TEST("bulk keeps error_types from input sender") = [] {
        constexpr int n = 4; // NOLINT

        auto pre_sndr = ex::just_error(std::string{"error"});
        auto snd = pre_sndr | ex::bulk(n, [](int) {});
        using Pre_Sndr = decltype(pre_sndr);
        using Sndr = decltype(snd);

        using E_pS = ex::cmplsigs::error_types_of_t<Pre_Sndr, ex::empty_env, std::tuple>;
        using Add_S = std::tuple<std::exception_ptr>;

        using E_S = ex::cmplsigs::error_types_of_t<Sndr, ex::empty_env, std::tuple>;
        using T = decltype(std::tuple_cat(std::declval<Add_S>(), std::declval<E_pS>()));
        static_assert(std::is_same_v<E_S, T>);
    };

    TEST("bulk can be used with a function") = [] {
        constexpr int n = 9;     // NOLINT
        static int counter[n]{}; // NOLINT

        bool called{false};
        test::channel c{test::channel::NO_CALL};
        std::fill_n(counter, n, 0);

        ex::sender auto snd = ex::just() | ex::bulk(n, function<int, n, counter>);
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
        ex::sender auto snd = ex::just() | ex::bulk(n, fn);

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
        ex::sender auto snd = ex::just() | ex::bulk(n, [&](int i) { counter[i]++; });

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
                   | ex::bulk(n, [&](int i, int val) {
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
                   | ex::bulk(n, [&](std::size_t i, std::vector<int> &vals) {
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
                   | ex::bulk(n, [](int) -> int { throw std::logic_error{"err"}; });
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

        auto snd =
            ex::just_error(std::string{"err"}) | ex::bulk(n, [&count](int) { count++; });

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

        auto snd = ex::just_stopped() | ex::bulk(n, [&count](int) { count++; });

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
        ex::sender auto s =
            ex::just(non_default_constructible{1}) | ex::bulk(1, [](int, auto &) {});
        auto [ret] = mcs::this_thread::sync_wait(s).value();
        EXPECT(ret == non_default_constructible{1});
    };

    TEST("static thread pool works with non-default constructible types") = [] {
        ex::static_thread_pool<4> pool{};
        ex::scheduler auto sch = pool.get_scheduler();

        auto m_id = std::this_thread::get_id();
        ex::sender auto s =
            ex::just(non_default_constructible{1}) | ex::continues_on(sch) |
            ex::bulk(1, [&](int, auto &) { EXPECT(m_id != std::this_thread::get_id()); });
        mcs::this_thread::sync_wait(s);
    };

    return 0;
}

#include "../test_base_head.hpp"
#include <string>
#include <type_traits>
#include <optional>
#include <exception>

using namespace mcs::execution; // NOLINT

int main()
{

    TEST("single_sender ") = [] {
        // 满足 single_sender
        {
            auto snd = just() | then([] {});
            static_assert(snd::single_sender<decltype(snd), ex::empty_env>);
        }
        {
            auto snd = just(1) | then([](int) {});
            static_assert(snd::single_sender<decltype(snd), ex::empty_env>);
        }
        // Note: single_sender_value_type 要求 保证 单一类型的返回值即可
        {
            auto snd = just(1) | then([](int) { return 1.0; });
            static_assert(snd::single_sender<decltype(snd), ex::empty_env>);
            using Sndr = decltype(snd);
            using Env = empty_env;
            using V = cmplsigs::single_sender_value_type<Sndr, Env>;
            static_assert(std::is_same_v<V, double>);
        }
        {
            auto snd = just(1) | then([](int) { return std::string("hello"); });
            static_assert(snd::single_sender<decltype(snd), ex::empty_env>);
            using Sndr = decltype(snd);
            using Env = empty_env;
            using V = cmplsigs::single_sender_value_type<Sndr, Env>;
            static_assert(std::is_same_v<V, std::string>);
        }
        {
            auto snd = just(1) | then([](int) { return; });
            static_assert(snd::single_sender<decltype(snd), ex::empty_env>);
            using Sndr = decltype(snd);
            using Env = empty_env;
            using V = cmplsigs::single_sender_value_type<Sndr, Env>;
            static_assert(std::is_same_v<V, void>);
        }
    };

    TEST("stopped_as_optional: v => optional<v>") = [] {
        // Note: 一点问题都没有
        auto first = ex::just(1);
        using Sndr = decltype(first);
        using Env = decltype(ex::empty_env{});
        {
            using T0 = ex::cmplsigs::value_types_of_t<Sndr, std::decay_t,
                                                      std::type_identity_t, Env>;
            static_assert(std::is_same_v<T0, int>);
            using T1 =
                ex::cmplsigs::value_types_of_t<Sndr, std::tuple, std::variant, Env>;
            static_assert(std::is_same_v<std::variant<std::tuple<int>>, T1>);

            using T2 = ex::cmplsigs::value_types_of_t<Sndr, ex::decayed_tuple,
                                                      std::type_identity_t, Env>;
            static_assert(std::is_same_v<std::tuple<int>, T2>);
        }

        {
            auto snd = ex::stopped_as_optional(first);
            using Sndr = decltype(snd);
            using T0 = ex::cmplsigs::value_types_of_t<Sndr, std::decay_t,
                                                      std::type_identity_t, Env>;
            static_assert(std::is_same_v<T0, std::optional<int>>);

            using T1 =
                ex::cmplsigs::value_types_of_t<Sndr, std::tuple, std::variant, Env>;
            static_assert(std::is_same_v<std::variant<std::tuple<T0>>, T1>);

            using T2 = ex::cmplsigs::value_types_of_t<Sndr, ex::decayed_tuple,
                                                      std::type_identity_t, Env>;
            static_assert(std::is_same_v<std::tuple<T0>, T2>);
        }
    };
    // TODO(mcs): stopped_as_optional 或许要和 stop_source 一起才好用
    // ex::upon_stopped 更好用
    TEST("stopped_as_optional: set_value(v...) => optional<v...>") = [] {
        auto snd = ex::stopped_as_optional(ex::just(1, 1.0));
        using Env = decltype(ex::empty_env{});
        using Sndr = decltype(snd);

        using T0 =
            ex::cmplsigs::value_types_of_t<Sndr, std::decay_t, std::type_identity_t, Env>;
        static_assert(std::is_same_v<T0, std::optional<std::tuple<int, double>>>);

        using T1 = ex::cmplsigs::value_types_of_t<Sndr, std::tuple, std::variant, Env>;
        static_assert(std::is_same_v<std::variant<std::tuple<T0>>, T1>);
        using T2 = ex::cmplsigs::value_types_of_t<Sndr, ex::decayed_tuple,
                                                  std::type_identity_t, Env>;
        static_assert(std::is_same_v<std::tuple<T0>, T2>);

        auto [ret] = mcs::this_thread::sync_wait(snd).value();
        EXPECT(ret.has_value() == true);
        auto [a, b] = ret.value();
        EXPECT(a == 1);
        EXPECT(b == 1.0);
    };

    TEST("stopped_as_optional: set_value(v...) => optional<v...>") = [] {
        auto snd = ex::stopped_as_optional(ex::just_stopped() |
                                           ex::upon_stopped([] { return 1; }));
        using Env = decltype(ex::empty_env{});
        using Sndr = decltype(snd);

        using T0 =
            ex::cmplsigs::value_types_of_t<Sndr, std::decay_t, std::type_identity_t>;
        static_assert(std::is_same_v<T0, std::optional<int>>);

        using T1 = ex::cmplsigs::value_types_of_t<Sndr, std::tuple, std::variant>;
        static_assert(std::is_same_v<std::variant<std::tuple<T0>>, T1>);
        using T2 =
            ex::cmplsigs::value_types_of_t<Sndr, ex::decayed_tuple, std::type_identity_t>;
        static_assert(std::is_same_v<std::tuple<T0>, T2>);

        auto [ret] = mcs::this_thread::sync_wait(snd).value();
        EXPECT(ret.has_value() == true);
        EXPECT(ret.value() == 1);
    };

    TEST("stopped_as_optional: set_value(v...) => optional<v...>") = [] {
        auto s = ex::just(1) | ex::then([](int) noexcept { return; }) |
                 ex::let_value([]() noexcept { return just_stopped(); });
        using CS = snd::completion_signatures_of_t<decltype(s)>;
        static_assert(ex::snd::single_sender<decltype(s), ex::empty_env>);
        // auto snd = ex::stopped_as_optional(s); //NOTE: 编译期失败
        // using Env = decltype(ex::empty_env{});
        // using Sndr = decltype(snd);
        // static_assert(snd::single_sender<decltype(s), ex::empty_env>);
        // using CS0 = snd::completion_signatures_of_t<decltype(s), ex::empty_env>;
        // static_assert(std::is_same_v<CS0, cmplsigs::completion_signatures<
        //                                       mcs::execution::recv::set_stopped_t()>>);

        // NOTE: 编译失败，
        //  using CS1 = snd::completion_signatures_of_t<Sndr, Env>;
    };
    return 0;
}
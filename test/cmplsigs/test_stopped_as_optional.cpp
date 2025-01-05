
#include "../test_base_head.hpp"
#include <string>
#include <type_traits>
#include <optional>
#include <exception>

using namespace mcs::execution; // NOLINT

template <typename... T>
    requires(sizeof...(T) == 1)
using Collect_V_Sig =
    cmplsigs::completion_signatures<set_value_t(std::optional<std::decay_t<T>>...)>;

template <typename T>
using Collect_E_Sig = cmplsigs::completion_signatures<set_error_t(T)>;

int main()
{

    using Sigs = cmplsigs::completion_signatures<recv::set_value_t(int)>;
    TEST("CS 1") = [] {
        using T = tfxcmplsigs::transform_completion_signatures<
            Sigs, completion_signatures<set_error_t(std::exception_ptr)>, Collect_V_Sig,
            Collect_E_Sig, cmplsigs::completion_signatures<>>;
        // V => optional,stop 当作 optional not set value
        static_assert(
            tool::eq_set_sigs_v<T,
                                completion_signatures<set_value_t(std::optional<int>),
                                                      set_error_t(std::exception_ptr)>>);
    };

    TEST("CS 2") = [] {
        using T = tfxcmplsigs::transform_completion_signatures<
            Sigs,
            // handle repeat Es
            completion_signatures<set_error_t(std::exception_ptr),
                                  set_error_t(std::exception_ptr)>,
            Collect_V_Sig, Collect_E_Sig, cmplsigs::completion_signatures<>>;
        // V => optional,stop 当作 optional not set value
        static_assert(
            tool::eq_set_sigs_v<T,
                                completion_signatures<set_value_t(std::optional<int>),
                                                      set_error_t(std::exception_ptr)>>);
    };

    TEST("CS 3") = [] {
        using T = tfxcmplsigs::transform_completion_signatures<
            Sigs,
            // handle repeat Es
            completion_signatures<set_error_t(std::exception_ptr), set_value_t(),
                                  set_value_t(int, double)>,
            Collect_V_Sig, Collect_E_Sig, cmplsigs::completion_signatures<>>;
        // V => optional,stop 当作 optional not set value
        static_assert(tool::eq_set_sigs_v<
                      T, completion_signatures<set_value_t(std::optional<int>),
                                               set_value_t(int, double), set_value_t(),
                                               set_error_t(std::exception_ptr)>>);
    };
    TEST("CS 3") = [] {
        using Sigs = cmplsigs::completion_signatures<>;
        using T = tfxcmplsigs::transform_completion_signatures<
            Sigs, completion_signatures<set_error_t(std::exception_ptr)>, Collect_V_Sig,
            Collect_E_Sig, cmplsigs::completion_signatures<>>;
        // V => optional,stop 当作 optional not set value
        static_assert(
            tool::eq_set_sigs_v<T,
                                completion_signatures<set_error_t(std::exception_ptr)>>);

        // Note: single_sender,single_sender_value_type 比较宽松
        auto snd = just_stopped();
        using Sndr = decltype(snd);
        using Env = empty_env;
        using V = cmplsigs::single_sender_value_type<Sndr, Env>;
        static_assert(std::is_same_v<V, void>);
        {
            using Sigs [[maybe_unused]] = cmplsigs::completion_signatures<set_value_t()>;
            // 编译错误： Collect_V_Sig 实例化失败，不满足要求
            // using T = tfxcmplsigs::transform_completion_signatures<
            //     Sigs, completion_signatures<set_error_t(std::exception_ptr)>,
            //     Collect_V_Sig, Collect_E_Sig, cmplsigs::completion_signatures<>>;
            // Note: std::optional<void> 是不允许的; set_value_t() => std::optional<void>
            // std::optional<void> a{};
        }
        {
            using Sigs [[maybe_unused]] = cmplsigs::completion_signatures<>;
            // 这个却可以
            using T [[maybe_unused]] = tfxcmplsigs::transform_completion_signatures<
                Sigs, completion_signatures<set_error_t(std::exception_ptr)>,
                Collect_V_Sig, Collect_E_Sig, cmplsigs::completion_signatures<>>;
        }
        {
            using Sigs [[maybe_unused]] =
                cmplsigs::completion_signatures<set_value_t(int, double)>;
            // 这个也不可以
            // using T [[maybe_unused]] = tfxcmplsigs::transform_completion_signatures<
            //     Sigs, completion_signatures<set_error_t(std::exception_ptr)>,
            //     Collect_V_Sig, Collect_E_Sig, cmplsigs::completion_signatures<>>;
            // Note: std::optional<int, double> 是不允许的;
            // std::optional<int, double> a;
        }

        // 满足 single_sender
        static_assert(snd::single_sender<decltype(snd), ex::empty_env>);
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

    TEST("CS 4") = [] {
        // Note: 一点问题都没有
        auto first = ex::just(1);
        using Sndr = decltype(first);
        using Env = decltype(ex::empty_env{});
        {
            using T0 = ex::cmplsigs::value_types_of_t<Sndr, Env, std::decay_t,
                                                      std::type_identity_t>;
            static_assert(std::is_same_v<T0, int>);
            using T1 =
                ex::cmplsigs::value_types_of_t<Sndr, Env, std::tuple, std::variant>;
            static_assert(std::is_same_v<std::variant<std::tuple<int>>, T1>);

            using T2 = ex::cmplsigs::value_types_of_t<Sndr, Env, ex::decayed_tuple,
                                                      std::type_identity_t>;
            static_assert(std::is_same_v<std::tuple<int>, T2>);
        }

        {
            auto snd = ex::stopped_as_optional(first);
        }
    };
    return 0;
}
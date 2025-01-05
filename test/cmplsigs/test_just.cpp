#include "../test_base_head.hpp"
#include <algorithm>
#include <cassert>
#include <cstring>
#include <functional>
#include <type_traits>
#include <utility>

struct A
{
    double a{};
    double b{};
    double c{};
    std::string str;
};

int main()
{
    TEST("fun args trans") = [] {
        A a;
        auto fun = [&](A b) {
            EXPECT(&a != &b); // Note: 非0开销.移动开销
        };
        fun(std::move(a));

        {
            A a;
            auto fun = [&](A &&b) {
                EXPECT(&a == &b);
            };

            fun(std::move(a));
        }
        {
            A a;
            auto fun = [&](const A &b) {
                EXPECT(&a == &b);
            };
            fun(std::move(a));
        }
    };
    // Note: make_sender return 。args 的 生命周期早已转移到 sndr了。接收
    // Returns: A prvalue of type basic-sender<Tag, decay_t<Data>,decay_t<Child>...>
    TEST("fun args for int") = [] {
        auto fun = [](int a) {
        };
        auto a = 1;
        auto &refa = a;
        fun(1);
        fun(a);
        fun(refa);
        fun(std::move(a));
    };
    TEST("next fun args avoid lref args") = [] {
        auto fun = [](A a) {
        };
        auto a = A{};
        auto &refa = a;
        fun(A{});
        fun(a);
        fun(refa);
        fun(std::move(a));
        {
            auto a = A{};
            auto &refa = a;
            auto fun_R_ref = [](A &&a) {
            };
            fun_R_ref(A{});
            // fun_ref(a);
            // fun_ref(refa);
            fun_R_ref(std::move(a));
        }
        // Note: just的参数，不能用 A&接收。 A& 和A&& 不兼容
        {
            auto a = A{};
            auto &refa = a;
            auto fun_l_ref = [&](A &b) {
                assert(&a == &b);
            };
            // fun_l_ref(A{});
            fun_l_ref(a);
            fun_l_ref(refa);
            // fun_l_ref(std::move(a));
        }
        {
            auto a = A{};
            auto &refa = a;
            auto fun_cl_ref = [&](const A &b) {
            };
            fun_cl_ref(A{});
            fun_cl_ref(a);
            fun_cl_ref(refa);
            // 提示不会有作用
            fun_cl_ref(std::move(a));
        }
    };

    TEST("just base") = [] {
        {
            auto a = A{};
            auto sndr = ex::just(a);
            using T = decltype(sndr);

            using CS = ex::snd::completion_signatures_of_t<T>;
            static_assert(
                std::is_same_v<
                    CS, ex::cmplsigs::completion_signatures<ex::recv::set_value_t(A)>>);

            {
                auto sndr = ex::just(A{});
                using T = decltype(sndr);

                using CS = ex::snd::completion_signatures_of_t<T>;
                static_assert(std::is_same_v<CS, ex::cmplsigs::completion_signatures<
                                                     ex::recv::set_value_t(A)>>);
            }
        }
    };

    TEST("just lv") = [] {
        {
            int a = 1;
            auto sndr = ex::just(std::ref(a));
            using T = decltype(sndr);

            using CS = ex::snd::completion_signatures_of_t<T>;
            static_assert(
                std::is_same_v<CS,
                               ex::cmplsigs::completion_signatures<ex::recv::set_value_t(
                                   std::reference_wrapper<int>)>>);
            static_assert(not std::is_same_v<std::reference_wrapper<int>, int &>);

            {
                int a = 1;
                int &refa = a;
                auto sndr = ex::just(refa);
                using T = decltype(sndr);

                using CS = ex::snd::completion_signatures_of_t<T>;
                static_assert(std::is_same_v<CS, ex::cmplsigs::completion_signatures<
                                                     ex::recv::set_value_t(int)>>);
            }
        }
        {
            auto a = A{};
            auto sndr = ex::just(std::ref(a));
            using T = decltype(sndr);

            using CS = ex::snd::completion_signatures_of_t<T>;
            static_assert(
                std::is_same_v<CS,
                               ex::cmplsigs::completion_signatures<ex::recv::set_value_t(
                                   std::reference_wrapper<A>)>>);
            static_assert(not std::is_same_v<std::reference_wrapper<A>, A &>);
            {
                auto a = A{};
                auto &refa = a;
                auto sndr = ex::just(refa);
                using T = decltype(sndr);

                using CS = ex::snd::completion_signatures_of_t<T>;
                static_assert(std::is_same_v<CS, ex::cmplsigs::completion_signatures<
                                                     ex::recv::set_value_t(A)>>);
            }
        }
    };
    TEST("just rv") = [] {
        auto a = A{};
        auto &refa = a;
        auto sndr = ex::just(std::move(refa));
        using T = decltype(sndr);
        using CS = ex::snd::completion_signatures_of_t<T>;
        static_assert(std::is_same_v<
                      CS, ex::cmplsigs::completion_signatures<ex::recv::set_value_t(A)>>);
        {
            auto a = A{};
            auto sndr = ex::just(std::move(a));
            using T = decltype(sndr);
            using CS = ex::snd::completion_signatures_of_t<T>;
            static_assert(
                std::is_same_v<
                    CS, ex::cmplsigs::completion_signatures<ex::recv::set_value_t(A)>>);
        }
    };

    TEST("just mv") = [] {
        auto sndr = mcs::execution::factories::just(1, 1.0, 1.0F);
        using T = decltype(sndr);
        using CS = ex::snd::completion_signatures_of_t<T>;
        static_assert(std::is_same_v<CS, ex::cmplsigs::completion_signatures<
                                             ex::recv::set_value_t(int, double, float)>>);
    };

    TEST("just stop") = []() {
        auto sndr = ex::just_stopped();
        using T = decltype(sndr);
        using CS = ex::snd::completion_signatures_of_t<T>;
        static_assert(
            std::is_same_v<
                CS, ex::cmplsigs::completion_signatures<ex::recv::set_stopped_t()>>);
        using CS = ex::snd::completion_signatures_of_t<T>;
    };

    TEST("just stop") = []() {
        auto sndr = ex::just_stopped();
        using T = decltype(sndr);
        using CS = ex::snd::completion_signatures_of_t<T>;
        static_assert(
            std::is_same_v<
                CS, ex::cmplsigs::completion_signatures<ex::recv::set_stopped_t()>>);
        using CS = ex::snd::completion_signatures_of_t<T>;

        {
            // Note:
            auto sndr = ex::just_stopped() | ex::then([]() {});
            using T = decltype(sndr);
            // Note: 直接编译错误吗？
            // using CS = ex::snd::completion_signatures_of_t<T>;
            // using V =
            // ex::cmplsigs::__detail::filter_sigs_by_completion<ex::set_value_t,
            //                                                             CS>::type;
            // Note: 没有 set_value_t 的完成签名，不可能 sync_wait 成功
            // static_assert(std::is_same_v<V, ex::cmplsigs::completion_signatures<>>);
        }
    };
    return 0;
}
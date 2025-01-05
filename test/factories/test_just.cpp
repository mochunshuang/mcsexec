#include "../test_base_head.hpp"
#include <cassert>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <string>

int main()
{
    using namespace test;           // NOLINT
    using namespace mcs::execution; // NOLINT

    TEST("Simple test for just") = [] {
        bool called{false};
        auto o1 = connect(just(1), value_receiver(&called, 1));
        start(o1);
        auto o2 = connect(just(2), value_receiver(&called, 2));
        start(o2);
        auto o3 = connect(just(3), value_receiver(&called, 3));
        start(o3);

        auto o4 = connect(just(std::string("this")),
                          value_receiver(&called, std::string("this")));
        start(o4);
        auto o5 = connect(just(std::string("that")),
                          value_receiver(&called, std::string("that")));
        start(o5);
    };

    TEST("just returns a sender") = [] {
        using t = decltype(ex::just(1));
        static_assert(ex::sender<t>, "ex::just must return a sender");
        EXPECT(ex::sender<t> == true);
        EXPECT(ex::snd::enable_sender<t> == true);
    };

    TEST("just can handle multiple values") = [] {
        bool executed{false};
        auto f = [&](int x, double d) {
            EXPECT(x == 3);
            EXPECT(d == 0.14);
            executed = true;
        };
        auto op = ex::connect(ex::just(3, 0.14), make_fun_receiver(std::move(f)));
        start(op);
        EXPECT(executed);
    };

    TEST("value types are properly set for just") = [] {
        static_assert(std::is_same_v<cmplsigs::value_types_of_t<decltype(ex::just(1))>,
                                     std::variant<std::tuple<int>>>);
        static_assert(
            std::is_same_v<
                std::variant<std::tuple<int, double, std::string>>,
                cmplsigs::value_types_of_t<decltype(ex::just(1, 1.0, std::string{}))>>);
    };

    TEST("error types are properly set for just") = [] {
        using ET = cmplsigs::error_types_of_t<decltype(ex::just(1))>;
        static_assert(std::is_same_v<cmplsigs::error_types_of_t<decltype(ex::just(1))>,
                                     ex::cmplsigs::empty_variant>);
    };

    TEST("cpo for just") = [] {
        using CO = ex::snd::completion_signatures_of_t<decltype(ex::just(1, 1.0))>;
        static_assert(std::is_same_v<cmplsigs::completion_signatures<
                                         mcs::execution::recv::set_value_t(int, double)>,
                                     CO>);
    };

    TEST("called status for just") = [] {
        bool called{false};
        auto recv = test::value_receiver{&called, 1, 1.0};
        static_assert(recv::receiver<decltype(recv)>);

        EXPECT(not called);
        auto op = connect(just(1, 1.0), std::move(recv));
        EXPECT(not called);

        start(op);
        EXPECT(called);
    };

    TEST("sync_wait for just") = [] {
        // .value() 之后，才能 解绑定到局部变量
        auto [a, b] = mcs::this_thread::sync_wait(ex::just(1, 1.0)).value();
        EXPECT(a == 1);
        EXPECT(b == 1.0);
    };

    TEST("just and move_only_type ") = [] {
        auto [ret] = mcs::this_thread::sync_wait(ex::just(move_only_type{1})).value();
        EXPECT(ret.val == 1);
        // Note: conn::connect 无需校验 sndr，recr 了
#if 0 
        auto sndr = ex::just(move_only_type{1});
        using Sndr = decltype(sndr);
        // snd::apply_sender(snd::general::get_domain_early(sndr),
        //                   mcs::this_thread::sync_wait, ::std::forward<Sndr>(sndr));

        auto dom = snd::general::get_domain_early(sndr);
        // auto r = snd::apply_sender(dom, mcs::this_thread::sync_wait, std::move(sndr));
        // dom.apply_sender(mcs::execution::consumers::__sync_wait::sync_wait_t(),
        //                  std::move(sndr));
        auto t = mcs::execution::consumers::__sync_wait::sync_wait_t();
        // auto v = t.apply_sender(std::move(sndr));
        // static_assert(
        //     ex::snd::sender_to<
        //         Sndr,
        //         mcs::execution::consumers::__sync_wait::sync_wait_receiver<Sndr>>);

        using Recv = mcs::execution::consumers::__sync_wait::sync_wait_receiver<Sndr>;
        Recv rcvr;

        static_assert(std::constructible_from<std::remove_cvref_t<Sndr>, Sndr>);
        {
            using old_Sndr = Sndr;
            using Sndr = decltype((sndr));
            static_assert(std::is_same_v<old_Sndr &, Sndr>);
            // Note: 不满足 snd::sender 的原因。 A& -> A ,走 A的复制构造，没有就G了
            static_assert(not std::constructible_from<std::remove_cvref_t<Sndr>, Sndr>);
        }
        // static_assert(snd::sender<decltype((sndr))>);

        // static_assert(snd::sender<decltype((sndr))> && receiver<decltype((rcvr))>);

        // ex::conn::connect(std::move(sndr), std::move(rcvr));
#endif
    };

    return 0;
}
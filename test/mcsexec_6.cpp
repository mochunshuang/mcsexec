
#include "../include/execution.hpp"

#include <iostream>
#include <tuple>
#include <utility>

using namespace mcs::execution;

// temp

template <typename Completion, typename Sigs>
using filter_sigs_by_completion = cmplsigs::__detail::tpl_param_trnsfr_t<
    cmplsigs::completion_signatures,
    typename cmplsigs::__detail::filter_tuple<
        cmplsigs::__detail::select_tag<Completion>::template predicate,
        cmplsigs::__detail::tpl_param_trnsfr_t<std::tuple, Sigs>>::type>;

template <typename Fun, typename Sig>
struct compute_fun_result;

template <typename Fun, typename Completion, typename... Sig>
    requires(requires() { typename functional::call_result_t<Fun, Sig...>; })
struct compute_fun_result<Fun, cmplsigs::completion_signatures<Completion(Sig...)>>
{
    using type = functional::call_result_t<Fun, Sig...>;
};

template <typename Fun, typename Completion, typename Sig>
struct compute_let_sigs;
template <typename Fun, typename Completion, typename... Sig>
struct compute_let_sigs<Fun, Completion, cmplsigs::completion_signatures<Sig...>>
{
    using type =
        filter_sigs_by_completion<Completion, cmplsigs::completion_signatures<Sig...>>;
};

template <typename Sender, typename Env>
struct completion_signatures_for_impl_2;
template <typename Completion, typename Fun, typename Sender, typename Env>
struct completion_signatures_for_impl_2<
    snd::__detail::basic_sender<adapt::__let_t<Completion>, Fun, Sender>, Env>
{
    using type = compute_let_sigs<Fun, Completion,
                                  snd::completion_signatures_of_t<Sender, Env>>::type;
    using type_fun = Fun;
};

//
template <typename Sender, typename Env>
struct completion_signatures_for_impl_3;
template <typename Completion, typename Fun, typename Sender, typename Env>
struct completion_signatures_for_impl_3<
    snd::__detail::basic_sender<adapt::__let_t<Completion>, Fun, Sender>, Env>
{
    using type = snd::completion_signatures_of_t<
        typename compute_fun_result<
            Fun, typename compute_let_sigs<
                     Fun, Completion,
                     snd::completion_signatures_of_t<Sender, Env>>::type>::type,
        Env>;
};

int main()
{
    {
        auto sndr = just(1) | then([](int a) {
                        std::cout << "then: " << a << "\n";
                        return a;
                    });
        auto fun = [&]() {
            return sndr;
        };
        // 只有 mcs::execution::recv::set_value_t (int)
        // TODO(mcs): 是不是then 的签名有问题。需要添加just的完成签名吗
        using T = decltype(sndr.get_completion_signatures(empty_env{}));
        static_thread_pool<3> thread_pool;
        scheduler auto sched6 = thread_pool.get_scheduler();
        sender auto start = schedule(sched6);
        sender auto task = let_value(start, fun);
        // 实验性计算
        {
            // mcs::execution::cmplsigs::completion_signatures<mcs::execution::recv::set_value_t
            // (), mcs::execution::recv::set_error_t
            // (std::__exception_ptr::exception_ptr), mcs::execution::recv::set_stopped_t
            // ()>
            // Note: public runloop的内部类才行
#if 0
            using T0 = completion_signatures_for_impl_2<decltype(task), empty_env>::type;
            using T1 =
                completion_signatures_for_impl_2<decltype(task), empty_env>::type_fun;
            static_assert(std::is_same_v<decltype(fun), T1>);

            using T2 = compute_fun_result<T1, T0>::type;

            using T3 =
                decltype(std::declval<T2>().get_completion_signatures(empty_env{}));
            using T3 = snd::completion_signatures_of_t<T2>;

            using R = completion_signatures_for_impl_3<decltype(task), empty_env>::type;

            static_assert(std::is_same_v<T3, R>);
            static_assert(
                std::is_same_v<T3, cmplsigs::completion_signatures<
                                       mcs::execution::recv::set_value_t(int)>>);
#endif
        }
    }
    {
        // Note: 因为 start 传来的是空的 参数，因此函数的参数列表必须是空的
        auto fun = []() -> sender auto {
            std::cout << "let_value:  \n";
            return just(1) | then([](int a) {
                       std::cout << "then: " << a << "\n";
                       return a;
                   });
        };

        static_thread_pool<3> thread_pool;
        scheduler auto sched6 = thread_pool.get_scheduler();
        sender auto start = schedule(sched6);
        sender auto task = let_value(start, fun);
        auto [a] = mcs::this_thread::sync_wait(task).value();
        std::cout << "sync_wait: " << a << "\n";

        {
            using Sndr = decltype(task);
            using Rcvr = mcs::execution::consumers::__sync_wait::sync_wait_receiver<Sndr>;
            using impl = snd::general::impls_for<snd::tag_of_t<Sndr>>;

            Rcvr r;
            // using Sigs = decltype(impl::get_state(task, r));
            auto [fn, env, args, ops2] = impl::get_state(task, r);
            using Fn = decltype(fn);
            using Evn = decltype(env);
            using Args = decltype(args);
            // TODO(mcs): 算错签名了吗？
            static_assert(
                std::is_same_v<std::variant<std::monostate, std::tuple<>>, Args>);
            using Ops2 = decltype(ops2);
        }
    }
    std::cout << "hello world\n";
    return 0;
}
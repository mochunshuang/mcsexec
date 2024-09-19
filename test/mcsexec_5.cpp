#include "../include/execution.hpp"

#include <iostream>
#include <tuple>
#include <utility>

using namespace mcs::execution;

// template
namespace __datail
{

    template <typename _Sig>
    struct as_tuple;
    template <typename Tag, typename... Args>
    struct as_tuple<Tag(Args...)>
    {
        using type = ::mcs::execution::decayed_tuple<Args...>;
    };
    //
    template <typename Fn, typename... Args>
    using as_sndr2 = functional::call_result_t<Fn, std::decay_t<Args> &...>;

    template <typename Fn, typename Sig, typename Rcvr, typename Env>
    struct compute_connect_result_t;

    template <typename Fn, typename Rcvr, typename Env, typename Tag, typename... Args>
        requires(requires() { typename as_sndr2<Fn, Args...>; })
    struct compute_connect_result_t<Fn, Tag(Args...), Rcvr, Env>
    {
        using type =
            conn::connect_result_t<as_sndr2<Fn, Args...>, adapt::receiver2<Rcvr, Env>>;
    };

} // namespace __datail
template <typename Sigs>
struct compute_args_variant_t;
template <typename... Sig>
struct compute_args_variant_t<std::tuple<Sig...>>
{
    using type = typename tfxcmplsigs::unique_variadic_template<
        std::variant<std::monostate, typename __datail::as_tuple<Sig>::type...>>::type;
};
template <typename Fn, typename Completion, typename Rcvr, typename Env>
struct compute_ops2_variant_t;

template <typename Fn, typename Rcvr, typename Env, typename... Sig>
struct compute_ops2_variant_t<Fn, std::tuple<Sig...>, Rcvr, Env>
{

    // using type = std::tuple<Sig...>;
    using type = typename tfxcmplsigs::unique_variadic_template<std::variant<
        std::monostate,
        typename __datail::compute_connect_result_t<Fn, Sig, Rcvr, Env>::type...>>::type;
};
/*
`let_value`、`let_error` 和 `let_stopped`
分别将发送者的值、错误和停止完成转换为一个新的子异步操作。
它们通过将发送者的结果数据传递给用户指定的可调用对象，该可调用对象返回一个新的发送者，【然后连接并启动该发送者】。
*/
int main()
{
    using Completion = mcs::execution::recv::set_value_t;
    auto fun = []() -> sender auto {
        std::cout << "let_value:  \n";
        return just();
    };
    constexpr auto let_env = adapt::let_env_t<Completion>();
    static_thread_pool<3> thread_pool;
    scheduler auto sched6 = thread_pool.get_scheduler();
    sender auto start = schedule(sched6);
    sender auto task = let_value(start, fun);

    {

        using Sndr = decltype(task);
        using Env = mcs::execution::empty_env;

        // start

        using s_s = snd::completion_signatures_of_t<decltype(start), Env>;
        using s_t = snd::completion_signatures_of_t<Sndr, Env>;
        {
            using Tag = snd::tag_of_t<Sndr>;
            using Tag2 = snd::tag_of_t<decltype(start)>;
            using Completion = mcs::execution::recv::set_value_t;

            using __let_t = mcs::execution::adapt::__let_t<Completion>;

            static_assert(snd::sender_for<Sndr, __let_t>); // 概念是合法的
            auto l_env = adapt::let_env_t<Completion>{};
            mcs::execution::empty_env{};

            // TODO(mcs): 但是 SCHED_ENV 需要 domain. 从 childen找
            // 会是 just吗？？
            auto a = snd::general::SCHED_ENV(
                queries::get_completion_scheduler<Completion>(queries::get_env(task)));

            auto e = l_env(task);

            // auto s = let_value.transform_env(task, __let_t{});
        }

        static_assert(
            cmplsigs::__detail::has_completion_type<
                mcs::execution::cmplsigs::completion_signatures_for_impl, Sndr, Env>);

        // 未实现
        using T = decltype(task.get_completion_signatures(Env{}));

        using S_T = cmplsigs::value_types_of_t<
            Sndr, mcs::execution::consumers::__sync_wait::sync_wait_env,
            mcs::execution::decayed_tuple, std::type_identity_t>;

        using Rcvr = mcs::execution::consumers::__sync_wait::sync_wait_receiver<Sndr>;

        // static_assert(
        //      snd::sender_to<
        //         Sndr,
        //         mcs::execution::consumers::__sync_wait::sync_wait_receiver<Sndr>>);

        static_assert(sender_in<Sndr, queries::env_of_t<Rcvr>>);
        static_assert(
            recv::receiver_of<
                Rcvr, snd::completion_signatures_of_t<Sndr, queries::env_of_t<Rcvr>>>);

        // connect
        // static_assert(mcs::execution::snd::__detail::valid_specialization<
        //               mcs::execution::snd::__detail::mate_type::state_type, Sndr,
        //               Rcvr>);

        using impl = snd::general::impls_for<snd::tag_of_t<Sndr>>;

        // impl::get_state(Sndr &&sndr, Rcvr &)
        Rcvr r{};
        {
            // using Sigs = decltype(impl::get_state_test(task, r));
            using Sigs = mcs::execution::cmplsigs::completion_signatures<
                mcs::execution::recv::set_value_t(),
                mcs::execution::recv::set_error_t(std::__exception_ptr::exception_ptr),
                mcs::execution::recv::set_stopped_t()>;

            using LetSigs = typename mcs::execution::cmplsigs::__detail::filter_tuple<
                mcs::execution::cmplsigs::__detail::select_tag<
                    Completion>::template predicate,
                cmplsigs::__detail::tpl_param_trnsfr_t<std::tuple, Sigs>>::type;
            {
                // filter_Sigs f(); // 不行确实
            }

            // maybe you need that
            static_assert(
                std::is_same_v<std::tuple<mcs::execution::recv::set_value_t()>, LetSigs>);
            //
            using u = typename compute_args_variant_t<LetSigs>::type;
            // u uu{}; // 不行确实,variant 使用的时候，要确定是哪个类型
            // 显式地指定初始化为 std::monostate
            // u uu = std::monostate{};
            // u uu = std::tuple<>{};
            {
                {
                    // empty_env
                    using Env = decltype(let_env(just())); // start is let-cpo child
                    // TODO(mcs): just是没用这个domain的
                    // auto d = get_domain(get_env(just())); // 也算是不行的
                }
                using Fn = decltype(fun);
                using Env = decltype(let_env(start)); // start is let-cpo child

                using J = decltype(just());
                using sndr2 = mcs::execution::snd::__detail::basic_sender<
                    mcs::execution::factories::__just_t<
                        mcs::execution::recv::set_value_t>,
                    mcs::execution::snd::__detail::product_type<>>;
                static_assert(std::is_same_v<J, sndr2>); // ok

                using lambda_t = Fn; // receiver2 是和 let_value 连接的 receiver
                using receiver2 = mcs::execution::adapt::receiver2<
                    mcs::execution::consumers::__sync_wait::sync_wait_receiver<
                        mcs::execution::snd::__detail::basic_sender<
                            mcs::execution::adapt::__let_t<
                                mcs::execution::recv::set_value_t>,
                            lambda_t, mcs::execution::ctx::run_loop::scheduler::sender>>,
                    mcs::execution::snd::general::SCHED_ENV<
                        mcs::execution::ctx::run_loop::scheduler>>;
                // Note: 关键在签名上
                //  测试 sync_wait_result_type
                {
                    // Note: 目前直接用 fun的结果，做签名是错误的
                    using T =
                        mcs::execution::consumers::__sync_wait::sync_wait_result_type<
                            decltype(task)>;

                    // Note 从 Sndr 获取器其签名
                    // completion_signatures<mcs::execution::recv::set_value_t (int)>
                    using Sig = decltype(std::declval<decltype(just() | then([]() {
                                                                   return 1;
                                                               }))>()
                                             .get_completion_signatures(empty_env{}));

                    // 看看普通的
                    {
                        using T =
                            mcs::execution::consumers::__sync_wait::sync_wait_result_type<
                                decltype(just())>;
                        static_assert(
                            std::is_same_v<T,
                                           mcs::execution::consumers::__sync_wait::
                                               sync_wait_result_type<decltype(just())>>);
                    }
                    // Note: 目前 let_value(start, fun) 的完成签名是 fun()==sndr 是错误的
                    // Note:  应该 继续提取 sndr 的返回签名。需要自定义签名
                }
                //  connect
                {
                    auto sched_env = snd::general::SCHED_ENV(
                        queries::get_completion_scheduler<Completion>(
                            queries::get_env(start))); // 合法的

                    // run_loop::env
                    auto sndr_env = queries::get_env(start);
                    // TODO(mcs): 非法，确实没有定义，just 也没有
                    //  no domain  get_domain(get_env(start));
                    // Note: 修改SCHED_ENV可以了
                    using T = conn::connect_result_t<sndr2, receiver2>;
                    {
                        auto fun = []() -> sender auto {
                            return just(1) | then([](int a) { return a; });
                        };
                        using sndr2 = decltype(fun());
                        // set_value_t (int)。没用问题
                        using Sig =
                            decltype(fun().get_completion_signatures(empty_env{}));
                        using T2 = conn::connect_result_t<sndr2, receiver2>;
                    }
                }

                using ops2_variant_t =
                    typename compute_ops2_variant_t<Fn, LetSigs, Rcvr, Env>::type;
            }
        }
        {
            using Sigs = decltype(impl::get_state(task, r));
        }

        // using t = mcs::execution::snd::__detail::basic_operation<Sndr, Rcvr>; // 失败
        // auto op = task.connect(std::move(Rcvr{})); // 失败,看来得自定义了
        // auto p = conn::connect(std::forward<Sndr>(task), std::forward<Rcvr>(Rcvr{}));

        {
            auto d = snd::general::get_domain_early(task);
            // auto s = snd::apply_sender(d, mcs::this_thread::sync_wait,
            //                            ::std::forward<Sndr>(task));
        }
        mcs::this_thread::sync_wait(task);
    }

    std::cout << "hello world\n";
    return 0;
}
#include <algorithm>
#include <iostream>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

#include "../include/execution.hpp"

namespace stdexec = mcs::execution;

template <class R, class F>
class _then_receiver : public R // NOLINT
{
    F f_; // NOLINT

  public:
    _then_receiver(R r, F f) : R(std::move(r)), f_(std::move(f)) {}

    // Customize set_value by invoking the callable and passing the result to
    // the inner receiver
    template <class... As>
        requires std::invocable<F, As...>
    void set_value(As &&...as) && noexcept // NOLINT
    {
        try
        {
            stdexec::set_value(std::move(*this).base(),
                               std::invoke((F &&)f_, (As &&)as...)); // NOLINT
        }
        catch (...)
        {
            stdexec::set_error(std::move(*this).base(), std::current_exception());
        }
    }
};

template <stdexec::sender S, class F>
struct _then_sender // NOLINT
{
    using sender_concept = stdexec::sender_t;
    S s_; // NOLINT
    F f_; // NOLINT

    template <class... Args>
    using _set_value_t = stdexec::completion_signatures<stdexec::set_value_t(
        std::invoke_result_t<F, Args...>)>;

    using _except_ptr_sig =
        stdexec::completion_signatures<stdexec::set_error_t(std::exception_ptr)>;

    // TODO(mcs): transform_completion_signatures_of 需要
    // S, Env，addition_c,templete_s,templete_e,template_s 6 个参数，明显不够
    // 而且 then_sender 的 completion_signatures 需要确定 前一个 S的 completion_signatures
    //  才能确定 当前 then_sender的  completion_signatures
    // 初始化的时候，是确定了。
    // 但是需要 将 前一个S,的 Args... 来初始化，实例化当前的get_completion_signatures
    // 同时当前的 get_completion_signatures 需要修改，同时顺序不能错
    // _except_ptr_sig 不是模板，目前需要更改
    // 为什么是模板？ 因为 传递过来的 As..，可以被修改
    // 即：set_value_t set_error_t 可以被修改
    // Compute the completion signatures
    using AdditionalSignatures =
        stdexec::completion_signatures<stdexec::set_value_t(int), // int is result
                                       stdexec::set_error_t(std::exception_ptr)>;
    template <class Env> // NOLINTNEXTLINE
    auto get_completion_signatures(Env && /*env*/) && noexcept
        // -> stdexec::transform_completion_signatures_of<S, Env,
        // _except_ptr_sig,
        //                                                _set_value_t>
        -> stdexec::transform_completion_signatures_of<S, Env, AdditionalSignatures>
    {
        return {};
    }

    template <class... Args>
    using SetValue = stdexec::completion_signatures<stdexec::set_value_t(
        std::invoke_result_t<F, Args...>)>;
    
    // template <class... Args>
    // using SetError = stdexec::completion_signatures<stdexec::set_value_t(Args...)>;
    template <class Env> // NOLINTNEXTLINE
    auto get_completion_signatures_2(Env && /*env*/) && noexcept
        -> stdexec::transform_completion_signatures_of<
            S, Env,
            stdexec::completion_signatures<stdexec::set_error_t(std::exception_ptr)>>
    {
        return {};
    }

    // Connect:
    template <stdexec::receiver R>
    auto connect(R r) && -> stdexec::connect_result_t<S, _then_receiver<R, F>>
    {
        return stdexec::connect((S &&)s_, _then_receiver{(R &&)r, (F &&)f_}); // NOLINT
    }

    decltype(auto) get_env() const noexcept // NOLINT
    {
        return stdexec::get_env(s_);
    }
};

template <stdexec::sender S, class F>
stdexec::sender auto then(S s, F f)
{
    return _then_sender<S, F>{(S &&)s, (F &&)f};
}

/*

*/
//
/// test
template <class... Args>
using test_set_value_t = stdexec::completion_signatures<stdexec::set_value_t(int)>;

template <template <class...> class SetValue>
struct test_template
{
};
// 通用的模板转换结构
template <template <typename...> class TargetTemplate, typename Source>
struct template_converter;

// 特化：将 List 转换为 Tuple
template <template <typename...> class TargetTemplate, template <typename...> class List,
          typename... Ts>
struct template_converter<TargetTemplate, List<Ts...>>
{
    using type = TargetTemplate<Ts...>;
};

// 辅助类型别名
template <template <typename...> class TargetTemplate, typename Source>
using template_converter_t = typename template_converter<TargetTemplate, Source>::type;

int main()
{
    stdexec::sender auto j = stdexec::just(3.14, 1);
    {
        stdexec::sender auto t = stdexec::then(j, [](double a, int b) {
            std::cout << "a :" << a << " b :" << b << '\n';
            return 0;
        });

        mcs::this_thread::sync_wait(std::move(t));
    }
    {
        auto fun = [](double a, int b) {
            std::cout << "a :" << a << " b :" << b << '\n';
            return 0;
        };
        _then_sender sndr = _then_sender(j, fun);
        // 不行，因为 _set_value_t 算不出来，是模板哦
        // sndr 必须是右值 &&,这是签名要求的
        // using S = decltype(sndr.get_completion_signatures(stdexec::empty_env{}));
        using Sig =
            decltype((static_cast<_then_sender<decltype(j), decltype(fun)> &&>(sndr))
                         .get_completion_signatures(stdexec::empty_env{}));
        {
            using Sig =
                decltype((static_cast<_then_sender<decltype(j), decltype(fun)> &&>(sndr))
                             .get_completion_signatures_2(stdexec::empty_env{}));
        }

        using j_c = stdexec::completion_signatures_of_t<decltype(j), stdexec::empty_env>;
        static_assert(
            std::is_same_v<j_c, mcs::execution::cmplsigs::completion_signatures<
                                    mcs::execution::recv::set_value_t(double, int)>>);
        // 用 double, int 来算出 sndr的 提交信号
        // lambda 是失败的
        using s_c = std::invoke_result_t<decltype(fun), double, int>;
        {
            using AdditionalSignatures =
                stdexec::completion_signatures<stdexec::set_value_t(int),
                                               stdexec::set_error_t(std::exception_ptr)>;
            using S = decltype(j);
            using Env = stdexec::empty_env;
            using T =
                stdexec::transform_completion_signatures_of<S, Env, AdditionalSignatures>;

            static_assert(std::is_same_v<Sig, T>);
        }

        stdexec::sender auto t = then(sndr, [](int s) {
            std::cout << "s :" << s << '\n';
            return 0;
        });

        // mcs::this_thread::sync_wait(std::move(t));
    }

    std::cout << "hello world\n";
    return 0;
}

/*
===============================================================================
 test_gather_signatures

*/
template <typename...>
struct type_list
{
};
template <typename...>
struct variant
{
};
template <typename...>
struct tuple
{
};
template <int>
struct arg
{
    using T = tuple<tuple<>>;
};

auto test_gather_signatures() -> void
{
    using namespace mcs::execution;
    using namespace mcs::execution::cmplsigs;
    static_assert(
        std::same_as<variant<>, gather_signatures<set_error_t,
                                                  completion_signatures<set_stopped_t()>,
                                                  tuple, variant>>);

    static_assert(
        std::same_as<variant<>, gather_signatures<set_error_t,
                                                  completion_signatures<set_stopped_t()>,
                                                  std::type_identity_t, variant>>);

    static_assert(std::same_as<
                  variant<tuple<>>,
                  gather_signatures<set_stopped_t, completion_signatures<set_stopped_t()>,
                                    tuple, variant>>);

    static_assert(
        std::same_as<tuple<>, gather_signatures<set_stopped_t,
                                                completion_signatures<set_stopped_t()>,
                                                tuple, std::type_identity_t>>);

    static_assert(
        std::same_as<
            variant<tuple<int>, tuple<arg<0>>>,
            gather_signatures<set_error_t,
                              completion_signatures<
                                  set_value_t(), set_value_t(int, arg<0>),
                                  set_error_t(int), set_error_t(arg<0>), set_stopped_t()>,
                              tuple, variant>>);

    static_assert(
        std::same_as<
            variant<int, arg<0>>,
            gather_signatures<set_error_t,
                              completion_signatures<
                                  set_value_t(), set_value_t(int, arg<0>),
                                  set_error_t(int), set_error_t(arg<0>), set_stopped_t()>,
                              std::type_identity_t, variant>>);

    static_assert(
        std::same_as<
            variant<tuple<>, tuple<int, arg<0>, arg<1>>>,
            gather_signatures<set_value_t,
                              completion_signatures<
                                  set_value_t(), set_value_t(int, arg<0>, arg<1>),
                                  set_error_t(int), set_error_t(arg<0>), set_stopped_t()>,
                              tuple, variant>>);
}
/*


*/
template <class... As>
using my_set_value =
    mcs::execution::completion_signatures<mcs::execution::set_value_t(As &...)>;

template <class... As>
using my_set_value2 = mcs::execution::completion_signatures<mcs::execution::set_value_t(
    std::add_rvalue_reference_t<As>...)>;

template <class As>
using my_set_error =
    mcs::execution::completion_signatures<mcs::execution::set_error_t(As &)>;

template <typename T>
struct cmplsigs_instance : std::false_type
{
};

template <typename... Args>
struct cmplsigs_instance<mcs::execution::completion_signatures<Args...>> : std::true_type
{
};
template <typename T>
inline constexpr bool is_cmplsigs_instance = false; // NOLINT

template <class... As> // NOLINTNEXTLINE
inline constexpr bool is_cmplsigs_instance<mcs::execution::completion_signatures<As...>> =
    true;

template <template <class... As> typename>
concept same_template = true;

auto test_template_s() -> void
{
    /*
   template <class... As>
    using default_set_value = cmplsigs::completion_signatures<set_value_t(As...)>;
*/
    namespace stdexec = mcs::execution;
    static_assert(stdexec::valid_completion_signatures<stdexec::default_set_value<int>>);
    static_assert(stdexec::valid_completion_signatures<my_set_value<int>>);
    static_assert(stdexec::valid_completion_signatures<my_set_error<int>>);

    static_assert(cmplsigs_instance<stdexec::default_set_value<>>::value);
    static_assert(cmplsigs_instance<my_set_value<>>::value);

    static_assert(cmplsigs_instance<my_set_error<int>>::value);

    static_assert(cmplsigs_instance<my_set_value2<>>::value);
    static_assert(cmplsigs_instance<my_set_value2<int>>::value);
    //
    static_assert(is_cmplsigs_instance<stdexec::default_set_value<>>);
    static_assert(is_cmplsigs_instance<my_set_value<>>);
    static_assert(is_cmplsigs_instance<my_set_error<int>>);
    static_assert(is_cmplsigs_instance<my_set_value2<>>);
    static_assert(is_cmplsigs_instance<my_set_value2<int>>);

    // Note 是否是
    // 变参模板，是否是单个参数形参模板，5个形参模板。都是声明时要求了，无需校验
    // 因此，判断是不是 cmplsigs_instance，就够了
    // template<template <class...> class T> int A;
    // template<template <class,class> class T> int B;
    // template<template <class> class T>int C;
}

template <template <class...> class T>
struct A;
template <template <class, class> class T>
struct B;
template <template <class> class T>
struct C;

namespace test
{
    using namespace mcs::execution;

    namespace __detail
    {
        template <template <class...> class SetError>
        struct error_list
        {
            template <typename... Ts>
            using type = mcs::execution::type_list<SetError<Ts>...>;
        };

        template <typename T>
        inline constexpr bool is_cmplsigs_instance = false; // NOLINT

        template <class... As>
        inline constexpr bool // NOLINTNEXTLINE
            is_cmplsigs_instance<cmplsigs::completion_signatures<As...>> = true;

    }; // namespace __detail

    template <cmplsigs::valid_completion_signatures _InputSignatures,
              cmplsigs::valid_completion_signatures _AdditionalSignatures =
                  cmplsigs::completion_signatures<>,
              template <class...> class _SetValue = default_set_value,
              template <class> class _SetError = default_set_error,
              cmplsigs::valid_completion_signatures _SetStopped =
                  cmplsigs::completion_signatures<set_stopped_t()>>
        requires(__detail::is_cmplsigs_instance<_SetValue<>> &&
                 __detail::is_cmplsigs_instance<_SetError<int>>)
    using my_transform_completion_signatures = bool;

    /*

*/
    using test_completion_signatures = cmplsigs::completion_signatures<
        set_value_t(), set_value_t(int, float), set_error_t(std::exception_ptr),
        set_value_t(int, float), set_error_t(std::error_code), set_stopped_t()>;
    static_assert(valid_completion_signatures<test_completion_signatures>);
    using Test_T = my_transform_completion_signatures<test_completion_signatures>;

    /*

0*/
    using InputSignatures = test_completion_signatures;
    using V = stdexec::cmplsigs::gather_signatures<stdexec::set_value_t, InputSignatures,
                                                   tfxcmplsigs::default_set_value,
                                                   stdexec::type_list>;

    using E = cmplsigs::gather_signatures<
        set_error_t, InputSignatures, //
        std::type_identity_t,
        tfxcmplsigs::__detail::error_list<tfxcmplsigs::default_set_error>::template type>;

    using _SetStopped = cmplsigs::completion_signatures<set_stopped_t()>;

    using S =
        std::conditional_t<std::is_same_v<cmplsigs::gather_signatures<
                                              set_stopped_t, InputSignatures,
                                              stdexec::type_list, stdexec::type_list>,
                                          stdexec::type_list<>>,
                           cmplsigs::completion_signatures<>, _SetStopped>;

    using AdditionalSignatures =
        cmplsigs::completion_signatures<set_value_t(char), set_value_t(int)>;

    //
    using Test = ::mcs::execution::transform_completion_signatures<InputSignatures,
                                                                   AdditionalSignatures>;
    // impl
    namespace impl_test
    {
        // completion_signatures<As...>
        using AdditionalSignatures =
            cmplsigs::completion_signatures<set_value_t(char), set_value_t(int)>;

        using V =
            stdexec::cmplsigs::gather_signatures<stdexec::set_value_t, InputSignatures,
                                                 my_set_value, stdexec::type_list>;

        using E = cmplsigs::gather_signatures<
            set_error_t, InputSignatures, //
            std::type_identity_t,
            tfxcmplsigs::__detail::error_list<my_set_error>::template type>;

        using _SetStopped = cmplsigs::completion_signatures<set_stopped_t()>;

        using S =
            std::conditional_t<std::is_same_v<cmplsigs::gather_signatures<
                                                  set_stopped_t, InputSignatures,
                                                  stdexec::type_list, stdexec::type_list>,
                                              stdexec::type_list<>>,
                               cmplsigs::completion_signatures<>, _SetStopped>;

        /*

*/

        // V: type_list<completion_signatures<As>...>; => As...
        // using set_value_t => add &
        static_assert(
            std::is_same_v<V, mcs::execution::type_list<
                                  completion_signatures<set_value_t()>,
                                  completion_signatures<set_value_t(int &, float &)>,
                                  completion_signatures<set_value_t(int &, float &)>>>);

        // E: type_list<completion_signatures<As>...>; => As...
        // my_set_error => add &
        static_assert(std::is_same_v<
                      E, mcs::execution::type_list<
                             completion_signatures<
                                 set_error_t(std::__exception_ptr::exception_ptr &)>,
                             completion_signatures<set_error_t(std::error_code &)>>>);

        // completion_signatures<set_stopped_t()> or
        // completion_signatures<>
        static_assert(std::is_same_v<S, completion_signatures<set_stopped_t()>>);

        using All_T =
            completion_signatures<set_value_t(char), set_value_t(int), set_value_t(),
                                  set_value_t(int &, float &),
                                  set_value_t(int &, float &),
                                  set_error_t(std::__exception_ptr::exception_ptr &),
                                  set_error_t(std::error_code &), set_stopped_t()>;

        using IMPL = __completion_signatures_set<AdditionalSignatures, V, E, S>;
        using T = IMPL::type;
        static_assert(std::is_same_v<T, All_T>);

        namespace test_no_stop
        {
            // no stop
            using test_completion_signatures = cmplsigs::completion_signatures<
                set_value_t(), set_value_t(int, float), set_error_t(std::exception_ptr),
                set_value_t(int, float), set_error_t(std::error_code)>;
            using InputSignatures = test_completion_signatures;

            using AdditionalSignatures =
                cmplsigs::completion_signatures<set_value_t(char), set_value_t(int)>;

            using V = stdexec::cmplsigs::gather_signatures<
                stdexec::set_value_t, InputSignatures, my_set_value, stdexec::type_list>;

            using E = cmplsigs::gather_signatures<
                set_error_t, InputSignatures, //
                std::type_identity_t,
                tfxcmplsigs::__detail::error_list<my_set_error>::template type>;

            using _SetStopped = cmplsigs::completion_signatures<set_stopped_t()>;

            using S = std::conditional_t<
                std::is_same_v<
                    cmplsigs::gather_signatures<set_stopped_t, InputSignatures,
                                                stdexec::type_list, stdexec::type_list>,
                    stdexec::type_list<>>,
                cmplsigs::completion_signatures<>, _SetStopped>;

            using All_T =
                completion_signatures<set_value_t(char), set_value_t(int), set_value_t(),
                                      set_value_t(int &, float &),
                                      set_value_t(int &, float &),
                                      set_error_t(std::__exception_ptr::exception_ptr &),
                                      set_error_t(std::error_code &)>;

            using IMPL = __completion_signatures_set<AdditionalSignatures, V, E, S>;
            using T = IMPL::type;
            static_assert(std::is_same_v<T, All_T>);

            /**
             * @brief Taget test
             *
             */
            using Taget_T =
                completion_signatures<set_value_t(char), set_value_t(int), set_value_t(),
                                      set_value_t(int &, float &),
                                      set_error_t(std::__exception_ptr::exception_ptr &),
                                      set_error_t(std::error_code &)>;

            using Taget = mcs::execution::transform_completion_signatures<
                InputSignatures, AdditionalSignatures, my_set_value, my_set_error>;
            static_assert(std::is_same_v<Taget, Taget_T>);

        }; // namespace test_no_stop

    }; // namespace impl_test

}; // namespace test
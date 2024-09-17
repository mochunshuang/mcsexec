#include <concepts>
#include <iostream>
#include <exception>

namespace mcs::execution
{
    struct set_value_t;
    struct set_error_t;
    struct set_stopped_t;
    struct sender_t;

    template <typename...>
    struct type_list
    {
    };

    template <typename...>
    struct Variant
    {
    };

    template <typename...>
    struct Tuple
    {
    };
    template <typename, typename>
    struct two_types
    {
    };

}; // namespace mcs::execution

namespace mcs::execution::cmplsigs
{
    namespace __detail
    {
        template <class _Sig>
        inline constexpr bool __is_compl_sig = false; // NOLINT

        template <class... _Args>
        inline constexpr bool __is_compl_sig<set_value_t(_Args...)> = true; // NOLINT
        template <class _Error>
        inline constexpr bool __is_compl_sig<set_error_t(_Error)> = true; // NOLINT
        template <>
        inline constexpr bool __is_compl_sig<set_stopped_t()> = true; // NOLINT

        template <class Fn>
        concept completion_signature = __is_compl_sig<Fn>;

    }; // namespace __detail

    template <class Fn>
    concept completion_signature = __detail::completion_signature<Fn>;
}; // namespace mcs::execution::cmplsigs

namespace mcs::execution::cmplsigs
{
    template <completion_signature...>
    struct completion_signatures
    {
    };

    namespace __detail
    {
        template <class>
        inline constexpr bool __is_completion_signatures = false; // NOLINT

        template <cmplsigs::completion_signature... _Sigs>
        inline constexpr bool // NOLINTNEXTLINE
            __is_completion_signatures<cmplsigs::completion_signatures<_Sigs...>> = true;
    }; // namespace __detail

    template <class _Completions>
    concept valid_completion_signatures =
        __detail::__is_completion_signatures<_Completions>;

}; // namespace mcs::execution::cmplsigs

namespace mcs::execution
{
    template <int>
    struct arg
    {
    };
    struct empty_env
    {
    };
    struct none_env
    {
    };
    struct error
    {
    };

    using test_completion_signatures =
        cmplsigs::completion_signatures<set_value_t(), set_value_t(int, float),
                                        set_error_t(std::exception_ptr),
                                        set_error_t(std::error_code), set_stopped_t()>;

    struct my_sender
    {
        using sender_concept = sender_t;

        using empty_signatures = cmplsigs::completion_signatures<
            set_error_t(std::error_code &&), set_error_t(std::error_code const &),
            set_value_t(), set_value_t(arg<0>, arg<1> &, arg<2> &&, arg<3> const &),
            set_stopped_t()>;

        // NOLINTNEXTLINE
        auto get_completion_signatures(empty_env /*unused*/) const noexcept
        {
            return empty_signatures();
        }

        using none_signatures = cmplsigs::completion_signatures<>; // NOLINTNEXTLINE
        auto get_completion_signatures(none_env) const noexcept
        {
            return none_signatures();
        }
    };

}; // namespace mcs::execution

namespace mcs::execution::tfxcmplsigs
{
    ////////////////////////////////////
    // [exec.utils.cmplsigs]
    template <bool>
    struct indirect_meta_apply
    {
        template <template <typename...> typename T, typename... As>
        using meta_apply = T<As...>;
    };

    template <typename...>
    concept always_true = true;

    template <template <typename...> typename T, typename... As>
    using META_APPLY =
        typename indirect_meta_apply<always_true<As...>>::template meta_apply<T, As...>;

    template <typename, typename>
    struct same_tag;
    template <typename Tag, typename R, typename... A>
    struct same_tag<Tag, R(A...)>
    {
        static constexpr bool value = std::same_as<Tag, R>; // NOLINT
    };
    template <typename Tag>
    struct select_tag
    {
        template <typename Fn>
        using predicate = same_tag<Tag, Fn>;
    };
    template <typename, template <typename...> class>
    struct gather_signatures_apply;

    template <typename R, typename... A, template <typename...> class Transform>
        requires requires { typename META_APPLY<Transform, A...>; }
    struct gather_signatures_apply<R(A...), Transform>
    {
        using type = META_APPLY<Transform, A...>;
    };

    /// fileter
    template <typename TypeList>
    struct List_to_tuple;

    template <template <typename...> class List, typename... Ts>
    struct List_to_tuple<List<Ts...>>
    {
        using type = std::tuple<Ts...>;
    };

    // 将元组转换为类型列表的模板
    template <template <typename...> class List, typename Tuple>
    struct tuple_to_List;

    template <template <typename...> class List, typename... Ts>
    struct tuple_to_List<List, std::tuple<Ts...>>
    {
        using type = List<Ts...>;
    };

    template <template <typename> class Predicate, typename Tuple>
    struct filter_tuple;

    template <template <typename> class Predicate, typename... Ts>
    struct filter_tuple<Predicate, std::tuple<Ts...>>
    {
        using type = decltype(std::tuple_cat(
            std::declval<std::conditional_t<Predicate<Ts>::value, std::tuple<Ts>,
                                            std::tuple<>>>()...));
    };

    /////

    template <cmplsigs::valid_completion_signatures,
              template <typename...> class, //
              template <typename...> class>
    struct gather_signatures_helper;

    template <typename... Signatures, template <typename...> class Tuple,
              template <typename...> class Variant>
    struct gather_signatures_helper<cmplsigs::completion_signatures<Signatures...>, Tuple,
                                    Variant>
    {
        using type2 = indirect_meta_apply<
            always_true<typename gather_signatures_apply<Signatures, Tuple>::type...>>::
            template meta_apply<
                Variant, typename gather_signatures_apply<Signatures, Tuple>::type...>;

        using type =
            META_APPLY<Variant,
                       typename gather_signatures_apply<Signatures, Tuple>::type...>;
    };

    template <class Tag, cmplsigs::valid_completion_signatures Completions,
              template <class...> class Tuple, template <class...> class Variant>
    using gather_signatures = typename gather_signatures_helper<
        typename tuple_to_List<
            cmplsigs::completion_signatures,
            typename filter_tuple<select_tag<Tag>::template predicate,
                                  typename List_to_tuple<Completions>::type>::type>::type,
        Tuple, Variant>::type;

}; // namespace mcs::execution::tfxcmplsigs

namespace mcs::execution::tfxcmplsigs
{
    namespace __detail
    {
        template <typename T, typename... S>
        concept left_contains = (std::same_as<T, S> || ...); // NOLINT

        template <typename T, typename U>
        struct __unique_variadic_template_impl;

        template <template <class...> class Tuple, typename T, typename... Ts,
                  typename... As>
        struct __unique_variadic_template_impl<Tuple<T, Ts...>, Tuple<As...>>
        {
            using type =
                typename std::conditional_t<left_contains<T, As...>, // 逻辑或
                                            typename __unique_variadic_template_impl<
                                                Tuple<Ts...>, Tuple<As...>>::type,
                                            typename __unique_variadic_template_impl<
                                                Tuple<Ts...>, Tuple<As..., T>>::type>;

            using type2 = __unique_variadic_template_impl<  //
                Tuple<Ts...>,                               //
                std::conditional_t<left_contains<T, As...>, //
                                   Tuple<As...>, Tuple<As..., T>>>::type2;
        };

        template <template <class...> class Tuple, typename... As>
        struct __unique_variadic_template_impl<Tuple<>, Tuple<As...>>
        {
            using type = Tuple<As...>;
            using type2 = Tuple<As...>;
        };
    }; // namespace __detail

    template <typename T>
    struct template_empty;

    template <template <class...> class Tuple, typename... As>
    struct template_empty<Tuple<As...>>
    {
        using type = Tuple<>;
    };
    template <typename T>
    struct unique_variadic_template;
    template <template <class...> class Tuple, typename... As>
    struct unique_variadic_template<Tuple<As...>>
    {
        using type =
            __detail::__unique_variadic_template_impl<Tuple<As...>, Tuple<>>::type;
        using type2 =
            __detail::__unique_variadic_template_impl<Tuple<As...>, Tuple<>>::type2;
    };

}; // namespace mcs::execution::tfxcmplsigs

namespace mcs::execution::test
{
    auto test_completion_signature()
    {
        //  not sig type
        static_assert(not cmplsigs::completion_signature<int>);
        static_assert(not cmplsigs::completion_signature<int()>);

        static_assert(cmplsigs::completion_signature<set_error_t(error)>);
        static_assert(not cmplsigs::completion_signature<set_error_t()>);
        static_assert(not cmplsigs::completion_signature<set_error_t(int, int)>);

        static_assert(cmplsigs::completion_signature<set_stopped_t()>);
        static_assert(not cmplsigs::completion_signature<set_stopped_t(int)>);

        static_assert(cmplsigs::completion_signature<set_value_t()>);
        static_assert(cmplsigs::completion_signature<set_value_t(arg<0>)>);
        static_assert(cmplsigs::completion_signature<set_value_t(arg<0>, arg<1>)>);
        static_assert(
            cmplsigs::completion_signature<set_value_t(arg<0>, arg<1>, arg<2>)>);
    };
    auto test_completion_signatures() -> void
    {
        cmplsigs::completion_signatures<>();

        cmplsigs::completion_signatures<set_error_t(error)>();

        cmplsigs::completion_signatures<set_stopped_t()>();

        cmplsigs::completion_signatures<set_value_t()>();
        cmplsigs::completion_signatures<set_value_t(arg<0>)>();
        cmplsigs::completion_signatures<set_value_t(arg<0>, arg<1>)>();
        cmplsigs::completion_signatures<set_value_t(arg<0>, arg<1>, arg<2>)>();

        cmplsigs::completion_signatures<
            set_value_t(), set_value_t(arg<0>), set_value_t(arg<0>, arg<1>),
            set_value_t(arg<0>, arg<1>, arg<2>), set_error_t(error), set_error_t(error),
            set_stopped_t(), set_stopped_t()>();

        // error
        // cmplsigs::completion_signatures<set_stopped_t(int)>(); //  there are param

        // cmplsigs::completion_signatures<set_error_t()>(); //  there are no param

        // completion_signatures<set_error_t(error, error)>(); //  there are two param
    }

    auto test_valid_completion_signatures() -> void
    {
        static_assert(not cmplsigs::valid_completion_signatures<int>);
        // 空模板判断
        static_assert(
            cmplsigs::valid_completion_signatures<cmplsigs::completion_signatures<>>);

        static_assert(cmplsigs::valid_completion_signatures<
                      cmplsigs::completion_signatures<set_value_t()>>);

        static_assert(cmplsigs::valid_completion_signatures<
                      cmplsigs::completion_signatures<set_value_t(), set_value_t(int)>>);

        // Fns be a pack of the arguments of the completion_signatures
        static_assert(cmplsigs::valid_completion_signatures<
                      typename ::mcs::execution::test_completion_signatures>);
    }

    auto test_indirect_meta_apply() -> void
    {
        static_assert(std::same_as<type_list<bool, char, double>,
                                   tfxcmplsigs::indirect_meta_apply<true>::meta_apply<
                                       type_list, bool, char, double>>);

        static_assert(
            std::same_as<two_types<bool, char>, tfxcmplsigs::indirect_meta_apply<true>::
                                                    meta_apply<two_types, bool, char>>);
    }

    auto test_always_true() -> void
    {
        static_assert(tfxcmplsigs::always_true<>);
        static_assert(tfxcmplsigs::always_true<arg<0>>);
        static_assert(tfxcmplsigs::always_true<arg<0>, bool, char, double>);
    }
    auto test_filter() -> void
    {
        using test_completion_signatures = cmplsigs::completion_signatures<
            set_value_t(), set_value_t(int, float), set_error_t(std::exception_ptr),
            set_error_t(std::error_code), set_stopped_t()>;
        static_assert(cmplsigs::valid_completion_signatures<test_completion_signatures>);
        using T = tfxcmplsigs::List_to_tuple<test_completion_signatures>::type;

        // std::tuple<mcs::execution::set_value_t (), mcs::execution::set_value_t (int,
        // float)>
        using filter = tfxcmplsigs::filter_tuple<
            tfxcmplsigs::select_tag<set_value_t>::template predicate, T>::type;
        static_assert(std::is_same_v<std::tuple<mcs::execution::set_value_t(),
                                                mcs::execution::set_value_t(int, float)>,
                                     filter>);

        using target =
            tfxcmplsigs::tuple_to_List<cmplsigs::completion_signatures, filter>::type;
        static_assert(
            std::is_same_v<
                cmplsigs::completion_signatures<mcs::execution::set_value_t(),
                                                mcs::execution::set_value_t(int, float)>,
                target>);
        {
            using filter = tfxcmplsigs::filter_tuple<
                tfxcmplsigs::select_tag<set_error_t>::template predicate, T>::type;
            using target =
                tfxcmplsigs::tuple_to_List<cmplsigs::completion_signatures, filter>::type;

            static_assert(std::is_same_v<
                          cmplsigs::completion_signatures<set_error_t(std::exception_ptr),
                                                          set_error_t(std::error_code)>,
                          target>);
        }
        {

            using target = tfxcmplsigs::tuple_to_List<
                cmplsigs::completion_signatures,
                tfxcmplsigs::filter_tuple<
                    tfxcmplsigs::select_tag<set_stopped_t>::template predicate,
                    tfxcmplsigs::List_to_tuple<test_completion_signatures>::type>::type>::
                type;

            static_assert(
                std::is_same_v<cmplsigs::completion_signatures<set_stopped_t()>, target>);
        }

        {
            static_assert(
                std::same_as<
                    cmplsigs::completion_signatures<mcs::execution::set_value_t(),
                                                    mcs::execution::set_value_t(int,
                                                                                float)>,
                    tfxcmplsigs::indirect_meta_apply<true>::meta_apply<
                        cmplsigs::completion_signatures, mcs::execution::set_value_t(),
                        mcs::execution::set_value_t(int, float)>>);
        }
    }

    auto test_gather_signatures_helper()
    {
        using T =
            cmplsigs::completion_signatures<mcs::execution::set_value_t(),
                                            mcs::execution::set_value_t(),
                                            mcs::execution::set_value_t(int, float)>;
        using Target = tfxcmplsigs::gather_signatures_helper<T, Tuple, Variant>::type;
        static_assert(
            std::is_same_v<
                mcs::execution::Variant<mcs::execution::Tuple<>, mcs::execution::Tuple<>,
                                        mcs::execution::Tuple<int, float>>,
                Target>);

        using Target2 = tfxcmplsigs::gather_signatures_helper<T, Tuple, Variant>::type2;
        static_assert(std::is_same_v<Target, Target2>);
    }

    auto test_unique_variadic_template()
    {
        using T =
            mcs::execution::Variant<mcs::execution::Tuple<>, mcs::execution::Tuple<>,
                                    mcs::execution::Tuple<int, float>>;
        using Target = tfxcmplsigs::unique_variadic_template<T>::type;
        static_assert(
            std::is_same_v<mcs::execution::Variant<mcs::execution::Tuple<>,
                                                   mcs::execution::Tuple<int, float>>,
                           Target>);

        static_assert(std::is_same_v<tfxcmplsigs::unique_variadic_template<T>::type,
                                     tfxcmplsigs::unique_variadic_template<T>::type2>);
        {
            using namespace mcs::execution; // NOLINT
            // 顺序测试
            using T = Variant<Tuple<>, Tuple<int, float>, Tuple<>, Tuple<int>, Tuple<>,
                              Tuple<>, Tuple<>, Tuple<char>, Tuple<>, Tuple<int>>;
            using Target = Variant<Tuple<>, Tuple<int, float>, Tuple<int>, Tuple<char>>;
            using type = tfxcmplsigs::unique_variadic_template<T>::type;
            using type2 = tfxcmplsigs::unique_variadic_template<T>::type2;

            static_assert(std::is_same_v<Target, type>);
            static_assert(std::is_same_v<Target, type2>);
        }
    }

    auto test_gather_signatures()
    {
        using Completions = cmplsigs::completion_signatures<
            set_value_t(), set_value_t(int, float), set_error_t(std::exception_ptr),
            set_error_t(std::error_code), set_stopped_t()>;

        using Target =
            tfxcmplsigs::gather_signatures<set_value_t, Completions, Tuple, Variant>;
        static_assert(
            std::is_same_v<Target,
                           mcs::execution::Variant<mcs::execution::Tuple<>,
                                                   mcs::execution::Tuple<int, float>>>);

        using Sig = mcs::execution::cmplsigs::completion_signatures<
            set_value_t(), set_error_t(std::__exception_ptr::exception_ptr),
            set_stopped_t()>;
        using set_Sig = tfxcmplsigs::gather_signatures<set_value_t, Sig, Tuple, Variant>;
        // 如果有多个
        {
            using Sig = mcs::execution::cmplsigs::completion_signatures<
                set_value_t(), set_value_t(), set_value_t(int),
                set_error_t(std::__exception_ptr::exception_ptr), set_stopped_t()>;
            using set_Sig =
                tfxcmplsigs::gather_signatures<set_value_t, Sig, Tuple, Variant>;
            // 则存在重复
            using u_set_Sig = tfxcmplsigs::unique_variadic_template<set_Sig>::type;

            static_assert(
                std::is_same_v<mcs::execution::Variant<mcs::execution::Tuple<>,
                                                       mcs::execution::Tuple<int>>,
                               u_set_Sig>);
        }
    }

}; // namespace mcs::execution::test

int main()
{
    std::cout << "hello world\n";
    return 0;
}
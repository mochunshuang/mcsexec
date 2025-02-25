#include <iostream>
#include <version>

// NOTE: GCC15 才支持
#if (__cpp_pack_indexing == 202311LL)
#if false
// 定义一个模板函数，使用包索引访问参数包中的元素
template <typename... Ts>
void print_element(Ts... args)
{
    // 访问参数包中的第一个元素
    std::cout << "First element: " << args...[0] << "\n";

    // 访问参数包中的第二个元素
    std::cout << "Second element: " << args...[1] << "\n";
}

int main()
{
    print_element(10, 20, 30, 40); // 调用函数
    return 0;
}
#endif

#include <algorithm>
#include <iostream>
#include <string_view>
#include <tuple>
#include <typeinfo>
#include <type_traits>
#include <utility>

// General utilities
template <class From, class To>
using _copy_cvref_t = decltype(std::forward_like<From>(std::declval<To &>()));

template <class T, template <class...> class>
inline constexpr bool _is_specialization_of = false;

template <class... Ts, template <class...> class C>
inline constexpr bool _is_specialization_of<C<Ts...>, C> = true;

template <class... Ts>
using _front = Ts...[0];

template <template <class...> class C, class... Ts>
concept _can_be_instantiated_with = requires { typename C<Ts...>; };

template <class... Ts>
concept _decay_copyable = requires(Ts && (*...ts)()) { (auto(ts()), ...); };

template <class Fn, class... Ts>
concept _callable_with =
    requires(Fn &&fn, Ts &&...ts) { static_cast<Fn &&>(fn)(static_cast<Ts &&>(ts)...); };

template <class Fn, class... Ts>
concept _nothrow_callable_with = requires(Fn &&fn, Ts &&...ts) {
    { static_cast<Fn &&>(fn)(static_cast<Ts &&>(ts)...) } noexcept;
};

template <class Fn, class... Ts>
concept _meta_callable_with =
    requires(Fn &&fn) { static_cast<Fn &&>(fn).template operator()<Ts...>(); };

template <class Fn, class... Ts>
concept _nothrow_meta_callable_with = requires(Fn &&fn) {
    { static_cast<Fn &&>(fn).template operator()<Ts...>() } noexcept;
};

// _typeset: a set of unique types
template <class T>
struct _box
{
};

template <class...>
struct _typeset
{
    template <class... Ts, class U>
    constexpr auto operator+(this _typeset<Ts...> set, _box<U>) noexcept
    {
        if constexpr (__is_base_of(_box<U>, _typeset<Ts...>))
        {
            return set;
        }
        else
        {
            return _typeset<U, Ts...>();
        }
    }

    template <class... Ts, template <class...> class C, class... Us>
    constexpr auto operator+(this _typeset<Ts...> set, C<Us...>) noexcept
    {
        return (set + ... + _box<Us>());
    }

    template <class... Ts, _meta_callable_with<Ts...> Fn>
    constexpr decltype(auto) apply(this _typeset<Ts...>,
                                   Fn fn) noexcept(_nothrow_meta_callable_with<Fn, Ts...>)
    {
        return fn.template operator()<Ts...>();
    }
};

template <class T, class... Ts>
struct _typeset<T, Ts...> : _box<T>, _typeset<Ts...>
{
};

// Exception types
struct std_exception
{
    std_exception() = default;
    virtual constexpr const char *what() const noexcept
    {
        return "unknown exception";
    }
};

template <class Derived>
struct compile_time_error : std_exception
{
    compile_time_error() = default;

    constexpr const char *what() const noexcept override
    {
        return typeid(Derived *).name();
    }
};

template <class Data, class... What>
struct _sender_type_check_failure
    : compile_time_error<_sender_type_check_failure<Data, What...>>
{
    _sender_type_check_failure()
        requires std::default_initializable<Data>
    = default;
    explicit constexpr _sender_type_check_failure(Data data) : data_(data) {}

    Data data_;
};

struct _dependent_sender_error_base : std_exception
{
    constexpr char const *what() const noexcept override
    {
        return what_;
    }
    char const *what_;
};

template <class Sndr>
struct _dependent_sender_error : _dependent_sender_error_base
{
    constexpr _dependent_sender_error() noexcept
    {
        what_ = "This sender needs to know its execution environment before it can know "
                "how it will complete.";
    }
};

// Completion tags (set_value, set_error, set_stopped):
enum class _disposition
{
    _value,
    _error,
    _stopped
};

template <_disposition Disp>
struct _set_xxx_t
{
    template <enum _disposition Other>
    constexpr bool operator==(_set_xxx_t<Other>) const noexcept
    {
        return Disp == Other;
    }
    static constexpr _disposition _disposition = Disp;
};

inline constexpr struct set_value_t : _set_xxx_t<_disposition::_value>
{
} set_value{};
inline constexpr struct set_error_t : _set_xxx_t<_disposition::_error>
{
} set_error{};
inline constexpr struct set_stopped_t : _set_xxx_t<_disposition::_stopped>
{
} set_stopped{};

template <class... Ts>
inline constexpr set_value_t (*_set_value_sig_v)(Ts...) = nullptr;

template <>
inline constexpr set_value_t (*_set_value_sig_v<void>)() = nullptr;

// completion_signatures
template <class... Sigs>
struct completion_signatures;

inline constexpr auto _to_completions = []<class... Sigs>() {
    return completion_signatures<Sigs...>();
};

template <class... Sigs>
struct completion_signatures
{
    completion_signatures() = default;

    explicit(sizeof...(Sigs) == 1) constexpr completion_signatures(Sigs *...) noexcept
        requires(sizeof...(Sigs) != 0)
    {
    }

    static constexpr std::size_t size() noexcept
    {
        return sizeof...(Sigs);
    }

    template <std::size_t I>
    static constexpr auto get() noexcept
    {
        return static_cast<Sigs...[I] *>(nullptr);
    }

    template <std::size_t I>
    friend constexpr auto get(completion_signatures) noexcept
    {
        return static_cast<Sigs...[I] *>(nullptr);
    }

    template <class... Other>
    constexpr auto operator+(completion_signatures<Other...> cs) const noexcept
    {
        return (_typeset<Sigs...>() + cs).apply(_to_completions);
    }

    template <class Tag, class... Args>
    constexpr friend auto operator+(completion_signatures, Tag (*)(Args...)) noexcept
    {
        return (_typeset<Sigs...>() + _box<Tag(Args...)>()).apply(_to_completions);
    }

    template <class Tag, class... Args>
    constexpr friend auto operator+(Tag (*)(Args...), completion_signatures) noexcept
    {
        return (_typeset<Sigs...>() + _box<Tag(Args...)>()).apply(_to_completions);
    }
};

template <class... Sigs, _callable_with<Sigs *...> Fn>
constexpr decltype(auto) apply(Fn fn, completion_signatures<Sigs...>) noexcept(
    _nothrow_callable_with<Fn, Sigs *...>)
{
    return fn(static_cast<Sigs *>(nullptr)...);
}

namespace std
{
    template <class... Sigs>
    struct tuple_size<completion_signatures<Sigs...>>
        : integral_constant<size_t, sizeof...(Sigs)>
    {
    };

    template <size_t I, class... Sigs>
    struct tuple_element<I, completion_signatures<Sigs...>>
    {
        using type = Sigs...[I] *;
    };
} // namespace std

template <class Tag, class... Args>
auto _normalize2(Args &&...) -> Tag (*)(Args...);

template <class Tag, class... Args>
constexpr auto _normalize1(Tag (*)(Args...))
{
    return decltype(::_normalize2<Tag>(std::declval<Args>()...))();
}
template <class... Sigs>
constexpr auto _unique(Sigs *...)
{
    return (_typeset() + completion_signatures<Sigs...>()).apply(_to_completions);
}

//! @brief builds a completion_signatures specialization by normalizing
//! all the signatures and then removing duplicates. To normalize a signature
//! is to remove rvalue references from arguments. For example,
//! set_value_t(int&&, float&) normalizes to set_value_t(int, float&).
template <class... Sigs>
inline constexpr auto _completions_v =
    ::_unique(::_normalize1(static_cast<Sigs *>(nullptr))...);

template <bool PotentiallyThrowing>
inline constexpr auto eptr_completion_if =
    std::conditional_t<PotentiallyThrowing,
                       completion_signatures<set_error_t(std::exception_ptr)>,
                       completion_signatures<>>();

template <class T>
concept _valid_completion_signatures = _is_specialization_of<T, completion_signatures>;

template <class... Sigs1, class... Sigs2>
constexpr auto make_completion_signatures(Sigs2 *...sigs2) noexcept
{
    return _completions_v<Sigs1..., Sigs2...>;
}

template <class... What, class... Values>
[[noreturn, nodiscard]] consteval completion_signatures<> invalid_completion_signature(
    Values... values)
{
    if constexpr (sizeof...(Values) == 1)
    {
        throw _sender_type_check_failure<Values...[0], What...>(values...);
    }
    else
    {
        (void)invalid_completion_signature<What...>([... data = values] {});
    }
}

// Sender concepts
struct sender_t
{
};

template <class Sndr>
concept sender = std::derived_from<typename Sndr::sender_concept, sender_t>;

template <completion_signatures Completions>
concept _has_constexpr_completions_aux = true;

template <class Sndr, class... Env>
concept _has_constexpr_completions = _has_constexpr_completions_aux<
    std::remove_reference_t<Sndr>::template get_completion_signatures<Sndr, Env...>()>;

template <class Sndr, class... Env>
concept sender_in = sender<Sndr> && _has_constexpr_completions<Sndr, Env...>;

template <class Sndr, class... Env>
consteval auto get_completion_signatures();

template <class Sndr>
consteval bool _is_dependent_sender_aux()
{
    try
    {
        (void)get_completion_signatures<Sndr>();
    }
    catch (_dependent_sender_error_base &)
    {
        return true;
    }
    return false;
}

template <class Sndr>
concept dependent_sender =
    sender<Sndr> && std::bool_constant<_is_dependent_sender_aux<Sndr>()>::value;

// get_completion_signatures
template <class Sndr, class... Env>
using _completion_signatures_of =
    decltype(std::remove_reference_t<Sndr>::template get_completion_signatures<Sndr,
                                                                               Env...>());

template <class Sndr, class... Env>
consteval auto get_completion_signatures()
{
    using Self = std::remove_reference_t<Sndr>;
    if constexpr (_has_constexpr_completions<Sndr, Env...>)
    {
        return Self::template get_completion_signatures<Sndr, Env...>();
    }
    else if constexpr (sizeof...(Env) == 0 &&
                       !_can_be_instantiated_with<_completion_signatures_of, Sndr>)
    {
        return (throw _dependent_sender_error<Sndr>{}, completion_signatures<>());
    }
    else if constexpr (!_can_be_instantiated_with<_completion_signatures_of, Sndr,
                                                  Env...>)
    {
        return invalid_completion_signature</*TODO*/>();
    }
    else
    {
        return (Self::template get_completion_signatures<Sndr, Env...>(),
                invalid_completion_signature</*TODO*/>());
    }
}

template <class Sndr>
constexpr auto _type_check_sender(Sndr sndr)
{
    if constexpr (!dependent_sender<Sndr>)
    {
        (void)get_completion_signatures<Sndr>();
    }
    return sndr;
}

template <class Parent, class Child, class... Env>
consteval auto get_child_completion_signatures()
{
    return get_completion_signatures<_copy_cvref_t<Parent, Child>, Env...>();
}

template <class Sndr, class... Env>
    requires sender_in<Sndr, Env...>
using completion_signatures_of_t = decltype(get_completion_signatures<Sndr, Env...>());

// transform_completion_signatures:
template <class... As, class Fn>
consteval auto _apply_transform(const Fn &fn)
{
    if constexpr (!requires {
                      { fn.template operator()<As...>() } -> _valid_completion_signatures;
                  })
    {
        return invalid_completion_signature<
            struct IN_TRANSFORM_COMPLETION_SIGNATURES,
            struct
            A_TRANSFORM_FUNCTION_RETURNED_A_TYPE_THAT_IS_NOT_A_COMPLETION_SIGNATURES_SPECIALIZATION,
            struct WITH_FUNCTION(Fn), struct WITH_ARGUMENTS(As...)>();
    }
    else
    {
        return fn.template operator()<As...>();
    }
}

inline constexpr auto _default_value_fn = []<class... Values>() {
    return _completions_v<set_value_t(Values...)>;
};
inline constexpr auto _default_error_fn = []<class Error>() {
    return _completions_v<set_error_t(Error)>;
};

template <_valid_completion_signatures Completions,
          class ValueFn = decltype(_default_value_fn),
          class ErrorFn = decltype(_default_error_fn),
          _valid_completion_signatures Stopped = completion_signatures<set_stopped_t()>,
          _valid_completion_signatures ExtraSigs = completion_signatures<>>
constexpr auto transform_completion_signatures(Completions cs, ValueFn value_fn = {},
                                               ErrorFn error_fn = {}, Stopped stop = {},
                                               ExtraSigs extra = {})
{
    auto transform1 = [=]<class Tag, class... Ts>(Tag (*)(Ts...)) {
        if constexpr (Tag() == set_value)
            return _apply_transform<Ts...>(value_fn);
        else if constexpr (Tag() == set_error)
            return _apply_transform<Ts...>(error_fn);
        else
            return stop;
    };
    auto transform_all = [=](auto *...sigs) {
        return (transform1(sigs) + ... + completion_signatures());
    };
    return apply(transform_all, cs) + extra;
}

// sender algorithm: just
template <const auto &Algorithm>
struct IN_ALGORITHM;

extern const inline struct just_t just;
inline constexpr struct just_t
{
  private:
    template <class Tag, class... Ts>
    struct sender
    {
        using sender_concept = sender_t;
        Tag tag;
        std::tuple<Ts...> data;

        template <class Self, class... Env>
        static constexpr auto get_completion_signatures()
        {
            return _completions_v<set_value_t(Ts...)>;
        }
    };

  public:
    template <class... Ts>
    constexpr auto operator()(Ts... ts) const
    {
        return sender{just_t{}, std::tuple{ts...}};
    }
} just{};

// sender algorithm: then
extern inline const struct then_t then;

inline constexpr struct then_t
{
    template <class Sndr, class Fn>
    constexpr auto operator()(Sndr sndr, Fn fn) const
    {
        return _type_check_sender(sender{then, fn, sndr});
    }

    template <class Fn>
    constexpr auto operator()(Fn fn) const
    {
        return _closure{fn};
    }

  private:
    template <class Fn>
    static constexpr auto _nothrow_fn = []<class Tag, class... Ts>(Tag (*)(Ts...)) {
        return (Tag() == set_value) ? std::is_nothrow_invocable_v<Fn, Ts...> : true;
    };

    template <class Tag, class Sndr, class Fn>
    struct sender
    {
        using sender_concept = sender_t;
        Tag tag;
        Fn fn;
        Sndr sndr;

        template <class Self, class... Env>
        static consteval auto get_completion_signatures()
        {
            // get_completion_signatures for the `then` sender transforms the
            // value completion signatures by reducing the value types with
            // the result of Fn when invoked with them. if the function is not
            // invocable with the given arguments, throw an exception with the
            // error information.
            auto cs = get_child_completion_signatures<Self, Sndr, Env...>();

            constexpr bool nothrow = apply(
                [](auto... sigs) { return (then_t::_nothrow_fn<Fn>(sigs) && ...); }, cs);

            // Transform the predecessor sender's values by invoking Fn with them
            auto transform_value_sig = []<class... Args>() {
                if constexpr (!std::invocable<Fn, Args...>)
                {
                    return invalid_completion_signature<IN_ALGORITHM<then>,
                                                        struct WITH_FUNCTION(Fn),
                                                        struct WITH_ARGUMENTS(Args...)>(
                        "The function passed to std::execution::then is not callable "
                        "with the"
                        " values sent by the predecessor sender.");
                }
                else
                {
                    return completion_signatures{
                        _set_value_sig_v<std::invoke_result_t<Fn, Args...>>};
                }
            };

            return transform_completion_signatures(cs, transform_value_sig, {}, {},
                                                   eptr_completion_if<!nothrow>);
        }

        template <class Self, class Rcvr>
        constexpr auto connect(this Self &&self, Rcvr rcvr)
            requires _decay_copyable<_copy_cvref_t<Self, Fn>>
        {
            // TODO
        }
    };

    template <class Fn>
    struct _closure
    {
        template <class Sndr>
        friend constexpr auto operator|(Sndr sndr, _closure self)
        {
            return _type_check_sender(sender{then, self.fn_, sndr});
        }

        Fn fn_;
    };
} then{};

// sender algorithm: read_env
extern const struct read_env_t read_env;
inline constexpr struct read_env_t
{
  private:
    template <class Tag, class Query>
    struct sender : Tag, Query
    {
        using sender_concept = sender_t;

        template <class, class Env>
        static constexpr auto get_completion_signatures()
        {
            return completion_signatures<set_value_t(int)>();
        }

        static constexpr auto connect(auto rcvr)
        {
            // TODO
        }
    };

  public:
    template <class Query>
    constexpr auto operator()(Query query) const noexcept
    {
        return sender{read_env, query};
    }
} read_env{};

inline constexpr struct get_env_t
{
    // TODO
} get_env{};

inline constexpr struct get_stop_token_t
{
    // TODO
} get_stop_token{};

int main(int argc, char *argv[])
{
    auto x = just(argc, 0) | then([](int, int) { return 0; }) |
             then([](int i) { return "hello world"; });
    static_assert(!dependent_sender<decltype(x)>);

    auto [tag, fun, child] = just(0, 0) | then([](int, int) {});

#ifdef ERROR
    auto y = just(0, 0) | then([](int, int *) { return 0; }) |
             then([](int i) { return "hello world"; });
#endif

    constexpr auto z = read_env(get_stop_token);
    // Note: 不行。 算不出来
    // constexpr auto a = read_env(get_stop_token) | then([]() {}); // OK

    // static_assert(sender<decltype(z)>);
    // static_assert(dependent_sender<decltype(z)>);
    // static_assert(sender_in<decltype(z), int>);
    // static_assert(!sender_in<decltype(z)>);
}

#else
int main()
{
    std::cout << "main done\n";
    return 0;
}
#endif
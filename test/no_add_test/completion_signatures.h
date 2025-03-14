#pragma once

#include <concepts>
#include <type_traits>

struct set_value_t;
struct set_error_t;
struct set_stopped_t;

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
}; // namespace __detail

template <class Fn>
concept completion_signature = __detail::__is_compl_sig<Fn>;

// 移除右值引用修饰符的别名模板
template <typename T>
using remove_rvalue_reference_t =
    std::conditional_t<std::is_rvalue_reference_v<T>, // 如果是右值引用
                       std::remove_reference_t<T>,    // 移除引用
                       T                              // 否则保留原类型
                       >;
template <typename T>
struct _NORMALIZE_SIG;
template <typename R, typename... Args>
struct _NORMALIZE_SIG<R(Args...)>
{
    using type = R(remove_rvalue_reference_t<Args>...);
};
template <typename T>
using NORMALIZE_SIG = typename _NORMALIZE_SIG<T>::type;

template <typename F1, typename F2>
concept MATCHING_SIG = std::same_as<NORMALIZE_SIG<F1>, NORMALIZE_SIG<F2>>; // NOLINT

template <typename Fn, typename Tag>
constexpr inline int is_tag_sig = 0; // NOLINT
template <typename Tag, typename... Args>
constexpr inline int is_tag_sig<Tag(Args...), Tag> = 1; // NOLINT

template <completion_signature... Sigs>
struct completion_signatures
{
    template <class Sig> // NOLINTNEXTLINE
    static constexpr bool contains = (MATCHING_SIG<Sig, Sigs> || ...);

    template <class Tag> // NOLINTNEXTLINE
    static constexpr int count = (0 + ... + is_tag_sig<Sigs, Tag>);

    template <class Tag>
    static consteval auto filter_sigs_by_Tag() // NOLINT
    {
        return (std::conditional_t<is_tag_sig<Sigs, Tag>, completion_signatures<Sigs>,
                                   completion_signatures<>>{} +
                ... + completion_signatures<>{});
    }

    template <class Sig>
    consteval auto operator+(completion_signatures<Sig> /*unused*/) const noexcept
    {
        if constexpr (contains<Sig>)
            return *this;
        else
            return completion_signatures<Sigs..., NORMALIZE_SIG<Sig>>{};
    }

    // NOTE: 包的展开方式 非常重要：左，中间，右 是不一样的。
    template <class... Ts>
    consteval auto operator+(completion_signatures<Ts...> /*unused*/) const noexcept
    {
        return (*this + ... + completion_signatures<NORMALIZE_SIG<Ts>>{});
    }
};

namespace __detail
{
    template <typename> // NOLINTNEXTLINE
    constexpr inline bool is_completion_signatures = false;
    template <typename... Sig> // NOLINTNEXTLINE
    constexpr inline bool is_completion_signatures<completion_signatures<Sig...>> = true;
}; // namespace __detail

template <typename CS>
concept valid_completion_signatures = __detail::is_completion_signatures<CS>;
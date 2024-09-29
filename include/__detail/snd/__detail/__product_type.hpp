#pragma once

#include <tuple>
#include <utility>

namespace mcs::execution::snd::__detail
{

    template <::std::size_t, typename T>
    struct product_type_element
    {
        T value; // NOLINT
    };

    template <typename, typename...>
    struct product_type_base;

    template <::std::size_t... I, typename... T>
    struct product_type_base<::std::index_sequence<I...>, T...>
        : product_type_element<I, T>...
    {
        static constexpr ::std::size_t size()
        {
            return sizeof...(T);
        }

        template <::std::size_t J, typename S>
        static auto element_get( // NOLINT
            product_type_element<J, S> &self) noexcept -> S &
        {
            return self.value;
        }
        template <::std::size_t J, typename S>
        static auto element_get( // NOLINT
            product_type_element<J, S> &&self) noexcept -> S
        {
            return ::std::move(self.value);
        }
        template <::std::size_t J, typename S>
        static auto element_get( // NOLINT
            product_type_element<J, S> const &self) noexcept -> S const &
        {
            return self.value;
        }

        template <::std::size_t J>
        auto get() & -> decltype(auto)
        {
            return this->element_get<J>(*this);
        }
        template <::std::size_t J>
        auto get() && -> auto
        {
            return this->element_get<J>(::std::move(*this));
        }
        template <::std::size_t J>
        [[nodiscard]] auto get() const & -> decltype(auto)
        {
            return this->element_get<J>(*this);
        }

        auto operator==(product_type_base const &) const -> bool = default;
    };

    template <typename... T>
    struct product_type : product_type_base<::std::index_sequence_for<T...>, T...>
    {
        template <class Self, class Fn>
            requires ::std::invocable<Fn, T &...>
        constexpr decltype(auto) apply(this Self &&self, Fn &&fn) noexcept(
            ::std::is_nothrow_invocable_v<Fn, T &...>)
        {
            return
                [&]<::std::size_t... I>(::std::index_sequence<I...>) -> decltype(auto) {
                    return ::std::forward<Fn>(fn)(
                        ::std::forward_like<Self>(self.template get<I>())...);
                }(::std::index_sequence_for<T...>{});
        }
    };

    template <typename... T>
    product_type(T &&...) -> product_type<::std::decay_t<T>...>;

}; // namespace mcs::execution::snd::__detail

namespace std
{
    template <typename... T>
    struct tuple_size<::mcs::execution::snd::__detail::product_type<T...>> // NOLINT
        : ::std::integral_constant<std::size_t, sizeof...(T)>
    {
    };

    template <::std::size_t I, typename... T>
    struct tuple_element<I, ::mcs::execution::snd::__detail::product_type<T...>> // NOLINT
    {
        using type = ::std::decay_t<
            decltype(::std::declval<::mcs::execution::snd::__detail::product_type<T...>>()
                         .template get<I>())>;
    };
}; // namespace std
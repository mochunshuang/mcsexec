#pragma once

#include <tuple>
#include <utility>

namespace mcs::execution::snd::__detail
{
    template <::std::size_t, typename T>
    struct product_type_element
    {
        T value; // NOLINT
        auto operator==(const product_type_element &) const -> bool = default;
    };

    template <typename, typename...>
    struct product_type_base;

    template <::std::size_t... I, typename... T>
    struct product_type_base<::std::index_sequence<I...>, T...>
        : ::mcs::execution::snd::__detail::product_type_element<I, T>...
    {
        static consteval ::std::size_t size() noexcept
        {
            return sizeof...(T);
        }

        template <::std::size_t J, typename S>
        constexpr static auto element_get( // NOLINT
            ::mcs::execution::snd::__detail::product_type_element<J, S> &self) noexcept
            -> S &
        {
            return self.value;
        }
        template <::std::size_t J, typename S>
        constexpr static auto element_get( // NOLINTNEXTLINE // NOLINT
            ::mcs::execution::snd::__detail::product_type_element<J, S> &&self) noexcept
            -> S &&
        {
            return ::std::move(self.value);
        }
        template <::std::size_t J, typename S>
        constexpr static auto element_get( // NOLINT
            const ::mcs::execution::snd::__detail::product_type_element<J, S>
                &self) noexcept -> const S &
        {
            return self.value;
        }

        template <::std::size_t J>
        constexpr auto get() & noexcept -> decltype(auto)
        {
            return this->element_get<J>(*this);
        }
        template <::std::size_t J>
        constexpr auto get() && noexcept -> decltype(auto)
        {
            return this->element_get<J>(::std::move(*this));
        }
        template <::std::size_t J>
        [[nodiscard]] constexpr auto get() const & noexcept -> decltype(auto)
        {
            return this->element_get<J>(*this);
        }

        auto operator==(const product_type_base &) const -> bool = default;
    };

    template <typename... T>
    struct product_type : ::mcs::execution::snd::__detail::product_type_base<
                              ::std::index_sequence_for<T...>, T...>
    {

        template <typename Self, typename Fun, ::std::size_t... I> // NOLINTNEXTLINE
        constexpr auto apply_elements(
            this Self &&self, ::std::index_sequence<I...>,
            Fun &&fun) noexcept(noexcept(::std::
                                             forward<Fun>(fun)(::std::forward_like<Self>(
                                                 self.template get<I>())...)))
            -> decltype(auto)
        {
            return ::std::forward<Fun>(fun)(
                ::std::forward_like<Self>(self.template get<I>())...);
        }

        template <typename Self, typename Fun>
        constexpr auto apply(this Self &&self, Fun &&fun) noexcept(noexcept(
            ::std::forward<Self>(self).apply_elements(::std::index_sequence_for<T...>{},
                                                      ::std::forward<Fun>(fun))))
            -> decltype(auto)
        {
            return ::std::forward<Self>(self).apply_elements(
                ::std::index_sequence_for<T...>{}, ::std::forward<Fun>(fun));
        }
    };
    template <typename... T>
    product_type(T &&...) -> product_type<::std::decay_t<T>...>;

    // helper
    template <typename T>
    constexpr bool is_product_type_v = false; // NOLINT

    template <typename... U>
    constexpr bool // NOLINTNEXTLINE
        is_product_type_v<::mcs::execution::snd::__detail::product_type<U...>> = true;

    template <typename T>
    concept is_product_type = is_product_type_v<T>;

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

namespace std
{
    // for std::get<>
    template <std::size_t I, typename T>
        requires ::mcs::execution::snd::__detail::is_product_type<
            ::std::remove_cvref_t<T>>
    constexpr auto get(T &&t) noexcept -> decltype(auto) // NOLINT
    {
        return std::forward<T>(t).template get<I>();
    }
}; // namespace std

#pragma once

#include <tuple>
#include <utility>
#include <memory>

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
        static constexpr ::std::size_t size()
        {
            return sizeof...(T);
        }

        template <::std::size_t J, typename S>
        static auto element_get( // NOLINT
            ::mcs::execution::snd::__detail::product_type_element<J, S> &self) noexcept
            -> S &
        {
            return self.value;
        }
        template <::std::size_t J, typename S>
        static auto element_get( // NOLINT
            ::mcs::execution::snd::__detail::product_type_element<J, S> &&self) noexcept
            -> S &&
        {
            return ::std::move(self.value);
        }
        template <::std::size_t J, typename S>
        static auto element_get( // NOLINT
            const ::mcs::execution::snd::__detail::product_type_element<J, S>
                &self) noexcept -> const S &
        {
            return self.value;
        }

        template <::std::size_t J>
        auto get() & -> decltype(auto)
        {
            return this->element_get<J>(*this);
        }
        template <::std::size_t J>
        auto get() && -> decltype(auto)
        {
            return this->element_get<J>(::std::move(*this));
        }
        template <::std::size_t J>
        [[nodiscard]] auto get() const & -> decltype(auto)
        {
            return this->element_get<J>(*this);
        }

        template <::std::size_t J, typename Allocator, typename Self> // NOLINTNEXTLINE
        static auto make_element(Allocator &&alloc, Self &&self) -> decltype(auto)
        {
            using type = ::std::remove_cvref_t<decltype(product_type_base::element_get<J>(
                std::forward<Self>(self)))>;
            if constexpr (::std::uses_allocator_v<type, Allocator>)
                return ::std::make_obj_using_allocator<type>(
                    alloc, product_type_base::element_get<J>(std::forward<Self>(self)));
            else
                return product_type_base::element_get<J>(std::forward<Self>(self));
        }

        auto operator==(const product_type_base &) const -> bool = default;
    };

    template <typename... T>
    struct product_type : ::mcs::execution::snd::__detail::product_type_base<
                              ::std::index_sequence_for<T...>, T...>
    {

        template <typename Allocator, typename Product, std::size_t... I>
        static auto make_from(Allocator &&allocator, Product &&product, // NOLINT
                              std::index_sequence<I...> /*unused*/) -> product_type
        {
            return {product_type::template make_element<I>(
                allocator, ::std::forward<Product>(product))...};
        }

        template <typename Allocator, typename Product> // NOLINTNEXTLINE
        static auto make_from(Allocator &&allocator, Product &&product) -> product_type
        {
            return product_type::make_from(::std::forward<Allocator>(allocator),
                                           ::std::forward<Product>(product),
                                           ::std::index_sequence_for<T...>{});
        }

        template <typename Fun, ::std::size_t... I> // NOLINTNEXTLINE
        constexpr auto apply_elements(::std::index_sequence<I...>,
                                      Fun &&fun) const -> decltype(auto)
        {
            // Note: 可以不用forward_like的理由：this已经暴露，get<I>3个版本
            return ::std::forward<Fun>(fun)(this->template get<I>()...);
        }
        template <typename Fun>
        constexpr auto apply(Fun &&fun) const -> decltype(auto)
        {
            return apply_elements(::std::index_sequence_for<T...>{},
                                  ::std::forward<Fun>(fun));
        }
        template <typename Fun, ::std::size_t... I> // NOLINTNEXTLINE
        constexpr auto apply_elements(::std::index_sequence<I...>,
                                      Fun &&fun) -> decltype(auto)
        {
            return ::std::forward<Fun>(fun)(this->template get<I>()...);
        }
        template <typename Fun>
        constexpr auto apply(Fun &&fun) -> decltype(auto)
        {
            return apply_elements(::std::index_sequence_for<T...>{},
                                  ::std::forward<Fun>(fun));
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

#pragma once

#include "./__sender_adaptor_closure.hpp"

#include "../snd/__sender.hpp"
#include "../snd/__detail/__product_type.hpp"
#include <utility>

namespace mcs::execution::pipeable
{

    template <typename Adaptor, typename... T>
    struct sender_adaptor
        : snd::__detail::product_type<std::decay_t<Adaptor>, std::decay_t<T>...>,
          sender_adaptor_closure<sender_adaptor<Adaptor, T...>>
    {

        template <snd::sender Sndr, typename Self>
        constexpr static auto apply(Sndr &&sndr, Self &&self) noexcept
        {
            auto &&fun = self.template get<0>(); // for safe life time
            return [&]<std::size_t... I>(std::index_sequence<I...>) {
                return fun(std::forward<Sndr>(sndr),
                           std::forward_like<Self>(self.template get<I + 1>())...);
            }(std::make_index_sequence<sender_adaptor::size() - 1U>{});
        }

        template <snd::sender Sndr>
        constexpr auto operator()(Sndr &&sndr) & noexcept
        {
            return apply(std::forward<Sndr>(sndr), *this);
        }

        template <snd::sender Sndr>
        constexpr auto operator()(Sndr &&sndr) const & noexcept
        {
            return apply(std::forward<Sndr>(sndr), *this);
        }

        template <snd::sender Sndr>
        constexpr auto operator()(Sndr &&sndr) && noexcept
        {
            return apply(std::forward<Sndr>(sndr), std::move(*this));
        }
    };

    template <typename... T>
    sender_adaptor(T &&...) -> sender_adaptor<T...>;

}; // namespace mcs::execution::pipeable
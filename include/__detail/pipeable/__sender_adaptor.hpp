#pragma once

#include "./__sender_adaptor_closure.hpp"

#include "../snd/__sender.hpp"
#include "../snd/__detail/__product_type.hpp"

namespace mcs::execution::pipeable
{

    template <typename Adaptor, typename T0, typename... T>
    struct sender_adaptor
        : snd::__detail::product_type<std::decay_t<Adaptor>, std::decay_t<T0>,
                                      std::decay_t<T>...>,
          sender_adaptor_closure<sender_adaptor<Adaptor, T0, T...>>
    {

        template <snd::sender Sndr>
        static auto apply(Sndr &&sndr, auto &&self) noexcept
        {
            return [&]<std::size_t... I>(std::index_sequence<I...>) {
                return (self.template get<0>())(std::forward<Sndr>(sndr),
                                                self.template get<I + 1>()...);
            }(std::make_index_sequence<sender_adaptor::size() - 1U>{});
        }

        template <snd::sender Sndr>
        auto operator()(Sndr &&sndr) noexcept
        {
            return apply(std::forward<Sndr>(sndr), *this);
        }

        template <snd::sender Sender>
        auto operator()(Sender &&sender) const noexcept
        {
            return apply(std::forward<Sender>(sender), *this);
        }
    };

    template <typename... T>
    sender_adaptor(T &&...) -> sender_adaptor<T...>;

}; // namespace mcs::execution::pipeable
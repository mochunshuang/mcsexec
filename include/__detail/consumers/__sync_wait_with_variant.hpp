#pragma once

#include "./__sync_wait.hpp"
#include "../adapt/__into_variant.hpp"
#include <concepts>
namespace mcs::execution::consumers
{
    namespace __sync_wait
    {
        struct sync_wait_with_variant_t
        {
            template <typename Sndr>
            auto operator()(Sndr &&sndr) const
                requires(snd::sender_in<decltype(adapt::into_variant(sndr)),
                                        sync_wait_env>) &&
                        requires() {
                            typename sync_wait_with_variant_result_type<
                                decltype(adapt::into_variant(sndr))>;
                        } &&
                        std::same_as<decltype(snd::apply_sender(
                                         snd::general::get_domain_early(sndr), *this,
                                         std::forward<Sndr>(sndr))),
                                     sync_wait_with_variant_result_type<
                                         decltype(adapt::into_variant(sndr))>>
            {
                auto dom = snd::general::get_domain_early(sndr);
                return snd::apply_sender(dom, *this, std::forward<Sndr>(sndr));
            }

            template <typename Sndr>
            auto apply_sender(Sndr &&sndr) // NOLINT
                requires functional::callable<sync_wait_t,
                                              decltype(adapt::into_variant(sndr))>
            {
                using result_type =
                    sync_wait_with_variant_result_type<decltype(adapt::into_variant(
                        sndr))>;
                if (auto opt_value = sync_wait(adapt::into_variant(sndr)))
                {
                    return result_type(std::move(std::get<0>(*opt_value)));
                }
                return result_type(std::nullopt);
            }
        };

    }; // namespace __sync_wait

    using __sync_wait::sync_wait_with_variant_t;
    inline constexpr sync_wait_with_variant_t sync_wait_with_variant{}; // NOLINT

}; // namespace mcs::execution::consumers
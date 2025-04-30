#pragma once

#include "../factories/__write_env.hpp"
#include "../queries/__prop.hpp"
#include "../__stoptoken/__never_stop_token.hpp"
#include "../queries/__get_stop_token.hpp"

namespace mcs::execution
{
    namespace adapt
    {
        inline constexpr struct unstoppable_t
        {
            template <snd::sender Sndr>
            auto operator()(Sndr sndr) const noexcept
            {
                return factories::write_env(std::move(sndr),
                                            queries::prop(queries::get_stop_token,
                                                          stoptoken::never_stop_token()));
            }

        } unstoppable{}; // NOLINT

    }; // namespace adapt

    // template <>
    // struct snd::general::impls_for<adapt::unstoppable_t> : snd::__detail::default_impls
    // {
    // };

    template <typename Sndr, typename Env>
    struct cmplsigs::completion_signatures_for_impl<
        snd::__detail::basic_sender<adapt::unstoppable_t, Sndr>, Env>
    {
        using NewEnv = decltype(queries::prop(queries::get_stop_token,
                                              stoptoken::never_stop_token()));
        using type = cmplsigs::completion_signatures_for_impl<
            snd::__detail::basic_sender<factories::write_env_t, NewEnv, Sndr>, Env>::type;
    };
}; // namespace mcs::execution
#pragma once

#include "../__stoptoken/__stoppable_token.hpp"
#include "../__stoptoken/__stop_callback_of_t.hpp"

#include "../snd/__sender.hpp"
#include "../snd/general/__get_domain_early.hpp"
#include "../snd/__make_sender.hpp"
#include "../snd/general/__impls_for.hpp"
#include "../snd/__completion_signatures_of_t.hpp"

#include "../queries/__get_env.hpp"
#include "../queries/__get_stop_token.hpp"
#include "../queries/__stop_token_of_t.hpp"

#include <optional>

namespace mcs::execution
{
    namespace scope
    {
        /**
         * @brief Sender osnd from an exposition-only sender algorithm stop_when(sender
         * auto&& snd, stoppable_token auto stoken) that maps its input sender, snd, such
         * that, when osnd is connected to a receiver r, the resulting operation-state
         * behaves the same as connecting the original sender, snd, to r, except that the
         * operation will receive a stop request when either the token returned from
         * get_stop_token(r) receives a stop request or when stoken receives a stop
         * request
         *
         */
        struct stop_when_t
        {
            template <snd::sender Sndr, stoptoken::stoppable_token Token>
            auto operator()(Sndr &&sndr, Token &&token) const noexcept -> snd::sender auto
            {
                auto dom = snd::general::get_domain_early(std::as_const(sndr));
                return snd::transform_sender(
                    dom, snd::make_sender(*this, std::forward<Token>(token),
                                          std::forward<Sndr>(sndr)));
            }
        };
        inline constexpr stop_when_t stop_when{}; // NOLINT
    }; // namespace scope

    namespace scope
    {
        template <typename Rcvr>
        struct on_stop_request
        {
            Rcvr &rcvr;    // NOLINT
            bool &stopped; // NOLINT
            void operator()() noexcept
            {
                if (not stopped)
                {
                    stopped = true;
                    recv::set_stopped(std::move(rcvr));
                }
            }
        };

        template <typename Token, typename Rcvr>
        struct stop_when_state
        {
            using RcvrToken = queries::stop_token_of_t<queries::env_of_t<Rcvr>>;
            using TokenCallback =
                stoptoken::stop_callback_of_t<Token, on_stop_request<Rcvr>>;
            using RcvrTokenCallback =
                stoptoken::stop_callback_of_t<RcvrToken, on_stop_request<Rcvr>>;

            stop_when_state(Token t, Rcvr &r)
                : token(t), rcvr(r),
                  rcvr_token(queries::get_stop_token(queries::get_env(rcvr)))
            {
            }

            // NOLINTBEGIN
            bool stopped = false;
            Token token;
            Rcvr &rcvr;
            RcvrToken rcvr_token;
            std::optional<TokenCallback> token_callback;
            std::optional<RcvrTokenCallback> rcvr_token_callback;

            void register_callback() noexcept
            {
                token_callback.emplace(token, on_stop_request<Rcvr>{rcvr, stopped});
                rcvr_token_callback.emplace(rcvr_token,
                                            on_stop_request<Rcvr>{rcvr, stopped});
            }
            void unregister_callbacks() noexcept
            {
                token_callback.reset();
                rcvr_token_callback.reset();
            }
            [[nodiscard]] bool stop_requested() const noexcept
            {
                return token.stop_requested() || rcvr_token.stop_requested();
            }
            // NOLINTEND
        };
    }; // namespace scope

    template <>
    struct snd::general::impls_for<scope::stop_when_t> : snd::__detail::default_impls
    {
        static constexpr auto get_state = // NOLINT
            []<class Rcvr>(auto &&sender, Rcvr &rcvr) noexcept {
                auto &[_, token, sndr] = sender;
                return scope::stop_when_state(token, rcvr);
            };

        // NOTE:
        // 默认的statt 是调用 inner_ops.start()...
        // start => Completion
        static constexpr auto start = // NOLINT
            []<class State, class Rcvr, class... Ops>(State &state, Rcvr & /*rcvr*/,
                                                      Ops &...ops) noexcept -> void {
            // Note: 标准规定，request_stop之后的 注册的callback，都立即调用
            state.register_callback();
            if (state.stop_requested())
                state.unregister_callbacks();
            else
            {
                (opstate::start(ops), ...);
            };
        };

        // NOTE: gdb 打印类型： -exec whatis rcvr
        static constexpr auto complete = // NOLINT
            []<class Index, class Tag, class... Args>(Index, auto &state, auto &rcvr, Tag,
                                                      Args &&...args) noexcept -> void {
            // state.rcvr;
            // recv::set_stopped(std::move(state.rcvr));
            state.unregister_callbacks();
            if constexpr (std::is_same_v<Tag, set_stopped_t>)
            {
                if (not state.stop_requested())
                    Tag()(std::move(rcvr), std::forward<Args>(args)...);
            }
            else if constexpr (std::is_same_v<Tag, set_error_t>)
            {
                // NOTE: spawn_receiver no set_error_t handle fun
                std::rethrow_exception(std::forward<Args>(args)...);
            }
            else
            {
                Tag()(std::move(rcvr), std::forward<Args>(args)...);
            }
        };
    };

    template <typename Token, typename Sndr, typename... Env>
    struct cmplsigs::completion_signatures_for_impl<
        snd::__detail::basic_sender<scope::stop_when_t, Token, Sndr>, Env...>
    {
        using type = decltype(snd::completion_signatures_of_t<Sndr, Env...>{} +
                              cmplsigs::completion_signatures<set_stopped_t()>{});
    };

}; // namespace mcs::execution

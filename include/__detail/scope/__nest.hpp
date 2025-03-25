#pragma once
#include <optional>
#include <type_traits>
#include <utility>

#include "./__async_scope_token.hpp"

#include "../snd/__transform_sender.hpp"
#include "../snd/__make_sender.hpp"

#include "../snd/general/__get_domain_early.hpp"
#include "../snd/general/__impls_for.hpp"
#include "../snd/__completion_signatures_of_t.hpp"

#include "../pipeable/__sender_adaptor.hpp"

#include "../recv/__set_stopped.hpp"

namespace mcs::execution
{

    namespace scope
    {
        template <scope::async_scope_token Token, snd::sender Sndr>
        struct nest_data
        {
            using wrap_sender = std::remove_cvref_t<decltype(std::declval<Token &>().wrap(
                std::declval<Sndr>()))>;

            std::optional<wrap_sender> sndr; // NOLINT
            Token token;                     // NOLINT

            nest_data &operator=(const nest_data &) = delete;
            nest_data &operator=(nest_data &&) = delete;
            nest_data(Token t, Sndr &&s) noexcept
                : sndr(t.wrap(std::forward<Sndr>(s))), token(t)
            {
                if (not token.try_associate())
                    sndr.reset();
            }

            // copy
            nest_data(const nest_data &other) noexcept
                requires std::copy_constructible<wrap_sender>
                : token(other.token)
            {
                if (other.sndr.has_value() && token.try_associate())
                {
                    try
                    {
                        sndr.emplace(*other.sndr);
                    }
                    catch (...)
                    {
                        token.disassociate();
                        throw;
                    }
                }
            }

            // alternative copy-constructor implementation:
            nest_data(const nest_data &other) noexcept
                requires(not std::copy_constructible<wrap_sender>)
                : sndr(other.sndr), token(other.token)
            {
                if (other.sndr.has_value() && !token.try_associate())
                    sndr.reset();
            }

            // move
            nest_data(nest_data &&other) noexcept
                : sndr(std::move(other).sndr), token(std::move(other).token)
            {
                static_assert(noexcept(std::is_nothrow_move_constructible_v<wrap_sender>),
                              "need nothrow");
                other.sndr.reset();
            }

            ~nest_data() noexcept
            {
                if (sndr.has_value())
                {
                    sndr.reset();
                    token.disassociate();
                }
            }
        };
        template <async_scope_token Token, snd::sender Sndr>
        nest_data(Token, Sndr &&) -> nest_data<std::decay_t<Token>, std::decay_t<Sndr>>;

        // nest tries to associate a sender with an async scope such that the scope can
        // track
        // the lifetime of any async operations created with the sender.
        struct nest_t
        {
            /**
             * additional effects:
             * 1.the association ends when the nest-sender is destroyed or, if it is
             *   connected, when the resulting operation state is destroyed
             * 2. whatever effects are added by the token’s wrap() method.
             */
            template <snd::sender Sndr, scope::async_scope_token Token>
            auto operator()(Sndr &&sndr, Token &&token) const noexcept
            {
                auto dom = snd::general::get_domain_early(std::as_const(sndr));
                return snd::transform_sender(
                    dom, snd::make_sender(*this, nest_data(std::forward<Token>(token),
                                                           std::forward<Sndr>(sndr))));
            }

            template <scope::async_scope_token Token>
            auto operator()(Token &&token) const noexcept
                -> pipeable::sender_adaptor<nest_t, Token>
            {
                return {*this, std::forward<Token>(token)};
            }
        };
        inline constexpr nest_t nest{}; // NOLINT
    }; // namespace scope

    template <>
    struct snd::general::impls_for<scope::nest_t> : snd::__detail::default_impls
    {
        static constexpr auto get_state = // NOLINT
            []<class Sndr, class Rcvr>(Sndr &&sndr, Rcvr &rcvr) noexcept {
                auto &&[_, data] = std::forward<Sndr>(sndr);
                static_assert(std::is_same_v<decltype(_), scope::nest_t>);

                using scope_token = decltype(data.token);
                using Sndr_t = std::decay_t<decltype(data.sndr.value())>;

                // static_assert(sizeof...(child) == 0);
                using index = Sndr::indices_for;
                static_assert(std::is_same_v<std::index_sequence<>, index>,
                              "Sndr must no child");

                using wrap_sender = std::decay_t<decltype(data)>::wrap_sender;

                struct op_state
                {
                    using op_t = decltype(conn::connect(std::declval<Sndr_t>(),
                                                        std::declval<Rcvr>()));

                    bool associated = false; // NOLINT
                    scope_token token;       // NOLINT
                    union {                  // NOLINT
                        Rcvr *rcvr;
                        op_t op;
                    };

                    op_state(const op_state &) = delete;
                    op_state &operator=(const op_state &) = delete;
                    op_state &operator=(op_state &&) = delete;
                    op_state(op_state &&) = delete;

                    ~op_state() noexcept
                    {
                        if (associated)
                        {
                            token.disassociate();
                            op.~op_t(); // NOLINT
                        }
                    }

                    op_state(scope_token token, Rcvr &r) noexcept
                        : token(std::move(token)), rcvr(std::addressof(r))
                    {
                    }
                    op_state(scope_token token, wrap_sender &&sndr, Rcvr &r) noexcept
                        : associated(true), token(std::move(token)),
                          op(conn::connect(std::move(sndr), std::move(r)))
                    {
                    }
                    op_state(scope_token token, const wrap_sender &sndr, Rcvr &r) noexcept
                        : associated(token.try_associate()), token(std::move(token)),
                          rcvr(std::addressof(r))
                    {
                        if (associated)
                            ::new (static_cast<void *>(&op)) // NOLINT
                                op_t(conn::connect(sndr, std::move(r)));
                    }

                    void start() & noexcept
                    {
                        if (associated)
                            op.start(); // NOLINT
                        else
                            recv::set_stopped(std::move(*rcvr)); // NOLINT
                    }
                };

                if (data.sndr.has_value())
                    return op_state{std::forward_like<Sndr>(data.token),
                                    std::forward_like<Sndr>(data.sndr.value()), rcvr};
                return op_state{data.token, rcvr};
            };

        static constexpr auto start = // NOLINT
            [](auto &state, auto &) noexcept -> void {
            state.start();
        };
    };

    template <typename Token, typename Sndr, typename... Env>
    struct cmplsigs::completion_signatures_for_impl<
        snd::__detail::basic_sender<scope::nest_t, scope::nest_data<Token, Sndr>>, Env...>
    {
        using Add_Sig = cmplsigs::completion_signatures<recv::set_stopped_t()>;
        using type =
            decltype(snd::completion_signatures_of_t<Sndr, Env...>{} + Add_Sig{});
    };

}; // namespace mcs::execution
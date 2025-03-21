#pragma once

#include <memory>

#include "./__async_scope_token.hpp"
#include "../queries/__get_allocator.hpp"
#include "../queries/__get_env.hpp"
#include "../queries/__prop.hpp"
#include "../snd/general/__JOIN_ENV.hpp"
#include "../factories/__write_env.hpp"
#include "./__spawn_state.hpp"

namespace mcs::execution::scope
{

    struct spawn_future_t
    {
        /**
         * @brief spawn_future attempts to associate the given input sender with the given
         * token’s async scope and, on success, eagerly starts the input sender; the
         * return value is a sender that, when connected and started,
         * completes with either the result of the eagerly-started input sender
         * or with set_stopped if the input sender was not started.
         *
         */
        template <snd::sender Sndr, scope::async_scope_token Token, queryable Env>
        auto operator()(Sndr &&sndr, Token &&token, Env &&env) const noexcept
        {
            auto [alloc, senv] = [&]() noexcept {
                if constexpr (requires { queries::get_allocator(env); })
                {
                    using alloc_t = decltype(queries::get_allocator(env));
                    using env_t = std::decay_t<Env>;
                    struct result
                    {
                        alloc_t alloc;
                        env_t env;
                    };
                    return result{queries::get_allocator(env), env};
                }
                else if constexpr (requires {
                                       queries::get_allocator(
                                           queries::get_env(token.wrap(sndr)));
                                   })
                {
                    using alloc_t = decltype(queries::get_allocator(
                        queries::get_env(token.wrap(sndr))));
                    auto alloc =
                        queries::get_allocator(queries::get_env(token.wrap(sndr)));
                    using env_t = decltype(snd::general::JOIN_ENV(
                        env, queries::prop(queries::get_allocator, alloc)));
                    struct result
                    {
                        alloc_t alloc;
                        env_t env;
                    };
                    return result{alloc,
                                  snd::general::JOIN_ENV(
                                      env, queries::prop(queries::get_allocator, alloc))};
                }
                else
                {
                    using alloc_t = std::allocator<void>;
                    using env_t = std::decay_t<Env>;
                    struct result
                    {
                        alloc_t alloc;
                        env_t env;
                    };
                    return result{{}, env};
                }
            }();
            auto makeSender = [&] noexcept {
                return factories::write_env(token.wrap(std::forward<Sndr>(sndr)), senv);
            };

            using sender_t = decltype(makeSender());

            using state_t = spawn_state<decltype(alloc), Token, sender_t>;
            using alloc_t =
                std::allocator_traits<decltype(alloc)>::template rebind_alloc<state_t>;
            using traits_t = std::allocator_traits<alloc_t>;

            alloc_t stateAlloc{alloc};
            auto *op = traits_t::allocate(stateAlloc, 1);

            try
            {
                traits_t::construct(stateAlloc, op, alloc, makeSender(), token);
            }
            catch (...)
            {
                traits_t::deallocate(stateAlloc, op, 1);
                throw;
            }

            try
            {
                op->run();
            }
            catch (...)
            {
                traits_t::destroy(stateAlloc, op);
                traits_t::deallocate(stateAlloc, op, 1);
                throw;
            }
        }
        template <snd::sender Sndr, scope::async_scope_token Token>
        auto operator()(Sndr &&sndr, Token &&token) const noexcept
        {
            return this(std::forward(sndr), std::forward<Token>(token), empty_env{});
        }
    };
    inline constexpr spawn_future_t spawn_future{}; // NOLINT

}; // namespace mcs::execution::scope

#pragma once

#include "./__async_scope_token.hpp"
#include "../snd/__sender.hpp"
#include <memory>
#include <utility>

#include "../queries/__get_allocator.hpp"
#include "../queries/__prop.hpp"
#include "../snd/general/__JOIN_ENV.hpp"
#include "../factories/__write_env.hpp"

#include "./__spawn_state.hpp"

namespace mcs::execution
{
    namespace scope
    {
        namespace __detail
        {
            struct inline_scheduler
            {
                struct env
                {
                    [[nodiscard]] static constexpr inline_scheduler query(
                        const queries::get_completion_scheduler_t<set_value_t>
                            & /*unused*/) noexcept
                    {
                        return {};
                    }
                };
                template <recv::receiver Receiver>
                struct state
                {
                    using operation_state_concept = operation_state_t;
                    std::remove_cvref_t<Receiver> receiver; // NOLINT
                    void start() & noexcept
                    {
                        recv::set_value(std::move(receiver));
                    }
                };
                struct sender
                {
                    using sender_concept = sender_t;
                    using completion_signatures =
                        cmplsigs::completion_signatures<set_value_t()>;

                    [[nodiscard]] env get_env() const noexcept // NOLINT
                    {
                        return {};
                    }
                    template <recv::receiver Receiver>
                    state<Receiver> connect(Receiver &&receiver) noexcept
                    {
                        return {std::forward<Receiver>(receiver)};
                    }
                };
                static_assert(snd::sender<sender>);

                using scheduler_concept = scheduler_t;
                inline_scheduler() = default;

                static constexpr sender schedule() noexcept
                {
                    return {};
                }
                bool operator==(const inline_scheduler &) const = default;
            };

            struct scheduler_env_t
            {
                [[nodiscard]] constexpr auto query( // NOLINT
                    const mcs::execution::queries::get_scheduler_t & /*unused*/)
                    const noexcept
                {
                    return inline_scheduler();
                }
            };

            template <typename Env>
            auto __may_add_get_scheduler_env(Env &&env) -> decltype(auto)
            {
                if constexpr (requires { queries::get_scheduler(env); })
                {
                    return std::forward<Env>(env);
                }
                else
                {
                    return snd::general::JOIN_ENV(scheduler_env_t{},
                                                  std::forward<Env>(env));
                }
            }

            template <snd::sender Sndr, class Env>
            auto __choose_allocator_and_env(const Sndr &newSndr, const Env &env) noexcept
            {
                if constexpr (requires { queries::get_allocator(env); })
                {
                    return std::pair{queries::get_allocator(env), env};
                }
                else if constexpr (requires {
                                       queries::get_allocator(queries::get_env(newSndr));
                                   })
                {
                    auto alloc = queries::get_allocator(queries::get_env(newSndr));
                    return std::pair{
                        alloc, snd::general::JOIN_ENV(
                                   queries::prop{queries::get_allocator, alloc}, env)};
                }
                else
                    return std::pair{std::allocator<void>{}, env};
            }
        }; // namespace __detail

        // spawn attempts to associate the given input sender with the given token’s async
        // scope and, on success, eagerly starts the input sender.
        struct spawn_t
        {
            template <snd::sender Sndr, async_scope_token Token,
                      class Env = empty_env> // NOLINTNEXTLINE
            auto operator()(Sndr &&sndr, Token token, Env &&env = {}) const -> void
            {

                auto newSndr = token.wrap(std::forward<Sndr>(sndr));
                auto [alloc, senv] = __detail::__choose_allocator_and_env(newSndr, env);

                auto makeSender = [&]() noexcept {
                    return factories::write_env(
                        std::move(newSndr),
                        __detail::__may_add_get_scheduler_env(std::move(senv)));
                };

                using sender_t = decltype(makeSender());

                using state_t = spawn_state<decltype(alloc), Token, sender_t>;
                using alloc_t = typename std::allocator_traits<
                    decltype(alloc)>::template rebind_alloc<state_t>;
                using allocator_t = std::allocator_traits<alloc_t>;

                // NOTE: 标准库的 deallocate 的有异常的.堆分配容易有异常
                alloc_t stateAlloc{alloc};
                auto *op = allocator_t::allocate(stateAlloc, 1);

                try
                {
                    allocator_t::construct(stateAlloc, op, alloc, makeSender(), token);
                }
                catch (...)
                {
                    allocator_t::deallocate(stateAlloc, op, 1);
                    throw;
                }

                try
                {
                    op->run();
                }
                catch (...)
                {
                    allocator_t::destroy(stateAlloc, op);
                    allocator_t::deallocate(stateAlloc, op, 1);
                    throw;
                }
            }
        };
        inline constexpr spawn_t spawn{}; // NOLINT
    }; // namespace scope

}; // namespace mcs::execution

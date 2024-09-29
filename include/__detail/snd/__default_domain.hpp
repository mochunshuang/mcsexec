#pragma once

#include "./__sender.hpp"
#include "./__tag_of_t.hpp"

namespace mcs::execution::snd
{
    namespace __detail
    {
        template <typename Sndr, typename... Env>
        concept has_transform_sender = requires(Sndr &&sndr, const Env &...env) {
            { tag_of_t<Sndr>().transform_sender(std::forward<Sndr>(sndr), env...) };
        };

        template <typename Sndr, typename Env>
        concept has_transform_env = requires(Sndr &&sndr, Env &&env) {
            {
                tag_of_t<Sndr>().transform_env(std::forward<Sndr>(sndr),
                                               std::forward<Env>(env))
            } noexcept;
        };
        template <typename Tag, typename Sndr, typename... Args>
        concept has_apply_sender = requires(Tag t, Sndr &&sndr, Args &&...args) {
            { t.apply_sender(std::forward<Sndr>(sndr), std::forward<Args>(args)...) };
        };
    }; // namespace __detail

    struct default_domain
    {
        /////////////////////////////////////////////////////
        // transform_sender
        template <sender Sndr, queryable... Env>
            requires(sizeof...(Env) <= 1) && __detail::has_transform_sender<Sndr, Env...>
        static constexpr sender auto transform_sender( // NOLINT
            Sndr &&sndr,
            const Env &...env) noexcept(noexcept(tag_of_t<Sndr>()
                                                     .transform_sender(
                                                         std::forward<Sndr>(sndr),
                                                         env...)))
        {
            return tag_of_t<Sndr>().transform_sender(std::forward<Sndr>(sndr), env...);
        }
        template <sender Sndr, queryable... Env>
            requires(sizeof...(Env) <= 1) &&
                    (not __detail::has_transform_sender<Sndr, Env...>)
        static constexpr sender auto transform_sender( // NOLINT
            Sndr &&sndr,
            const Env &.../*env*/) noexcept(noexcept(std::forward<Sndr>(sndr)))
        {
            return std::forward<Sndr>(sndr);
        }

        /////////////////////////////////////////////////////
        // transform_env
        template <sender Sndr, queryable Env>
            requires(__detail::has_transform_env<Sndr, Env>) // NOLINTNEXTLINE
        static constexpr queryable auto transform_env(Sndr &&sndr, Env &&env) noexcept
        {
            return tag_of_t<Sndr>().transform_env(std::forward<Sndr>(sndr),
                                                  std::forward<Env>(env));
        }
        template <sender Sndr, queryable Env>
            requires(not __detail::has_transform_env<Sndr, Env>) &&
                    requires(Sndr &&sndr, Env &&env) {
                        { static_cast<Env>(std::forward<Env>(env)) } noexcept;
                    } // NOLINTNEXTLINE
        static constexpr queryable auto transform_env(Sndr && /*sndr*/,
                                                      Env &&env) noexcept
        {
            return static_cast<Env>(std::forward<Env>(env));
        }

        /////////////////////////////////////////////////////
        // apply_sender
        template <class Tag, sender Sndr, class... Args>
            requires __detail::has_apply_sender<Tag, Sndr, Args...>
        static constexpr auto apply_sender( // NOLINT
            Tag /*unused*/, Sndr &&sndr,
            Args &&...args) noexcept(noexcept(Tag().apply_sender(std::forward<Sndr>(sndr),
                                                                 std::forward<Args>(
                                                                     args)...)))
        {
            return Tag().apply_sender(std::forward<Sndr>(sndr),
                                      std::forward<Args>(args)...);
        }
    };
}; // namespace mcs::execution::snd

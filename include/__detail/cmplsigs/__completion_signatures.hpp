#pragma once

#include "./__completion_signature.hpp"

#include "../snd/general/__MATCHING_SIG.hpp"

namespace mcs::execution::cmplsigs
{
    template <completion_signature... Sigs>
    struct completion_signatures
    {
        template <typename Fn, typename Tag>
        static constexpr inline int is_tag_sig = 0; // NOLINT
        template <typename Tag, typename... Args>
        static constexpr inline int is_tag_sig<Tag(Args...), Tag> = 1; // NOLINT
        template <class Tag>
        static constexpr size_t count = (0 + ... + is_tag_sig<Sigs, Tag>); // NOLINT

        template <class Sig> // NOLINTNEXTLINE
        static constexpr bool contains = (snd::general::MATCHING_SIG<Sig, Sigs> || ...);

        consteval auto operator+(completion_signatures<> /*unused*/) const noexcept
        {
            return *this;
        }

        template <class Sig>
        consteval auto operator+(completion_signatures<Sig> /*unused*/) const noexcept
        {
            if constexpr (contains<Sig>)
                return *this;
            else
                return completion_signatures<Sigs..., snd::general::NORMALIZE_SIG<Sig>>{};
        }

        template <class... Ts>
        consteval auto operator+(completion_signatures<Ts...> /*unused*/) const noexcept
        {
            return (*this + ... +
                    completion_signatures<snd::general::NORMALIZE_SIG<Ts>>{});
        }

        template <class Fn>
        static consteval auto transform_sigs(Fn &&fn) // NOLINT
        {
            return (std::forward<Fn>(fn)(static_cast<Sigs *>(nullptr)) + ... +
                    completion_signatures<>{});
        }

        template <class Tag>
        static consteval auto filter_sigs() // NOLINT
        {
            return (std::conditional_t<(is_tag_sig<Sigs, Tag> == 1),
                                       completion_signatures<Sigs>,
                                       completion_signatures<>>{} +
                    ... + completion_signatures<>{});
        }

        template <class Fn>
        static consteval void check_sigs(Fn &&fn) // NOLINT
        {
            (std::forward<Fn>(fn)(static_cast<Sigs *>(nullptr)), ...);
        }
    };
}; // namespace mcs::execution::cmplsigs
#pragma once

#include "__default_domain.hpp"

namespace mcs::execution::snd
{
    namespace __detail
    {
        template <class Domain, class Tag, class Sndr, class... Args>
        concept dom_apply_sender =
            requires(Domain dom, Tag tag, Sndr &&sndr, Args &&...args) {
                dom.apply_sender(Tag(), std::forward<Sndr>(sndr),
                                 std::forward<Args>(args)...);
            };

    }; // namespace __detail

    //////////////////////////////////////////////////
    // [exec.snd.apply], sender algorithm application
    template <class Domain, class Tag, sender Sndr, class... Args>
        requires(__detail::dom_apply_sender<Domain, Tag, Sndr, Args...>)
    constexpr decltype(auto) apply_sender(
        Domain dom, Tag /*unused*/, Sndr &&sndr,
        Args &&...args) noexcept(noexcept(dom.apply_sender(Tag(),
                                                           std::forward<Sndr>(sndr),
                                                           std::forward<Args>(args)...)))
    {
        return dom.apply_sender(Tag(), std::forward<Sndr>(sndr),
                                std::forward<Args>(args)...);
    }

    template <class Domain, class Tag, sender Sndr, class... Args>
        requires(not __detail::dom_apply_sender<Domain, Tag, Sndr, Args...>) &&
                requires(Tag tag, Sndr &&sndr, Args &&...args) {
                    default_domain().apply_sender(Tag(), std::forward<Sndr>(sndr),
                                                  std::forward<Args>(args)...);
                }
    constexpr decltype(auto) apply_sender(
        Domain /*dom*/, Tag /*unused*/, Sndr &&sndr,
        Args &&...args) noexcept(noexcept(default_domain()
                                              .apply_sender(Tag(),
                                                            std::forward<Sndr>(sndr),
                                                            std::forward<Args>(args)...)))
    {
        return default_domain().apply_sender(Tag(), std::forward<Sndr>(sndr),
                                             std::forward<Args>(args)...);
    }
}; // namespace mcs::execution::snd

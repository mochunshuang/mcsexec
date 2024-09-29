#pragma once

#include "./__default_domain.hpp"

namespace mcs::execution::snd
{
    namespace __detail
    {
        template <class Domain, class Sndr, class... Env>
        concept dom_can_transform_sender =
            requires(Domain dom, Sndr &&sndr, const Env &...env) {
                { dom.transform_sender(std::forward<Sndr>(sndr), env...) } -> sender;
            };
    }; // namespace __detail

    /////////////////////////////////////////////
    // [exec.snd.transform], sender transformations
    // dom_can_transform_sender: 0
    // Note: use RVO not decltype(auto)
    template <class Domain, sender Sndr, queryable... Env>
        requires(sizeof...(Env) <= 1 &&
                 __detail::dom_can_transform_sender<Domain, Sndr, Env...>)
    constexpr sender auto transform_sender(
        Domain dom, Sndr &&sndr,
        const Env
            &...env) noexcept(noexcept(dom.transform_sender(std::forward<Sndr>(sndr),
                                                            env...)))
        requires(std::is_same_v<std::remove_cvref_t<decltype(dom.transform_sender(
                                    std::forward<Sndr>(sndr), env...))>,
                                std::remove_cvref_t<Sndr>>)

    {
        return dom.transform_sender(std::forward<Sndr>(sndr), env...);
    }
    // dom_can_transform_sender: 1
    template <class Domain, sender Sndr, queryable... Env>
        requires(sizeof...(Env) <= 1 &&
                 __detail::dom_can_transform_sender<Domain, Sndr, Env...>)
    constexpr sender auto transform_sender(
        Domain dom, Sndr &&sndr,
        const Env
            &...env) noexcept(noexcept(transform_sender(dom,
                                                        dom.transform_sender(
                                                            std::forward<Sndr>(sndr),
                                                            env...),
                                                        env...)))
        requires(not std::is_same_v<std::remove_cvref_t<decltype(dom.transform_sender(
                                        std::forward<Sndr>(sndr), env...))>,
                                    std::remove_cvref_t<Sndr>>)
    {
        return transform_sender(
            dom, dom.transform_sender(std::forward<Sndr>(sndr), env...), env...);
    }

    // not dom_can_transform_sender: 0
    template <class Domain, sender Sndr, queryable... Env>
        requires(sizeof...(Env) <= 1 &&
                 not __detail::dom_can_transform_sender<Domain, Sndr, Env...>)
    constexpr sender auto transform_sender(
        Domain /*dom*/, Sndr &&sndr,
        const Env &...env) noexcept(noexcept(default_domain()
                                                 .transform_sender(
                                                     std::forward<Sndr>(sndr), env...)))
        requires(
            std::is_same_v<std::remove_cvref_t<decltype(default_domain().transform_sender(
                               std::forward<Sndr>(sndr), env...))>,
                           std::remove_cvref_t<Sndr>>)
    {
        return default_domain().transform_sender(std::forward<Sndr>(sndr), env...);
    }
    // not dom_can_transform_sender: 1
    template <class Domain, sender Sndr, queryable... Env>
        requires(sizeof...(Env) <= 1 &&
                 not __detail::dom_can_transform_sender<Domain, Sndr, Env...>)
    constexpr sender auto transform_sender(
        Domain dom, Sndr &&sndr,
        const Env
            &...env) noexcept(noexcept(transform_sender(dom,
                                                        default_domain().transform_sender(
                                                            std::forward<Sndr>(sndr),
                                                            env...),
                                                        env...)))
        requires(not std::is_same_v<
                 std::remove_cvref_t<decltype(default_domain().transform_sender(
                     std::forward<Sndr>(sndr), env...))>,
                 std::remove_cvref_t<Sndr>>)
    {
        return transform_sender(
            dom, default_domain().transform_sender(std::forward<Sndr>(sndr), env...),
            env...);
    }

}; // namespace mcs::execution::snd
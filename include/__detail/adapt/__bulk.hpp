#pragma once

#include <utility>

#include "../snd/__transform_sender.hpp"
#include "../snd/__make_sender.hpp"
#include "../snd/__completion_signatures_of_t.hpp"

#include "../snd/general/__get_domain_early.hpp"
#include "../snd/general/__impls_for.hpp"

namespace mcs::execution
{
    namespace adapt
    {
        template <typename T>
        concept shape = true; // std::integral<T>;

        // bulk runs a task repeatedly for every index in an index space.
        struct bulk_t
        {
            template <snd::sender Sndr, shape Shape, movable_value Fun>
            auto operator()(Sndr &&sndr, Shape &&shape, Fun &&f) const // noexcept
            {
                auto dom = snd::general::get_domain_early(std::as_const(sndr));
                return snd::transform_sender(
                    dom, snd::make_sender(
                             *this,
                             snd::__detail::product_type{std::forward<Shape>(shape),
                                                         std::forward<Fun>(f)},
                             std::forward<Sndr>(sndr)));
            }
        };
        inline constexpr bulk_t bulk{}; // NOLINT
    }; // namespace adapt

    template <>
    struct snd::general::impls_for<adapt::bulk_t> : snd::__detail::default_impls
    {
        // Note: args is a pack of lvalue subexpressions referring to the value completion
        // Note: result datums of the input sender,otherwise undefined behavior.
        static constexpr auto complete = // NOLINT
            []<class Index, class State, class Rcvr, class Tag, class... Args>(
                Index, State &state, Rcvr &rcvr, Tag, Args &&...args) noexcept -> void
            requires(
                not std::same_as<Tag, set_value_t> ||
                std::is_invocable_v<decltype(state.template get<1>()),
                                    decltype(auto(state.template get<0>())), Args &...>)
        {
            if constexpr (std::same_as<Tag, set_value_t>)
            {
                auto &[shape, f] = state;
                try
                {
                    [&]() noexcept(noexcept(f(auto(shape), args...))) {
                        for (decltype(auto(shape)) i = 0; i < shape; ++i)
                        {

                            f(auto(i), args...);
                        }
                        Tag()(std::move(rcvr), std::forward<Args>(args)...);
                    }();
                }
                catch (...)
                {
                    recv::set_error(std::move(rcvr), std::current_exception());
                }
            }
            else
            {
                Tag()(std::move(rcvr), std::forward<Args>(args)...);
            }
        };
    };

    template <typename Sched, typename Sndr, typename Env>
    struct cmplsigs::completion_signatures_for_impl<
        snd::__detail::basic_sender<adapt::bulk_t, Sched, Sndr>, Env>
    {
        using type = snd::completion_signatures_of_t<Sndr, Env>;
    };

}; // namespace mcs::execution
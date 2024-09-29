#pragma once

#include "__default_domain.hpp"
#include <utility>

namespace mcs::execution::snd
{
    /////////////////////////////////////////////
    // [exec.snd.transform.env], environment transformations
    template <class Domain, sender Sndr, queryable Env>
    constexpr queryable auto transform_env(Domain dom, Sndr &&sndr, Env &&env) noexcept
    {
        if constexpr (requires {
                          {
                              dom.transform_env(std::forward<Sndr>(sndr),
                                                std::forward<Env>(env))
                          } noexcept;
                      })
        {
            return dom.transform_env(std::forward<Sndr>(sndr), std::forward<Env>(env));
        }
        else if constexpr (requires {
                               {
                                   default_domain().transform_env(
                                       std::forward<Sndr>(sndr), std::forward<Env>(env))
                               } noexcept;
                           })
        {
            return default_domain().transform_env(std::forward<Sndr>(sndr),
                                                  std::forward<Env>(env));
        }
    }

}; // namespace mcs::execution::snd

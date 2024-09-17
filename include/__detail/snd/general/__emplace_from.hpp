#pragma once

#include "../../__functional/__callable.hpp"
#include "../../__functional/__call_result_t.hpp"
#include "../../__functional/__nothrow_callable.hpp"

namespace mcs::execution::snd::general
{

    // emplace-from is used to emplace non-movable types into tuple, optional,
    // variant, and similar types.
    template <functional::callable Fun>
        requires std::is_nothrow_move_constructible_v<Fun>
    struct emplace_from // exposition only
    {
        Fun fun; // exposition only // NOLINT
        using type = functional::call_result_t<Fun>;

        constexpr operator type() && noexcept(functional::nothrow_callable<Fun>) // NOLINT
        {
            return std::move(fun)();
        }
        constexpr type operator()() && noexcept(functional::nothrow_callable<Fun>)
        {
            return std::move(fun)();
        }
    };

}; // namespace mcs::execution::snd::general
#pragma once

namespace mcs::execution::snd::__detail
{
    template <template <class...> class T, class... Args>
    concept valid_specialization = requires { typename T<Args...>; }; // exposition only

}; // namespace mcs::execution::snd::__detail
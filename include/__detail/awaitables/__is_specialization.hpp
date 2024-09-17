#pragma once

#include <type_traits>

namespace mcs::execution::awaitables::__detail
{
    template <class Test, template <class...> class Ref>
    struct __is_specialization : std::false_type
    {
    };

    template <template <class...> class Ref, class... Args>
    struct __is_specialization<Ref<Args...>, Ref> : std::true_type
    {
    };

    template <class Test, template <class...> class Ref> // NOLINTNEXTLINE
    inline constexpr bool __is_specialization_v = __is_specialization<Test, Ref>::value;

}; // namespace mcs::execution::awaitables::__detail
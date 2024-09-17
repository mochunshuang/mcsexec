
#include <utility>

namespace mcs::execution::functional
{
    // exposition only:
    template <class Fn, class... Args>
    using call_result_t = decltype(std::declval<Fn>()(std::declval<Args>()...));

}; // namespace mcs::execution::functional

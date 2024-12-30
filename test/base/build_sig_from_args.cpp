
#include "../../include/execution.hpp"
#include <tuple>
#include <type_traits>

using namespace mcs::execution; // NOLINT

template <typename Tag, typename Tuple>
struct build_sig_from_args;

template <typename Tag, typename... T>
struct build_sig_from_args<Tag, std::tuple<T...>>
{
    using type = Tag(T...);
};

int main()
{

    using args = std::tuple<int, double, float &>;
    using Target =
        cmplsigs::completion_signatures<recv::set_value_t(int, double, float &)>;

    static_assert(
        std::is_same_v<Target, cmplsigs::completion_signatures<
                                   build_sig_from_args<recv::set_value_t, args>::type>>);
    {
        using args = std::tuple<>;
        using Target = cmplsigs::completion_signatures<recv::set_value_t()>;
        static_assert(
            std::is_same_v<Target, cmplsigs::completion_signatures<build_sig_from_args<
                                       recv::set_value_t, args>::type>>);
    }

    return 0;
}
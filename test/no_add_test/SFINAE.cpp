#include <type_traits>
#include <iostream>

struct set_value_t;
struct set_error_t;
struct set_stopped_t;

namespace __detail
{
    template <class _Sig>
    inline constexpr bool __is_compl_sig = false; // NOLINT

    template <class... _Args>
    inline constexpr bool __is_compl_sig<set_value_t(_Args...)> = true; // NOLINT
    template <class _Error>
    inline constexpr bool __is_compl_sig<set_error_t(_Error)> = true; // NOLINT
    template <>
    inline constexpr bool __is_compl_sig<set_stopped_t()> = true; // NOLINT

    template <class Fn>
    concept completion_signature = __is_compl_sig<Fn>;

}; // namespace __detail

////////////////////////////////////
// [exec.utils.cmplsigs]
template <class Fn>
concept completion_signature = __detail::completion_signature<Fn>;

template <completion_signature... Sigs>
struct completion_signatures
{
};
namespace __detail
{
    template <class Sigs>
    inline constexpr bool __completion_signatures = false; // NOLINT

    template <completion_signature... Sigs>
    inline constexpr bool // NOLINTNEXTLINE
        __completion_signatures<completion_signatures<Sigs...>> = true;
}; // namespace __detail

template <class Completions>
concept valid_completion_signatures = __detail::__completion_signatures<Completions>;

template <class Sndr, class... Env>
concept has_constexpr_completions = // exposition only
    valid_completion_signatures<
        decltype(std::remove_reference_t<Sndr>::template get_completion_signatures<
                 Sndr, Env...>())>;

struct MySndr
{
};

struct Sndr
{
    template <typename S, typename... E>
    static consteval auto get_completion_signatures() // NOLINT
    {
        return completion_signatures<set_value_t()>{};
    }
};

int main()
{
    // NOTE: decltype 自带 SFINAE
    {
        constexpr bool v = has_constexpr_completions<MySndr>; // NOLINT
        static_assert(not v);
    }
    {
        constexpr bool v = has_constexpr_completions<Sndr>; // NOLINT
        static_assert(v);
    }
    std::cout << "main done\n";
    return 0;
}
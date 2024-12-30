#include "../../include/execution.hpp"

#include <cstdlib>
#include <iostream>
#include <type_traits>

namespace __detail
{
    template <typename F1, typename F2>
    inline constexpr bool convertible_args = false; // NOLINT

    template <typename R1, typename R2, typename... Args1, typename... Args2>
        requires(sizeof...(Args1) == sizeof...(Args2)) &&
                    ((std::is_same_v<std::decay_t<Args1>, std::decay_t<Args2>> &&
                      std::is_convertible_v<Args1, Args2>) &&
                     ...)
    inline constexpr bool convertible_args<R1(Args1...), R2(Args2...)> = true; // NOLINT

    template <typename F1, typename F2>
    struct convertible_sig
    {
        static constexpr bool value = false; // NOLINT
    };
    template <typename R1, typename R2, typename... Args1, typename... Args2>
        requires std::same_as<R1, R2> && convertible_args<R1(Args1...), R2(Args2...)>
    struct convertible_sig<R1(Args1...), R2(Args2...)>
    {
        static constexpr bool value = true; // NOLINT
    };
}; // namespace __detail

template <typename From, typename To>
inline constexpr bool CONVERTIBLE_SIG = // NOLINT
    __detail::convertible_sig<From, To>::value;

template <typename List, typename To>
inline constexpr bool HAS_CONVERTIBLE_SIG = false; // NOLINT

template <template <typename...> typename List, typename To, typename... From>
    requires(__detail::convertible_args<From, To> || ...)
inline constexpr bool HAS_CONVERTIBLE_SIG<List<From...>, To> = true; // NOLINT

int main()
{
    using namespace mcs::execution; // NOLINT

    // From int&&
    {
        using From = int &&;
        {
            using To = int;
            static_assert(std::is_convertible_v<From, To>);
        }
        {
            using To = int &;
            static_assert(not std::is_convertible_v<From, To>);
        }
        {
            using To = const int &;
            static_assert(std::is_convertible_v<From, To>);
        }
    }

    // Note: 完成签名一般都是 move
    //  sig: from inT&&
    {

        using From = recv::set_value_t(int &&);
        {
            using To = recv::set_value_t(int);
            static_assert(std::is_convertible_v<int &&, int>);
            static_assert(CONVERTIBLE_SIG<From, To>);
        }
        {
            using To = recv::set_value_t(int &);
            static_assert(not CONVERTIBLE_SIG<From, To>);
        }
        {
            using To = recv::set_value_t(const int &);
            static_assert(CONVERTIBLE_SIG<From, To>);
        }
        {
            using To = recv::set_value_t(int &&);
            static_assert(CONVERTIBLE_SIG<From, To>);
        }
    }
    // sig: from inT&
    {

        using From = recv::set_value_t(int &);
        {
            using To = recv::set_value_t(int);
            static_assert(std::is_convertible_v<int &&, int>);
            static_assert(CONVERTIBLE_SIG<From, To>);
        }
        {
            using To = recv::set_value_t(int &);
            static_assert(CONVERTIBLE_SIG<From, To>);
        }
        {
            using To = recv::set_value_t(const int &);
            static_assert(CONVERTIBLE_SIG<From, To>);
        }
        {
            using To = recv::set_value_t(int &&);
            static_assert(not CONVERTIBLE_SIG<From, To>);
        }
    }
    // list
    {
        // Note: 小心
        static_assert(std::is_convertible_v<int, double>);

        using From = int &&;
        using To = int;
        static_assert(std::is_convertible_v<From, To>);
        static_assert(__detail::convertible_args<int(int), int(int)>);
        static_assert(
            __detail::convertible_args<recv::set_value_t(From), recv::set_value_t(To)>);
        static_assert(not __detail::convertible_args<recv::set_value_t(int),
                                                     recv::set_value_t(double)>);
        static_assert(
            __detail::convertible_args<recv::set_value_t(From), recv::set_value_t(To)>);

        using List = cmplsigs::completion_signatures<recv::set_value_t(int &&),
                                                     recv::set_value_t(double)>;
        static_assert(HAS_CONVERTIBLE_SIG<List, recv::set_value_t(To)>);

        static_assert(not HAS_CONVERTIBLE_SIG<cmplsigs::completion_signatures<>,
                                              recv::set_value_t(To)>);
    }
    return 0;
}
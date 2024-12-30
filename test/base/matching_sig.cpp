
#include "../../include/execution.hpp"

#include <concepts>

namespace __detail
{
    template <typename F1, typename F2>
    struct matching_sig
    {
        static constexpr bool value = false; // NOLINT
    };
    template <typename R1, typename R2, typename... Args1, typename... Args2>
        requires std::same_as<R1(Args1 &&...), R2(Args2 &&...)>
    struct matching_sig<R1(Args1...), R2(Args2...)>
    {
        static constexpr bool value = true; // NOLINT
    };
}; // namespace __detail

template <typename F1, typename F2>
inline constexpr bool MATCHING_SIG = // NOLINT
    __detail::matching_sig<F1, F2>::value;

int main()
{
    namespace ex = mcs::execution;

    {
        static_assert(not std::same_as<int, int &>);
        static_assert(not std::same_as<int, int &&>);
        static_assert(not std::same_as<int, const int &>);

        static_assert(not std::same_as<int &, const int &>);
        static_assert(not std::same_as<int &, int &&>);

        static_assert(not std::same_as<int &&, const int &>);
        static_assert(not std::same_as<int &&, int &>);
    }

    // To  int
    {
        using To = ex::set_value_t(int);
        {
            using From = ex::set_value_t(int &&);
            static_assert(MATCHING_SIG<To, From>);
        }
        {
            using From = ex::set_value_t(int &);
            static_assert(not MATCHING_SIG<To, From>);
        }
        {
            using From = ex::set_value_t(const int &);
            static_assert(not MATCHING_SIG<To, From>);
        }
    }
    // from  int && to  ...
    {

        using From = ex::set_value_t(int &&);
        {
            using To = ex::set_value_t(int);
            static_assert(MATCHING_SIG<To, From>);
        }
        {

            using To = ex::set_value_t(int &);
            static_assert(not MATCHING_SIG<To, From>);
        }
        {
            using To = ex::set_value_t(const int &);
            static_assert(not MATCHING_SIG<To, From>);
        }
    }
    // from  int  to  ...
    {

        using From = ex::set_value_t(int);
        {
            using To = ex::set_value_t(int &&);
            static_assert(MATCHING_SIG<To, From>);
        }
        {

            using To = ex::set_value_t(int &);
            static_assert(not MATCHING_SIG<To, From>);
        }
        {
            using To = ex::set_value_t(const int &);
            static_assert(not MATCHING_SIG<To, From>);
        }
    }
    return 0;
}
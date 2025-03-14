#include "completion_signatures.h"
#include <exception>

constexpr void testtypeset_deduplication_order() // NOLINT
{
    using namespace std; // NOLINT

    completion_signatures<set_value_t(int), set_error_t(exception_ptr)> cs1;
    completion_signatures<set_stopped_t(), set_error_t(exception_ptr)> cs2;
    auto cs3 = cs1 + cs2;

    static_assert(
        std::is_same_v<decltype(cs3),
                       completion_signatures<set_value_t(int), set_error_t(exception_ptr),
                                             set_stopped_t()>>);

    static_assert(
        std::is_same_v<decltype(completion_signatures<set_value_t(int),
                                                      set_error_t(exception_ptr)>{} +
                                completion_signatures<set_stopped_t(),
                                                      set_error_t(exception_ptr)>{}),
                       completion_signatures<set_value_t(int), set_error_t(exception_ptr),
                                             set_stopped_t()>>);
    {
        static_assert(decltype(cs3)::count<set_value_t> == 1);
        static_assert(decltype(cs3)::count<set_error_t> == 1);
        static_assert(decltype(cs3)::count<set_stopped_t> == 1);
        static_assert(completion_signatures<>::count<set_value_t> == 0);
        static_assert(completion_signatures<>::count<set_error_t> == 0);
        static_assert(completion_signatures<>::count<set_stopped_t> == 0);

        static_assert(
            decltype(cs3 +
                     completion_signatures<set_value_t(double)>{})::count<set_value_t> ==
            2);
        using CS = decltype(cs3 + completion_signatures<set_value_t(double)>{});
        using CS2 = decltype(CS::filter_sigs_by_Tag<set_value_t>());
        static_assert(
            std::is_same_v<CS2,
                           completion_signatures<set_value_t(int), set_value_t(double)>>);

        {
            using CS2 =
                decltype(completion_signatures<>::filter_sigs_by_Tag<set_value_t>());
            static_assert(std::is_same_v<CS2, completion_signatures<>>);
        }
    }

    {
        completion_signatures<> empty;
        auto cs3 = cs1 + empty;
        static_assert(std::is_same_v<decltype(cs3), decltype(cs1)>);

        auto cs4 = cs2 + empty;
        static_assert(std::is_same_v<decltype(cs4), decltype(cs2)>);

        auto cs5 = cs3 + empty;
        static_assert(std::is_same_v<decltype(cs5), decltype(cs3)>);
    }
    {
        completion_signatures<> empty;
        auto cs3 = empty + cs1;
        static_assert(std::is_same_v<decltype(cs3), decltype(cs1)>);
        auto cs4 = empty + cs2;
        static_assert(std::is_same_v<decltype(cs4), decltype(cs2)>);
        auto cs5 = empty + cs3;
        static_assert(std::is_same_v<decltype(cs5), decltype(cs3)>);
    }
    {
        completion_signatures<> empty;
        auto cs3 = cs1 + empty + cs1;
        static_assert(std::is_same_v<decltype(cs3), decltype(cs1)>);
        auto cs4 = empty + cs2 + cs2;
        static_assert(std::is_same_v<decltype(cs4), decltype(cs2)>);
        auto cs5 = empty + cs3 + empty;
        static_assert(std::is_same_v<decltype(cs5), decltype(cs3)>);
    }
}

int main()
{
    testtypeset_deduplication_order();
    return 0;
}
// NOLINTEND
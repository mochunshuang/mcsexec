#include "../test_base_head.hpp"
#include <string>

int main()
{
    TEST("let_error CS") = [] {
        ex::sender auto snd [[maybe_unused]] =
            ex::just() | ex::let_error([](std::exception_ptr &&) { return ex::just(); });
        using T = decltype(snd);
        using CS = ex::cmplsigs::get_completion_signatures<T>;
        static_assert(std::is_same_v<CS, mcs::execution::cmplsigs::completion_signatures<
                                             mcs::execution::recv::set_value_t()>>);
        {
            ex::sender auto snd [[maybe_unused]] =
                ex::just() | ex::let_error([](std::exception_ptr &&) {
                    return ex::just(1, 1.0, 1.0F);
                });
            using T = decltype(snd);
            using CS = ex::cmplsigs::get_completion_signatures<T>;
            static_assert(
                std::is_same_v<
                    CS, mcs::execution::cmplsigs::completion_signatures<
                            mcs::execution::recv::set_value_t(int, double, float)>>);
        }
        {
            ex::sender auto snd [[maybe_unused]] =
                ex::just() | ex::let_error([](std::exception_ptr &&) {
                    return ex::just_error(404); // NOLINT
                });
            using T = decltype(snd);
            using CS = ex::cmplsigs::get_completion_signatures<T>;
            static_assert(
                std::is_same_v<CS, mcs::execution::cmplsigs::completion_signatures<
                                       mcs::execution::recv::set_error_t(int)>>);
        }
    };
    TEST("let_error : from  logic_error to error_str ") = [] {
        ex::sender auto snd = ex::just() //
                              | ex::then([]() -> std::string {
                                    throw std::logic_error{"error description"};
                                }) //
                              | ex::let_error([](std::exception_ptr &&eptr) {
                                    try
                                    {
                                        std::rethrow_exception(std::move(eptr));
                                    }
                                    catch (const std::exception &e)
                                    {
                                        return ex::just(std::string{e.what()});
                                    }
                                });
        using T = decltype(snd);
        using CS = ex::cmplsigs::get_completion_signatures<T>;
        static_assert(
            std::is_same_v<CS, mcs::execution::cmplsigs::completion_signatures<
                                   mcs::execution::recv::set_value_t(std::string)>>);
    };

    {
        ex::sender auto snd =
            ex::just()                                   //
            | ex::then([] { return std::string("13"); }) //
            | ex::let_error([&](std::exception_ptr) { return ex::just(0); });
        using T = ex::cmplsigs::get_completion_signatures<decltype(snd)>;
        // static_assert(std::is_same_v<T, int>);
    }
    return 0;
}
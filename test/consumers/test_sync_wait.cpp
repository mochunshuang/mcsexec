#include "../test_base_head.hpp"
#include <algorithm>
#include <optional>
#include <stdexcept>
#include <tuple>

int main()
{
    // sync_wait mandates that the input sender has exactly one value completion
    // signature.
    /**
     * @brief
     * Note:  returning type of sync_wait
     * 1、For a value completion: std::optional<std::tuple<T...>>
     * 2、For a value completion: an exception is thrown.
     * 3、For a value completion: a disengaged std::optional
     *
     * Note: returning type of sync_wait_with_variant
     * 1、For a value completion: std::optional<std::variant<std::tuple<T...>>>
     * 2、For a value completion: an exception is thrown.
     * 3、For a value completion: a disengaged std::optional
     */
    using namespace std;              // NOLINT
    using namespace mcs::this_thread; // NOLINT
    TEST("sync_wait simple test") = [] {
        optional<tuple<int>> res = sync_wait(ex::just(1));
        EXPECT(std::get<0>(res.value()) == 1);
    };
    TEST("sync_wait can wait on void values") = [] {
        optional<tuple<>> res = sync_wait(ex::just());
        EXPECT(res.has_value());
    };
    TEST("sync_wait can wait on senders sending value packs") = [] {
        optional<tuple<int, double>> res = sync_wait(ex::just(3, 0.1415));
        EXPECT(res.has_value());
        EXPECT(std::get<0>(res.value()) == 3);
        EXPECT(std::get<1>(res.value()) == 0.1415); // NOLINT
    };

    TEST("sync_wait rethrows received exception") = [] {
        auto snd = ex::just(2) | ex::then([](int a) {
                       if (a == 2)
                           throw std::logic_error{"err"};
                       return 1;
                   });
        try
        {

            sync_wait(std::move(snd));
            UNEXPECT("It's impossible to reach");
        }
        catch (const std::logic_error &e)
        {
            EXPECT(std::string{e.what()} == "err");
        }
        catch (...)
        {
            UNEXPECT("invalid exception received");
        }
    };

    TEST("sync_wait handling error_code errors") = [] {
        try
        {
            // Note:
            //  Now set  ex::let_error([](std::logic_error &&),
            //  ags not std::exception_ptr and E_CS of pre_sndr can`t call fun,
            //  set compile error
            auto snd =
                ex::just() | ex::then([] { throw std::logic_error("111"); }) |
                //    ex::let_error([](std::logic_error &&) {
                //        return ex::just_error(
                //            std::make_error_code(std::errc::argument_out_of_domain));
                //    });
                ex::let_error([](std::exception_ptr &&) {
                    return ex::just_error(
                        std::make_error_code(std::errc::argument_out_of_domain));
                });
            using T = ex::cmplsigs::get_completion_signatures<decltype(snd)>;
            using Forward = ex::cmplsigs::completion_signatures<
                ex::recv::set_value_t(), ex::recv::set_error_t(std::exception_ptr)>;

            // sync_wait(std::move(snd));
            // UNEXPECT("It's impossible to reach");
        }
        catch (const std::system_error &e)
        {
            EXPECT(e.code() == std::errc::argument_out_of_domain);
        }
        catch (...)
        {
            UNEXPECT("invalid exception received");
        }
    };

    TEST("sync_wait_with_variant accepts single-value senders") = [] {
        ex::sender auto snd = ex::just(13);
        static_assert(std::invocable<decltype(mcs::this_thread::sync_wait_with_variant),
                                     decltype(snd)>);

        variant<std::tuple<std::variant<std::tuple<int>>>> res =
            mcs::this_thread::sync_wait_with_variant(std::move(snd)).value();
        auto [ret] = std::get<0>(std::get<0>(std::get<0>(res)));
        EXPECT(ret == 13);
    };

    TEST("sync_wait_with_variant accepts mult-value senders") = [] {
        ex::sender auto snd = ex::just(13, 1.0);
        static_assert(std::invocable<decltype(mcs::this_thread::sync_wait_with_variant),
                                     decltype(snd)>);
        std::optional<variant<std::tuple<std::variant<std::tuple<int, double>>>>> res =
            mcs::this_thread::sync_wait_with_variant(std::move(snd));
        auto [ret, f] = std::get<0>(std::get<0>(std::get<0>(res.value())));
        EXPECT(ret == 13);
        EXPECT(f == 1.0);
    };

    return 0;
}
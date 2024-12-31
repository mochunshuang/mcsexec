#include "../test_base_head.hpp"
#include <algorithm>
#include <stdexcept>

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
            auto snd = ex::just() | ex::then([] { throw std::logic_error("111"); }) |
                       ex::let_error([](std::logic_error &&) {
                           return ex::just_error(
                               std::make_error_code(std::errc::argument_out_of_domain));
                       });
            using T = ex::cmplsigs::get_completion_signatures<decltype(snd)>;
            static_assert(
                std::is_same_v<T, ex::cmplsigs::undefine_completion_signatures_for>);

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

    return 0;
}
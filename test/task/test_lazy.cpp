#include "../test_base_head.hpp"

int main()
{
    static_assert(ex::snd::sender<mcs::execution::task::lazy<int>>);

    TEST(" lazy<int> is sender") = [] {
        static_assert(ex::snd::sender<mcs::execution::task::lazy<int>>);
    };

    TEST("lazy<int> ") = [] {
        auto rc = mcs::this_thread::sync_wait([] -> ex::lazy<int> { // NOLINT
            co_return 17;                                           // NOLINT
        }());
        assert(rc);
        auto [value] = rc.value_or(std::tuple{0});
        EXPECT(value == 17);
    };

    return 0;
}
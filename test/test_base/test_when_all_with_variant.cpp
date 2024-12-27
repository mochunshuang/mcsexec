
#include "../../include/execution.hpp"

#include <iostream>
#include <utility>

void test_base();
int main()
{
    test_base();
    std::cout << "hello world\n";
    return 0;
}
void test_base()
{
    /**
     * @brief This causes the when_all_with_variant(sndrs...) sender to become
     * when_all(into_variant(sndrs)...) when it is connected with a receiver whose
     * execution domain does not customize when_all_with_variant
     *
     */
    namespace ex = mcs::execution;
    auto snd =
        ex::when_all(ex::into_variant(ex::just(1)), ex::into_variant(ex::just(1.0)));
    using T = decltype(snd.get_completion_signatures(ex::empty_env{}));

    auto snd2 = ex::when_all_with_variant(ex::just(1), ex::just(1.0));
    using T2 = decltype(snd2.get_completion_signatures(ex::empty_env{}));

    static_assert(std::is_same_v<T, T2>);

    auto v = mcs::this_thread::sync_wait(std::move(snd));
    {
        auto v = mcs::this_thread::sync_wait(std::move(snd2));
    }
}

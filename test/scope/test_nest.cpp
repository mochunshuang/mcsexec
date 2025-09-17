#include "../test_base_head.hpp"

#include <algorithm>
#include <iostream>

struct recever_all_but_empty_env
{
    using receiver_concept = ex::receiver_t;

    template <typename... A> // NOLINTNEXTLINE
    auto set_value(A &&...a) && noexcept -> void
    {
        std::cout << "recever_all_but_empty_env: set_value:" << '\n';
        ((std::cout << a << " , "), ...);
        std::cout << '\n';
    }

    template <typename E> // NOLINTNEXTLINE
    auto set_error(E &&e) && noexcept -> void
    {
    }

    void set_stopped() && noexcept // NOLINT
    {
    }

    constexpr auto get_env() const noexcept // NOLINT
    {

        return ex::empty_env{};
    }
};

int main()
{
    TEST("base nest") = [] {
        auto snd = ex::just(1, 2);
        ex::simple_counting_scope scope;
        auto ret = ex::nest(std::move(snd), scope.get_token());
        {
            // NOTE: 必须 析构op. 带有 disassociate()
            auto op = std::move(ret).connect(recever_all_but_empty_env{});
            ex::opstate::start(op);
        }
        mcs::this_thread::sync_wait(scope.join());
    };
    std::cout << "main done\n";
    return 0;
}
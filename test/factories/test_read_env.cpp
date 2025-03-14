#include "../test_base_head.hpp"
#include <iostream>
#include <utility>
// NOLINTBEGIN
struct domain
{
    int value{};
    auto operator==(const domain &) const -> bool = default;
};

struct env
{
    int value{};
    constexpr auto query(ex::get_domain_t) const noexcept -> domain
    {
        return {this->value};
    }
};

struct env_throw
{
    int value{};
    constexpr auto query(ex::get_domain_t) const noexcept(false) -> domain
    {
        return {this->value};
    }
};

struct receiver
{
    using receiver_concept = ex::receiver_t;

    int value{};
    bool *called{};

    auto set_value(domain d) && noexcept -> void
    {
        EXPECT(d == domain{this->value});
        *this->called = true;
    }
    auto set_error(auto &&) && noexcept -> void
    {
        UNEXPECT("error function was incorrectly called");
    }
    auto get_env() const noexcept -> env
    {
        return {this->value};
    }
};

int main()
{
    TEST("Q and Env") = [] {
        constexpr env e{};
        static_assert(e.query(ex::get_domain).value == 0);
        static_assert(e.query(ex::get_domain).value == 0);

        // NOTE: 等价
        static_assert(ex::get_domain(env{}).value == 0);
        static_assert(ex::get_domain_t()(env{}).value == 0);
        using T = decltype(ex::get_domain_t()(std::declval<env>()));
        static_assert(std::is_same_v<T, domain>);

        EXPECT(domain{17} == ex::get_domain(env{17}));
        EXPECT(domain{17} == ex::get_domain(ex::get_env(receiver{17})));

        // NOTE: may throw is compile time error
        // static_assert(ex::get_domain_t()(env_throw{}).value == 0);
    };
    TEST("test_read_env") = [] {
        static_assert(ex::receiver<receiver>);

        auto sender = ex::read_env(ex::get_domain);
        static_assert(ex::sender<decltype(sender)>);

        using Sndr = decltype(sender);
        static_assert(ex::sender_in<Sndr, env>);

        TEST("read_env is a dependent sender") = [] {
            static_assert(ex::snd::dependent_sender<Sndr>);
        };

        TEST("query() need no_throw") = [] {
            using CS = decltype(ex::get_completion_signatures<Sndr, env>());
            static_assert(
                std::same_as<ex::completion_signatures<ex::set_value_t(domain)>, CS>);
        };

        TEST("query throw compile time error") = [] {
            // static_assert(ex::get_domain_t()(env_throw{}).value == 0);
            // using CS = decltype(ex::get_completion_signatures<Sndr, env_throw>());
            // static_assert(
            //     std::same_as<ex::completion_signatures<ex::set_value_t(domain)>, CS>);
        };

        bool called{};
        auto op{ex::connect(ex::read_env(ex::get_domain), receiver{17, &called})};
        EXPECT(not called);
        ex::start(op);
        EXPECT(called);
    };

    std::cout << "main done\n";
    return 0;
}
// NOLINTEND
#include <cassert>
#include <iostream>
#include <tuple>
#include <utility>
#include <variant>

void base() // NOLINT
{
    using T = std::variant<std::monostate, int, int>;
    using T0 = std::variant<std::monostate, int>;
    static_assert(not std::is_same_v<T, T0>);
}
struct Scope
{
    struct Token
    {
        auto *getScope() noexcept // NOLINT
        {
            return scope;
        }

      private:
        explicit Token(Scope *s) noexcept : scope(s) {}
        Scope *scope; // NOLINT
        friend Scope;
    };
    auto get_token() noexcept // NOLINT
    {
        return Token{this};
    }

    int count{}; // NOLINT
};

template <typename Fn, typename... Args>
auto make_state(Fn fun) noexcept // NOLINT
{
    using scope_type = Scope;
    using scope_token_type = decltype(std::declval<scope_type>().get_token());

    using args_variant_type =
        std::variant<std::monostate, std::tuple<scope_token_type, std::decay_t<Args>...>>;
    struct state
    {
        Fn fn;
        scope_type scope;
        args_variant_type args;
    };
    return state{std::move(fun), {}, {}};
}

template <class... Ts>
using decayed_tuple = std::tuple<std::decay_t<Ts>...>;
template <typename... Args>
auto apply_state(auto &state, Args &&...args) noexcept // NOLINT
{
    using scope_token_type = decltype(state.scope.get_token());
    auto create_scope_token = [&]() noexcept { // NOLINT
        return state.scope.get_token();
    };

    auto &args_variant =
        state.args.template emplace<decayed_tuple<scope_token_type, Args...>>(
            create_scope_token(), std::forward<Args>(args)...);

    // NOTE: args_variant 是引用类型,因此 fun的 必须是引用类型
    return std::apply(std::move(state.fn), args_variant);
}

int main()
{
    {
        using Ags0 = int;
        using Ags1 = std::string;
        using token_type = decltype(std::declval<Scope>().get_token());
        auto test0 = Ags0{1};
        auto test1 = Ags1{"msg"};
        auto fun = [&](token_type &token, Ags0 &a, std::string &b) {
            assert(a == test0);
            assert(b == test1);
            return token;
        };
        using Fun = decltype(fun);
        auto state = make_state<Fun, Ags0, Ags1>(std::move(fun));
        auto t = apply_state(state, auto(test0), auto(test1));
        assert(&(state.scope) == t.getScope());

        using T = decltype(state.args);
        static_assert(
            std::is_same_v<
                std::variant<std::monostate,
                             std::tuple<Scope::Token, int, std::basic_string<char>>>,
                T>);
    }

    std::cout << "main done\n";
    return 0;
}
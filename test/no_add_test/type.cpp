#include <iostream>

struct set_value_t
{
    template <typename R, typename... Ts>
    constexpr auto operator()(R &&rcvr, Ts &&...vs) const noexcept
        requires(std::is_rvalue_reference_v<decltype(rcvr)> &&
                 not std::is_const_v<std::remove_reference_t<decltype(rcvr)>> //
                 && requires {
                        {
                            std::forward<R>(rcvr).set_value(std::forward<Ts>(vs)...)
                        } noexcept;
                    })
    {
        return std::forward<R>(rcvr).set_value(std::forward<Ts>(vs)...);
    }
};
constexpr inline set_value_t set_value{}; // NOLINT

struct sender_t
{
};
struct receiver_t
{
};
struct operation_state_t
{
};

template <class... _Sigs>
struct completion_signatures
{
};

// receiver_of 约束 sig 能否被 recv 转发

struct receiver
{
    using receiver_concept = receiver_t;

    void set_value(auto &&v) noexcept // NOLINT
    {
        std::cout << "set_value: " << v << '\n';
    }
};

struct sender
{
    template <typename Recv>
    struct operation
    {
        Recv recv; // NOLINT
        void start() & noexcept
        {
            set_value(std::move(recv), 1);
        }
    };

    template <typename Recever>
    auto connect(Recever recv) noexcept
    {
        // NOTE: 类型转化在这里，这是最后的机会。
        return operation{std::move(recv)};
    }
};

int main()
{
    std::cout << "main done\n";
    return 0;
}
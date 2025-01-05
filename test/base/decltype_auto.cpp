
#include <type_traits>
#include <utility>

template <typename T>
struct product_type
{
    std::decay_t<T> value;
};
template <typename T>
product_type(T &&) -> product_type<std::decay_t<T>>; // NOLINT

struct move_only_type
{
    move_only_type() : val(0) {}
    explicit move_only_type(int v) : val(v) {}
    ~move_only_type() = default;

    move_only_type(const move_only_type &) = delete;
    move_only_type &operator=(const move_only_type &) = delete;

    move_only_type &operator=(move_only_type &&) = default;
    move_only_type(move_only_type &&) = default;
    int val; // NOLINT
};
void base(); // NOLINT
int main()
{
    base();
    return 0;
}
void base()
{
    auto make_sender [[maybe_unused]] = []<typename T>(T &&t) -> decltype(auto) {
        return product_type<std::decay_t<T>>{std::forward<T>(t)};
    };
    {
        // auto just = make_sender(move_only_type{1});
        // auto then = make_sender(just); // 编译失败
    }
    {
        // auto &&just = make_sender(move_only_type{1});
        // auto then = make_sender(just); // 编译失败
    }
    {
        auto &&just = make_sender(move_only_type{1});
        auto then [[maybe_unused]] = make_sender(std::move(just)); // 编译成功
    }
    {
        auto then [[maybe_unused]] = make_sender(make_sender(move_only_type{1})); // 成功
    }
    {
        // Note: 无论你  auto && 和 auto  接收返回值，都不可能一次计算 then
        //  auto &&just = make_sender(move_only_type{1});
        //  auto &&then = make_sender(just);
    }
    {
        // Note: 无论你  auto && 和 auto  接收返回值，都不可能一次计算 then
        auto &&just = make_sender(move_only_type{1});
        // auto then = just; // 编译错误
        auto &then [[maybe_unused]] = just;
        // auto &&then2 = make_sender(std::forward<decltype(then)>(then)); //编译错误
        // auto &&then2 = make_sender(std::forward<decltype((then))>(then)); // 编译错误
    }

    {
        // Note: 无论你  auto && 和 auto  接收返回值，都不可能一次计算 then
        auto &&just = make_sender(move_only_type{1});
        // auto then = just; // 编译错误
        auto &&then [[maybe_unused]] = just;
        // auto &&then2 = make_sender(std::forward<decltype(then)>(then)); // 编译错误
        // auto &&then2 = make_sender(std::forward<decltype((then))>(then)); // 编译错误
    }
    // Note: std::decay_t + 左值 + move_only_type 不可能
    // Note: std::decay_t + 右值 + move_only_type 才是标配
}
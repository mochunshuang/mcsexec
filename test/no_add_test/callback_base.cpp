#include <algorithm>
#include <cassert>
#include <forward_list>
#include <optional>
#include <tuple>
#include <type_traits>
#include <vector>
#include <iostream>
#include <memory>

// NOLINTBEGIN
struct callback_base
{
    using callback_fun_t = void (*)(callback_base *self) noexcept;
    callback_fun_t execute;
};

template <typename Callable>
struct callback_derived : public callback_base
{
    Callable func;

    explicit callback_derived(Callable f) : func(std::move(f))
    {
        execute = &callback_derived::invoke;
    }

    static void invoke(callback_base *self) noexcept
    {
        auto *derived = static_cast<callback_derived *>(self);
        derived->func();
    }
};

// 辅助函数用于创建并管理回调对象
template <typename Callable>
callback_base *make_callback(Callable &&f)
{
    return new callback_derived<std::decay_t<Callable>>(std::forward<Callable>(f));
}

int main()
{
    std::forward_list<callback_base *> callbacks;
    std::vector<std::unique_ptr<callback_base>> owners; // 管理生命周期

    // 添加lambda回调
    auto *cb1 = make_callback([] { std::cout << "Lambda called\n"; });
    owners.emplace_back(cb1);
    callbacks.push_front(cb1);

    // 添加函数指针回调
    void (*func_ptr)() = [] {
        std::cout << "Function pointer called\n";
    };
    auto *cb2 = make_callback(func_ptr);
    owners.emplace_back(cb2);
    callbacks.push_front(cb2);

    // 执行所有回调
    for (auto *cb : callbacks)
    {
        cb->execute(cb);
    }

    // 无需手动delete，owners的unique_ptr会自动释放内存

    // NOTE: 指针存取
    callback_base *p = nullptr;
    std::tuple t = {p};
    static_assert(std::is_same_v<std::tuple<callback_base *>, decltype(t)>);
    static_assert(std::is_same_v<callback_base *, std::decay_t<decltype(p)>>);
    auto [p1] = t;
    assert(p1 == p);

    // using 不依赖  typename
    using A = int;
    using B = A; // NOLINT

    // NOTE:引用
    bool called = false;
    [&c = called]() {
        c = true;
    }();
    assert(called);

    // optional
    std::optional<int> op{1};
    using T = decltype(op.value());
    static_assert(std::is_same_v<T, int &>);
    static_assert(std::is_same_v<std::decay_t<T>, int>);

    {
        int a = 1;
        auto fun = []<class T>(T &&) {
            static_assert(std::is_same_v<T, int>);
        };
        fun(1);      // OK
        fun(int{1}); // OK

        fun(std::move(a)); // OK, 移动/和无引用类型差不多

        // fun(a); // NOTE: 编译错误
    }

    return 0;
}
// NOLINTEND
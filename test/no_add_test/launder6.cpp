#include <cassert>
#include <cstddef>
#include <iostream>
#include <new>
#include <type_traits>
#include <utility>
#include <variant>

// NOLINTBEGIN

template <typename T>
concept scheduler_concept = requires(T t) {
    { t.schedule() } noexcept;
};

struct run_loop
{
};

struct sender
{
    run_loop *loop;
};

struct scheduler
{
    static constexpr auto tap = "   ";
    run_loop *loop;
    constexpr auto schedule() noexcept
    {
        std::cout << tap << ">>> schedule()...\n";
        return sender{loop};
    }

    constexpr scheduler() noexcept : loop(nullptr)
    {
        std::cout << tap << ">>> scheduler()\n";
    }

    constexpr explicit scheduler(run_loop *l) noexcept : loop(l)
    {
        std::cout << tap << ">>> scheduler(run_loop *l)\n";
    }

    constexpr scheduler(const scheduler &other) noexcept : loop(other.loop)
    {
        std::cout << tap << ">>> scheduler(const scheduler &other)\n";
    }
    constexpr scheduler(scheduler &&other) noexcept
        : loop(std::exchange(other.loop, nullptr))
    {
        std::cout << tap << ">>> scheduler(scheduler &&other) noexcept\n";
    }
    constexpr scheduler &operator=(const scheduler &other) noexcept
    {
        std::cout << tap << ">>> scheduler &operator=(const scheduler &other)\n";
        if (this != &other)
        {
            loop = other.loop;
        }
        return *this;
    }
    constexpr scheduler &operator=(scheduler &&other) noexcept
    {
        std::cout << tap << ">>> scheduler &operator=(scheduler &&other)\n";
        if (this != &other)
        {
            loop = std::exchange(other.loop, nullptr);
        }
        return *this;
    }
    constexpr ~scheduler() noexcept
    {
        std::cout << tap << ">>> ~scheduler()\n";
        loop = nullptr;
    }
};

template <scheduler_concept S>
struct class_fun_table
{
    using value_type = std::decay_t<S>;

    static constexpr decltype(auto) launder_pointer(void *data) noexcept
    {
        return std::launder(static_cast<value_type *>(data));
    }
    static constexpr auto schedule(void *data) noexcept
    {
        return launder_pointer(data)->schedule();
    }
    static constexpr auto destroy(void *data) noexcept
    {
        std::destroy_at(launder_pointer(data));
    }
    static constexpr auto move_construct(void *dest, void *src) noexcept
    {
        new (dest) value_type(std::move(*launder_pointer(src)));
    }
    static constexpr auto move_assign(void *dest, void *src) noexcept
    {
        *launder_pointer(dest) = std::move(*launder_pointer(src));
    }

    static constexpr auto copy_construct(void *dest, void *src) noexcept(
        noexcept(new(dest) value_type(*launder_pointer(src))))
    {
        new (dest) value_type(*launder_pointer(src));
    }
    static constexpr auto copy_assign(void *dest, void *src) noexcept(
        noexcept(*launder_pointer(dest) = *launder_pointer(src)))
    {
        *launder_pointer(dest) = *launder_pointer(src);
    }

    static constexpr auto do_new()
    {
        // void *storage = ::operator new(sizeof(value_type), std::nothrow);
        // if (storage == nullptr)
        //     throw std::bad_alloc{};
        // return storage;
        return ::operator new(sizeof(value_type));
    }
    static constexpr void do_delete(void *storage) noexcept
    {
        ::operator delete(storage);
    }
};

int main()
{
    static constexpr auto k_max_size = sizeof(void *) * 2;
    // alignas(std::max_align_t) std::byte storage[k_max_size];
    // alignas(std::max_align_t) std::byte copy_storage[k_max_size];
    using F = class_fun_table<scheduler>;

    // 仅分配原始内存（不构造对象），无异常版本
#define Base 0
#if Base
    void *storage = ::operator new(k_max_size, std::nothrow);
    void *copy_storage = ::operator new(k_max_size, std::nothrow);
    assert(storage != nullptr);
    assert(copy_storage != nullptr);
#else
    void *storage = F::do_new();
    void *copy_storage = F::do_new();
#endif

    {
        run_loop loop;

        using type = scheduler;

        std::cout << "::new (storage) type(&loop)" << '\n';
        // ::new (&storage) type(&loop);
        ::new (storage) type(&loop); // NOTE: 使用 storage 而不是 &storage

        assert(F::launder_pointer(storage)->loop == &loop);

        std::cout << "F::schedule(&storage)" << '\n';
        auto s = F::schedule(&storage);
        static_assert(std::is_same_v<decltype(s), sender>);

        std::cout << "F::copy_construct(copy_storage, storage)" << '\n';
        F::copy_construct(copy_storage, storage);
        assert(F::launder_pointer(copy_storage)->loop == &loop);
        assert(F::launder_pointer(storage)->loop ==
               F::launder_pointer(copy_storage)->loop);
        static_assert(noexcept(F::copy_construct(copy_storage, storage)));

        std::cout << "F::copy_assign(copy_storage, storage)" << '\n';
        F::launder_pointer(copy_storage)->loop = nullptr;
        assert(F::launder_pointer(copy_storage)->loop == nullptr);
        F::copy_assign(copy_storage, storage);
        assert(F::launder_pointer(copy_storage)->loop == &loop);
        assert(F::launder_pointer(storage)->loop ==
               F::launder_pointer(copy_storage)->loop);
        static_assert(noexcept(F::copy_assign(copy_storage, storage)));

        std::cout << "F::launder_pointer(copy_storage)->loop = nullptr" << '\n';
        F::launder_pointer(copy_storage)->loop = nullptr;
        assert(F::launder_pointer(copy_storage)->loop == nullptr);

        std::cout << "F::move_construct(copy_storage, storage)" << '\n';
        assert(F::launder_pointer(storage)->loop == &loop);
        F::move_construct(copy_storage, storage);
        assert(F::launder_pointer(copy_storage)->loop == &loop);
        assert(F::launder_pointer(storage)->loop == nullptr);

        std::cout << "F::move_assign(copy_storage, storage)" << '\n';
        F::move_assign(storage, copy_storage);
        assert(F::launder_pointer(storage)->loop == &loop);
        assert(F::launder_pointer(copy_storage)->loop == nullptr);

        std::cout << "F::destroy all" << '\n';
        F::destroy(copy_storage);
        F::destroy(storage);
        assert(F::launder_pointer(copy_storage)->loop == nullptr);
        assert(F::launder_pointer(storage)->loop == nullptr);
    }

#if Base
    // 记得释放内存
    ::operator delete(storage);
    ::operator delete(copy_storage);

    ::operator delete(nullptr); // NOTE: 不会崩溃
    ::operator delete(nullptr);

#else
    F::do_delete(storage);
    F::do_delete(copy_storage);
#endif

    using T = std::variant<int>;
    static_assert(sizeof(T) != sizeof(int));
    static_assert(sizeof(T) == 8);
    static_assert(sizeof(int) == 4);

    using T2 = std::variant<double, int>;
    static_assert(sizeof(double) == 8);
    static_assert(sizeof(T2) == 16);

    using T3 = std::variant<std::monostate, double, int>;
    static_assert(sizeof(T3) == 16);

    using T4 = std::variant<std::monostate, double, float, int>;
    static_assert(sizeof(T4) == 16);

    using T5 = std::variant<std::monostate, double, float, int, short>;
    static_assert(sizeof(T5) == 16);
    {
        struct static_type
        {
            // NOTE: 必须包装成类
            alignas(std::max_align_t) std::byte storage[k_max_size];
        };
        struct runtime_type
        {
            // NOTE: 必须包装成类
            alignas(std::max_align_t) void *storage;
        };

        using T0 = static_type;
        using T1 = runtime_type;
        using Type = std::variant<std::monostate, T1, T0>;
        // static_assert(sizeof(T) == 32);
        // static_assert(sizeof(Type) == 24); // NOTE: 很好了

        std::cout << ">>>>>: sizeof(Type): " << sizeof(Type) << '\n';

        {
            // NOTE: 是错误的
            alignas(std::max_align_t) union {
                bool is_static_value;
                T1 runtime_value;
                T0 static_value;
            } Varint;
            static_assert(sizeof(Varint) == 16);
        }
        {
            struct alignas(8) Varint
            {
                bool is_static;
                union {
                    T1 runtime_value;
                    T0 static_value;
                } data;
            };
            std::cout << ">>>>>: sizeof(Type): " << sizeof(Varint) << '\n';
        }

        // NOTE: 自己写，也不会做得更好。 编译器实现不一样。大小不一定一样

        {
            enum class StorageType
            {
                Monostate,
                Static,
                Dynamic
            };

            struct CustomVariant
            {
                StorageType type;
                union {
                    T0 static_val;
                    T1 dynamic_val;
                };

                // 构造函数等
            }; // NOTE: 不用纠结了。编译器编译期 早就算出了。改不了
            std::cout << ">>>>>: sizeof(Type): " << sizeof(CustomVariant) << '\n';
        }
    }

    std::cout << "All tests passed successfully" << '\n';
    return 0;
}
// NOLINTEND
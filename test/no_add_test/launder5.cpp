#include <cassert>
#include <cstddef>
#include <iostream>
#include <type_traits>
#include <utility>

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
};

int main()
{
    static constexpr auto k_max_size = sizeof(void *) * 2;
    alignas(std::max_align_t) std::byte storage[k_max_size];

    alignas(std::max_align_t) std::byte copy_storage[k_max_size];

    {
        run_loop loop;

        using type = scheduler;
        using F = class_fun_table<scheduler>;
        std::cout << "::new (&storage) type(&loop)" << '\n';
        ::new (&storage) type(&loop);

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

        std::cout << "F::destroy(copy_storage)" << '\n';
        F::destroy(copy_storage);
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

    // ::operator delete(&storage); // NOTE: 崩溃
    // int a = 0;
    // ::operator delete(&a); //NOTE: 崩溃

    std::cout << "All tests passed successfully" << '\n';
    return 0;
}
// NOLINTEND
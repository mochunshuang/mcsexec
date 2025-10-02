#include <array>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <new>
#include <string>
#include <type_traits>

// NOLINTBEGIN
template <class T, std::size_t N>
class static_vector
{
    // Properly aligned uninitialized storage for N T's
    std::aligned_storage_t<sizeof(T), alignof(T)> data[N]; // NOTE: 无法替代
    // alignas(T) unsigned char data[N * sizeof(T)];
    // std::array<unsigned char, N * sizeof(T)> data{};
    std::size_t m_size = 0;

  public:
    // Create an object in aligned storage
    template <typename... Args>
    void emplace_back(Args &&...args)
    {
        if (m_size >= N) // Possible error handling
            throw std::bad_alloc{};

        // Construct value in memory of aligned storage using inplace operator new
        ::new (&data[m_size]) T(std::forward<Args>(args)...);
        ++m_size;
    }

    // Access an object in aligned storage
    const T &operator[](std::size_t pos) const
    {
        // Note: std::launder is needed after the change of object model in P0137R1
        return *std::launder(
            static_cast<const T *>(static_cast<const void *>(&data[pos])));
    }

    // Destroy objects from aligned storage
    ~static_vector()
    {
        for (std::size_t pos = 0; pos < m_size; ++pos)
            // Note: std::launder is needed after the change of object model in P0137R1
            std::destroy_at(
                std::launder(static_cast<T *>(static_cast<void *>(&data[pos]))));
    }
};

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
    run_loop *loop;
    auto schedule() noexcept
    {
        return sender{loop};
    }
    ~scheduler() noexcept
    {
        std::cout << " ~scheduler\n";
    }
};
struct scheduler_storage
{
    static constexpr auto max_size = sizeof(void *) * 2;
    using aligned_storage = std::aligned_storage_t<max_size, alignof(std::max_align_t)>;

    // Function pointer types
    using ScheduleFunc = sender (*)(void *);
    using DestroyFunc = void (*)(void *);

    aligned_storage data;
    ScheduleFunc schedule_func = nullptr;
    DestroyFunc destroy_func = nullptr;

    template <scheduler_concept S>
        requires(sizeof(S) <= max_size && alignof(S) <= alignof(std::max_align_t))
    scheduler_storage(S &&sched)
    {
        using ValueType = std::decay_t<S>;
        new (&data) ValueType(std::forward<S>(sched));
        schedule_func = [](void *data) -> sender {
            std::cout << "schedule_func called" << std::endl;
            return std::launder(static_cast<ValueType *>(data))->schedule();
        };
        destroy_func = [](void *data) {
            std::cout << "destroy_func called" << std::endl;
            // static_cast<ValueType *>(data)->~ValueType();
            std::destroy_at(std::launder(static_cast<ValueType *>(data)));
        };
    }

    auto schedule() noexcept
    {
        if (schedule_func == nullptr)
        {
            std::terminate();
        }
        return schedule_func(&data);
    }

    ~scheduler_storage()
    {
        if (destroy_func != nullptr)
        {
            destroy_func(&data);
        }
    }

    // Delete copy and move operations for simplicity
    scheduler_storage(const scheduler_storage &) = delete;
    scheduler_storage &operator=(const scheduler_storage &) = delete;
};

int main()
{
    {
        run_loop loop;
        scheduler sched{&loop};
        scheduler_storage storage(sched);
        auto s = storage.schedule();
        assert(s.loop == &loop);
        std::cout << "Schedule called successfully" << std::endl;
    }
    std::cout << "main done\n";
    return 0;
}

// NOLINTEND
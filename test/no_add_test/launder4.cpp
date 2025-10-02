#include <cassert>
#include <cstddef>
#include <iostream>
#include <type_traits>

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
    run_loop *loop;
    auto schedule() noexcept
    {
        return sender{loop};
    }

    // 1. 默认构造函数
    scheduler() : loop(nullptr)
    {
        std::cout << "scheduler[" << "]: 默认构造函数 (loop = nullptr)\n";
    }

    // 2. 带参构造函数
    explicit scheduler(run_loop *l) : loop(l)
    {
        std::cout << "scheduler[" << "]: 带参构造函数 (loop = " << l << ")\n";
    }

    // 3. 复制构造函数
    scheduler(const scheduler &other) : loop(other.loop)
    {
        std::cout << "scheduler[" << "]: 复制构造函数 (从 " << &other
                  << " 复制，loop = " << loop << ")\n";
    }

    // 4. 移动构造函数
    scheduler(scheduler &&other) noexcept : loop(other.loop)
    {
        other.loop = nullptr; // 转移后将原对象指针置空
        std::cout << "scheduler[" << "]: 移动构造函数 (从 " << &other
                  << " 移动，原loop = " << loop << ")\n";
    }

    // 复制赋值运算符
    scheduler &operator=(const scheduler &other)
    {
        // 自赋值检查
        if (this != &other)
        {
            // 复制资源
            loop = other.loop;
            std::cout << "scheduler[" << "]: 复制赋值运算符 (从 " << &other
                      << " 复制，新loop = " << loop << ")\n";
        }
        return *this;
    }

    // 移动赋值运算符
    scheduler &operator=(scheduler &&other) noexcept
    {
        // 自赋值检查
        if (this != &other)
        {
            // 释放当前拥有的资源（如果是转换构造创建的）
            // 转移资源
            loop = other.loop;
            other.loop = nullptr;
            std::cout << "scheduler[" << "]: 移动赋值运算符 (从 " << &other
                      << " 移动，新loop = " << loop << ")\n";
        }
        return *this;
    }

    // 析构函数
    ~scheduler()
    {
        // 对于转换构造函数创建的loop进行释放
        loop = nullptr;
    }
};

struct scheduler_storage
{
    static constexpr auto max_size = sizeof(void *) * 2;
    using aligned_storage = std::aligned_storage_t<max_size, alignof(std::max_align_t)>;

    // Function pointer types
    using ScheduleFunc = sender (*)(void *);
    using DestroyFunc = void (*)(void *);
    using MoveConstructFunc = void (*)(void *, void *);
    using MoveAssignFunc = void (*)(void *, void *);

    aligned_storage data;
    ScheduleFunc schedule_func = nullptr;
    DestroyFunc destroy_func = nullptr;
    MoveConstructFunc move_construct_func = nullptr;
    MoveAssignFunc move_assign_func = nullptr;

    template <scheduler_concept S>
        requires(sizeof(S) <= max_size && alignof(S) <= alignof(std::max_align_t))
    scheduler_storage(S &&sched)
    {
        using ValueType = std::decay_t<S>;
        new (&data) ValueType(std::forward<S>(sched));

        schedule_func = [](void *data) -> sender {
            return static_cast<ValueType *>(data)->schedule();
        };

        destroy_func = [](void *data) {
            static_cast<ValueType *>(data)->~ValueType();
        };

        move_construct_func = [](void *dest, void *src) {
            new (dest) ValueType(std::move(*static_cast<ValueType *>(src)));
        };

        move_assign_func = [](void *dest, void *src) {
            *static_cast<ValueType *>(dest) = std::move(*static_cast<ValueType *>(src));
        };
    }

    // Move constructor
    scheduler_storage(scheduler_storage &&other) noexcept
    {
        if (other.move_construct_func)
        {
            other.move_construct_func(&data, &other.data);
            schedule_func = other.schedule_func;
            destroy_func = other.destroy_func;
            move_construct_func = other.move_construct_func;
            move_assign_func = other.move_assign_func;

            // Reset the source object
            other.schedule_func = nullptr;
            other.destroy_func = nullptr;
            other.move_construct_func = nullptr;
            other.move_assign_func = nullptr;
        }
    }

    // Move assignment operator
    scheduler_storage &operator=(scheduler_storage &&other) noexcept
    {
        if (this != &other)
        {
            // Destroy current object
            if (destroy_func)
            {
                destroy_func(&data);
            }

            if (other.move_construct_func)
            {
                other.move_construct_func(&data, &other.data);
                schedule_func = other.schedule_func;
                destroy_func = other.destroy_func;
                move_construct_func = other.move_construct_func;
                move_assign_func = other.move_assign_func;

                // Reset the source object
                other.schedule_func = nullptr;
                other.destroy_func = nullptr;
                other.move_construct_func = nullptr;
                other.move_assign_func = nullptr;
            }
            else
            {
                schedule_func = nullptr;
                destroy_func = nullptr;
                move_construct_func = nullptr;
                move_assign_func = nullptr;
            }
        }
        return *this;
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

    // Delete copy operations
    scheduler_storage(const scheduler_storage &) = default;
    scheduler_storage &operator=(const scheduler_storage &) = default;
};

int main()
{
    // Test 1: Basic functionality
    run_loop loop;
    scheduler sched{&loop};
    scheduler_storage storage1(sched);
    auto s1 = storage1.schedule();
    assert(s1.loop == &loop);

    // Test 2: Move constructor
    scheduler_storage storage2(std::move(storage1));
    auto s2 = storage2.schedule();
    assert(s2.loop == &loop);

    // Test 3: Move assignment
    scheduler_storage storage3(scheduler{&loop});
    storage3 = std::move(storage2);
    auto s3 = storage3.schedule();
    assert(s3.loop == &loop);

    // Test 4: Verify moved-from object is in valid state
    // (should not crash when destroyed, but can't be used)

    std::cout << "All tests passed successfully" << '\n';
    return 0;
}
// NOLINTEND
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <type_traits>
#include <utility>
#include <variant>

// NOLINTBEGIN

static int new_count = 0;
static int delete_count = 0;

template <typename T>
concept scheduler_concept = requires(T t) {
    { t.schedule() } noexcept;
    requires std::copy_constructible<T>;
    requires std::move_constructible<T>;
    requires std::destructible<T>;
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

struct scheduler_function_table
{
    using destroy_type = void(void *data) noexcept;
    using move_construct_type = void(void *dest, void *src) noexcept;
    using move_assign_type = void(void *dest, void *src) noexcept;
    using copy_construct_type = void(void *dest, const void *src) noexcept;
    using copy_assign_type = void(void *dest, const void *src) noexcept;
    using do_new_type = void *();

    
    using schedule_type = sender(void *data) noexcept;

    constexpr auto destroy(void *data) noexcept
    {
        destroy_(data);
    }
    constexpr auto move_construct(void *dest, void *src) noexcept
    {
        move_construct_(dest, src);
    }
    constexpr auto move_assign(void *dest, void *src) noexcept
    {
        move_assign_(dest, src);
    }
    constexpr auto copy_construct(void *dest, const void *src) const noexcept
    {
        copy_construct_(dest, src);
    }
    constexpr auto copy_assign(void *dest, const void *src) const noexcept
    {
        copy_assign_(dest, src);
    }
    constexpr auto schedule(void *obj) noexcept
    {
        return schedule_(obj);
    }

    constexpr scheduler_function_table(destroy_type *destroy,
                                       move_construct_type *move_construct,
                                       copy_construct_type *copy_construct,
                                       move_assign_type *move_assign,
                                       copy_assign_type *copy_assign, do_new_type *do_new,
                                       schedule_type *schedule) noexcept
        : destroy_{destroy}, move_construct_{move_construct}, move_assign_{move_assign},
          copy_construct_{copy_construct}, copy_assign_{copy_assign}, do_new_{do_new},
          schedule_{schedule}
    {
    }
    [[nodiscard]] constexpr void *do_new() const
    {
        return do_new_();
    }
    [[nodiscard]] constexpr void *do_new()
    {
        return do_new_();
    }
    static constexpr void do_delete(void *storage) noexcept
    {
        ++delete_count;
        ::operator delete(storage);
    }

  private:
    destroy_type *destroy_;
    move_construct_type *move_construct_;
    move_assign_type *move_assign_;
    copy_construct_type *copy_construct_;
    copy_assign_type *copy_assign_;
    do_new_type *do_new_;
    schedule_type *schedule_;
};

template <scheduler_concept S>
struct class_func_table : scheduler_function_table
{
    using value_type = std::decay_t<S>;

    constexpr class_func_table() noexcept
        : scheduler_function_table{&destroy,     &move_construct, &copy_construct,
                                   &move_assign, &copy_assign,    &do_new,
                                   &schedule} {

          };

    // for static_type and runtime_type
    static constexpr decltype(auto) launder_pointer(void *data) noexcept
    {
        return std::launder(static_cast<value_type *>(data));
    }
    static constexpr decltype(auto) launder_pointer(const void *data) noexcept
    {
        return std::launder(static_cast<const value_type *>(data));
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

    static constexpr auto copy_construct(void *dest, const void *src) noexcept(
        noexcept(new(dest) value_type(*launder_pointer(src))))
    {
        new (dest) value_type(*launder_pointer(src));
    }
    static constexpr auto copy_assign(void *dest, const void *src) noexcept(
        noexcept(*launder_pointer(dest) = *launder_pointer(src)))
    {
        *launder_pointer(dest) = *launder_pointer(src);
    }

    // for runtime_type
    static constexpr auto do_new()
    {
        ++new_count;
        return ::operator new(sizeof(value_type));
    }
};

struct scheduler_interface
{
    static constexpr auto max_size = sizeof(void *) * 2;
    struct static_type
    {
        alignas(std::max_align_t) std::byte storage[max_size];
    };
    struct runtime_type
    {
        alignas(std::max_align_t) void *storage;
    };
    using storage_type = std::variant<std::monostate, static_type, runtime_type>;

    template <scheduler_concept Sch>
    constexpr explicit scheduler_interface(Sch &&sched)
        : v_table{class_func_table<std::decay_t<Sch>>{}}
    {
        using value_type = std::decay_t<Sch>;
        if (sizeof(value_type) <= max_size)
        {
            static_type st{};
            new (&st.storage) value_type(std::forward<Sch>(sched));
            data = std::move(st);
        }
        else
        {
            runtime_type rt{};
            rt.storage = class_func_table<value_type>::do_new();
            new (rt.storage) value_type(std::forward<Sch>(sched));
            data = std::move(rt);
        }
        assert(not std::holds_alternative<std::monostate>(data));
    }
    constexpr void reset() noexcept
    {
        if (std::holds_alternative<static_type>(data))
        {
            auto &st = std::get<static_type>(data);
            v_table.destroy(&st.storage);
        }
        else if (std::holds_alternative<runtime_type>(data))
        {
            auto &rt = std::get<runtime_type>(data);
            if (rt.storage != nullptr)
            {
                v_table.destroy(rt.storage);
                scheduler_function_table::do_delete(rt.storage);
            }
        }
        data = std::monostate{};
    }

    constexpr ~scheduler_interface() noexcept
    {
        reset();
    }
    constexpr scheduler_interface(scheduler_interface &&other) noexcept
        : v_table{other.v_table}
    {
        if (std::holds_alternative<static_type>(other.data))
        {
            static_type dest;
            auto &src = std::get<static_type>(other.data);
            v_table.move_construct(dest.storage, src.storage);
            other.v_table.destroy(src.storage);
            data = std::move(dest);
        }
        else if (std::holds_alternative<runtime_type>(other.data))
        {
            runtime_type dest;
            auto &src = std::get<runtime_type>(other.data);
            dest.storage = src.storage;
            src.storage = nullptr;
            data = std::move(dest);
        }
        other.reset();
    }
    constexpr scheduler_interface(const scheduler_interface &other)
        : v_table{other.v_table}
    {
        if (std::holds_alternative<static_type>(other.data))
        {
            using T = static_type;
            T dest;
            const auto &src = std::get<T>(other.data);
            v_table.copy_construct(dest.storage, src.storage);
            data = std::move(dest);
        }
        else if (std::holds_alternative<runtime_type>(other.data))
        {
            using T = runtime_type;
            const auto &src = std::get<runtime_type>(other.data);
            T dest;
            dest.storage = other.v_table.do_new();
            v_table.copy_construct(dest.storage, src.storage);
            data = std::move(dest);
        }
    }
    constexpr scheduler_interface &operator=(const scheduler_interface &other)
    {
        if (&other != this)
        {
            this->reset();
            v_table = other.v_table;
            if (std::holds_alternative<static_type>(other.data))
            {
                using T = static_type;
                T dest;
                const auto &src = std::get<T>(other.data);
                v_table.copy_construct(dest.storage, src.storage);
                data = std::move(dest);
            }
            else if (std::holds_alternative<runtime_type>(other.data))
            {
                using T = runtime_type;
                const auto &src = std::get<runtime_type>(other.data);
                T dest;
                dest.storage = other.v_table.do_new();
                v_table.copy_construct(dest.storage, src.storage);
                data = std::move(dest);
            }
        }
        return *this;
    };
    constexpr scheduler_interface &operator=(scheduler_interface &&other) noexcept
    {
        if (&other != this)
        {
            this->reset();
            v_table = other.v_table;
            if (std::holds_alternative<static_type>(other.data))
            {
                using T = static_type;
                T dest;
                auto &src = std::get<T>(other.data);
                v_table.move_construct(dest.storage, src.storage); // 使用 move_construct
                data = std::move(dest);
            }
            else if (std::holds_alternative<runtime_type>(other.data))
            {
                using T = runtime_type;
                T dest;
                auto &src = std::get<runtime_type>(other.data);
                dest.storage = src.storage;
                data = std::move(dest);
                src.storage = nullptr;
            }
            other.reset();
        }
        return *this;
    };

    scheduler_function_table v_table;
    storage_type data;
};

template <bool runtime = false>
constexpr auto base() noexcept
{

    constexpr auto info = runtime ? "[runtime]" : "[static]";
    std::cout << '\n' << info << "  start: \n";

    static constexpr auto k_max_size = sizeof(void *) * 2;
    alignas(std::max_align_t) std::byte storage_mem[k_max_size];
    alignas(std::max_align_t) std::byte copy_storage_mem[k_max_size];

    // 仅分配原始内存（不构造对象），无异常版本
    void *storage = runtime ? ::operator new(k_max_size, std::nothrow) : &storage_mem;
    void *copy_storage =
        runtime ? ::operator new(k_max_size, std::nothrow) : &copy_storage_mem;
    assert(storage != nullptr);
    assert(copy_storage != nullptr);

    {
        run_loop loop;

        using type = scheduler;
        using F = class_func_table<scheduler>;

        std::cout << "::new (storage) type(&loop)" << '\n';

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

    // 记得释放内存
    if constexpr (runtime)
    {
        ::operator delete(storage);
        ::operator delete(copy_storage);
    }
    std::cout << '\n' << info << "  [end]\n";
    return true;
}

// 测试函数
void test_scheduler_interface()
{
    std::cout << "=== Testing scheduler_interface ===\n";

    {
        std::cout << "\n1. Testing basic construction and destruction:\n";
        run_loop loop;
        scheduler_interface si{scheduler(&loop)};

        // 验证存储类型
        if (sizeof(scheduler) <= scheduler_interface::max_size)
        {
            assert(std::holds_alternative<scheduler_interface::static_type>(si.data));
            std::cout << "   Using static storage - OK\n";
        }
        else
        {
            assert(std::holds_alternative<scheduler_interface::runtime_type>(si.data));
            std::cout << "   Using dynamic storage - OK\n";
        }

        // 验证调度功能
        auto s = si.v_table.schedule(
            std::holds_alternative<scheduler_interface::static_type>(si.data)
                ? &std::get<scheduler_interface::static_type>(si.data).storage
                : std::get<scheduler_interface::runtime_type>(si.data).storage);
        assert(s.loop == &loop);
        std::cout << "   Schedule function works - OK\n";
    } // si 应该在这里正确析构

    {
        std::cout << "\n2. Testing copy construction:\n";
        run_loop loop;
        scheduler_interface si1{scheduler(&loop)};
        scheduler_interface si2{si1}; // 复制构造

        // 验证两个对象都有效
        void *si1_storage =
            std::holds_alternative<scheduler_interface::static_type>(si1.data)
                ? &std::get<scheduler_interface::static_type>(si1.data).storage
                : std::get<scheduler_interface::runtime_type>(si1.data).storage;

        void *si2_storage =
            std::holds_alternative<scheduler_interface::static_type>(si2.data)
                ? &std::get<scheduler_interface::static_type>(si2.data).storage
                : std::get<scheduler_interface::runtime_type>(si2.data).storage;

        auto s1 = si1.v_table.schedule(si1_storage);
        auto s2 = si2.v_table.schedule(si2_storage);

        assert(s1.loop == &loop);
        assert(s2.loop == &loop);
        std::cout << "   Copy construction works - OK\n";
    }

    {
        std::cout << "\n3. Testing move construction:\n";
        run_loop loop;
        scheduler_interface si1{scheduler(&loop)};

        // 获取原始指针
        void *original_storage =
            std::holds_alternative<scheduler_interface::static_type>(si1.data)
                ? &std::get<scheduler_interface::static_type>(si1.data).storage
                : std::get<scheduler_interface::runtime_type>(si1.data).storage;

        scheduler_interface si2{std::move(si1)}; // 移动构造

        // 验证源对象已被置空
        assert(std::holds_alternative<std::monostate>(si1.data));
        std::cout << "   Source object reset after move - OK\n";

        // 验证目标对象有效
        void *si2_storage =
            std::holds_alternative<scheduler_interface::static_type>(si2.data)
                ? &std::get<scheduler_interface::static_type>(si2.data).storage
                : std::get<scheduler_interface::runtime_type>(si2.data).storage;

        auto s2 = si2.v_table.schedule(si2_storage);
        assert(s2.loop == &loop);
        std::cout << "   Move construction works - OK\n";
    }

    {
        std::cout << "\n4. Testing copy assignment:\n";
        run_loop loop1, loop2;
        scheduler_interface si1{scheduler(&loop1)};
        scheduler_interface si2{scheduler(&loop2)};

        // 验证初始状态
        void *si2_storage =
            std::holds_alternative<scheduler_interface::static_type>(si2.data)
                ? &std::get<scheduler_interface::static_type>(si2.data).storage
                : std::get<scheduler_interface::runtime_type>(si2.data).storage;

        auto s2_before = si2.v_table.schedule(si2_storage);
        assert(s2_before.loop == &loop2);

        // 执行复制赋值
        si2 = si1;

        // 验证复制后状态
        void *si1_storage =
            std::holds_alternative<scheduler_interface::static_type>(si1.data)
                ? &std::get<scheduler_interface::static_type>(si1.data).storage
                : std::get<scheduler_interface::runtime_type>(si1.data).storage;

        si2_storage = std::holds_alternative<scheduler_interface::static_type>(si2.data)
                          ? &std::get<scheduler_interface::static_type>(si2.data).storage
                          : std::get<scheduler_interface::runtime_type>(si2.data).storage;

        auto s1_after = si1.v_table.schedule(si1_storage);
        auto s2_after = si2.v_table.schedule(si2_storage);

        assert(s1_after.loop == &loop1);
        assert(s2_after.loop == &loop1);
        std::cout << "   Copy assignment works - OK\n";
    }

    {
        std::cout << "\n5. Testing move assignment:\n";
        run_loop loop1, loop2;
        scheduler_interface si1{scheduler(&loop1)};
        scheduler_interface si2{scheduler(&loop2)};

        // 获取原始指针
        void *original_storage =
            std::holds_alternative<scheduler_interface::static_type>(si1.data)
                ? &std::get<scheduler_interface::static_type>(si1.data).storage
                : std::get<scheduler_interface::runtime_type>(si1.data).storage;

        // 执行移动赋值
        si2 = std::move(si1);

        // 验证源对象已被置空
        assert(std::holds_alternative<std::monostate>(si1.data));
        std::cout << "   Source object reset after move assignment - OK\n";

        // 验证目标对象有效
        void *si2_storage =
            std::holds_alternative<scheduler_interface::static_type>(si2.data)
                ? &std::get<scheduler_interface::static_type>(si2.data).storage
                : std::get<scheduler_interface::runtime_type>(si2.data).storage;

        auto s2_after = si2.v_table.schedule(si2_storage);
        assert(s2_after.loop == &loop1);
        std::cout << "   Move assignment works - OK\n";
    }

    {
        std::cout << "\n6. Testing self-assignment:\n";
        run_loop loop;
        scheduler_interface si{scheduler(&loop)};

        // 获取原始指针
        void *original_storage =
            std::holds_alternative<scheduler_interface::static_type>(si.data)
                ? &std::get<scheduler_interface::static_type>(si.data).storage
                : std::get<scheduler_interface::runtime_type>(si.data).storage;

        // 自我赋值
        si = si;

        // 验证对象仍然有效
        void *si_storage =
            std::holds_alternative<scheduler_interface::static_type>(si.data)
                ? &std::get<scheduler_interface::static_type>(si.data).storage
                : std::get<scheduler_interface::runtime_type>(si.data).storage;

        auto s_after = si.v_table.schedule(si_storage);
        assert(s_after.loop == &loop);
        std::cout << "   Self-assignment works - OK\n";
    }

    {
        std::cout << "\n7. Testing reset function:\n";
        run_loop loop;
        scheduler_interface si{scheduler(&loop)};

        // 验证初始状态
        assert(!std::holds_alternative<std::monostate>(si.data));

        // 重置
        si.reset();

        // 验证重置后状态
        assert(std::holds_alternative<std::monostate>(si.data));
        std::cout << "   Reset function works - OK\n";
    }

    std::cout << "\n=== All scheduler_interface tests passed! ===\n\n";
}

// 测试大对象（使用动态存储）
struct large_scheduler
{
    static constexpr auto tap = "   ";
    run_loop *loop;
    char large_buffer[sizeof(void *) * 3]; // 确保超过 max_size

    constexpr auto schedule() noexcept
    {
        std::cout << tap << ">>> large_scheduler::schedule()...\n";
        return sender{loop};
    }

    constexpr large_scheduler() noexcept : loop(nullptr)
    {
        std::cout << tap << ">>> large_scheduler()\n";
    }

    constexpr explicit large_scheduler(run_loop *l) noexcept : loop(l)
    {
        std::cout << tap << ">>> large_scheduler(run_loop* l)\n";
    }

    constexpr large_scheduler(const large_scheduler &other) noexcept : loop(other.loop)
    {
        std::cout << tap << ">>> large_scheduler(const large_scheduler& other)\n";
    }

    constexpr large_scheduler(large_scheduler &&other) noexcept
        : loop(std::exchange(other.loop, nullptr))
    {
        std::cout << tap << ">>> large_scheduler(large_scheduler&& other) noexcept\n";
    }

    constexpr large_scheduler &operator=(const large_scheduler &other) noexcept
    {
        std::cout << tap
                  << ">>> large_scheduler& operator=(const large_scheduler& other)\n";
        if (this != &other)
        {
            loop = other.loop;
        }
        return *this;
    }

    constexpr large_scheduler &operator=(large_scheduler &&other) noexcept
    {
        std::cout << tap << ">>> large_scheduler& operator=(large_scheduler&& other)\n";
        if (this != &other)
        {
            loop = std::exchange(other.loop, nullptr);
        }
        return *this;
    }

    constexpr ~large_scheduler() noexcept
    {
        std::cout << tap << ">>> ~large_scheduler()\n";
        loop = nullptr;
    }
};

// 确保 large_scheduler 符合概念要求
static_assert(scheduler_concept<large_scheduler>);

void test_large_scheduler_interface()
{
    std::cout << "=== Testing scheduler_interface with large object ===\n";

    {
        std::cout << "\n1. Testing basic construction and destruction:\n";
        run_loop loop;
        scheduler_interface si{large_scheduler(&loop)};

        // 验证使用动态存储
        assert(std::holds_alternative<scheduler_interface::runtime_type>(si.data));
        std::cout << "   Using dynamic storage - OK\n";

        // 验证调度功能
        auto s = si.v_table.schedule(
            std::get<scheduler_interface::runtime_type>(si.data).storage);
        assert(s.loop == &loop);
        std::cout << "   Schedule function works - OK\n";
    } // si 应该在这里正确析构

    std::cout << "new_count: " << new_count << " , delete_count:" << delete_count << '\n';

    std::cout << "\n=== All large scheduler_interface tests passed! ===\n\n";
}

void test_mixed_scheduler_operations()
{
    std::cout << "=== Testing Mixed Scheduler Operations ===\n";

    // 保存初始计数，以便后续比较
    int initial_new_count = new_count;
    int initial_delete_count = delete_count;

    {
        run_loop loop1, loop2;

        std::cout << "\n1. Testing large-to-large copy construction:\n";
        scheduler_interface si1{large_scheduler(&loop1)};
        scheduler_interface si2{si1}; // 复制构造

        // 验证两个对象都有效
        void *si1_storage = std::get<scheduler_interface::runtime_type>(si1.data).storage;
        void *si2_storage = std::get<scheduler_interface::runtime_type>(si2.data).storage;

        auto s1 = si1.v_table.schedule(si1_storage);
        auto s2 = si2.v_table.schedule(si2_storage);

        assert(s1.loop == &loop1);
        assert(s2.loop == &loop1);
        std::cout << "   Large-to-large copy construction works - OK\n";

        std::cout << "\n2. Testing large-to-large move construction:\n";
        scheduler_interface si3{std::move(si1)}; // 移动构造

        // 验证源对象已被置空
        assert(std::holds_alternative<std::monostate>(si1.data));

        // 验证目标对象有效
        void *si3_storage = std::get<scheduler_interface::runtime_type>(si3.data).storage;
        auto s3 = si3.v_table.schedule(si3_storage);
        assert(s3.loop == &loop1);
        std::cout << "   Large-to-large move construction works - OK\n";

        std::cout << "\n3. Testing large-to-large copy assignment:\n";
        scheduler_interface si4{large_scheduler(&loop2)};
        si4 = si2; // 复制赋值

        // 验证赋值后状态
        void *si4_storage = std::get<scheduler_interface::runtime_type>(si4.data).storage;
        auto s4 = si4.v_table.schedule(si4_storage);
        assert(s4.loop == &loop1);
        std::cout << "   Large-to-large copy assignment works - OK\n";

        std::cout << "\n4. Testing large-to-large move assignment:\n";
        scheduler_interface si5{large_scheduler(&loop2)};
        si5 = std::move(si3); // 移动赋值

        // 验证源对象已被置空
        assert(std::holds_alternative<std::monostate>(si3.data));

        // 验证目标对象有效
        void *si5_storage = std::get<scheduler_interface::runtime_type>(si5.data).storage;
        auto s5 = si5.v_table.schedule(si5_storage);
        assert(s5.loop == &loop1);
        std::cout << "   Large-to-large move assignment works - OK\n";
    }

    // 检查内存泄漏
    assert(new_count - initial_new_count == delete_count - initial_delete_count);
    std::cout << "   No memory leaks in large-to-large operations - OK\n";

    {
        run_loop loop1, loop2;

        std::cout << "\n5. Testing small-to-small operations:\n";
        scheduler_interface si1{scheduler(&loop1)};
        scheduler_interface si2{si1};            // 复制构造
        scheduler_interface si3{std::move(si1)}; // 移动构造

        // 验证复制和移动操作
        void *si2_storage = &std::get<scheduler_interface::static_type>(si2.data).storage;
        void *si3_storage = &std::get<scheduler_interface::static_type>(si3.data).storage;

        auto s2 = si2.v_table.schedule(si2_storage);
        auto s3 = si3.v_table.schedule(si3_storage);

        assert(s2.loop == &loop1);
        assert(s3.loop == &loop1);
        std::cout << "   Small-to-small operations work - OK\n";

        std::cout << "\n6. Testing small-to-large assignment:\n";
        scheduler_interface si_large{large_scheduler(&loop2)};
        si_large = si2; // 将小对象赋值给大对象

        // NOTE: si_large 就变成小对象了
        assert(std::holds_alternative<scheduler_interface::static_type>(si_large.data));

        // 验证赋值后状态
        void *si_large_storage =
            std::get<scheduler_interface::static_type>(si_large.data).storage;
        auto s_large = si_large.v_table.schedule(si_large_storage);
        assert(s_large.loop == &loop1);
        std::cout << "   Small-to-large assignment works - OK\n";

        std::cout << "\n7. Testing large-to-small assignment:\n";
        scheduler_interface si_small{scheduler(&loop2)};
        si_small = si_large; // 将大对象赋值给小对象

        // 验证赋值后状态
        void *si_small_storage =
            &std::get<scheduler_interface::static_type>(si_small.data).storage;
        auto s_small = si_small.v_table.schedule(si_small_storage);
        assert(s_small.loop == &loop1);
        std::cout << "   Large-to-small assignment works - OK\n";
    }

    // 再次检查内存泄漏
    assert(new_count - initial_new_count == delete_count - initial_delete_count);
    std::cout << "   No memory leaks in mixed operations - OK\n";

    std::cout << "\n=== All mixed scheduler operations tests passed! ===\n\n";
}

void test_edge_cases()
{
    std::cout << "=== Testing Edge Cases ===\n";

    int initial_new_count = new_count;
    int initial_delete_count = delete_count;

    {
        std::cout << "\n1. Testing assignment to self:\n";
        run_loop loop;
        scheduler_interface si{large_scheduler(&loop)};

        // 保存原始指针
        void *original_storage =
            std::get<scheduler_interface::runtime_type>(si.data).storage;

        // 自我赋值
        si = si;

        // 验证对象仍然有效
        void *current_storage =
            std::get<scheduler_interface::runtime_type>(si.data).storage;
        auto s = si.v_table.schedule(current_storage);
        assert(s.loop == &loop);
        std::cout << "   Self-assignment for large object works - OK\n";
    }

    {
        std::cout << "\n2. Testing move assignment to self:\n";
        run_loop loop;
        scheduler_interface si{large_scheduler(&loop)};

        // 保存原始指针
        void *original_storage =
            std::get<scheduler_interface::runtime_type>(si.data).storage;

        // 移动自我赋值
        si = std::move(si);

        // 验证对象仍然有效
        void *current_storage =
            std::get<scheduler_interface::runtime_type>(si.data).storage;
        auto s = si.v_table.schedule(current_storage);
        assert(s.loop == &loop);
        std::cout << "   Move self-assignment for large object works - OK\n";
    }

    {
        std::cout << "\n3. Testing empty object assignment:\n";
        run_loop loop;
        scheduler_interface si1{large_scheduler(&loop)};

        // 由于我们的实现删除了默认构造函数，这个测试需要调整
        // 我们可以使用 reset() 创建一个空对象
        si1.reset(); // 现在 si1 是空的

        scheduler_interface si3{large_scheduler(&loop)};
        si3 = si1; // 将空对象赋值给非空对象

        assert(std::holds_alternative<std::monostate>(si3.data));
        std::cout << "   Empty object assignment works - OK\n";
    }

    // 检查内存泄漏
    assert(new_count - initial_new_count == delete_count - initial_delete_count);
    std::cout << "   No memory leaks in edge cases - OK\n";

    std::cout << "\n=== All edge case tests passed! ===\n\n";
}

int main()
{
    std::cout << "new_count: " << new_count << " , delete_count:" << delete_count << '\n';

    base<>();
    std::cout << "new_count: " << new_count << " , delete_count:" << delete_count << '\n';

    base<true>();
    std::cout << "new_count: " << new_count << " , delete_count:" << delete_count << '\n';

    // 测试
    // 运行基本测试
    test_scheduler_interface();
    std::cout << "new_count: " << new_count << " , delete_count:" << delete_count << '\n';

    test_large_scheduler_interface();
    std::cout << "new_count: " << new_count << " , delete_count:" << delete_count << '\n';

    // 运行新的混合操作测试
    test_mixed_scheduler_operations();
    std::cout << "new_count: " << new_count << " , delete_count:" << delete_count << '\n';

    // 运行边界情况测试
    test_edge_cases();
    std::cout << "new_count: " << new_count << " , delete_count:" << delete_count << '\n';

    assert(new_count == delete_count);
    std::cout << "All tests passed successfully" << '\n';
    return 0;
}
// NOLINTEND
#include <bit>
#include <cstddef>
#include <iostream>
#include <memory>
#include <type_traits>
#include <utility>

// NOLINTBEGIN

struct set_value_t;

template <typename T>
struct get_completion_scheduler_t
{
};

struct scheduler
{
    template <typename R>
    struct operation
    {
        R recv_;

        operation(R recv) : recv_(std::move(recv)) {}

        void start() & noexcept
        {
            std::cout << "operation start()\n";
            recv_.set_value();
        }
    };

    struct sender
    {
        scheduler *sch;

        struct env
        {
            constexpr auto query(get_completion_scheduler_t<set_value_t>) const noexcept
            {
                return *sch;
            }
            scheduler *sch;
        };

        constexpr auto get_env() const noexcept -> env
        {
            return {sch};
        }

        template <typename R>
        auto connect(R recv) -> operation<R>
        {
            std::cout << "scheduler::sender::connect() called\n";
            return operation<R>{std::move(recv)};
        }
    };

    auto schedule() noexcept
    {
        return sender{this};
    }
};

template <size_t N>
struct compile_string
{
    char data[N]{}; // 包含空字符

    constexpr compile_string(const char (&str)[N])
    {
        for (size_t i = 0; i < N; ++i)
        {
            data[i] = str[i];
        }
    }

    constexpr const char *c_str() const
    {
        return data;
    }
    constexpr size_t size() const
    {
        return N - 1;
    } // 不包括空字符
};

// 推导指南

template <size_t N>
std::ostream &operator<<(std::ostream &os, const compile_string<N> &cs)
{
    return os << cs.c_str();
}
template <size_t N>
compile_string(const char (&)[N]) -> compile_string<N>;

template <std::size_t size = 1 * sizeof(void *), size_t align = alignof(std::max_align_t)>
struct allocator_storage
{
    static constexpr auto buffer_size = size;
    static constexpr auto align_size = align;

    union storage_union {
        alignas(align) std::byte stack_buffer[size];
        void *heap_ptr;

        constexpr storage_union() : stack_buffer{} {}
    } storage_;

    template <typename T>
    constexpr T *as_small() noexcept
    {
        return std::bit_cast<T *>(&storage_.stack_buffer[0]);
    }

    template <typename T>
    constexpr T *as_large() noexcept
    {
        return static_cast<T *>(storage_.heap_ptr);
    }

    // 添加 const 版本
    template <typename T>
    constexpr const T *as_small() const noexcept
    {
        return std::bit_cast<const T *>(&storage_.stack_buffer[0]);
    }

    template <typename T>
    constexpr const T *as_large() const noexcept
    {
        return static_cast<const T *>(storage_.heap_ptr);
    }

    template <typename T>
    static consteval bool is_small() noexcept
    {
        return sizeof(T) <= buffer_size && alignof(T) <= align_size;
    }

    template <typename T>
    static constexpr T *get_pointer(void *ptr) noexcept
    {
        if constexpr (is_small<T>())
            return std::bit_cast<allocator_storage *>(ptr)->template as_small<T>();
        else
            return std::bit_cast<allocator_storage *>(ptr)->template as_large<T>();
    }

    // 获取分配器引用
    template <typename T>
    constexpr T &get_allocator() noexcept
    {
        if constexpr (is_small<T>())
        {
            return *as_small<T>();
        }
        else
        {
            return *as_large<T>();
        }
    }

    template <typename Allocator>
    explicit constexpr allocator_storage(Allocator &&alloc)
    {
        construct(std::forward<Allocator>(alloc));
    }

    // 使用编译器生成的拷贝和移动操作
    constexpr allocator_storage() = default;
    constexpr allocator_storage(const allocator_storage &) = default;
    constexpr allocator_storage(allocator_storage &&) = default;
    constexpr allocator_storage &operator=(const allocator_storage &) = default;
    constexpr allocator_storage &operator=(allocator_storage &&) = default;

    template <typename Allocator>
    constexpr void construct(Allocator &&alloc)
    {
        using T = std::decay_t<Allocator>;
        using ReboundAlloc = typename std::allocator_traits<
            std::decay_t<Allocator>>::template rebind_alloc<T>;
        ReboundAlloc rebound_alloc(alloc);

        if constexpr (is_small<T>())
        {
            std::cout << "✅ Allocator 使用栈缓冲区构造" << '\n';
            auto *ptr = as_small<T>();
            std::allocator_traits<ReboundAlloc>::construct(
                rebound_alloc, ptr, std::forward<Allocator>(alloc));
        }
        else
        {
            std::cout << "🔄 Allocator 使用堆分配构造" << '\n';
            static_assert(false, "not supported");
        }
    }

    ~allocator_storage() noexcept = default;
};

template <std::size_t size, size_t align, compile_string id = "">
struct any_storage
{
    static constexpr auto buffer_size = size;
    static constexpr auto align_size = align;

    using allocator_storage_type = allocator_storage<>;

    struct storage_ops
    {
        void (*destroy)(any_storage *self) noexcept;
        void (*copy_construct)(any_storage *dest, const any_storage *src);
        void (*move_construct)(any_storage *dest, any_storage *src) noexcept;
    };

    allocator_storage_type allocator_;
    union storage_union {
        alignas(align) std::byte stack_buffer[size];
        void *heap_ptr;

        constexpr storage_union() : stack_buffer{} {}
    } storage_;

    const storage_ops *ops_ = nullptr;

    template <typename T>
    constexpr T *as_small() noexcept
    {
        return std::bit_cast<T *>(&storage_.stack_buffer[0]);
    }

    template <typename T>
    constexpr T *as_large() noexcept
    {
        return static_cast<T *>(storage_.heap_ptr);
    }

    // 添加 const 版本
    template <typename T>
    constexpr const T *as_small() const noexcept
    {
        return std::bit_cast<const T *>(&storage_.stack_buffer[0]);
    }

    template <typename T>
    constexpr const T *as_large() const noexcept
    {
        return static_cast<const T *>(storage_.heap_ptr);
    }

    template <typename T>
    static consteval bool is_small() noexcept
    {
        return sizeof(T) <= buffer_size && alignof(T) <= align_size;
    }

    template <typename T>
    static constexpr T *get_pointer(void *storage) noexcept
    {
        if constexpr (is_small<T>())
            return std::bit_cast<any_storage *>(storage)->template as_small<T>();
        else
            return std::bit_cast<any_storage *>(storage)->template as_large<T>();
    }

    template <typename T>
    static constexpr T *get_allocator(void *storage) noexcept
    {
        return allocator_storage_type::get_pointer<T>(
            &std::bit_cast<any_storage *>(storage)->allocator_);
    }

    // 具体的操作实现
    template <typename T, typename Allocator>
    static void destroy_impl(any_storage *self) noexcept
    {
        using ReboundAlloc =
            typename std::allocator_traits<Allocator>::template rebind_alloc<T>;
        ReboundAlloc rebound_alloc(self->allocator_.template get_allocator<Allocator>());

        if constexpr (is_small<T>())
        {
            std::cout << "🗑️ " << id.c_str() << " 销毁栈对象" << '\n';
            auto *ptr = self->template as_small<T>();
            std::allocator_traits<ReboundAlloc>::destroy(rebound_alloc, ptr);
        }
        else
        {
            std::cout << "🗑️ " << id.c_str() << " 销毁堆对象" << '\n';
            auto *ptr = self->template as_large<T>();
            if (ptr)
            {
                std::allocator_traits<ReboundAlloc>::destroy(rebound_alloc, ptr);
                std::allocator_traits<ReboundAlloc>::deallocate(rebound_alloc, ptr, 1);
                self->storage_.heap_ptr = nullptr;
            }
        }
    }

    template <typename T, typename Allocator>
    static void copy_construct_impl(any_storage *dest, const any_storage *src)
    {
        using ReboundAlloc =
            typename std::allocator_traits<Allocator>::template rebind_alloc<T>;
        ReboundAlloc rebound_alloc(dest->allocator_.template get_allocator<Allocator>());

        const T *src_obj = nullptr;
        if constexpr (is_small<T>())
        {
            src_obj = src->template as_small<T>();
        }
        else
        {
            src_obj = src->template as_large<T>();
        }

        if constexpr (is_small<T>())
        {
            std::cout << "📋 " << id.c_str() << " 拷贝构造到栈" << '\n';
            auto *ptr = dest->template as_small<T>();
            std::allocator_traits<ReboundAlloc>::construct(rebound_alloc, ptr, *src_obj);
        }
        else
        {
            std::cout << "📋 " << id.c_str() << " 拷贝构造到堆" << '\n';
            T *ptr = std::allocator_traits<ReboundAlloc>::allocate(rebound_alloc, 1);
            try
            {
                std::allocator_traits<ReboundAlloc>::construct(rebound_alloc, ptr,
                                                               *src_obj);
                dest->storage_.heap_ptr = ptr;
            }
            catch (...)
            {
                std::allocator_traits<ReboundAlloc>::deallocate(rebound_alloc, ptr, 1);
                throw;
            }
        }
    }

    template <typename T, typename Allocator>
    static void move_construct_impl(any_storage *dest, any_storage *src) noexcept
    {
        using ReboundAlloc =
            typename std::allocator_traits<Allocator>::template rebind_alloc<T>;
        ReboundAlloc rebound_alloc(dest->allocator_.template get_allocator<Allocator>());

        if constexpr (is_small<T>())
        {
            std::cout << "🚚 " << id.c_str() << " 移动构造到栈" << '\n';
            auto *src_ptr = src->template as_small<T>();
            auto *dest_ptr = dest->template as_small<T>();
            std::allocator_traits<ReboundAlloc>::construct(rebound_alloc, dest_ptr,
                                                           std::move(*src_ptr));
        }
        else
        {
            std::cout << "🚚 " << id.c_str() << " 移动构造到堆" << '\n';
            // 直接转移堆指针所有权
            dest->storage_.heap_ptr = src->storage_.heap_ptr;
            src->storage_.heap_ptr = nullptr;
        }
    }

    // 创建操作表
    template <typename T, typename Allocator>
    static constexpr storage_ops create_ops() noexcept
    {
        return storage_ops{.destroy = &destroy_impl<T, Allocator>,
                           .copy_construct = &copy_construct_impl<T, Allocator>,
                           .move_construct = &move_construct_impl<T, Allocator>};
    }

    template <typename Obj, typename Allocator>
    constexpr void construct(Obj &&obj, Allocator &&src_alloc)
    {
        using T = std::decay_t<Obj>;
        using ReboundAlloc = typename std::allocator_traits<
            std::decay_t<Allocator>>::template rebind_alloc<T>;
        ReboundAlloc rebound_alloc(src_alloc);

        // 设置操作表
        static constexpr storage_ops ops = create_ops<T, std::decay_t<Allocator>>();
        this->ops_ = &ops;

        if constexpr (is_small<T>())
        {
            std::cout << "✅ " << id.c_str() << " 使用栈缓冲区构造" << '\n';
            auto *ptr = as_small<T>();
            std::allocator_traits<ReboundAlloc>::construct(rebound_alloc, ptr,
                                                           std::forward<Obj>(obj));
        }
        else
        {
            std::cout << "🔄 " << id.c_str() << " 使用堆分配构造" << '\n';
            T *ptr = std::allocator_traits<ReboundAlloc>::allocate(rebound_alloc, 1);
            try
            {
                std::allocator_traits<ReboundAlloc>::construct(rebound_alloc, ptr,
                                                               std::forward<Obj>(obj));
                storage_.heap_ptr = ptr;
            }
            catch (...)
            {
                std::allocator_traits<ReboundAlloc>::deallocate(rebound_alloc, ptr, 1);
                throw;
            }
        }
    }

    template <typename Obj, typename Allocator>
    constexpr any_storage(Obj &&obj, Allocator &&alloc)
        : allocator_{allocator_storage_type{std::forward<Allocator>(alloc)}}
    {
        construct(std::forward<Obj>(obj), std::forward<Allocator>(alloc));
    }

    // 拷贝构造函数
    constexpr any_storage(const any_storage &other)
        : allocator_{other.allocator_}, ops_(other.ops_)
    {
        if (ops_)
        {
            ops_->copy_construct(this, &other);
        }
    }

    // 移动构造函数
    constexpr any_storage(any_storage &&other) noexcept
        : allocator_{std::move(other.allocator_)}, ops_(other.ops_)
    {
        if (ops_)
        {
            ops_->move_construct(this, &other);
            other.ops_ = nullptr;
        }
    }

    // 拷贝赋值运算符
    constexpr any_storage &operator=(const any_storage &other)
    {
        if (this != &other)
        {
            // 先销毁当前对象
            if (ops_)
            {
                ops_->destroy(this);
            }
            // 拷贝构造
            allocator_ = other.allocator_;
            ops_ = other.ops_;
            if (ops_)
            {
                ops_->copy_construct(this, &other);
            }
        }
        return *this;
    }

    // 移动赋值运算符
    constexpr any_storage &operator=(any_storage &&other) noexcept
    {
        if (this != &other)
        {
            // 先销毁当前对象
            if (ops_)
            {
                ops_->destroy(this);
            }
            // 移动构造
            allocator_ = std::move(other.allocator_);
            ops_ = other.ops_;
            if (ops_)
            {
                ops_->move_construct(this, &other);
                other.ops_ = nullptr;
            }
        }
        return *this;
    }

    // 析构函数
    ~any_storage() noexcept
    {
        if (ops_ && ops_->destroy)
        {
            ops_->destroy(this);
        }
    }
};

template <std::size_t size = 3 * sizeof(void *), size_t align = alignof(std::max_align_t)>
struct scheduler_storage : any_storage<size, align, "Sch">
{
    using any_storage<size, align, "Sch">::any_storage;
};

template <std::size_t size = 3 * sizeof(void *), size_t align = alignof(std::max_align_t)>
struct sender_storage : any_storage<size, align, "Sndr">
{
    using any_storage<size, align, "Sndr">::any_storage;
};

template <std::size_t size = 3 * sizeof(void *), size_t align = alignof(std::max_align_t)>
struct operation_storage : any_storage<size, align, "Oper">
{
    using any_storage<size, align, "Oper">::any_storage;
};

// sender_any 的签名是固定的，签名可以确定。但是 env_t呢？
// NOTE: 类型擦除，总得手动还原....。 就是 env_any 也解决不了返回值传递的过程
// NOTE: 如果  receiver 内部有成员呢？ 总之具体类型，替换，替代模板参数R是错误的
struct receiver_any
{
    void set_value()
    {
        std::cout << "receiver set_value()\n";
    }
    void set_error(std::error_code)
    {
        std::cout << "receiver set_error()\n";
    }
    void set_stopped()
    {
        std::cout << "receiver set_stopped()\n";
    }
};

struct scheduler_any
{
    using scheduler_storage_type = scheduler_storage<>;
    using sender_storage_type = sender_storage<>;
    using operation_storage_type = operation_storage<>;

    struct operation_any
    {

        struct vtable_t
        {
            void (*start)(void *op_storage) noexcept;
        };

        const vtable_t *op_vtable_;
        operation_storage_type op_storage_;

        template <typename Operation, typename Allocator>
        constexpr operation_any(Operation &&op, Allocator alloc)
            : op_vtable_{create_operation_vtable<Operation>()},
              op_storage_{std::forward<Operation>(op), std::forward<Allocator>(alloc)}
        {
        }

        constexpr void start() & noexcept
        {
            std::cout << "operation_any start() called\n";
            op_vtable_->start(&op_storage_);
        }

      private:
        template <typename ConcreteOp>
        constexpr static vtable_t *create_operation_vtable()
        {
            static vtable_t vt = {.start = &start_impl<ConcreteOp>};
            return &vt;
        }

        template <typename ConcreteOp>
        constexpr static void start_impl(void *op_storage) noexcept
        {
            ConcreteOp *op = operation_storage_type::get_pointer<ConcreteOp>(op_storage);
            op->start();
        }
    };

    struct sender_any
    {
        struct vtable_t
        {
            operation_any (*connect)(void *sndr_storage, receiver_any &&recv) noexcept;
        };
        const vtable_t *sndr_vtable_;
        sender_storage_type sndr_;

        template <typename Sndr, typename Allocator>
        constexpr sender_any(Sndr &&sndr, Allocator &&alloc)
            : sndr_vtable_{create_sender_vtable<Sndr, Allocator>()},
              sndr_{std::forward<Sndr>(sndr), std::forward<Allocator>(alloc)}
        {
        }

        template <typename R>
        constexpr auto connect(R) noexcept -> operation_any
        {
            std::cout << "sender_any connect() called\n";
            return sndr_vtable_->connect(&sndr_, receiver_any{});
        }

        struct env
        {

            [[nodiscard]] constexpr auto query( // NOLINT
                get_completion_scheduler_t<set_value_t> /*unused*/) const noexcept
            {
                // return task_scheduler{};
            }
        };

        // NOLINTNEXTLINE
        [[nodiscard]] constexpr auto get_env() const noexcept -> env
        {
            return {};
        }

      private:
        template <typename Sndr, typename Allocator>
        constexpr static vtable_t *create_sender_vtable()
        {
            static vtable_t vt = {.connect = &connect_impl<Sndr, Allocator>};
            return &vt;
        }

        template <typename Sndr, typename Allocator>
        constexpr static operation_any connect_impl(void *sndr_storage,
                                                    receiver_any &&recv) noexcept
        {
            Sndr *sndr = sender_storage_type::get_pointer<Sndr>(sndr_storage);
            auto *alloc = sender_storage_type::get_allocator<Allocator>(sndr_storage);
            using concrete = concrete_operation<Sndr, receiver_any>;
            return operation_any(concrete{sndr, std::move(recv)}, Allocator{*alloc});
        }

        template <typename Sndr, typename R>
        struct concrete_operation
        {
            using op_type = decltype(std::declval<Sndr>().connect(std::declval<R>()));
            op_type op_;

            constexpr concrete_operation(Sndr *sndr, R &&recv) noexcept
                : op_(sndr->connect(std::move(recv)))
            {
            }
            constexpr void start() & noexcept
            {
                op_.start();
            }
        };
    };

    struct vtable_t
    {
        sender_any (*schedule)(void *sched_storage) noexcept;
    };

    constexpr sender_any schedule() noexcept
    {
        std::cout << "scheduler_any schedule() called\n";
        return vtable_->schedule(&sch_);
    }

    template <class Sch, typename Allocator = std::allocator<std::byte>>
    constexpr explicit scheduler_any(Sch &&sch, Allocator alloc = Allocator{})
        : vtable_(create_vtable<Sch, Allocator>()),
          sch_(std::forward<Sch>(sch), std::forward<Allocator>(alloc))
    {
    }

    friend bool operator==(const scheduler_any &lhs, const scheduler_any &rhs) noexcept
    {
        if (lhs.vtable_ != rhs.vtable_)
            return false;
        return true; // NOTE: 得实现全部的再说
    }

    template <class Sch>
        requires(!std::same_as<sender_any, Sch>)
    friend bool operator==(const scheduler_any &lhs, const Sch &rhs) noexcept;

  private:
    template <class Sch, typename Allocator>
    constexpr static const vtable_t *create_vtable()
    {
        static const vtable_t vt = {.schedule = &schedule_impl<Sch, Allocator>};
        return &vt;
    }

    template <class Sch, typename Allocator>
    constexpr static sender_any schedule_impl(void *sched_storage) noexcept
    {
        auto *sched = scheduler_storage_type::get_pointer<Sch>(sched_storage);
        auto *alloc = scheduler_storage_type::get_allocator<Allocator>(sched_storage);
        return sender_any(sched->schedule(), Allocator{*alloc});
    }

    const vtable_t *vtable_;
    scheduler_storage_type sch_;
};

int main()
{
    std::cout << "=== 开始测试 scheduler_any ===\n";

    scheduler_any any{scheduler{}};
    auto sndr = any.schedule();

    receiver_any recv;
    auto op = sndr.connect(recv);

    std::cout << "=== 开始执行 operation ===\n";
    op.start();

    std::cout << "=== 测试完成 ===\n";
    return 0;
}
// NOLINTEND
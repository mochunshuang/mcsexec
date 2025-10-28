#include <algorithm>
#include <bit>
#include <cstddef>
#include <exception>
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
            std::cout << ".........operation start()\n";
            std::move(recv_).set_value();
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
    }
};

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

    template <typename T>
    constexpr T &get_allocator() noexcept
    {
        if constexpr (is_small<T>())
            return *as_small<T>();
        else
            return *as_large<T>();
    }

    template <typename Allocator>
    explicit constexpr allocator_storage(Allocator &&alloc)
    {
        construct(std::forward<Allocator>(alloc));
    }

    constexpr allocator_storage() = default;
    constexpr allocator_storage(const allocator_storage &) = default;
    constexpr allocator_storage(allocator_storage &&) = default;
    constexpr allocator_storage &operator=(const allocator_storage &) = default;
    constexpr allocator_storage &operator=(allocator_storage &&) = default;
    constexpr ~allocator_storage() noexcept = default;

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
        void (*move_destroy)(any_storage *src) noexcept;
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
            dest->storage_.heap_ptr = src->storage_.heap_ptr;
            src->storage_.heap_ptr = nullptr;
        }
    }

    // 移动后销毁源对象
    template <typename T, typename Allocator>
    static void move_destroy_impl(any_storage *src) noexcept
    {
        using ReboundAlloc =
            typename std::allocator_traits<Allocator>::template rebind_alloc<T>;
        ReboundAlloc rebound_alloc(src->allocator_.template get_allocator<Allocator>());

        if constexpr (is_small<T>())
        {
            std::cout << "🗑️ " << id.c_str() << " 移动后销毁源栈对象" << '\n';
            auto *ptr = src->template as_small<T>();
            std::allocator_traits<ReboundAlloc>::destroy(rebound_alloc, ptr);
        }
        else
        {
            std::cout << "🗑️ " << id.c_str() << " 移动后销毁源堆对象" << '\n';
            if (src->storage_.heap_ptr)
            {
                auto *ptr = src->template as_large<T>();
                std::allocator_traits<ReboundAlloc>::destroy(rebound_alloc, ptr);
                std::allocator_traits<ReboundAlloc>::deallocate(rebound_alloc, ptr, 1);
                src->storage_.heap_ptr = nullptr;
            }
        }
    }

    template <typename T, typename Allocator>
    static constexpr storage_ops create_ops() noexcept
    {
        return storage_ops{.destroy = &destroy_impl<T, Allocator>,
                           .copy_construct = &copy_construct_impl<T, Allocator>,
                           .move_construct = &move_construct_impl<T, Allocator>,
                           .move_destroy = &move_destroy_impl<T, Allocator>};
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

    constexpr any_storage() = default;
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
        : allocator_{std::move(other.allocator_)},
          ops_(std::exchange(other.ops_, nullptr))
    {
        if (ops_)
        {
            ops_->move_construct(this, &other);
            ops_->move_destroy(&other);
        }
    }

    // 拷贝赋值运算符
    constexpr any_storage &operator=(const any_storage &other)
    {
        if (this != &other)
        {
            if (ops_)
                ops_->destroy(this);

            allocator_ = other.allocator_;
            ops_ = other.ops_;
            if (ops_)
                ops_->copy_construct(this, &other);
        }
        return *this;
    }

    // 移动赋值运算符
    constexpr any_storage &operator=(any_storage &&other) noexcept
    {
        if (this != &other)
        {
            if (ops_)
                ops_->destroy(this);

            allocator_ = std::move(other.allocator_);
            ops_ = std::exchange(other.ops_, nullptr);
            if (ops_)
            {
                ops_->move_construct(this, &other);
                ops_->move_destroy(&other);
            }
        }
        return *this;
    }

    friend constexpr void swap(any_storage &a, any_storage &b) noexcept
    {
        using std::swap;
        swap(a.allocator_, b.allocator_);
        swap(a.storage_, b.storage_);
        swap(a.ops_, b.ops_);
    }

    constexpr ~any_storage() noexcept
    {
        if (ops_)
        {
            ops_->destroy(this);
            ops_ = nullptr;
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

template <std::size_t size = 5 * sizeof(void *), size_t align = alignof(std::max_align_t)>
struct operation_storage : any_storage<size, align, "Oper">
{
    using any_storage<size, align, "Oper">::any_storage;
};

template <std::size_t size = 2 * sizeof(void *), size_t align = alignof(std::max_align_t)>
struct receiver_storage : any_storage<size, align, "Recv">
{
    using any_storage<size, align, "Recv">::any_storage;
};

// 类型擦除的 receiver
struct receiver_any
{
    struct vtable_t
    {
        void (*set_value)(void *recv_storage) noexcept;
        void (*set_error)(void *recv_storage, std::error_code) noexcept;
        void (*set_error2)(void *recv_storage, std::exception_ptr) noexcept;
        void (*set_stopped)(void *recv_storage) noexcept;
    };

    const vtable_t *recv_vtable_;
    receiver_storage<> recv_storage_;

    template <typename R, typename Allocator = std::allocator<std::byte>>
    constexpr receiver_any(R &&recv, Allocator alloc = Allocator{})
        : recv_vtable_{create_receiver_vtable<R>()},
          recv_storage_{std::forward<R>(recv), std::forward<Allocator>(alloc)}
    {
    }

    void set_value() noexcept
    {
        std::cout << "receiver_any set_value()\n";
        recv_vtable_->set_value(&recv_storage_);
    }

    void set_error(std::error_code ec) noexcept
    {
        std::cout << "receiver_any set_error()\n";
        recv_vtable_->set_error(&recv_storage_, ec);
    }

    void set_error(std::exception_ptr e) noexcept
    {
        std::cout << "receiver_any set_error()\n";
        recv_vtable_->set_error2(&recv_storage_, e);
    }

    void set_stopped() noexcept
    {
        std::cout << "receiver_any set_stopped()\n";
        recv_vtable_->set_stopped(&recv_storage_);
    }

  private:
    template <typename R>
    constexpr static vtable_t *create_receiver_vtable()
    {
        static vtable_t vt = {.set_value = &set_value_impl<R>,
                              .set_error = &set_error_impl<R>,
                              .set_error2 = &set_error_impl2<R>,
                              .set_stopped = &set_stopped_impl<R>};
        return &vt;
    }

    template <typename R>
    constexpr static void set_value_impl(void *recv_storage) noexcept
    {
        R *recv = receiver_storage<>::get_pointer<R>(recv_storage);
        recv->set_value();
    }

    template <typename R>
    constexpr static void set_error_impl(void *recv_storage, std::error_code ec) noexcept
    {
        R *recv = receiver_storage<>::get_pointer<R>(recv_storage);
        if constexpr (requires() { recv->set_error(ec); })
        {
            recv->set_error(ec);
        }
    }
    template <typename R>
    constexpr static void set_error_impl2(void *recv_storage,
                                          std::exception_ptr e) noexcept
    {
        R *recv = receiver_storage<>::get_pointer<R>(recv_storage);
        if constexpr (requires() { recv->set_error(e); })
        {
            recv->set_error(e);
        }
    }

    template <typename R>
    constexpr static void set_stopped_impl(void *recv_storage) noexcept
    {
        R *recv = receiver_storage<>::get_pointer<R>(recv_storage);
        recv->set_stopped();
    }
};

struct scheduler_any
{
    using scheduler_storage_type = scheduler_storage<>;
    using sender_storage_type = sender_storage<>;
    using operation_storage_type = operation_storage<>;
    using receiver_storage_type = receiver_storage<>;

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
            operation_any (*connect)(void *sndr_storage, void *recv) noexcept;
        };
        const vtable_t *sndr_vtable_;
        sender_storage_type sndr_;
        scheduler_any *scheduler_;

        template <typename Sndr, typename Allocator>
        constexpr sender_any(Sndr &&sndr, Allocator &&alloc, scheduler_any *sch)
            : sndr_vtable_{create_sender_vtable<Sndr, Allocator>()},
              sndr_{std::forward<Sndr>(sndr), std::forward<Allocator>(alloc)},
              scheduler_{sch}
        {
        }

        struct env
        {
            const sender_any *sndr_;

            [[nodiscard]] constexpr auto query( // NOLINT
                get_completion_scheduler_t<set_value_t> /*unused*/) const noexcept
            {
                return *(sndr_->scheduler_);
            }
        };

        // NOLINTNEXTLINE
        [[nodiscard]] constexpr auto get_env() const noexcept -> env
        {
            return {this};
        }

        template <typename Env>
        struct operation
        {
            Env env;
            operation_any op;
            operation(Env &&r, operation_any &&op) noexcept
                : env(std::move(r)), op{std::move(op)}
            {
            }
            constexpr void start() & noexcept
            {
                op.start();
            }

            [[nodiscard]] constexpr auto get_env() const noexcept
            {
                return env;
            }
        };

        template <typename R>
        constexpr auto connect(R &&r) noexcept
        {
            std::cout << "sender_any connect() called\n";
            auto env = r.get_env();
            receiver_any recv_any{std::move(r)};
            return operation{std::move(env), sndr_vtable_->connect(&sndr_, &recv_any)};
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
                                                    void *recv) noexcept
        {
            Sndr *sndr = sender_storage_type::get_pointer<Sndr>(sndr_storage);
            auto *alloc = sender_storage_type::get_allocator<Allocator>(sndr_storage);
            auto *r = static_cast<receiver_any *>(recv);

            using concrete = concrete_operation<Sndr, receiver_any>;
            return operation_any(concrete{sndr, std::move(*r)}, Allocator{*alloc});
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
        sender_any (*schedule)(void *sched_storage, scheduler_any *sch) noexcept;
    };

    constexpr sender_any schedule() noexcept
    {
        std::cout << "scheduler_any schedule() called\n";
        return vtable_->schedule(&sch_, this);
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
    constexpr static sender_any schedule_impl(void *sched_storage,
                                              scheduler_any *sch) noexcept
    {
        auto *sched = scheduler_storage_type::get_pointer<Sch>(sched_storage);
        auto *alloc = scheduler_storage_type::get_allocator<Allocator>(sched_storage);
        return sender_any(sched->schedule(), Allocator{*alloc}, sch);
    }

    const vtable_t *vtable_;
    scheduler_storage_type sch_;
};

// 测试用的具体 receiver
struct my_receiver
{
    void set_value()
    {
        std::cout << ".....my_receiver set_value()\n";
    }
    void set_error(std::error_code)
    {
        std::cout << "my_receiver set_error()\n";
    }
    void set_stopped()
    {
        std::cout << "my_receiver set_stopped()\n";
    }

    [[nodiscard]] constexpr auto get_env() const noexcept
    {
        struct empty_env
        {
        };
        return empty_env{};
    }
};

// 测试函数
void test_any_storage_memory_safety();

// 在 main 函数中调用测试
int main()
{
    std::cout << "=== 开始测试 scheduler_any ===\n";

    scheduler_any any{scheduler{}};
    auto sndr = any.schedule();

    my_receiver recv;
    auto op = sndr.connect(recv);

    std::cout << "=== 开始执行 operation ===\n";
    op.start();

    static_assert(std::is_same_v<decltype(recv.get_env()), decltype(op.get_env())>);

    std::cout << "=== 基本功能测试完成 ===\n\n";

    // 运行内存安全测试
    test_any_storage_memory_safety();

    return 0;
}

#include <vector>
#include <memory>
#include <cassert>
#include <array>

// 测试用的小对象（适合栈存储）
struct SmallObject
{
    int value;
    std::vector<int> data; // 但实际数据在堆上

    SmallObject(int v = 0) : value(v), data{1, 2, 3, 4, 5}
    {
        std::cout << "SmallObject constructed: " << value << "\n";
    }

    ~SmallObject()
    {
        std::cout << "SmallObject destroyed: " << value << "\n";
    }

    SmallObject(const SmallObject &other) : value(other.value), data(other.data)
    {
        std::cout << "SmallObject copied: " << value << "\n";
    }

    SmallObject(SmallObject &&other) noexcept
        : value(other.value), data(std::move(other.data))
    {
        std::cout << "SmallObject moved: " << value << "\n";
        other.value = -1;
    }

    SmallObject &operator=(const SmallObject &other)
    {
        value = other.value;
        data = other.data;
        std::cout << "SmallObject copy assigned: " << value << "\n";
        return *this;
    }

    SmallObject &operator=(SmallObject &&other) noexcept
    {
        value = other.value;
        data = std::move(other.data);
        other.value = -1;
        std::cout << "SmallObject move assigned: " << value << "\n";
        return *this;
    }
};

// 测试用的大对象（需要堆存储）
struct LargeObject
{
    std::array<char, 1024> buffer{}; // 大缓冲区
    int id;
    std::unique_ptr<int> unique_data;

    LargeObject(int i = 0) : id(i), unique_data(std::make_unique<int>(i))
    {
        std::fill(buffer.begin(), buffer.end(), 'A' + (i % 26));
        std::cout << "LargeObject constructed: " << id << "\n";
    }

    ~LargeObject()
    {
        std::cout << "LargeObject destroyed: " << id << "\n";
    }

    LargeObject(const LargeObject &other)
        : buffer(other.buffer), id(other.id),
          unique_data(other.unique_data ? std::make_unique<int>(*other.unique_data)
                                        : nullptr)
    {
        std::cout << "LargeObject copied: " << id << "\n";
    }

    LargeObject(LargeObject &&other) noexcept
        : buffer(std::move(other.buffer)), id(other.id),
          unique_data(std::move(other.unique_data))
    {
        std::cout << "LargeObject moved: " << id << "\n";
        other.id = -1;
    }

    LargeObject &operator=(const LargeObject &other)
    {
        buffer = other.buffer;
        id = other.id;
        unique_data =
            other.unique_data ? std::make_unique<int>(*other.unique_data) : nullptr;
        std::cout << "LargeObject copy assigned: " << id << "\n";
        return *this;
    }

    LargeObject &operator=(LargeObject &&other) noexcept
    {
        buffer = std::move(other.buffer);
        id = other.id;
        unique_data = std::move(other.unique_data);
        other.id = -1;
        std::cout << "LargeObject move assigned: " << id << "\n";
        return *this;
    }
};

// 内存泄漏检测辅助类
struct MemoryTracker
{
    static inline int constructions = 0;
    static inline int destructions = 0;

    int id;

    MemoryTracker(int i = 0) : id(i)
    {
        ++constructions;
        std::cout << "MemoryTracker constructed: " << id << " (total: " << constructions
                  << ")\n";
    }

    ~MemoryTracker()
    {
        ++destructions;
        std::cout << "MemoryTracker destroyed: " << id << " (total: " << destructions
                  << ")\n";
    }

    MemoryTracker(const MemoryTracker &other) : id(other.id)
    {
        ++constructions;
        std::cout << "MemoryTracker copied: " << id << " (total: " << constructions
                  << ")\n";
    }

    MemoryTracker(MemoryTracker &&other) noexcept : id(other.id)
    {
        ++constructions;
        other.id = -1;
        std::cout << "MemoryTracker moved: " << id << " (total: " << constructions
                  << ")\n";
    }

    MemoryTracker &operator=(const MemoryTracker &other)
    {
        id = other.id;
        std::cout << "MemoryTracker copy assigned: " << id << "\n";
        return *this;
    }

    MemoryTracker &operator=(MemoryTracker &&other) noexcept
    {
        id = other.id;
        other.id = -1;
        std::cout << "MemoryTracker move assigned: " << id << "\n";
        return *this;
    }

    static void reset()
    {
        constructions = 0;
        destructions = 0;
    }

    static bool check_leaks()
    {
        bool leak = constructions != destructions;
        if (leak)
        {
            std::cout << "⚠️ MEMORY LEAK DETECTED! Constructions: " << constructions
                      << ", Destructions: " << destructions << "\n";
        }
        else
        {
            std::cout << "✅ No memory leaks detected\n";
        }
        return leak;
    }
};
void test_any_storage_memory_safety()
{
    std::cout << "\n=== 开始 any_storage 内存安全测试 ===\n\n";

    // 测试1: 纯栈对象测试
    std::cout << "--- 测试1: 纯栈对象测试 ---\n";
    {
        MemoryTracker::reset();
        any_storage<sizeof(MemoryTracker), alignof(MemoryTracker), "Test1"> storage1{
            MemoryTracker(1), std::allocator<std::byte>{}};

        any_storage<sizeof(MemoryTracker), alignof(MemoryTracker), "Test1"> storage2 =
            storage1; // 拷贝构造
        any_storage<sizeof(MemoryTracker), alignof(MemoryTracker), "Test1"> storage3 =
            std::move(storage1); // 移动构造

        storage2 = storage3;            // 拷贝赋值
        storage3 = std::move(storage2); // 移动赋值

        std::cout << "离开作用域，应该自动销毁所有对象...\n";
    }
    bool leak1 = MemoryTracker::check_leaks();
    assert(!leak1 && "测试1: 检测到内存泄漏!");

    // 测试2: 纯堆对象测试（如果支持的话）
    std::cout << "\n--- 测试2: 大对象测试 ---\n";
    {
        // 使用足够大的缓冲区来存储 LargeObject
        any_storage<2000, alignof(LargeObject), "Test2"> storage{
            LargeObject(100), std::allocator<std::byte>{}};

        std::cout << "离开作用域，大对象应该被正确销毁...\n";
    }

    // 测试3: 混合类型测试
    std::cout << "\n--- 测试3: 混合类型测试 ---\n";
    {
        MemoryTracker::reset();

        // 创建多个不同类型的存储
        any_storage<sizeof(SmallObject), alignof(SmallObject), "Test3"> small_storage{
            SmallObject(10), std::allocator<std::byte>{}};

        any_storage<sizeof(MemoryTracker), alignof(MemoryTracker), "Test3">
            tracker_storage{MemoryTracker(20), std::allocator<std::byte>{}};

        // 在容器中存储 any_storage
        std::vector<any_storage<64, 8, "Test3">> storages;
        storages.emplace_back(SmallObject(30), std::allocator<std::byte>{});
        storages.emplace_back(MemoryTracker(40), std::allocator<std::byte>{});

        std::cout << "离开作用域，所有对象应该被正确销毁...\n";
    }
    bool leak3 = MemoryTracker::check_leaks();
    assert(!leak3 && "测试3: 检测到内存泄漏!");

    // 测试4: 异常安全测试
    std::cout << "\n--- 测试4: 异常安全测试 ---\n";
    try
    {
        MemoryTracker::reset();
        any_storage<sizeof(MemoryTracker), alignof(MemoryTracker), "Test4"> storage{
            MemoryTracker(50), std::allocator<std::byte>{}};

        // 模拟异常情况
        throw std::runtime_error("测试异常");
    }
    catch (const std::exception &e)
    {
        std::cout << "捕获异常: " << e.what() << "\n";
        std::cout << "异常情况下对象应该被正确清理...\n";
    }
    bool leak4 = MemoryTracker::check_leaks();
    assert(!leak4 && "测试4: 异常情况下检测到内存泄漏!");

    // 测试5: 多次拷贝移动测试
    std::cout << "\n--- 测试5: 多次拷贝移动测试 ---\n";
    {
        MemoryTracker::reset();

        auto create_and_move = []() {
            any_storage<sizeof(MemoryTracker), alignof(MemoryTracker), "Test5"> original{
                MemoryTracker(60), std::allocator<std::byte>{}};

            // 多次移动
            auto moved1 = std::move(original);
            auto moved2 = std::move(moved1);
            auto moved3 = std::move(moved2);

            return moved3;
        };

        auto storage = create_and_move(); // 返回值优化 + 移动

        // 多次拷贝
        auto copy1 = storage;
        auto copy2 = copy1;
        auto copy3 = copy2;

        std::cout << "离开作用域，所有副本应该被正确销毁...\n";
    }
    bool leak5 = MemoryTracker::check_leaks();
    assert(!leak5 && "测试5: 检测到内存泄漏!");

    // 测试6: 自赋值测试
    std::cout << "\n--- 测试6: 自赋值测试 ---\n";
    {
        MemoryTracker::reset();
        [[maybe_unused]] any_storage<sizeof(MemoryTracker), alignof(MemoryTracker),
                                     "Test6"> storage{MemoryTracker(70),
                                                      std::allocator<std::byte>{}};

        // 自赋值
        storage = storage;

        // 自移动（技术上不应该这样做，但测试健壮性）
        storage = std::move(storage);

        std::cout << "离开作用域，自赋值后对象应该仍然有效...\n";
    }
    bool leak6 = MemoryTracker::check_leaks();
    assert(!leak6 && "测试6: 检测到内存泄漏!");

    std::cout << "\n=== any_storage 内存安全测试完成 ===\n";
    std::cout << "✅ 所有内存安全测试通过！\n\n";
}
// NOLINTEND
#include <algorithm>
#include <bit>
#include <cstddef>
#include <exception>
#include <iostream>
#include <memory>
#include <type_traits>
#include <utility>

#include <cassert>
#include <cstring>

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
        constexpr operation() = default;
        constexpr operation(const operation &) = default;
        constexpr operation(operation &&) = default;
        constexpr operation &operator=(const operation &) = default;
        constexpr operation &operator=(operation &&) = default;
        constexpr ~operation() noexcept = default;

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
    char data[N]{};

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
            std::cout << "✅ Allocator constructed on stack\n";
            auto *ptr = as_small<T>();
            std::allocator_traits<ReboundAlloc>::construct(
                rebound_alloc, ptr, std::forward<Allocator>(alloc));
        }
        else
        {
            std::cout << "🔄 Allocator constructed on heap\n";
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
        void (*destroy)(any_storage &self) noexcept;
        void (*copy_construct)(any_storage &dest, const any_storage &src);
        void (*move_construct)(any_storage &dest, any_storage &src) noexcept;
        bool (*equals)(const any_storage &a, const any_storage &b) noexcept;
        const std::type_info &(*type_info_T)() noexcept;
        const std::type_info &(*type_info_Allocator)() noexcept;
    };

    allocator_storage_type allocator_;
    union storage_union {
        alignas(align) std::byte stack_buffer[size];
        void *heap_ptr;
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
    static constexpr const T *get_pointer(const void *storage) noexcept
    {
        if constexpr (is_small<T>())
            return std::bit_cast<const any_storage *>(storage)->template as_small<T>();
        else
            return std::bit_cast<const any_storage *>(storage)->template as_large<T>();
    }

    template <typename T>
    static constexpr T *get_allocator(void *storage) noexcept
    {
        return allocator_storage_type::get_pointer<T>(
            &std::bit_cast<any_storage *>(storage)->allocator_);
    }

    template <typename T, typename Allocator>
    static void destroy_impl(any_storage &self) noexcept
    {
        using ReboundAlloc =
            typename std::allocator_traits<Allocator>::template rebind_alloc<T>;
        ReboundAlloc rebound_alloc(self.allocator_.template get_allocator<Allocator>());

        if constexpr (is_small<T>())
        {
            std::cout << "🗑️ " << id.c_str() << " destroy stack object\n";
            auto *ptr = self.template as_small<T>();
            std::allocator_traits<ReboundAlloc>::destroy(rebound_alloc, ptr);
        }
        else
        {
            std::cout << "🗑️ " << id.c_str() << " destroy heap object\n";
            auto *ptr = self.template as_large<T>();
            if (ptr)
            {
                std::allocator_traits<ReboundAlloc>::destroy(rebound_alloc, ptr);
                std::allocator_traits<ReboundAlloc>::deallocate(rebound_alloc, ptr, 1);
                self.storage_.heap_ptr = nullptr;
            }
        }
        self.ops_ = nullptr;
    }

    template <typename T, typename Allocator>
    static void copy_construct_impl(any_storage &dest, const any_storage &src)
    {
        using ReboundAlloc =
            typename std::allocator_traits<Allocator>::template rebind_alloc<T>;
        ReboundAlloc rebound_alloc(dest.allocator_.template get_allocator<Allocator>());

        const T *src_obj = nullptr;
        if constexpr (is_small<T>())
        {
            src_obj = src.template as_small<T>();
        }
        else
        {
            src_obj = src.template as_large<T>();
        }

        if constexpr (is_small<T>())
        {
            std::cout << "📋 " << id.c_str() << " copy construct to stack\n";
            auto *ptr = dest.template as_small<T>();
            std::allocator_traits<ReboundAlloc>::construct(rebound_alloc, ptr, *src_obj);
        }
        else
        {
            std::cout << "📋 " << id.c_str() << " copy construct to heap\n";
            T *ptr = std::allocator_traits<ReboundAlloc>::allocate(rebound_alloc, 1);
            try
            {
                std::allocator_traits<ReboundAlloc>::construct(rebound_alloc, ptr,
                                                               *src_obj);
                dest.storage_.heap_ptr = ptr;
            }
            catch (...)
            {
                std::allocator_traits<ReboundAlloc>::deallocate(rebound_alloc, ptr, 1);
                throw;
            }
        }
    }

    template <typename T, typename Allocator>
    static void move_construct_impl(any_storage &dest, any_storage &src) noexcept
    {
        using ReboundAlloc =
            typename std::allocator_traits<Allocator>::template rebind_alloc<T>;
        ReboundAlloc rebound_alloc(dest.allocator_.template get_allocator<Allocator>());

        if constexpr (is_small<T>())
        {
            std::cout << "🚚 " << id.c_str() << " move construct to stack\n";
            auto *src_ptr = src.template as_small<T>();
            auto *dest_ptr = dest.template as_small<T>();
            std::allocator_traits<ReboundAlloc>::construct(rebound_alloc, dest_ptr,
                                                           std::move(*src_ptr));
        }
        else
        {
            std::cout << "🚚 " << id.c_str() << " move construct to heap\n";
            dest.storage_.heap_ptr = std::exchange(src.storage_.heap_ptr, nullptr);
        }
    }

    template <typename T>
    static const std::type_info &type_info_T_impl() noexcept
    {
        return typeid(T);
    }

    template <typename Allocator>
    static const std::type_info &type_info_Allocator_impl() noexcept
    {
        return typeid(Allocator);
    }

    friend bool operator==(const any_storage &a, const any_storage &b) noexcept
    {
        if ((a.ops_ == nullptr) && (b.ops_ == nullptr))
            return true;
        if ((a.ops_ == nullptr) || (b.ops_ == nullptr))
            return false;
        if (a.ops_->type_info_T() != b.ops_->type_info_T() ||
            a.ops_->type_info_Allocator() != b.ops_->type_info_Allocator())
            return false;
        return a.ops_->equals(a, b);
    }

    friend bool operator!=(const any_storage &a, const any_storage &b) noexcept
    {
        return !(a == b);
    }

    [[nodiscard]] constexpr const std::type_info &stored_type() const noexcept
    {
        return ops_ == nullptr ? typeid(void) : ops_->type_info_T();
    }

    [[nodiscard]] constexpr const std::type_info &allocator_type() const noexcept
    {
        return ops_ == nullptr ? typeid(void) : ops_->type_info_Allocator();
    }

    template <typename T>
    constexpr static bool equals_impl(const any_storage &a, const any_storage &b) noexcept
    {
        if constexpr (std::is_empty_v<T>)
            return true;
        else
        {
            const T *obj_a = nullptr;
            const T *obj_b = nullptr;

            if constexpr (is_small<T>())
            {
                obj_a = a.template as_small<T>();
                obj_b = b.template as_small<T>();
            }
            else
            {
                obj_a = a.template as_large<T>();
                obj_b = b.template as_large<T>();
            }
            if (!obj_a || !obj_b)
                return false;

            if constexpr (requires { *obj_a == *obj_b; })
            {
                return *obj_a == *obj_b;
            }
            else if constexpr (std::is_trivially_copyable_v<T> && is_small<T>())
            {
                return std::memcmp(obj_a, obj_b, sizeof(T)) == 0;
            }
            else
            {
                return obj_a == obj_b;
            }
        }
    }

    template <typename T, typename Allocator>
    static constexpr const storage_ops *create_ops() noexcept
    {
        static const auto vt =
            storage_ops{.destroy = &destroy_impl<T, Allocator>,
                        .copy_construct = &copy_construct_impl<T, Allocator>,
                        .move_construct = &move_construct_impl<T, Allocator>,
                        .equals = &equals_impl<T>,
                        .type_info_T = &type_info_T_impl<T>,
                        .type_info_Allocator = &type_info_Allocator_impl<Allocator>};
        return &vt;
    }

    template <typename Obj, typename Allocator>
    constexpr void construct(Obj &&obj, Allocator &&src_alloc)
    {
        using T = std::decay_t<Obj>;
        using ReboundAlloc = typename std::allocator_traits<
            std::decay_t<Allocator>>::template rebind_alloc<T>;
        ReboundAlloc rebound_alloc(src_alloc);

        if constexpr (is_small<T>())
        {
            std::cout << "✅ " << id.c_str() << " construct on stack\n";
            auto *ptr = as_small<T>();
            std::allocator_traits<ReboundAlloc>::construct(rebound_alloc, ptr,
                                                           std::forward<Obj>(obj));
        }
        else
        {
            std::cout << "🔄 " << id.c_str() << " construct on heap\n";
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
        : allocator_{alloc}, storage_{},
          ops_{create_ops<std::decay_t<Obj>, std::decay_t<Allocator>>()}
    {
        construct(std::forward<Obj>(obj), std::forward<Allocator>(alloc));
    }

    constexpr any_storage(const any_storage &other)
        : allocator_{other.allocator_}, ops_(other.ops_)
    {
        if (ops_)
            ops_->copy_construct(*this, other);
    }

    constexpr any_storage(any_storage &&other) noexcept
        : allocator_{std::move(other.allocator_)},
          ops_(std::exchange(other.ops_, nullptr))
    {
        if (ops_)
        {
            ops_->move_construct(*this, other);
            ops_->destroy(other);
        }
    }

    constexpr any_storage &operator=(const any_storage &other)
    {
        if (this != &other)
        {
            if (ops_)
                ops_->destroy(*this);
            allocator_ = other.allocator_;
            ops_ = other.ops_;
            if (ops_)
                ops_->copy_construct(*this, other);
        }
        return *this;
    }

    constexpr any_storage &operator=(any_storage &&other) noexcept
    {
        if (this != &other)
        {
            if (ops_)
                ops_->destroy(*this);
            allocator_ = std::move(other.allocator_);
            ops_ = std::exchange(other.ops_, nullptr);
            if (ops_)
            {
                ops_->move_construct(*this, other);
                ops_->destroy(other);
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
            ops_->destroy(*this);
    }
};

template <std::size_t size = 1 * sizeof(void *), size_t align = alignof(std::max_align_t)>
struct scheduler_storage : any_storage<size, align, "Sch">
{
    using any_storage<size, align, "Sch">::any_storage;
};

template <std::size_t size = 1 * sizeof(void *), size_t align = alignof(std::max_align_t)>
struct sender_storage : any_storage<size, align, "Sndr">
{
    using any_storage<size, align, "Sndr">::any_storage;
};

template <std::size_t size = 8 * sizeof(void *), size_t align = alignof(std::max_align_t)>
struct operation_storage : any_storage<size, align, "Oper">
{
    using any_storage<size, align, "Oper">::any_storage;
};

template <std::size_t size = 1 * sizeof(void *), size_t align = alignof(std::max_align_t)>
struct receiver_storage : any_storage<size, align, "Recv">
{
    using any_storage<size, align, "Recv">::any_storage;
};

struct receiver_any
{
    struct vtable_t
    {
        void (*set_value)(void *recv_storage) noexcept;
        void (*set_error_ec)(void *recv_storage, std::error_code &&) noexcept;
        void (*set_error_ptr)(void *recv_storage, std::exception_ptr &&) noexcept;
        void (*set_stopped)(void *recv_storage) noexcept;
    };

    const vtable_t *vtable_;
    receiver_storage<> recv_storage_;

    template <typename R, typename Allocator = std::allocator<std::byte>>
    constexpr explicit receiver_any(R &&recv, Allocator alloc = Allocator{})
        : vtable_{create_vtable<std::decay_t<R>>()},
          recv_storage_{std::forward<R>(recv), std::forward<Allocator>(alloc)}
    {
    }

    constexpr void set_value() && noexcept
    {
        std::cout << "receiver_any set_value()\n";
        vtable_->set_value(&recv_storage_);
    }

    constexpr void set_error(std::error_code &&ec) && noexcept
    {
        std::cout << "receiver_any set_error()\n";
        vtable_->set_error_ec(&recv_storage_, std::move(ec));
    }

    constexpr void set_error(std::exception_ptr &&e) && noexcept
    {
        std::cout << "receiver_any set_error()\n";
        vtable_->set_error_ptr(&recv_storage_, std::move(e));
    }

    constexpr void set_stopped() && noexcept
    {
        std::cout << "receiver_any set_stopped()\n";
        vtable_->set_stopped(&recv_storage_);
    }

  private:
    template <typename R>
    constexpr static const vtable_t *create_vtable() noexcept
    {
        const static vtable_t vt = {.set_value = &set_value_impl<R>,
                                    .set_error_ec = &set_error_impl<R, std::error_code>,
                                    .set_error_ptr =
                                        &set_error_impl<R, std::exception_ptr>,
                                    .set_stopped = &set_stopped_impl<R>};
        return &vt;
    }

    template <typename R>
    constexpr static void set_value_impl(void *recv_storage) noexcept
    {
        R *recv = receiver_storage<>::get_pointer<R>(recv_storage);
        recv->set_value();
    }

    template <typename R, typename E>
    constexpr static void set_error_impl(void *recv_storage, E &&e) noexcept
    {
        R *recv = receiver_storage<>::get_pointer<R>(recv_storage);
        recv->set_error(std::move(e));
    }

    template <typename R>
    constexpr static void set_stopped_impl(void *recv_storage) noexcept
    {
        R *recv = receiver_storage<>::get_pointer<R>(recv_storage);
        recv->set_stopped();
    }
};

struct task_scheduler
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

    struct sender
    {
        struct vtable_t
        {
            operation_any (*connect)(void *sndr_storage, void *recv) noexcept;
        };

        const vtable_t *sndr_vtable_;
        sender_storage_type sndr_;
        task_scheduler *scheduler_;

        template <typename Sndr, typename Allocator>
        constexpr sender(Sndr &&sndr, Allocator &&alloc, task_scheduler *sch)
            : sndr_vtable_{create_sender_vtable<std::decay_t<Sndr>,
                                                std::decay_t<Allocator>>()},
              sndr_{std::forward<Sndr>(sndr), std::forward<Allocator>(alloc)},
              scheduler_{sch}
        {
        }

        struct env
        {
            const sender *sndr_;

            [[nodiscard]] constexpr auto query(
                get_completion_scheduler_t<set_value_t>) const noexcept
            {
                return *(sndr_->scheduler_);
            }
        };

        [[nodiscard]] constexpr auto get_env() const noexcept -> env
        {
            return {this};
        }

        template <typename Env>
        struct operation
        {
            constexpr operation() = delete;
            constexpr operation(const operation &) = delete;
            constexpr operation(operation &&) = delete;
            constexpr operation &operator=(const operation &) = delete;
            constexpr operation &operator=(operation &&) = delete;
            constexpr ~operation() noexcept = default;

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
            receiver_any recv_any{std::forward<R>(r)};
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

            std::cout << ">>>> operation size: "
                      << sizeof(decltype(sndr->connect(std::move(*r)))) << " bytes\n";
            std::cout << ">>>> operation_any size: "
                      << sizeof(decltype(operation_any(sndr->connect(std::move(*r)),
                                                       *alloc)))
                      << " bytes\n";
            return {sndr->connect(std::move(*r)), *alloc};
        }
    };

    struct vtable_t
    {
        sender (*schedule)(void *sched_storage, task_scheduler *sch);
    };

    constexpr sender schedule() noexcept
    {
        std::cout << "task_scheduler schedule() called\n";
        return vtable_->schedule(&sch_, this);
    }

    template <class Sch, typename Allocator = std::allocator<std::byte>>
    constexpr explicit task_scheduler(Sch &&sch, Allocator alloc = Allocator{})
        : vtable_(create_vtable<std::decay_t<Sch>, std::decay_t<Allocator>>()),
          sch_(std::forward<Sch>(sch), std::forward<Allocator>(alloc))
    {
    }

    friend bool operator==(const task_scheduler &lhs, const task_scheduler &rhs) noexcept
    {
        if (lhs.vtable_ != rhs.vtable_)
            return false;
        return lhs.sch_ == rhs.sch_;
    }

    template <class Sch>
        requires(!std::same_as<sender, Sch>)
    friend bool operator==(const task_scheduler &lhs, const Sch &rhs) noexcept
    {
        using StoredType = std::decay_t<Sch>;

        if (lhs.sch_.stored_type() == typeid(void) ||
            lhs.sch_.stored_type() != typeid(StoredType))
        {
            return false;
        }

        const auto *stored = scheduler_storage_type::get_pointer<StoredType>(&lhs.sch_);
        static_assert(
            requires { *stored == rhs; },
            "Type must implement operator== for comparison with task_scheduler");
        return *stored == rhs;
    }

  private:
    template <class Sch, typename Allocator>
    constexpr static const vtable_t *create_vtable()
    {
        static const vtable_t vt = {.schedule = &schedule_impl<Sch, Allocator>};
        return &vt;
    }

    template <class Sch, typename Allocator>
    constexpr static sender schedule_impl(void *sched_storage, task_scheduler *sch)
    {
        auto *sched = scheduler_storage_type::get_pointer<Sch>(sched_storage);
        auto *alloc = scheduler_storage_type::get_allocator<Allocator>(sched_storage);
        return {sched->schedule(), *alloc, sch};
    }

    const vtable_t *vtable_;
    scheduler_storage_type sch_;
};

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
    void set_error(std::exception_ptr)
    {
        std::cout << "my_receiver set_error(exception_ptr)\n";
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

struct my_scheduler : scheduler
{
    int v = 0;

    friend bool operator==(const my_scheduler &lhs, const my_scheduler &rhs) noexcept
    {
        return lhs.v == rhs.v;
    }

    friend bool operator!=(const my_scheduler &lhs, const my_scheduler &rhs) noexcept
    {
        return !(lhs == rhs);
    }
};

struct my_scheduler2 : scheduler
{
    int *v = nullptr;

    friend bool operator==(const my_scheduler2 &lhs, const my_scheduler2 &rhs) noexcept
    {
        return lhs.v == rhs.v;
    }

    friend bool operator!=(const my_scheduler2 &lhs, const my_scheduler2 &rhs) noexcept
    {
        return !(lhs == rhs);
    }
};

int main()
{
    std::cout << "=== Testing task_scheduler ===\n";

    task_scheduler any{scheduler{}};
    auto sndr = any.schedule();

    my_receiver recv;
    auto op = sndr.connect(recv);

    std::cout << "=== Executing operation ===\n";
    op.start();

    static_assert(std::is_same_v<decltype(recv.get_env()), decltype(op.get_env())>);

    std::cout << "=== Basic functionality test completed ===\n\n";

    {
        assert(typeid(int) == typeid(int &));
        assert(typeid(my_scheduler) == typeid(my_scheduler &));
        assert(typeid(my_scheduler) == typeid(const my_scheduler &));
        assert(typeid(my_scheduler) != typeid(my_scheduler *));

        task_scheduler any2{scheduler{}};
        assert(any == any2);
        assert(any2 == any2);

        task_scheduler any3{my_scheduler{.v = 1}};
        task_scheduler any4{my_scheduler{}};
        assert(any2 != any3);
        assert(any4 != any3);

        assert(any3 == my_scheduler{.v = 1});
        assert(any3 != my_scheduler{.v = 2});

        int a = 0;
        int b = 0;
        task_scheduler any5{my_scheduler2{.v = &a}};
        assert(any5 == task_scheduler{my_scheduler2{.v = &a}});
        assert(any5 != task_scheduler{my_scheduler2{.v = &b}});
        assert(any5 != any3);
    }

    return 0;
}

// NOLINTEND
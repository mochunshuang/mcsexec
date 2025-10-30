#pragma once

#include "./__allocator_storage.hpp"
#include <utility>
#include <cstring>

namespace mcs::execution::task_v2::__detail
{
    // NOLINTBEGIN
    template <std::size_t size, std::size_t align>
        requires(size > 0)
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
        static constexpr T *get_pointer(const void *storage) noexcept
        {
            using S = const any_storage *;
            if constexpr (is_small<T>())
                return std::bit_cast<S>(storage)->template as_small<T>();
            else
                return std::bit_cast<S>(storage)->template as_large<T>();
        }

        template <typename T>
        static constexpr T *get_allocator(void *storage) noexcept
        {
            return allocator_storage_type::get_pointer<T>(
                &std::bit_cast<any_storage *>(storage)->allocator_);
        }

        template <typename T, typename Allocator>
        static constexpr void destroy_impl(any_storage &self) noexcept
        {
            using ReboundAlloc =
                typename std::allocator_traits<Allocator>::template rebind_alloc<T>;
            ReboundAlloc rebound_alloc(
                self.allocator_.template get_allocator<Allocator>());
            using allocator_traits = std::allocator_traits<ReboundAlloc>;

            if constexpr (is_small<T>())
            {
                auto *ptr = self.template as_small<T>();
                allocator_traits::destroy(rebound_alloc, ptr);
            }
            else
            {
                auto *ptr = self.template as_large<T>();
                if (ptr)
                {
                    allocator_traits::destroy(rebound_alloc, ptr);
                    allocator_traits::deallocate(rebound_alloc, ptr, 1);
                    self.storage_.heap_ptr = nullptr;
                }
            }
            self.ops_ = nullptr;
        }

        template <typename T, typename Allocator>
        static constexpr void copy_construct_impl(any_storage &dest,
                                                  const any_storage &src)
        {
            using ReboundAlloc =
                typename std::allocator_traits<Allocator>::template rebind_alloc<T>;
            ReboundAlloc rebound_alloc(
                dest.allocator_.template get_allocator<Allocator>());
            using allocator_traits = std::allocator_traits<ReboundAlloc>;

            const T *src_obj = nullptr;
            if constexpr (is_small<T>())
                src_obj = src.template as_small<T>();
            else
                src_obj = src.template as_large<T>();

            if constexpr (is_small<T>())
            {
                auto *ptr = dest.template as_small<T>();
                allocator_traits::construct(rebound_alloc, ptr, *src_obj);
            }
            else
            {
                T *ptr = allocator_traits::allocate(rebound_alloc, 1);
                try
                {
                    allocator_traits::construct(rebound_alloc, ptr, *src_obj);
                    dest.storage_.heap_ptr = ptr;
                }
                catch (...)
                {
                    allocator_traits::deallocate(rebound_alloc, ptr, 1);
                    throw;
                }
            }
        }

        template <typename T, typename Allocator>
        static constexpr void move_construct_impl(any_storage &dest,
                                                  any_storage &src) noexcept
        {
            using ReboundAlloc =
                typename std::allocator_traits<Allocator>::template rebind_alloc<T>;
            ReboundAlloc rebound_alloc(
                dest.allocator_.template get_allocator<Allocator>());
            using allocator_traits = std::allocator_traits<ReboundAlloc>;

            if constexpr (is_small<T>())
            {
                auto *src_ptr = src.template as_small<T>();
                auto *dest_ptr = dest.template as_small<T>();
                allocator_traits::construct(rebound_alloc, dest_ptr, std::move(*src_ptr));
            }
            else
            {
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
        friend constexpr bool operator!=(const any_storage &a,
                                         const any_storage &b) noexcept
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
        constexpr static bool equals_impl(const any_storage &a,
                                          const any_storage &b) noexcept
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
                    return *obj_a == *obj_b;
                else if constexpr (std::is_trivially_copyable_v<T> && is_small<T>())
                    return std::memcmp(obj_a, obj_b, sizeof(T)) == 0;
                else
                    return obj_a == obj_b;
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
            using allocator_traits = std::allocator_traits<ReboundAlloc>;

            if constexpr (is_small<T>())
            {
                auto *ptr = as_small<T>();
                allocator_traits::construct(rebound_alloc, ptr, std::forward<Obj>(obj));
            }
            else
            {
                T *ptr = allocator_traits::allocate(rebound_alloc, 1);
                try
                {
                    allocator_traits::construct(rebound_alloc, ptr,
                                                std::forward<Obj>(obj));
                    storage_.heap_ptr = ptr;
                }
                catch (...)
                {
                    allocator_traits::deallocate(rebound_alloc, ptr, 1);
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
            {
                ops_->copy_construct(*this, other);
            }
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
    // NOLINTEND
}; // namespace mcs::execution::task_v2::__detail
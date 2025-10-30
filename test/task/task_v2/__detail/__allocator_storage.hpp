#pragma once

#include <bit>
#include <cstddef>
#include <memory>

namespace mcs::execution::task_v2::__detail
{

    template <std::size_t size = 1 * sizeof(void *),
              std::size_t align = alignof(std::max_align_t)>
    struct allocator_storage // NOLINTBEGIN
    {
        static constexpr auto buffer_size = size;
        static constexpr auto align_size = align;

        union storage_union {
            alignas(align) std::byte stack_buffer[size];
            void *heap_ptr;

            constexpr storage_union() noexcept : stack_buffer{} {}
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
        explicit constexpr allocator_storage(Allocator &&alloc) noexcept
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
            using allocator_traits = std::allocator_traits<ReboundAlloc>;

            if constexpr (is_small<T>())
            {
                auto *ptr = as_small<T>();
                allocator_traits::construct(rebound_alloc, ptr,
                                            std::forward<Allocator>(alloc));
            }
            else
            {
                static_assert(false, "not supported");
            }
        }
    }; // NOLINTEND

}; // namespace mcs::execution::task_v2::__detail
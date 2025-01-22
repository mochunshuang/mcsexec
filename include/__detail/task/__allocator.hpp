#pragma once

#include <memory>
namespace mcs::execution::task
{
    template <typename>
    struct allocator_of
    {
        using type = std::allocator<std::byte>;
    };
    template <typename Context>
        requires requires { typename Context::allocator_type; }
    struct allocator_of<Context>
    {
        using type = typename Context::allocator_type;
    };
    template <typename Context>
    using allocator_of_t = typename allocator_of<Context>::type;

    template <typename Allocator>
    Allocator find_allocator()
    {
        return Allocator();
    }
    template <typename Allocator, typename Alloc, typename... A>
        requires requires(const Alloc &alloc) { Allocator(alloc); }
    Allocator find_allocator(const std::allocator_arg_t & /*unused*/, const Alloc &alloc,
                             const A &.../*unused*/)
    {
        return Allocator(alloc);
    }
    template <typename Allocator, typename A0, typename... A>
    Allocator find_allocator(A0 const & /*unused*/, const A &...a)
    {
        return ::mcs::execution::task::find_allocator<Allocator>(a...);
    }

    template <typename C, typename... A>
    void *coroutine_allocate(std::size_t size, const A &...a)
    {
        using allocator_type = allocator_of_t<C>;
        using traits = std::allocator_traits<allocator_type>;
        allocator_type alloc{find_allocator<allocator_type>(a...)};
        std::byte *ptr{traits::allocate(alloc, size + sizeof(allocator_type))};
        new (ptr + size) allocator_type(alloc);
        return ptr;
    }
    template <typename C>
    void coroutine_deallocate(void *ptr, std::size_t size)
    {
        using allocator_type = allocator_of_t<C>;
        using traits = std::allocator_traits<allocator_type>;
        void *vptr{static_cast<std::byte *>(ptr) + size};
        auto *aptr{static_cast<allocator_type *>(vptr)};
        allocator_type alloc(*aptr);
        aptr->~allocator_type();
        traits::deallocate(alloc, static_cast<std::byte *>(ptr), size);
    }
}; // namespace mcs::execution::task
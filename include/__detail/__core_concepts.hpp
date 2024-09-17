#pragma once
#include <concepts>

namespace mcs::execution
{
    // [exec.general], helper concepts
    template <class T>
    concept movable_value = // exposition only
        std::move_constructible<std::decay_t<T>> &&
        std::constructible_from<std::decay_t<T>, T> &&
        (not std::is_array_v<std::remove_reference_t<T>>);

    template <class From, class To>
    concept decays_to = std::same_as<std::decay_t<From>, To>;

    template <class T>
    concept class_type = decays_to<T, T> && std::is_class_v<T>;

    // [exec.queryable], queryable objects
    template <class T>
    concept queryable = std::destructible<T>;

    // [allocator.requirements.general]
    template <class Alloc>
    concept simple_allocator = requires(Alloc alloc, std::size_t n) {
        { *alloc.allocate(n) } -> std::same_as<typename Alloc::value_type &>;
        { alloc.deallocate(alloc.allocate(n), n) };
    } && std::copy_constructible<Alloc> && std::equality_comparable<Alloc>;

}; // namespace mcs::execution
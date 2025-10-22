#include <array>
#include <cstddef>
#include <iostream>

// NOLINTBEGIN
enum class storage_type
{
    empty,
    stack,
    heap
};



template <std::size_t _max_size>
struct polymorphic_object
{
    static constexpr auto max_size = _max_size; // NOLINT
    std::array<void *, max_size> object;

    struct vtable
    {
    };
};

int main()
{
    std::cout << "main done\n";
    return 0;
}
// NOLINTEND
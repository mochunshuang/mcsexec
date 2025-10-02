#include <array>
#include <cstddef>
#include <iostream>
#include <new>
#include <string>
#include <type_traits>

// NOLINTBEGIN

template <typename T>
using aligned_storage = std::byte[sizeof(T)];

template <class T, std::size_t N>
class static_vector
{
    // Properly aligned uninitialized storage for N T's
    std::aligned_storage_t<sizeof(T), alignof(T)> data[N];
    // https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2021/p1413r3.pdf
    // aligned_storage<T> data[N]; // NOTE: 看论文自定义实现
    // alignas(T) T data[N];
    // std::array<unsigned char, sizeof(T)> data[N];

    std::size_t m_size = 0;

  public:
    // Create an object in aligned storage
    template <typename... Args>
    void emplace_back(Args &&...args)
    {
        if (m_size >= N) // Possible error handling
            throw std::bad_alloc{};

        // Construct value in memory of aligned storage using inplace operator new
        ::new (&data[m_size]) T(std::forward<Args>(args)...);
        ++m_size;
    }

    // Access an object in aligned storage
    const T &operator[](std::size_t pos) const
    {
        // Note: std::launder is needed after the change of object model in P0137R1
        return *std::launder(
            static_cast<const T *>(static_cast<const void *>(&data[pos])));
    }

    // Destroy objects from aligned storage
    ~static_vector()
    {
        for (std::size_t pos = 0; pos < m_size; ++pos)
            // Note: std::launder is needed after the change of object model in P0137R1
            std::destroy_at(
                std::launder(static_cast<T *>(static_cast<void *>(&data[pos]))));
    }
};

int main()
{
    static_vector<std::string, 10> v1;
    v1.emplace_back(5, '*');
    v1.emplace_back(10, '*');
    std::cout << v1[0] << '\n' << v1[1] << '\n';
}
// NOLINTEND
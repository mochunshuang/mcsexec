#include <exception>
#include <utility>
namespace __detail
{
    template <typename T>
    auto decay_copy(T &&value) noexcept(noexcept(std::decay_t<T>(std::forward<T>(value))))
        -> std::decay_t<T>
    {
        return std::forward<T>(value);
    }

    template <typename... Ts>
    concept is_decay_copy_noexcept_v = (noexcept(decay_copy(std::declval<Ts>())) && ...);

}; // namespace __detail

struct none_such
{
};

/**
 * @brief Let copy-fail be exception_ptr if decay-copying any of the child
 * senders' result datums can potentially throw; otherwise, none-such, where
 * none-such is an unspecified empty class type.
 *
 * @tparam Es
 */
template <typename... Es>
using copy_fail = std::conditional_t<__detail::is_decay_copy_noexcept_v<Es...>, none_such,
                                     std::exception_ptr>;

#include <iostream>

int main()
{
    // 示例 1: 不会抛出异常的类型
    using T1 = int;
    using CopyFail1 = copy_fail<T1>;
    static_assert(std::is_same_v<CopyFail1, none_such>, "CopyFail1 should be none_such");

    // 示例 2: 可能抛出异常的类型
    // NOLINTNEXTLINE
    struct ThrowingType
    {
        ThrowingType() = default;
        ThrowingType(const ThrowingType & /*unused*/)
        {
            throw std::runtime_error("Copy failed");
        }
    };
    using T2 = ThrowingType;
    using CopyFail2 = copy_fail<T2>;
    static_assert(std::is_same_v<CopyFail2, std::exception_ptr>,
                  "CopyFail2 should be std::exception_ptr");

    {
        using CopyFail2 = copy_fail<T2, int>;
        static_assert(std::is_same_v<CopyFail2, std::exception_ptr>,
                      "CopyFail2 should be std::exception_ptr");
    }

    std::cout << "All tests passed!" << std::endl;
    return 0;
}
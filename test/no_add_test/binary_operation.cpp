// NOLINTBEGIN

#include <atomic>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <bitset>
#include <limits>

template <typename T>
void print_binary_operation(const std::string &op_name, T a, T b, T result)
{
    std::cout << op_name << " operation:\n";
    std::cout << "a: " << a << " (" << std::bitset<sizeof(T) * 8>(a) << ")\n";
    std::cout << "b: " << b << " (" << std::bitset<sizeof(T) * 8>(b) << ")\n";
    std::cout << "result: " << result << " (" << std::bitset<sizeof(T) * 8>(result)
              << ")\n\n";
}

template <typename T>
void print_unary_operation(const std::string &op_name, T a, T result)
{
    std::cout << op_name << " operation:\n";
    std::cout << "a: " << a << " (" << std::bitset<sizeof(T) * 8>(a) << ")\n";
    std::cout << "result: " << result << " (" << std::bitset<sizeof(T) * 8>(result)
              << ")\n\n";
}

// 模板函数：打印任意内置基本类型（包括枚举）的二进制表示
template <typename T>
void print_binary(const T &value)
{
    // 确保输入类型是内置基本类型或枚举
    static_assert(std::is_fundamental<T>::value || std::is_enum<T>::value,
                  "Input type must be a fundamental type or enum.");

    // 计算类型的位数
    constexpr std::size_t num_bits = sizeof(T) * CHAR_BIT;

    // 使用 std::bitset 将值转换为二进制字符串
    std::bitset<num_bits> bits(value);

    // 打印结果
    std::cout << "Value: " << value << "\n";
    std::cout << "Binary: " << bits << "\n";
    std::cout << "-------------------------\n";
}

int main()
{
    unsigned int a = 0b1100; // 12 in decimal
    unsigned int b = 0b1010; // 10 in decimal

    // Bitwise OR
    unsigned int or_result = a | b;
    print_binary_operation("a | b", a, b, or_result);

    // Bitwise AND
    unsigned int and_result = a & b;
    print_binary_operation("a & b", a, b, and_result);

    // Bitwise NOT
    unsigned int not_result = ~a;
    print_unary_operation("~a", a, not_result);

    // Bitwise XOR
    unsigned int xor_result = a ^ b;
    print_binary_operation("a ^ b", a, b, xor_result);

    // Right Shift
    unsigned int right_shift_result = a >> 1;
    print_unary_operation("a >> 1", a, right_shift_result);

    // Left Shift
    unsigned int left_shift_result = a << 1;
    print_unary_operation("a << 1", a, left_shift_result);

    {
        constexpr auto v = 0b000001000;
        constexpr auto v2 = 0xFF;
        static_assert(v == 8);
        static_assert(v2 == 255);
        static_assert(0x08 == 8);
        static_assert(0xFF == 0b11111111);

        constexpr auto v3 = std::numeric_limits<int8_t>::max();
        static_assert(v3 == 127);
        static_assert(std::numeric_limits<uint8_t>::max() == 0xFF);
    }
    {
        constexpr int8_t a = 127;
        constexpr uint8_t b =
            static_cast<uint8_t>(a); // b的值仍然是127，但类型变为uint8_t
        static_assert(b == 127);

        // NOTE: 垃圾的COUT； int8_t 和 uint8_t 本质上是 char 类型，std::cout
        // 会将其作为字符输出。
        {
            constexpr int8_t a = 128;
            // constexpr uint8_t b = static_cast<uint8_t>(a);
            // static_assert(b == 128);
            int8_t v = 1;
            uint8_t v2 = 65;
            std::cout << "a: " << v << '\n';  // 打印空
            std::cout << "b: " << v2 << '\n'; // A
        }
    }
    // NOTE: 组合状态压缩
    {
        enum State : std::uint8_t
        {
            init,
            start,
            end,
        };
        State s = start;
        print_binary(s);
        int count = 2;
        print_binary(count);
        print_binary(uint64_t{2});

        // 打包
        print_binary(static_cast<State>(s));
        print_binary(static_cast<uint64_t>(count) << 8);
        uint64_t packed = (static_cast<uint64_t>(count) << 8) | static_cast<uint8_t>(s);
        std::cout << "Packed value:\n";
        print_binary(packed);
        // 解包
        State unpacked_s = static_cast<State>(packed & 0xFF);
        int unpacked_count = static_cast<int>(packed >> 8);
        std::cout << "Unpacked values:\n";
        print_binary(static_cast<State>(unpacked_s));
        print_binary(static_cast<uint64_t>(unpacked_count) << 8);
        assert(s == unpacked_s);
        assert(count == unpacked_count);

        {
            std::atomic<uint64_t> state_count{0};
            {
                // 获取 packed 的值
                uint64_t packed = state_count.load(std::memory_order_acquire);
                // 解包
                State init_state = static_cast<State>(packed & 0xFF); // 提取低 8 位
                int init_count = static_cast<int>(packed >> 8);       // 提取高 56 位
                assert(init_state == init);
                assert(init_count == 0);
            }
        }
    }

    return 0;
}

// NOLINTEND
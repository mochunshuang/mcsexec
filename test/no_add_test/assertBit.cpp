#include <bit>
#include <bitset>
#include <cassert>
#include <iostream>
#include <limits>
#include <cstdint>

// 判断索引位是否为1（索引0对应最高有效位）
template <typename T>
bool isBitOne(T v, std::uint8_t index)
{
    constexpr uint8_t msb = std::numeric_limits<T>::digits - 1;
    return (v & (1 << (msb - index))) != 0;
}

// 判断索引位是否为0
template <typename T>
bool isBitZero(T v, uint8_t index)
{
    constexpr uint8_t msb = std::numeric_limits<T>::digits - 1;
    // return !isBitOne(v, index);
    return (v & (1 << (msb - index))) == 0;
}

// 将索引位置1
template <typename T>
T setBitToOne(T v, uint8_t index)
{
    using UnsignedT = std::make_unsigned_t<T>; // 强制转为无符号类型
    constexpr uint8_t msb = std::numeric_limits<UnsignedT>::digits - 1;
    if (index > msb)
    { // 索引越界检查（index是uint8_t，无需检查负数）
        throw std::out_of_range("Index exceeds bit width of type.");
    }
    UnsignedT unsigned_v = static_cast<UnsignedT>(v);
    UnsignedT mask = UnsignedT{1} << (msb - index); // 确保使用无符号类型生成掩码
    return static_cast<T>(unsigned_v | mask);
}

template <typename T>
T setBitToZero(T v, uint8_t index)
{
    using UnsignedT = std::make_unsigned_t<T>;
    constexpr uint8_t msb = std::numeric_limits<UnsignedT>::digits - 1;
    if (index > msb)
    {
        throw std::out_of_range("Index exceeds bit width of type.");
    }
    UnsignedT unsigned_v = static_cast<UnsignedT>(v);
    UnsignedT mask = UnsignedT{1} << (msb - index);
    return static_cast<T>(unsigned_v & ~mask);
}

// BUG
template <typename T>
T setBitToZero2(T v, int index)
{
    constexpr int msb = std::numeric_limits<T>::digits - 1;
    return v ^ (1 << (msb - index));
}

int main()
{
    std::uint8_t v = 0b00100101; // 二进制00100101 (索引0在最高位)

    assert(isBitOne(v, 2));  // 索引2对应位5（值1）
    assert(isBitZero(v, 3)); // 索引3对应位4（值0）

    assert(isBitOne(v, 5));
    assert(isBitOne(v, 7));
    assert(isBitZero(v, 0));

    assert(setBitToZero(v, 3) == setBitToZero(v, 3));

    v = setBitToOne(v, 3); // 设置位4为1 → 00110101
    assert(v == 0b00110101);

    v = setBitToZero(v, 2); // 设置位5为0 → 00010101
    assert(v == 0b00010101);

    {
        int value = 0b11111111; // 二进制 11111111
        int mask = 0b00001111;  // 二进制 00001111

        // 使用 &~ 操作
        int result1 = value & ~mask; // 结果: 11110000
        std::cout << "value & ~mask: " << std::bitset<8>(result1) << std::endl;

        // 使用 ^ 操作
        int result2 = value ^ mask; // 结果: 11110000 ^ 00001111 = 11110000
        std::cout << "value ^ mask: " << std::bitset<8>(result2) << std::endl;
    }
    {
        int value = 0b10101010; // 二进制 10101010
        int mask = 0b00001111;  // 二进制 00001111

        // 使用 &~ 操作
        int result1 = value & ~mask; // 结果: 10100000
        std::cout << "value & ~mask: " << std::bitset<8>(result1) << std::endl;

        // 使用 ^ 操作
        int result2 = value ^ mask; // 结果: 10100101
        std::cout << "value ^ mask: " << std::bitset<8>(result2) << std::endl;
    }
    // &~ 的作用是将 value 中与 mask 为1的对应位清零，其他位保持不变。
    // ^ 的作用是对 value 和 mask 的每一位进行比较，如果对应位不同则结果为1，否则为0。
    /*
        mask      = 1 << 2 = 0b0100
        ~mask     = 0b1011
        v & ~mask = 0b1010 & 0b1011 = 0b0010

        v ^ mask = 0b1010 ^ 0b0100 = 0b1110  // 第2位被翻转（1→0）
*/

    {
        uint8_t v = 0b00100101; // 二进制: 0x25 (37)
        v = setBitToOne(v, 3);  // 设置索引3（位4）→ 0b00110101 (53)
        assert(v == 0x35);

        v = setBitToZero(v, 2); // 清除索引2（位5）→ 0b00010101 (21)
        assert(v == 0x15);

        {
            int32_t num = 0x80000000;   // 二进制: 1000...0000（符号位为1）
            num = setBitToZero(num, 0); // 清除符号位 → 0x00000000
            assert(num == 0);
        }
        {
            try
            {
                uint8_t v = 0xFF;
                v = setBitToOne(v, 8); // 越界（uint8_t最大索引为7）
                assert(false);         // 应抛出异常
            }
            catch (const std::out_of_range &)
            {
            }
        }
    }
}
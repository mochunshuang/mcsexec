#pragma once

#include <cstdint>
#include <thread>

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
#include <immintrin.h>
#else
// 其他架构的替代方案
#endif

namespace mcs::execution::tool
{
    static inline void __spin_pause() noexcept // NOLINT
    {
#if defined(__x86_64__) || defined(_M_X64)
        _mm_pause(); // x86/x64 架构 // NOLINT
#elif defined(__aarch64__) || defined(_M_ARM64)
        __asm__ __volatile__("yield"); // ARM 架构
#elif defined(__powerpc__) || defined(__ppc__) || defined(__PPC__)
        __asm__ __volatile__("or 27,27,27"); // PowerPC 架构
#elif defined(__riscv)
        __asm__ __volatile__("fence iorw,iorw"); // RISC-V 架构
#elif defined(__mips__)
        __asm__ __volatile__("pause"); // MIPS 架构
#elif defined(__GNUC__) || defined(__clang__)
        __builtin_ia32_pause(); // GCC/Clang 内置函数（x86 架构）
#else
        std::this_thread::yield(); // 通用回退
#endif
    }

    struct spin_wait
    {
        constexpr spin_wait() noexcept = default;

        constexpr void wait() const noexcept
        {
            if (m_count++ < k_yield_threshold)
            {
                for (std::uint32_t i = 0; i < m_count; ++i)
                    __spin_pause();
            }
            else
            {
                if (m_count == 0)
                    m_count = k_yield_threshold;
                std::this_thread::yield();
            }
        }

      private:
        static constexpr uint32_t k_yield_threshold = 20;
        mutable uint32_t m_count = 0;
    };
}; // namespace mcs::execution::tool
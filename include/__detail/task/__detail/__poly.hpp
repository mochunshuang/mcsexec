#pragma once

#include <array>
namespace mcs::execution::__task::__detail
{
    /**
     * @brief Utility providing small object optimization and type erasure.
     *
     */
    template <typename Base, std::size_t Size = 4U * sizeof(void *)>
    class alignas(sizeof(double)) poly // NOLINT
    {
      private:
        std::array<std::byte, Size> buf{}; // NOLINT

        Base *pointer()
        {
            // NOLINTNEXTLINE(bugprone-casting-through-void)
            return static_cast<Base *>(static_cast<void *>(buf.data()));
        }
        const Base *pointer() const
        {
            // NOLINTNEXTLINE(bugprone-casting-through-void)
            return static_cast<const Base *>(static_cast<const void *>(buf.data()));
        }

      public:
        template <typename T, typename... Args>
            requires(sizeof(T) <= Size)
        explicit poly(T * /*unused*/, Args &&...args)
        {
            new (this->buf.data()) T(::std::forward<Args>(args)...);
            static_assert(sizeof(T) <= Size);
        }
        poly(poly &&other) noexcept
            requires requires(Base *b, void *t) { b->move(t); }
        {
            other.pointer()->move(this->buf.data());
        }
        poly &operator=(poly &&other) noexcept
            requires requires(Base *b, void *t) { b->move(t); }
        {
            if (this != &other)
            {
                this->pointer()->~Base();
                other.pointer()->move(this->buf.data());
            }
            return *this;
        }
        poly &operator=(const poly &other)
            requires requires(Base *b, void *t) { b->clone(t); }
        {
            if (this != &other)
            {
                this->pointer()->~Base();
                other.pointer()->clone(this->buf.data());
            }
            return *this;
        }
        poly(const poly &other)
            requires requires(Base *b, void *t) { b->clone(t); }
        {
            other.pointer()->clone(this->buf.data());
        }
        ~poly()
        {
            this->pointer()->~Base();
        }
        bool operator==(const poly &other) const
            requires requires(const Base &b) {
                { b.equals(&b) } -> std::same_as<bool>;
            }
        {
            return other.pointer()->equals(this->pointer());
        }
        Base *operator->()
        {
            return this->pointer();
        }
        const Base *operator->() const
        {
            return this->pointer();
        }
    };
}; // namespace mcs::execution::__task::__detail
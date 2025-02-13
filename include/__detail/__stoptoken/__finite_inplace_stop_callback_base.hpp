#pragma once

#include "./__finite_inplace_stop_source_base.hpp"

namespace mcs::execution::stoptoken
{
    template <__detail::invocable_destructible CB>
    class finite_inplace_stop_callback_base; // NOLINT

    template <__detail::invocable_destructible CB>
    class finite_inplace_stop_callback_base
        : protected finite_inplace_stop_source_base::callback_base
    {
        // Note: because as Constructor is template
        //  Constructor accepting a forwarding reference can hide the copy and move
        //  constructors
      public:
        ~finite_inplace_stop_callback_base() = default;
        finite_inplace_stop_callback_base(finite_inplace_stop_callback_base &&) = delete;
        finite_inplace_stop_callback_base(const finite_inplace_stop_callback_base &) =
            delete;
        finite_inplace_stop_callback_base &operator=(
            finite_inplace_stop_callback_base &&) = delete;
        finite_inplace_stop_callback_base &operator=(
            const finite_inplace_stop_callback_base &) = delete;

      protected:
        template <typename Initializer>
        explicit finite_inplace_stop_callback_base(Initializer &&init) noexcept(
            std::is_nothrow_constructible_v<CB, Initializer>)
            : callback_base{.execute = &execute_impl}, // this->execute = &execute_impl;
              callback(std::forward<Initializer>(init))
        {
        }

      private:
        constexpr static void execute_impl( // NOLINT
            finite_inplace_stop_source_base::callback_base *base) noexcept
        {
            auto &self = *static_cast<finite_inplace_stop_callback_base *>(base);
            self.callback();
        }
#if defined(_MSC_VER)
#define NO_UNIQUE_ADDRESS [[msvc::no_unique_address]]
#else
#define NO_UNIQUE_ADDRESS [[no_unique_address]]
#endif
        NO_UNIQUE_ADDRESS CB callback; // NOLINT
    };
}; // namespace mcs::execution::stoptoken
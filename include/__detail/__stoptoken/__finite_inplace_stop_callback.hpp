#pragma once

#include <concepts>
#include <cstddef>

#include "./__finite_inplace_stop_source.hpp"
#include "./__finite_inplace_stop_callback_base.hpp"

namespace mcs::execution::stoptoken
{
    template <std::size_t N, std::size_t Idx,
              __detail::invocable_destructible CB> // NOLINTNEXTLINE
    class finite_inplace_stop_callback;

    template <std::size_t N, std::size_t Idx, __detail::invocable_destructible CB>
    class finite_inplace_stop_callback : private finite_inplace_stop_callback_base<CB>
    {
      public:
        template <typename Init>
            requires std::constructible_from<CB, Init>
        finite_inplace_stop_callback(
            finite_inplace_stop_token<N, Idx> st,
            Init &&init) noexcept(std::is_nothrow_constructible_v<CB, Init>)
            : finite_inplace_stop_callback_base<CB>(std::forward<Init>(init)),
              stop_source(st.stop_source)
        {
            if (stop_source != nullptr)
            {
                if (!stop_source->try_register_callback(Idx, this))
                {
                    stop_source = nullptr;
                    this->execute(this);
                }
            }
        }

        ~finite_inplace_stop_callback()
        {
            if (stop_source != nullptr)
            {
                stop_source->deregister_callback(Idx, this);
            }
        }

        finite_inplace_stop_callback(finite_inplace_stop_callback &&) = delete;
        finite_inplace_stop_callback(const finite_inplace_stop_callback &) = delete;
        finite_inplace_stop_callback &operator=(finite_inplace_stop_callback &&) = delete;
        finite_inplace_stop_callback &operator=(const finite_inplace_stop_callback &) =
            delete;

      private:
        const finite_inplace_stop_source<N> *stop_source; // NOLINT
    };

    template <std::size_t N, std::size_t Idx, typename CB>
    finite_inplace_stop_callback(finite_inplace_stop_token<N, Idx>, CB)
        -> finite_inplace_stop_callback<N, Idx, CB>;

}; // namespace mcs::execution::stoptoken
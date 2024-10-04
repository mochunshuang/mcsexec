#pragma once

#include "./__invocable_destructible.hpp"
namespace mcs::execution::stoptoken
{
    // [stopcallback.inplace], class template inplace_stop_callback
    template <__detail::invocable_destructible CallbackFn>
    class inplace_stop_callback;

    class inplace_stop_source;

    // The class inplace_stop_token models the concept stoppable_token.
    // It references the stop state of its associated inplace_stop_source object
    // ([stopsource.inplace]), if any.
    class inplace_stop_token // NOLINT
    {
      public:
        template <class CallbackFn>
        using callback_type = inplace_stop_callback<CallbackFn>;

        inplace_stop_token() = default;
        bool operator==(const inplace_stop_token &) const = default;

        // [stoptoken.inplace.mem], member functions
        // Effects: Equivalent to:
        //  return stop-source != nullptr && stop-source->stop_requested();
        // Note:
        // As specified in [basic.life], the behavior of stop_requested() is undefined
        // unless the call strongly happens before the start of the destructor of the
        // associated inplace_stop_source, if any.
        bool stop_requested() const noexcept; // NOLINT
        // Returns: stop-source != nullptr.
        // Note:
        // As specified in [basic.stc.general],
        // the behavior of stop_possible() is implementation-defined unless the call
        // strongly happens before the end of the storage duration of
        // the associated inplace_stop_source object, if any
        bool stop_possible() const noexcept; // NOLINT

        // Effects: Exchanges the values of stop-source and rhs.stop-source.
        void swap(inplace_stop_token &rhs) noexcept;

      private:
        const inplace_stop_source *stop_source = nullptr; // NOLINT // exposition only

        template <__detail::invocable_destructible CallbackFn>
        friend class inplace_stop_callback;
        friend inplace_stop_source;

        // 唯一初始化 stop_source 的地方
        explicit inplace_stop_token(const inplace_stop_source *source) noexcept
            : stop_source(source)
        {
        }
    };
}; // namespace mcs::execution::stoptoken
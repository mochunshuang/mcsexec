#pragma once

#include "./__declarations.hpp"

namespace mcs::execution::stoptoken
{
    // The class inplace_stop_token models the concept stoppable_token.
    // It references the stop state of its associated inplace_stop_source object
    // ([stopsource.inplace]), if any.
    class inplace_stop_token
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
    };
}; // namespace mcs::execution::stoptoken
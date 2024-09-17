#pragma once
#include "./__declarations.hpp"
#include <type_traits>

namespace mcs::execution::stoptoken
{
    // Mandates: CallbackFn satisfies both invocable and destructible.
    template <class CallbackFn>
    class inplace_stop_callback
    {
      public:
        using callback_type = CallbackFn;

        // [stopcallback.inplace.cons], constructors and destructor
        // Remarks:
        // For a type Initializer,
        //  if stoppable-callback-for<CallbackFn,inplace_stop_token, Initializer> is
        //  satisfied, then stoppable-callback-for<CallbackFn, inplace_stop_token,
        //  Initializer> is modeled.
        //
        // For an inplace_stop_callback<CallbackFn> object, the exposition-only
        // callback-fn member is its associated callback function ([stoptoken.concepts]).
        //
        // Constraints: constructible_from<CallbackFn, Initializer> is satisfied.
        // Effects:
        // Initializes callback-fn with std::forward<Initializer>(init)
        // and executes a stoppable callback registration ([stoptoken.concepts]).
        template <class Initializer>
        explicit inplace_stop_callback(
            inplace_stop_token st,
            Initializer &&
                init) noexcept(std::is_nothrow_constructible_v<CallbackFn, Initializer>);
        // Effects: Executes a stoppable callback deregistration ([stoptoken.concepts]).
        ~inplace_stop_callback();

        inplace_stop_callback(inplace_stop_callback &&) = delete;
        inplace_stop_callback(const inplace_stop_callback &) = delete;
        inplace_stop_callback &operator=(inplace_stop_callback &&) = delete;
        inplace_stop_callback &operator=(const inplace_stop_callback &) = delete;

      private:
        CallbackFn callback_fn; // NOLINT // exposition only
    };

    template <class CallbackFn>
    inplace_stop_callback(inplace_stop_token,
                          CallbackFn) -> inplace_stop_callback<CallbackFn>;

}; // namespace mcs::execution::stoptoken
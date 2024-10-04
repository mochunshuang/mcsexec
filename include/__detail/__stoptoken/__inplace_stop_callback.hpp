#pragma once

#include <type_traits>

#include "./__inplace_callback_base.hpp"
#include "./__invocable_destructible.hpp"

namespace mcs::execution::stoptoken
{
    class inplace_stop_token;
    class inplace_stop_source;

    // Mandates: CallbackFn satisfies both invocable and destructible.
    template <__detail::invocable_destructible CallbackFn>
    class inplace_stop_callback final : public inplace_callback_base // NOLINT
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
        ~inplace_stop_callback() override;

        inplace_stop_callback(inplace_stop_callback &&) = delete;
        inplace_stop_callback(const inplace_stop_callback &) = delete;
        inplace_stop_callback &operator=(inplace_stop_callback &&) = delete;
        inplace_stop_callback &operator=(const inplace_stop_callback &) = delete;

      private:
        CallbackFn callback_fn;                    // NOLINT // exposition only
        inplace_stop_source *stop_source{nullptr}; // NOLINT
        bool registered{false};                    // NOLINT
        auto invoke_callback() -> void override;
    };

    template <class CallbackFn>
    inplace_stop_callback(inplace_stop_token,
                          CallbackFn) -> inplace_stop_callback<CallbackFn>;

}; // namespace mcs::execution::stoptoken
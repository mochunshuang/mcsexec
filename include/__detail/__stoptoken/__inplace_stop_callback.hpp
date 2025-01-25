#pragma once

#include "./__inplace_stop_source.hpp"

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
            Initializer
                &&init) noexcept(std::is_nothrow_constructible_v<CallbackFn, Initializer>)
            : inplace_callback_base{.invoke_callback =
                                        &inplace_stop_callback::invoke_callback_impl},
              callback_fn(std::forward<Initializer>(init)),
              stop_source(const_cast<inplace_stop_source *>(st.stop_source)) // NOLINT
        {
            // 1. Constraints: constructible_from<CallbackFn, Initializer> is satisfied.
            static_assert(std::constructible_from<CallbackFn, Initializer>);
            // 2. Effects: Initializes callback-fn with std::forward<Initializer>(init)
            // and executes a stoppable callback registration
            if (stop_source != nullptr &&
                ::mcs::execution::stoptoken::inplace_stop_source::stop_possible())
            {
                if (not stop_source->stop_requested())
                {
                    stop_source->registration(this);
                    registered = true;
                }
                else
                    std::forward<CallbackFn>(callback_fn)(); // immediately evaluated
            }
        }

        // Effects: Executes a stoppable callback deregistration ([stoptoken.concepts]).
        ~inplace_stop_callback() noexcept
        {
            // Note: it is shall have no effect if no registered
            if (registered)
            {
                // shall be removed from the associated stop state.
                stop_source->deregistration(this);
            }
            // The stoppable callback deregistration shall destroy callback_fn
        }

      private:
        CallbackFn callback_fn;                    // NOLINT // exposition only
        inplace_stop_source *stop_source{nullptr}; // NOLINT
        bool registered{false};                    // NOLINT
        static auto invoke_callback_impl(inplace_callback_base *base) noexcept // NOLINT
            -> void
        {
            auto &self = *static_cast<inplace_stop_callback *>(base);
            std::forward<CallbackFn>(self.callback_fn)();
        }
    };

    template <class CallbackFn>
    inplace_stop_callback(inplace_stop_token, CallbackFn)
        -> inplace_stop_callback<CallbackFn>;

}; // namespace mcs::execution::stoptoken
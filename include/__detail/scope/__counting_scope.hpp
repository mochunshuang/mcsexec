#pragma once

#include <utility>

#include "../__stoptoken/__inplace_stop_source.hpp"
#include "../snd/__sender.hpp"
#include "./__stop_when.hpp"

#include "./__simple_counting_scope.hpp"

namespace mcs::execution::scope
{
    struct counting_scope
    {
      public:
        // [exec.counting.token], token
        struct token
        {
            template <snd::sender Sndr>
            snd::sender auto wrap(Sndr &&sndr) noexcept
            {
                return stop_when(std::forward<Sndr>(sndr), scope->s_source.get_token());
            }

            [[nodiscard]] bool try_associate() const noexcept // NOLINT
            {
                return simple_token.try_associate();
            }

            void disassociate() const noexcept
            {
                simple_token.disassociate();
            }

          private:
            friend counting_scope;
            explicit token(counting_scope *s) noexcept
                : scope(s), simple_token(scope->simple_scope.get_token())
            {
            }
            using token_t = decltype(std::declval<simple_counting_scope>().get_token());

            counting_scope *scope; // exposition-only // NOLINT
            token_t simple_token;  // NOLINT
        };

        // [exec.counting.ctor], constructor and destructor
        counting_scope(counting_scope &&) = delete;
        counting_scope(const counting_scope &) = delete;
        counting_scope &operator=(counting_scope &&) = delete;
        counting_scope &operator=(const counting_scope &) = delete;

        // Postcondtions: count is 0 and state is unused
        counting_scope() noexcept = default;
        // Effects: If state is not one of joined, unused, or unused-and-closed,
        // invokes terminate (14.6.2 [except.terminate]). Otherwise, has no effects.
        ~counting_scope() noexcept = default;

        // NOTE: Calls to member functions get_token, close, join, and request_stop do
        // not introduce data races.
        //  [exec.counting.mem], members
        token get_token() noexcept // NOLINT
        {
            return token(this);
        }
        void close() noexcept
        {
            simple_scope.close();
        }

        void request_stop() noexcept // NOLINT
        {
            s_source.request_stop();
        }

        snd::sender auto join() noexcept
        {
            return simple_scope.join();
        }

        simple_counting_scope simple_scope;      // NOLINT
        stoptoken::inplace_stop_source s_source; // NOLINT
    };
}; // namespace mcs::execution::scope

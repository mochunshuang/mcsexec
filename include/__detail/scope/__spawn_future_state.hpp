#pragma once
#include <memory>

#include "../snd/__completion_signatures_of_t.hpp"
#include "../conn/__connect.hpp"

#include "./__spawn_future_state_base.hpp"
#include "./__async_scope_token.hpp"
#include "./__spawn_future_receiver.hpp"

namespace mcs::execution::scope
{
    template <class Alloc, async_scope_token Token, snd::sender Sender>
    struct spawn_future_state
        : spawn_future_state_base<snd::completion_signatures_of_t<Sender>>
    {
        using sigs_t =
            snd::completion_signatures_of_t<Sender, empty_env>;  // exposition only
        using receiver_t = scope::spawn_future_receiver<sigs_t>; // exposition only
        using op_t =
            decltype(connect(declval<Sender>(), receiver_t{nullptr})); // exposition only

        spawn_future_state(Alloc alloc, Sender &&sndr, Token token) // exposition only
            : alloc(std::move(alloc)),
              op(conn::connect(std::move(sndr),
                               scope::spawn_future_receiver{.state = this})),
              token(std::move(token))
        {
        }

        void run() // exposition only
        {
            if (associated == token.try_associate())
                op.start();
            else
            {
                this->result.template emplace<decayed_tuple<set_stopped_t>>(
                    set_stopped_t{});
                this->complete();
            }
        }
        /**
         * @brief
         * 1. No effects if the invocation of complete happens-before an invocation of
         * consume or abandon;
         * 2. otherwise, if an invocation of consume happened-before this invocation of
         * complete then there is a receiver, rcvr, registered and that receiver is
         * completed as if by consume(rcvr);
         * 3. otherwise, an invocation of abandon happened-before this invocation of
         * complete and destroy() is invoked.
         */
        void complete() override
        {
            // TODO(mcs):
        }
        void consume(receiver auto &rcvr) noexcept; // exposition only
        void abandon() noexcept;                    // exposition only

      private:
        using alloc_t = typename std::allocator_traits<Alloc>::template rebind_alloc<
            spawn_future_state>;
        // NOLINTBEGIN
        alloc_t alloc; // exposition only
        op_t op;       // exposition only
        Token token;   // exposition only
        bool associated = false;
        // NOLINTEND
        void destroy() noexcept; // exposition only
    };

}; // namespace mcs::execution::scope
#pragma once

#include <memory>

#include "./__spawn_state_base.hpp"
#include "./__async_scope_token.hpp"
#include "./__spawn_receiver.hpp"
#include "../conn/__connect.hpp"

namespace mcs::execution::scope
{
    template <class Alloc, async_scope_token Token, snd::sender Sndr>
    struct spawn_state : spawn_state_base
    {
        using op_t =
            decltype(conn::connect(std::declval<Sndr>(), spawn_receiver{nullptr}));

        spawn_state(Alloc alloc, Sndr &&sndr, Token token)
            : spawn_state_base{.complete = &spawn_state::invoke}, alloc(alloc),
              op(conn::connect(std::move(sndr), spawn_receiver{this})), token(token)
        {
        }

        void run()
        {
            if (token.try_associate())
                op.start();
            else
                destroy();
        }
        void complete_impl() noexcept // NOLINT
        {
            auto token = std::move(this->token);
            destroy();
            token.disassociate();
        }

      private:
        using alloc_t =
            typename std::allocator_traits<Alloc>::template rebind_alloc<spawn_state>;

        alloc_t alloc; // NOLINT
        op_t op;       // NOLINT
        Token token;   // NOLINT

        void destroy() // NOTE: 标准库的 deallocate 的有异常的
        {
            auto alloc = std::move(this->alloc);
            std::allocator_traits<alloc_t>::destroy(alloc, this);
            std::allocator_traits<alloc_t>::deallocate(alloc, this, 1);
        }

        static void invoke(spawn_state_base *self) noexcept
        {
            auto *derived = static_cast<spawn_state *>(self);
            derived->complete_impl();
        }
    };
}; // namespace mcs::execution::scope

#ifndef E4F1C886_2628_4A07_AC1C_C2EBC69D810F
#define E4F1C886_2628_4A07_AC1C_C2EBC69D810F
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
            : alloc(alloc), op(conn::connect(std::move(sndr), spawn_receiver{this})),
              token(token)
        {
        }

        void run() noexcept
        {
            if (token.try_associate())
                op.start();
            else
                destroy();
        }
        void complete() override
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

        void destroy() noexcept
        {
            auto alloc = std::move(this->alloc);
            std::allocator_traits<alloc_t>::destroy(alloc, this);
            std::allocator_traits<alloc_t>::deallocate(alloc, this, 1);
        }
    };
}; // namespace mcs::execution::scope

#endif /* E4F1C886_2628_4A07_AC1C_C2EBC69D810F */

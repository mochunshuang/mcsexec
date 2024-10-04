#include "./../__stop_callback_base.hpp"
#include "./__stop_token_impl.hpp"
#include <utility>

namespace mcs::execution::stoptoken
{
    inline stop_callback_base::stop_callback_base(
        ::mcs::execution::stoptoken::stop_token const &token)
        : state(token.stop_state)
    {
    }

    inline stop_callback_base::~stop_callback_base() = default;

    inline auto stop_callback_base::call() -> void
    {
        this->do_call();
    }

    inline auto stop_callback_base::registration() -> void
    {
        if (this->state)
        {
            {
                std::lock_guard guard(this->state->lock);
                if (!this->state->stop_requested)
                {
                    this->next = std::exchange(this->state->callbacks, this);
                    return;
                }
            }
            this->call();
        }
    }

} // namespace mcs::execution::stoptoken
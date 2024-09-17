#include "../snd/__sender.hpp"

#include "./__sender_adaptor_closure.hpp"

namespace mcs::execution::pipeable
{
    template <snd::sender Sndr, typename Adaptor>
        requires(not snd::sender<Adaptor>) &&
                std::derived_from<std::decay_t<Adaptor>,
                                  sender_adaptor_closure<std::decay_t<Adaptor>>> &&
                requires(Sndr &&sndr, Adaptor &&adaptor) {
                    { adaptor(std::forward<Sndr>(sndr)) } -> snd::sender;
                }
    auto operator|(Sndr &&sndr, Adaptor &&adaptor)
    {
        return adaptor(std::forward<Sndr>(sndr));
    }

}; // namespace mcs::execution::pipeable
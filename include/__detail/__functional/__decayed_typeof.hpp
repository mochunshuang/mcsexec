#pragma once

namespace mcs::execution::functional
{
    template <const auto &Tag>
    using decayed_typeof = decltype(auto(Tag)); // exposition only

}; // namespace mcs::execution::functional
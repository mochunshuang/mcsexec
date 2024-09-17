#pragma once
#include "./__sender.hpp"
#include "./__tag_of_t.hpp"

namespace mcs::execution::snd
{
    template <class Sndr, class Tag>
    concept sender_for = sender<Sndr> && std::same_as<tag_of_t<Sndr>, Tag>;
};
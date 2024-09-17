#pragma once

#include "./snd/__default_domain.hpp"
#include "./snd/__transform_sender.hpp"
#include "./snd/__transform_env.hpp"
#include "./snd/__apply_sender.hpp"
#include "./snd/__get_completion_signatures.hpp"

#include "./snd/__sender.hpp"
#include "./snd/__sender_in.hpp"
#include "./snd/__sender_to.hpp"
#include "./snd/__sender_for.hpp"

#include "snd/__make_sender.hpp"

namespace mcs::execution
{
    using ::mcs::execution::snd::default_domain;
    using ::mcs::execution::snd::transform_sender;
    using ::mcs::execution::snd::transform_env;
    using ::mcs::execution::snd::apply_sender;
    using ::mcs::execution::snd::get_completion_signatures_t;

    using ::mcs::execution::snd::completion_signatures_of_t;

    // concepts
    using ::mcs::execution::snd::sender;
    using ::mcs::execution::snd::sender_in;
    using ::mcs::execution::snd::sender_to;
    using ::mcs::execution::snd::sender_for;

    //
    using ::mcs::execution::snd::get_completion_signatures;

    //
    using ::mcs::execution::snd::tag_of_t;

    using ::mcs::execution::snd::make_sender;

}; // namespace mcs::execution
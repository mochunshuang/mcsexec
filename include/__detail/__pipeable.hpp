
#pragma once

#include "./pipeable/__pipeable.hpp"
#include "./pipeable/__sender_adaptor.hpp"
#include "./pipeable/__sender_adaptor_closure.hpp"

namespace mcs::execution
{

    using ::mcs::execution::pipeable::sender_adaptor_closure;
    using ::mcs::execution::pipeable::sender_adaptor;
    using ::mcs::execution::pipeable::operator|;

} // namespace mcs::execution
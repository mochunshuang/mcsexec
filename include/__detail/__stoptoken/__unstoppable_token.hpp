#pragma once

#include "./__stoppable_token.hpp"

namespace mcs::execution::stoptoken
{

    template <class Token>
    concept unstoppable_token = stoppable_token<Token> && requires { // NOLINT
        { std::bool_constant<Token::stop_possible()>{} } -> std::same_as<std::false_type>;
    };

}; // namespace mcs::execution::stoptoken

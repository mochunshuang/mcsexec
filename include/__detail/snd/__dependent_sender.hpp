#pragma once

#include "./__dependent_sender_error.hpp"
#include "./__get_completion_signatures.hpp"

#include <type_traits>

namespace mcs::execution::snd
{
    // TODO(mcs): 使用到了c++26 必定失败
    // template <class Sndr>
    // consteval bool is_dependent_sender_helper()
    // try
    // { // exposition only
    //     get_completion_signatures<Sndr>();
    //     return false;
    // }
    // catch (dependent_sender_error &)
    // {
    //     return true;
    // }
    template <class Sndr>
    consteval bool is_dependent_sender_helper()
    {
        // cmplsigs::completion_signatures<> as error
        using CS = decltype(get_completion_signatures<Sndr>());
        return std::is_same_v<CS, dependent_sender_error>;
    }

    template <class Sndr>
    concept dependent_sender =
        sender<Sndr> && std::bool_constant<is_dependent_sender_helper<Sndr>()>::value;
    ;
}; // namespace mcs::execution::snd
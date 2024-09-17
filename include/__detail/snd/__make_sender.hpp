#pragma once

#include "__detail/__basic_sender.hpp"

#include <type_traits>

namespace mcs::execution::snd
{
    /////////////////////////////////////////////////////////////
    // make_sender
    struct empty_data
    {
    };

    template <std::semiregular Tag, movable_value Data = empty_data, sender... Child>
        requires(not std::is_pointer_v<Tag>)
    constexpr auto make_sender(Tag tag, Data &&data = empty_data{},
                               Child &&...child) -> decltype(auto)
    {
        // Returns: A prvalue of type basic-sender<Tag, decay_t<Data>, decay_t<Child>...>
        // that has been direct-list-initialized with the forwarded arguments,
        return __detail::basic_sender<std::decay_t<Tag>, std::decay_t<Data>,
                                      std::decay_t<Child>...>{
            tag, std::forward<Data>(data), std::forward<Child>(child)...};
    }

}; // namespace mcs::execution::snd

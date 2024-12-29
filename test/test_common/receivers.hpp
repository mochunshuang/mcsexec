#pragma once

#include "./test_macro.hpp"

#include "./empty_recv/recv0.hpp"
#include "./empty_recv/recv0_ec.hpp"
#include "./empty_recv/recv_int.hpp"
#include "./empty_recv/recv_int_ec.hpp"

#include "./recv/base_expect_receiver.hpp"

#include "./recv/expect_error_receiver_ex.hpp"
#include "./recv/expect_error_receiver.hpp"

#include "./recv/expect_stopped_receiver_ex.hpp"
#include "./recv/expect_stopped_receiver.hpp"

#include "./recv/expect_value_receiver_ex.hpp"
#include "./recv/expect_value_receiver.hpp"

#include "./recv/expect_void_receiver_ex.hpp"
#include "./recv/expect_void_receiver.hpp"

#include "./recv/fun_receiver.hpp"
#include "./recv/logging_receiver.hpp"
#include "./recv/typecat_receiver.hpp"

#include "./recv/value_receiver.hpp"

namespace test
{
    template <typename T>
    struct __check
    {
        // std::variant<std::tuple<std::variant<Ts...>>> is EXPECT
        template <typename Arg>
        struct get_ARG;
        template <typename Arg>
        struct get_ARG<std::variant<std::tuple<Arg>>>
        {
            using type = Arg;
        };
        // NOLINTNEXTLINE
        static constexpr bool value = mcs::execution::snd::__detail::valid_specialization<
            std::variant, typename get_ARG<T>::type>;
    };
    static_assert(__check<std::variant<std::tuple<std::variant<int, double>>>>::value);

    // TODO(mcs): 观望一下
    template <class Sndr, class Env>
    concept single_value_variant_sender = true;
    // __check<mcs::execution::cmplsigs::value_types_of_t<Sndr, Env>>::value;

    template <mcs::execution::sender S, class... Ts>
    inline void wait_for_value(S &&snd, Ts &&...val)
    {
        // Ensure that the given sender type has only one variant for set_value calls
        // If not, sync_wait will not work
        static_assert(
            single_value_variant_sender<
                S, mcs::execution::consumers::__sync_wait::sync_wait_env>,
            "Sender passed to sync_wait needs to have one variant for sending set_value");

        std::optional<std::tuple<Ts...>> res =
            mcs::this_thread::sync_wait(std::forward<S>(snd));
        EXPECT(res.has_value());

        std::tuple<Ts...> expected(std::forward<Ts>(val)...);
        if constexpr (std::tuple_size_v<std::tuple<Ts...>> == 1)
            EXPECT(std::get<0>(res.value()) == std::get<0>(expected));
        else
            EXPECT(res.value() == expected);
    }
}; // namespace test
#include "../test_base_head.hpp"

int main()
{
    using namespace mcs::execution::awaitables; // NOLINT
    TEST("base") = [] {
        struct local_env
        {
        };

        env_promise<local_env> promise;

        static_assert(noexcept(promise.get_return_object()));
        static_assert(noexcept(promise.initial_suspend()));
        static_assert(noexcept(promise.final_suspend()));
        static_assert(noexcept(promise.unhandled_exception()));
        static_assert(noexcept(promise.return_void()));
        static_assert(noexcept(promise.unhandled_stopped()));

        static_assert(std::same_as<void, decltype(promise.unhandled_exception())>);
        static_assert(std::same_as<void, decltype(promise.return_void())>);
        static_assert(
            std::same_as<std::coroutine_handle<>, decltype(promise.unhandled_stopped())>);
        static_assert(noexcept(std::as_const(promise).get_env()));
        static_assert(std::same_as<const local_env &, decltype(promise.get_env())>);
    };

    return 0;
}
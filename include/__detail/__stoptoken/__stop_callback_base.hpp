#pragma once

#include <memory>
#include <thread>

namespace mcs::execution::stoptoken
{
    struct stop_state;
    struct stop_token;

    class stop_callback_base // NOLINT
    {
        using stop_state = ::mcs::execution::stoptoken::stop_state;
        std::shared_ptr<stop_state> state; // NOLINT

        virtual auto do_call() -> void = 0; // NOLINT

      protected:
        explicit stop_callback_base(::mcs::execution::stoptoken::stop_token const &);
        ~stop_callback_base();

      public:
        auto call() -> void;
        auto registration() -> void;
        auto deregistration() -> void;

        stop_callback_base *next{}; // NOLINT
        std::thread::id id{};       // NOLINT
    };
}; // namespace mcs::execution::stoptoken
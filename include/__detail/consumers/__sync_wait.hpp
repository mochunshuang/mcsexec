#pragma once

#include <utility>

#include "./__AS_EXCEPT_PTR.hpp"

#include "../__core_types.hpp"

#include "../ctx/__run_loop.hpp"

#include "../queries/__get_scheduler.hpp"
#include "../queries/__get_delegation_scheduler.hpp"

#include "../snd/__sender_in.hpp"
#include "../snd/__sender_to.hpp"
#include "../snd/__apply_sender.hpp"
#include "../snd/general/__get_domain_early.hpp"

#include "../conn/__connect.hpp"

#include "../opstate/__start.hpp"

#include "../cmplsigs/__value_types_of_t.hpp"

namespace mcs::execution::consumers
{
    namespace __sync_wait
    {

        // this_thread::sync_wait and this_thread::sync_wait_with_variant are used to
        // block the current thread of execution until the specified sender completes and
        // to return its async result. sync_wait mandates that the input sender has
        // exactly one value completion signature.
        struct sync_wait_env
        {
            ctx::run_loop *loop; // exposition only // NOLINT

            [[nodiscard]] auto query(
                queries::get_scheduler_t const & /*unused*/) const noexcept
            {
                return loop->get_scheduler();
            }

            [[nodiscard]] auto query(
                queries::get_delegation_scheduler_t const & /*unused*/) const noexcept
            {
                return loop->get_scheduler();
            }
        };

        template <snd::sender_in<sync_wait_env> Sndr>
        using sync_wait_result_type = std::optional<cmplsigs::value_types_of_t<
            Sndr, sync_wait_env, execution::decayed_tuple, std::type_identity_t>>;

        template <snd::sender_in<sync_wait_env> Sndr>
        using sync_wait_with_variant_result_type =
            std::optional<cmplsigs::value_types_of_t<Sndr, sync_wait_env>>;

        template <class Sndr>
        struct sync_wait_state // exposition only
        {
            ctx::run_loop loop;                 // exposition only
            std::exception_ptr error;           // exposition only
            sync_wait_result_type<Sndr> result; // exposition only
        };

        template <class Sndr>
        struct sync_wait_receiver // exposition only
        {
            using receiver_concept = execution::receiver_t;
            sync_wait_state<Sndr> *state; // exposition only // NOLINT

            template <class... Args>
            void set_value(Args &&...args) && noexcept // NOLINT
            {
                try
                {
                    state->result.emplace(std::forward<Args>(args)...);
                }
                catch (...)
                {
                    state->error = std::current_exception();
                }
                state->loop.finish();
            }

            template <class Error>
            void set_error(Error &&err) && noexcept // NOLINT
            {
                state->error =
                    AS_EXCEPT_PTR(std::forward<Error>(err)); // see [exec.general]
                state->loop.finish();
            }

            void set_stopped() && noexcept // NOLINT
            {
                state->loop.finish();
            }

            [[nodiscard]] sync_wait_env get_env() const noexcept // NOLINT
            {
                return {&state->loop};
            }
        };

        // [exec.consumers], consumers
        struct sync_wait_t
        {
            template <typename Sndr>
            auto operator()(Sndr &&sndr) const
                requires requires() {
                    typename sync_wait_result_type<Sndr>;
                    {
                        snd::apply_sender(snd::general::get_domain_early(sndr), *this,
                                          ::std::forward<Sndr>(sndr))
                    } -> ::std::same_as<sync_wait_result_type<Sndr>>;
                }
            {
                auto dom = snd::general::get_domain_early(sndr);
                return snd::apply_sender(dom, *this, std::forward<Sndr>(sndr));
            }

            // no const because: Tag() is not const
            // default_domain().apply_sender(Tag(), std::forward<Sndr>(sndr),
            // std::forward<Args>(args)...)
            template <typename Sndr>
                requires snd::sender_to<Sndr, sync_wait_receiver<Sndr>>
            auto apply_sender(Sndr &&sndr) // NOLINT
            {
                sync_wait_state<Sndr> state;
                auto op = conn::connect(std::forward<Sndr>(sndr),
                                        sync_wait_receiver<Sndr>{&state});
                opstate::start(op);

                state.loop.run();

                if (state.error)
                {
                    std::rethrow_exception(std::move(state.error));
                }
                return std::move(state.result);
            }
        };

    }; // namespace __sync_wait

    using __sync_wait::sync_wait_t;
    inline constexpr sync_wait_t sync_wait{}; // NOLINT
}; // namespace mcs::execution::consumers

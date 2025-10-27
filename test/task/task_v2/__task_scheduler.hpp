#pragma once

#include "../../test_base_head.hpp"
#include <algorithm>
#include <bit>
#include <cstddef>
#include <utility>

namespace mcs::execution::task_v2
{
    namespace __detail
    {

        template <typename Sch>
        struct schedule_model
        {
            struct storage_ops
            {
                void *(*get_object)(void *storage) noexcept;
                void (*destroy)(void *storage) noexcept;
                void (*copy_construct)(void *dest, const void *src);
                void (*move_construct)(void *dest, void *src) noexcept;
                bool (*uses_heap_storage)() noexcept;
            };
        };

        template <std::size_t size = 3 * sizeof(void *),
                  std::size_t align = alignof(std::max_align_t)>
        struct storage_schedule
        {
            static constexpr std::size_t BUFFER_SIZE = size;   // NOLINT
            static constexpr std::size_t BUFFER_ALIGN = align; // NOLINT
            union storage_union {
                alignas(BUFFER_ALIGN) std::byte stack_buffer[BUFFER_SIZE]; // NOLINT
                void *heap_ptr;
            };

            storage_union storage_; // NOLINT
        };
    }; // namespace __detail

    // [exec.task.scheduler]
    class task_scheduler // NOLINT
    {

        struct sender // exposition only // NOLINT
        {

            using scheduler_concept = scheduler_t;
            using completion_signatures = completion_signatures<
                recv::set_value_t(), recv::set_error_t(std::error_code),
                recv::set_error_t(std::exception_ptr), recv::set_stopped_t()>;

            template <recv::receiver O>
            state<O> connect(O &&rcvr)
            {
            }

            struct env
            {

                [[nodiscard]] constexpr auto query( // NOLINT
                    queries::get_completion_scheduler_t<set_value_t> /*unused*/)
                    const noexcept
                {
                    // return task_scheduler{};
                }
            };

            // NOLINTNEXTLINE
            [[nodiscard]] constexpr auto get_env() const noexcept -> env
            {
                return {};
            }
        };
        template <opstate::operation_state O>
        struct state // exposition only // NOLINT
        {
            O o; // NOLINT

            using operation_state_concept = operation_state_t;
            void start() & noexcept
            {
                opstate::start(o);
            }
        };

      public:
        using scheduler_concept = scheduler_t;

        template <class Sch, class Allocator = std::allocator<std::byte>>
            requires(!std::same_as<task_scheduler, std::remove_cvref_t<Sch>>) &&
                    scheduler<Sch>
        explicit task_scheduler(Sch &&sch, Allocator alloc = {});

        sender schedule();

        friend bool operator==(const task_scheduler &lhs,
                               const task_scheduler &rhs) noexcept = default;
        template <class Sch>
            requires(!std::same_as<task_scheduler, Sch>) && scheduler<Sch>
        friend bool operator==(const task_scheduler &lhs, const Sch &rhs) noexcept;

      private:
        __detail::storage_schedule<> sch_; // exposition only
    };

    // static_assert(mcs::execution::scheduler<task_scheduler>);

}; // namespace mcs::execution::task_v2
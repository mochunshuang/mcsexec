#pragma once
#include <atomic>
#include <cassert>
#include <condition_variable>
#include <utility>

#include "../__core_types.hpp"

#include "../queries/__get_stop_token.hpp"
#include "../queries/__get_completion_scheduler.hpp"
#include "../recv/__set_stopped.hpp"
#include "../recv/__set_value.hpp"
#include "../recv/__set_error.hpp"
#include "../recv/__receiver_of.hpp"

#include "../sched/__scheduler.hpp"
#include "../cmplsigs/__completion_signatures.hpp"

namespace mcs::execution::ctx
{
    struct thread_context; // friend

    /////////////////////////////////
    // [exec.run.loop]
    // 1.
    // run_loop是一种可以调度工作的执行资源。它维护一个线程安全的【先进先出】工作队列
    //    它的run（）成员函数从队列中删除元素，并在调用run（）的执行线程上【循环执行】它们
    // 2.
    // run_loop实例具有与其队列中的工作项数量相对应的关联计数。此外，run_loop实例具有关联状态，可以是启动、运行或结束状态之一。
    // 3.
    // 并发调用run及其析构函数以外的run_loop的成员函数不会引入数据竞争。成员函数pop_front、push_back和完成原子执行。
    // 4. 推荐做法：鼓励实现使用操作状态的侵入性队列来保存工作单元，以避免动态内存
    class run_loop // NOLINT
    {
        friend struct thread_context;

      public:
        enum class State : std::uint8_t
        {
            starting,  // NOLINT
            running,   // NOLINT
            finishing, // NOLINT
            finished   // NOLINT
        };

      private:
        // [exec.run.loop.types] Associated types
        // models scheduler: run_loop_scheduler
        // models sender: run_loop_scheduler::run_loop_sender

        struct run_loop_opstate_base     // NOLINT
        {                                // exposition only
            virtual void execute() = 0;  // exposition only
            run_loop *loop;              // exposition only // NOLINT
            run_loop_opstate_base *next; // exposition only // NOLINT

            // Note: No Need. children can't move copy
            //  virtual ~run_loop_opstate_base() = default;
        };

        // FIFO
        using state_node = run_loop_opstate_base;
        state_node *head{nullptr};                 // NOLINT
        state_node *tail{nullptr};                 // NOLINT
        std::atomic<State> state{State::starting}; // NOLINT
        std::mutex mtx;                            // NOLINT
        std::condition_variable cv;                // NOLINT

        template <typename Rcvr>
        struct run_loop_opstate : public run_loop_opstate_base
        {
            using operation_state_concept = operation_state_t;

            ~run_loop_opstate() noexcept = default;
            run_loop_opstate(run_loop_opstate &&) = delete;
            run_loop_opstate(const run_loop_opstate &) = delete;
            run_loop_opstate &operator=(run_loop_opstate &&) = delete;
            run_loop_opstate &operator=(const run_loop_opstate &) = delete;

            constexpr explicit run_loop_opstate(run_loop *l, Rcvr r) noexcept
                : rcvr(std::move(r))
            {
                loop = l;       // NOLINT
                next = nullptr; // NOLINT
            }

            void execute() noexcept override
            {
                if (queries::get_stop_token(rcvr).stop_requested())
                {
                    recv::set_stopped(std::move(rcvr));
                }
                else
                {
                    recv::set_value(std::move(rcvr));
                }
            }

            void start() & noexcept
            {
                try
                {
                    loop->push_back(this); // now no execute
                }
                catch (...)
                {
                    recv::set_error(std::move(rcvr), std::current_exception());
                }
            }

            Rcvr rcvr; // NOLINT
        };

        // Instances of run-loop-scheduler remain valid until the end of the lifetime of
        // the run_loop instance from which they were obtained.
        class scheduler // NOLINT
        {
            friend run_loop;

            constexpr explicit scheduler(run_loop *__loop) noexcept : run_loop_{__loop} {}

            run_loop *run_loop_{}; // NOLINT

            class sender // NOLINT
            {
                friend scheduler;

                constexpr explicit sender(run_loop *__loop) noexcept : run_loop_(__loop)
                {
                }
                run_loop *run_loop_{}; // NOLINT

              public:
                using sender_concept = sender_t; // for sender

                // run-loop-sender is an exposition-only type that satisfies sender.
                // For any type Env, completion_signatures_of_t<run-loop-sender, Env>
                // is:
                using completion_signatures = cmplsigs::completion_signatures<
                    set_value_t(), set_error_t(std::exception_ptr), set_stopped_t()>;

                struct env
                {
                    run_loop *run_loop_{}; // NOLINT

                    // Let C be either set_value_t or set_stopped_t
                    template <class Tag>
                        requires(std::is_same_v<Tag, set_value_t> ||
                                 std::is_same_v<Tag, set_stopped_t>)
                    [[nodiscard]] constexpr auto query(
                        queries::get_completion_scheduler_t<Tag> /*unused*/)
                        const noexcept -> scheduler
                    {
                        return scheduler{run_loop_};
                    }
                };

                // NOLINTNEXTLINE
                [[nodiscard]] constexpr auto get_env() const noexcept -> env
                {
                    return {run_loop_};
                }

                template <typename Self, typename Rcvr>
                auto connect(this Self &&self,
                             Rcvr rcvr) noexcept(noexcept((void(self), auto(rcvr))))
                    -> run_loop_opstate<std::decay_t<Rcvr>>
                    requires(recv::receiver_of<decltype(rcvr), completion_signatures>)
                {
                    return run_loop_opstate{std::forward<Self>(self).run_loop_,
                                            std::move(rcvr)};
                }

                template <decays_to<sender> Self, class Env>
                auto get_completion_signatures(this Self && /*self*/, // NOLINT
                                               Env && /*env*/) noexcept
                    -> completion_signatures
                {
                    return {};
                }
            };

          public:
            scheduler() = default;
            using scheduler_concept = scheduler_t; // models scheduler

            // Two instances of run-loop-scheduler compare equal if and only if they were
            // obtained from the same run_loop instance.
            auto operator==(const scheduler &) const noexcept -> bool = default;

            [[nodiscard]] constexpr auto schedule() const noexcept -> snd::sender auto
            {
                return sender{run_loop_};
            }
        };

        // [exec.run.loop.members] Member functions:
        state_node *pop_front() noexcept;      // NOLINT // exposition only
        void push_back(state_node *) noexcept; // NOLINT// exposition only

      public:
        // [exec.run.loop.ctor] construct/copy/destroy
        run_loop() noexcept = default;
        run_loop(run_loop &&) = delete;
        run_loop(const run_loop &) = delete;
        run_loop &operator=(run_loop &&) = delete;
        run_loop &operator=(const run_loop &) = delete;
        ~run_loop() noexcept
        {
            if (head != nullptr ||
                state.load(std::memory_order_acquire) == State::running)
                std::terminate();
        }

        // [exec.run.loop.members] Member functions:
        [[nodiscard]] auto get_scheduler() noexcept -> sched::scheduler auto // NOLINT
        {
            return scheduler{this};
        }

        void run() noexcept;
        void finish() noexcept;
    };
    /**
     * @brief Effects: Blocks ([defns.block]) until one of the following conditions is
     * true:
     *      count is 0 and state is finishing, in which case pop-front sets state to
     *      finished and returns nullptr; or
     *
     *      count is greater than 0, in which case an item is removed from the front of
     *      the queue, count is decremented by 1, and the removed item is returned.
     *
     * @return run_loop::Node*
     */
    run_loop::state_node *run_loop::pop_front() noexcept // NOLINT
    {
        std::unique_lock lk(mtx);
        cv.wait(lk, [this] {
            // block until one of the following conditions is true
            return head != nullptr ||
                   (head == nullptr &&
                    state.load(std::memory_order_acquire) == State::finishing);
        });
        if (head == nullptr && state.load(std::memory_order_acquire) == State::finishing)
        {
            state.store(State::finished);
            return nullptr;
        }
        // safe do wolk
        state_node *cur_op = std::exchange(head, head->next);
        if (head == nullptr)
            tail = nullptr;
        return cur_op;
    }

    void run_loop::push_back(run_loop::state_node *new_node) noexcept // NOLINT
    {
        std::unique_lock lk(mtx);
        if (head == nullptr)
            head = tail = new_node;
        else
        {
            // update
            tail->next = new_node;
            tail = new_node;
        }
        lk.unlock();
        cv.notify_one(); // synchronizes with the pop_front operation
    }

    /**
     * @brief Preconditions: state is one of starting or finishing.
        Effects: If state is starting, sets the state to running, otherwise leaves state
        unchanged. Then, equivalent to:
            while (auto* op = pop-front()) {

                op->execute();

            }

     */
    void run_loop::run() noexcept // NOLINT
    {

        // If state is starting, sets the state to running,or unchanged
        // Note: compare_exchange_strong() return true if state is expected
        if (State expected = State::starting;
            state.load(std::memory_order_acquire) != State::finishing &&
            not state.compare_exchange_strong(expected, State::running,
                                              std::memory_order_acq_rel,
                                              std::memory_order_acquire))
        {
            // 1 、 Preconditions: state is one of starting or finishing.
            std::terminate();
        }

        // run in loop until signal done or finish
        while (auto *op = pop_front())
            op->execute();
    }
    /**
     * @brief
     * Preconditions: state is one of starting or running.
     * Effects: Changes state to finishing.
     * Synchronization: finish synchronizes with the pop-front operation that returns
     * nullptr.
     *
     */
    void run_loop::finish() noexcept // NOLINT
    {
        // Note: notify no depend on lock but finish() is execute atomically.
        std::unique_lock lk(mtx);
        State expected = state.load();
        if (expected != State::starting && expected != State::running)
        {
            // 1 、 Preconditions: state is one of starting or running.
            std::terminate();
        }
        state.store(State::finishing);
        lk.unlock();
        cv.notify_all();
    }

}; // namespace mcs::execution::ctx

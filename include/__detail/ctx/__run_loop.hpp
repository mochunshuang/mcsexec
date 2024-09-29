#pragma once
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
        enum class State
        {
            starting, // NOLINT
            running,  // NOLINT
            finishing // NOLINT
        };

        [[nodiscard]] inline bool is_locked() const // NOLINT
        {
            return this->locked.load(std::memory_order_consume);
        }

      private:
        // [exec.run.loop.types] Associated types
        // models scheduler: run_loop_scheduler
        // models sender: run_loop_scheduler::run_loop_sender

        struct run_loop_opstate_base     // NOLINT
        {                                // exposition only
            virtual void execute() = 0;  // exposition only
            run_loop *loop;              // exposition only // NOLINT
            run_loop_opstate_base *next; // exposition only // NOLINT
        };

        // FIFO
        using Node = run_loop_opstate_base;
        Node *head{nullptr};                       // NOLINT
        Node *tail{nullptr};                       // NOLINT
        std::atomic<State> state{State::starting}; // NOLINT
        std::mutex mtx;                            // NOLINT
        std::condition_variable cv;                // NOLINT
        std::atomic<std::size_t> count{0};         // NOLINT
        std::atomic_bool locked{true};             // NOLINT

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
                auto &o = rcvr;
                if (queries::get_stop_token(o)
                        .stop_requested()) // TODO(mcs): 需要线程池支持
                {
                    recv::set_stopped(std::move(o));
                }
                else
                {
                    recv::set_value(std::move(o));
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
                    auto &o = rcvr;
                    recv::set_error(std::move(o), std::current_exception());
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
                using __tag_t = sender_t;

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
                                 std::is_same_v<Tag, set_error_t>)
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
                    -> run_loop_opstate<std::decay_t<decltype((rcvr))>>
                    requires(recv::receiver_of<decltype((rcvr)), completion_signatures>)
                {
                    return run_loop_opstate{self.run_loop_, std::move(rcvr)};
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
        Node *pop_front() noexcept;      // NOLINT // exposition only
        void push_back(Node *) noexcept; // NOLINT// exposition only

      public:
        // [exec.run.loop.ctor] construct/copy/destroy
        run_loop() noexcept = default;
        run_loop(run_loop &&) = delete;
        run_loop(const run_loop &) = delete;
        run_loop &operator=(run_loop &&) = delete;
        run_loop &operator=(const run_loop &) = delete;
        ~run_loop() noexcept
        {
            if (count.load(std::memory_order_consume) != 0 ||
                state.load(std::memory_order_consume) == State::running)
            {
                std::terminate();
            }
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
     * @brief there are not stop when State::finishing beacuse ~run_loop() check count
     *
     * @return run_loop::Node*
     */
    run_loop::Node *run_loop::pop_front() noexcept // NOLINT
    {
        if (count.load(std::memory_order_consume) == 0 &&
            state.load(std::memory_order_consume) == State::finishing) // no need to wait
            return nullptr;

        std::unique_lock lk(mtx);
        locked.store(true, std::memory_order_release);
        cv.wait(lk, [this] {
            return count.load(std::memory_order_consume) > 0 ||
                   state.load(std::memory_order_consume) == State::finishing;
        });

        if (count == 0)
        {
            lk.unlock();
            locked.store(false, std::memory_order_release);
            cv.notify_one();
            return nullptr;
        }

        Node *op = std::exchange(head, head->next);
        op->next = nullptr; // Ensure the returned task's next pointer is nullptr
        count--;

        lk.unlock();
        locked.store(false, std::memory_order_release);
        cv.notify_one();
        return op;
    }

    void run_loop::push_back(run_loop::Node *item) noexcept // NOLINT
    {
        // TODO(mcs): Define behavior of push_back  when state is finishing
        std::unique_lock lk(mtx);
        locked.store(true, std::memory_order_release);

        if (count.load(std::memory_order_consume) == 0)
        {
            // init
            head = tail = item;
        }
        else
        {
            // update
            tail->next = item;
            tail = item;
        }

        count++;
        tail->next = nullptr; // update chain tail.next

        lk.unlock();
        locked.store(false, std::memory_order_release);
        cv.notify_one(); // synchronizes with the pop_front operation
    }

    void run_loop::run() noexcept // NOLINT
    {
        // expected store  old value when compare_exchange_strong filed
        if (State expected = State::starting; not state.compare_exchange_strong(
                expected, State::running, std::memory_order_acq_rel,
                std::memory_order_acquire))
        {
            if (expected == State::running)
            {
                std::terminate();
            }
        }

        // run in loop until signal done or finish
        while (auto *op = pop_front())
            op->execute();
    }

    void run_loop::finish() noexcept // NOLINT
    {
        std::unique_lock lk(mtx);
        locked.store(true, std::memory_order_release);
        state.store(State::finishing, std::memory_order_release);

        lk.unlock();
        locked.store(false, std::memory_order_release);
        cv.notify_all(); // synchronizes with the pop_front operation
    }

}; // namespace mcs::execution::ctx

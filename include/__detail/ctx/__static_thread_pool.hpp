#pragma once

#include <algorithm>
#include <atomic>
#include <latch>
#include <thread>
#include <random>
#include <array>
#include <execution>

#include "./__thread_context.hpp"

namespace mcs::execution::ctx
{
    template <std::size_t Num>
    class static_thread_pool // NOLINT
    {
        static_assert(Num > 0, " static_thread size must > 0");

        friend class thread_item;

        class thread_item // NOLINT
        {
            std::size_t id;                  // NOLINT
            std::jthread worker;             // NOLINT
            thread_context run_loop;         // NOLINT
            static_thread_pool *thread_pool; // NOLINT

          public:
            void wait_done() noexcept // NOLINT
            {
                run_loop.finish();
                worker.join();
            }

            [[nodiscard]] auto get_scheduler() noexcept -> sched::scheduler auto // NOLINT
            {
                return run_loop.get_scheduler();
            }

            void start(static_thread_pool *pool, std::size_t index) noexcept
            {
                thread_pool = pool;
                id = index;

                std::latch worker_started(1);

                worker = std::jthread{[&](const std::stop_token &stoken) {
                    worker_started.count_down();

                    run_loop.state.store(run_loop::State::running,
                                         std::memory_order_release);
                    while (not stoken.stop_requested() &&
                           run_loop.state.load(std::memory_order_consume) ==
                               run_loop::State::running)
                    {
                        // do work
                        if (run_loop.count.load(std::memory_order_consume) > 0)
                        {
                            if (auto *op = run_loop.pop_front())
                                op->execute();
                        }
                        else
                        {
                            // stealing
                            bool success = try_steal_one();

                            // to sleep unitil push_back notify
                            if (not success &&
                                run_loop.count.load(std::memory_order_consume) == 0)
                            {
                                if (auto *op = run_loop.pop_front())
                                    op->execute();
                            }
                        }
                    }

                    // clear work
                    while (auto *op = run_loop.pop_front())
                        op->execute();
                }};

                worker_started.wait();
            }

          private:
            bool try_steal_one() noexcept // NOLINT
            {
                constexpr auto k_threshold = (Num - 1) / 3;

                for (thread_item &item : thread_pool->pool)
                {
                    if (item.id == id ||
                        item.run_loop.state.load(std::memory_order_consume) ==
                            thread_context::State::finishing)
                        continue;

                    if (item.run_loop.count.load(std::memory_order_consume) >
                            k_threshold &&
                        not item.run_loop.is_locked())
                    {
                        if (auto *op = item.run_loop.pop_front())
                            op->execute();
                        return true;
                    }
                }
                return false;
            }
        };

      public:
        constexpr static_thread_pool() noexcept
        {
            for (std::size_t i = 0; i < pool.size(); i++)
            {
                pool[i].start(this, i);
            }
        }
        ~static_thread_pool() noexcept
        {
            std::latch work_done{Num};
            // Note: par_unseq 不允许分配或解分配内存，使用非免锁的 std::atomic
            // Note: 特化获得互斥体,或者泛言之进行任何向量化不安全的操作
            // Note: 只能使用par，能与锁使用
            std::for_each(std::execution::par, pool.begin(), pool.end(),
                          [&](thread_item &item) {
                              item.wait_done();
                              work_done.count_down();
                          });
            work_done.wait();
        }
        [[nodiscard]] auto get_scheduler() noexcept -> sched::scheduler auto // NOLINT
        {
            return pool[dist(rd)].get_scheduler();
        }

      private:
        std::array<thread_item, Num> pool;                           // NOLINT
        std::random_device rd{};                                     // NOLINT
        std::uniform_int_distribution<std::size_t> dist{0, Num - 1}; // NOLINT
    };

}; // namespace mcs::execution::ctx

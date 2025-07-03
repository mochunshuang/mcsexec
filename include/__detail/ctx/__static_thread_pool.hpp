#pragma once

#include <algorithm>
#include <atomic>
#include <iostream>
#include <latch>
#include <memory>
#include <thread>
#include <random>
#include <array>
#include <execution>

#include "./__thread_context.hpp"
#include "../tool/spin_wait.hpp"

namespace mcs::execution::ctx
{
    template <std::size_t Num>
    class static_thread_pool // NOLINT
    {
        static_assert(Num > 0, " static_thread size must > 0");

        friend class thread_item;

        class thread_item // NOLINT
        {
            friend class static_thread_pool;

            std::size_t id;                  // NOLINT
            std::thread::id t_id;            // NOLINT
            std::jthread worker;             // NOLINT
            thread_context run_loop;         // NOLINT
            static_thread_pool *thread_pool; // NOLINT

          public:
            void wait_done() noexcept // NOLINT
            {
                run_loop.state.store(run_loop::State::finishing,
                                     std::memory_order_release);
                run_loop.cv.notify_one();
                tool::spin_wait spin_wait;
                while (run_loop.state.load(std::memory_order_relaxed) !=
                       run_loop::State::finished)
                    spin_wait.wait();
            }

            [[nodiscard]] auto get_scheduler() noexcept -> sched::scheduler auto // NOLINT
            {
                return run_loop.get_scheduler();
            }

            void start(static_thread_pool *pool, std::size_t index) noexcept
            {
                thread_pool = pool;
                id = index;

                if (not run_loop.state.is_lock_free())
                    std::terminate();

                std::latch worker_started(1);

                // Note: overwrite run_loop::run()
                // Note: can't steal as run_loop is execution resource of thread itself
                worker = std::jthread{[&](const std::stop_token &stoken) {
                    worker_started.count_down();
                    t_id = std::this_thread::get_id();

                    run_loop.state.store(run_loop::State::running);
                    while (not stoken.stop_requested() &&
                           run_loop.state.load() == run_loop::State::running)
                    {
                        // Note: overwrite run_loop::pop_front(). wake by push_back notify
                        std::unique_lock lk(run_loop.mtx);
                        run_loop.cv.wait(lk, [this] {
                            return run_loop.head != nullptr ||
                                   run_loop.state.load() == run_loop::State::finishing;
                        });
                        if (run_loop.head != nullptr)
                        {
                            auto *op = std::exchange(run_loop.head, run_loop.head->next);
                            if (run_loop.head == nullptr)
                                run_loop.tail = nullptr;
                            op->execute();
                        }
                    }
                    // Note: Clean up the remaining work
                    while (not stoken.stop_requested() && run_loop.head != nullptr)
                    {
                        auto *op = std::exchange(run_loop.head, run_loop.head->next);
                        if (run_loop.head == nullptr)
                            run_loop.tail = nullptr;
                        op->execute();
                    }
                    run_loop.state.store(run_loop::State::finished,
                                         std::memory_order_release);
                }};

                worker_started.wait();
            }

            std::thread::id thread_id() const noexcept // NOLINT
            {
                return t_id;
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

        void printInfo() const
        {
            for (std::size_t i = 0; i < pool.size(); i++)
            {
                std::cout << "index: " << i << ", thead_id: " << pool[i].t_id
                          << ", run_loop_address: " << std::addressof(pool[i].run_loop)
                          << '\n';
            }
        }
        constexpr auto &operator[](std::size_t i) noexcept
        {
            return pool[i];
        }
        consteval static auto size() noexcept
        {
            return Num;
        }

      private:
        std::array<thread_item, Num> pool;                           // NOLINT
        std::random_device rd{};                                     // NOLINT
        std::uniform_int_distribution<std::size_t> dist{0, Num - 1}; // NOLINT
    };

}; // namespace mcs::execution::ctx

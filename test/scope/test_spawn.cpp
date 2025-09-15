#include "../test_base_head.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <iostream>

// NOLINTBEGIN
#include <ostream>
#include <vector>
#include <thread>
#include <functional>
#include <queue>
#include <mutex>
#include <condition_variable>

template <std::size_t theads_count>
class ThreadPool
{
    class Thread
    {
      public:
        constexpr Thread() : stop(false)
        {
            thread = std::jthread([this] {
                while (true)
                {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(queue_mutex);
                        condition.wait(lock, [this] { return stop || !tasks.empty(); });
                        if (stop && tasks.empty())
                            return;
                        task = std::move(tasks.front());
                        tasks.pop();
                    }

                    if (id == 0 || id == (theads_count - 1))
                    {
                        auto start = std::chrono::high_resolution_clock::now();
                        task();
                        auto end = std::chrono::high_resolution_clock::now();
                        auto duration =
                            std::chrono::duration_cast<std::chrono::milliseconds>(end -
                                                                                  start);
                        std::cout << "任务id " << id
                                  << ",执行任务耗时: " << duration.count() << " 毫秒"
                                  << '\n';
                    }
                    else
                    {
                        task();
                    }
                }
            });
        }

        constexpr ~Thread() noexcept
        {
            {
                std::unique_lock<std::mutex> lock(queue_mutex);
                stop = true;
            }
            condition.notify_all();
            EXPECT(tasks.empty());
        }

        constexpr void add_task(std::function<void()> &&task)
        {
            {
                std::unique_lock<std::mutex> lock(queue_mutex);
                tasks.push(std::move(task));
            }
            EXPECT(not tasks.empty());
            condition.notify_one();
        }

        constexpr auto setId(int id)
        {
            this->id = id;
        }

      private:
        std::jthread thread;
        std::queue<std::function<void()>> tasks;
        std::mutex queue_mutex;
        std::condition_variable condition;
        bool stop;
        int id;
    };

  public:
    constexpr ThreadPool() : threads(theads_count) // 关键：使用初始化列表
    {
        int i = 0;
        for (auto &t : threads)
        {
            t.setId(i++);
        }
    }

    constexpr void add_task(size_t thread_index, std::function<void()> task)
    {
        if (thread_index < threads.size())
        {
            threads[thread_index].add_task(std::move(task));
        }
        else
        {
            std::cerr << "Thread index out of range!" << std::endl;
        }
    }

  private:
    std::vector<Thread> threads;
};

struct recever_all_but_empty_env
{
    using receiver_concept = mcs::execution::receiver_t;

    template <typename... A> // NOLINTNEXTLINE
    auto set_value(A &&...a) && noexcept -> void
    {
    }

    template <typename E> // NOLINTNEXTLINE
    auto set_error(E &&e) && noexcept -> void
    {
    }

    void set_stopped() && noexcept // NOLINT
    {
    }

    constexpr auto get_env() const noexcept // NOLINT
    {

        return ex::empty_env{};
    }
};

struct inline_scheduler
{
    struct env
    {
        [[nodiscard]] static constexpr inline_scheduler query(
            const ex::queries::get_completion_scheduler_t<ex::set_value_t>
                & /*unused*/) noexcept
        {
            return {};
        }
    };
    template <ex::recv::receiver Receiver>
    struct state
    {
        using operation_state_concept = ex::operation_state_t;
        std::remove_cvref_t<Receiver> receiver; // NOLINT
        void start() & noexcept
        {
            ex::recv::set_value(std::move(receiver));
        }
    };
    struct sender
    {
        using sender_concept = ex::sender_t;
        using completion_signatures =
            ex::cmplsigs::completion_signatures<ex::set_value_t()>;

        [[nodiscard]] env get_env() const noexcept // NOLINT
        {
            return {};
        }
        template <ex::recv::receiver Receiver>
        state<Receiver> connect(Receiver &&receiver) noexcept
        {
            return {std::forward<Receiver>(receiver)};
        }
    };
    static_assert(ex::snd::sender<sender>);

    using scheduler_concept = ex::scheduler_t;
    inline_scheduler() = default;

    static constexpr sender schedule() noexcept
    {
        return {};
    }
    bool operator==(const inline_scheduler &) const = default;
};

struct recever_all_with_inline_scheduler
{
    using receiver_concept = mcs::execution::receiver_t;

    template <typename... A> // NOLINTNEXTLINE
    auto set_value(A &&...a) && noexcept -> void
    {
    }

    template <typename E> // NOLINTNEXTLINE
    auto set_error(E &&) && noexcept -> void
    {
    }

    void set_stopped() && noexcept // NOLINT
    {
    }

    struct env_t
    {
        // template <class Tag>
        //     requires(std::is_same_v<Tag, mcs::execution::set_value_t> ||
        //              std::is_same_v<Tag, mcs::execution::set_stopped_t>)
        // [[nodiscard]] constexpr auto query(
        //     mcs::execution::queries::get_completion_scheduler_t<Tag> /*unused*/)
        //     const noexcept
        // {
        //     return inline_scheduler();
        // }
        [[nodiscard]] constexpr auto query( // NOLINT
            const mcs::execution::queries::get_scheduler_t & /*unused*/) const noexcept
        {
            return inline_scheduler();
        }
    };

    constexpr auto get_env() const noexcept // NOLINT
    {

        return env_t{};
    }
};

int main()
{
    // NOTE: scope 类似 when_all. 当 scope.join()执行完成 可以安全的 离开局部代码块
    TEST("base: ex::spawn + ThreadPool") = [] {
        std::cout << "\n test: [ base: ex::spawn + ThreadPool ]\n";
        ex::counting_scope scope;
        // TODO BUGBUG WITH clang++ //NOTE: 突然不行，代码我都没改过。太离谱
        // NOTE: 单独测试没问题 和 ctest 一起就有问题，太离谱了
        //   	 54 - scope-test_spawn (Exit code 0xc0000409
        constexpr auto k_time = 4; // github action 性能不行
        {
            // NOTE:debug可以 release不可以。一个编译器可以，一个不可。思路：生命周期
            //  NOTE: static 就能解决 Exit code 0xc0000409。 又是 生命周期....
            static ThreadPool<k_time> pool;
            auto start = std::chrono::high_resolution_clock::now();
            {
                auto start = std::chrono::high_resolution_clock::now();
                for (int i = 0; i < k_time; i++) // NOLINT
                {
                    auto task = [i, &scope] {
                        auto sndr = ex::just() | ex::then([i]() noexcept(true) {
                                        if (i == (k_time - 1)) // NOLINT
                                        {
                                            std::this_thread::sleep_for(
                                                std::chrono::milliseconds(auto(k_time)));
                                            std::cout << "work id: " << i << ", done \n";
                                        }
                                        else if (i == 0) // NOLINT
                                        {
                                            std::cout << "start work \n";
                                            std::this_thread::sleep_for(
                                                std::chrono::milliseconds(auto(k_time)));
                                        }
                                        else
                                        {
                                            std::this_thread::sleep_for(
                                                std::chrono::milliseconds(auto(k_time)));
                                        }
                                    });
                        ex::spawn(std::move(sndr), scope.get_token()); // NOLINT
                    };
                    pool.add_task(i, std::move(task));
                }
                auto end = std::chrono::high_resolution_clock::now();
                auto duration =
                    std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
                std::cout << "分发" << k_time << "个任务耗时: " << duration.count()
                          << " 毫秒" << '\n';
            }
            auto end = std::chrono::high_resolution_clock::now();
            auto duration =
                std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
            std::cout << "sync_wait 前耗时: " << duration.count() << " 毫秒" << '\n';

            mcs::this_thread::sync_wait(scope.join());

            end = std::chrono::high_resolution_clock::now();
            duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
            std::cout << "sync_wait 后耗时: " << duration.count() << " 毫秒" << '\n';
        }
    };

    // NOTE: 没有调度器就是阻塞执行
    TEST("base: no ThreadPool no shedule") = []() noexcept {
        std::cout << "\n test: [ base: no ThreadPool no shedule ]\n";
        constexpr auto times = 100;
        ex::counting_scope scope;
        // NOTE:
        for (int i = 0; i < times; ++i)
        {
            auto start = std::chrono::high_resolution_clock::now();
            auto sndr = ex::just() | ex::then([i]() noexcept(true) {
                            if (i % (times / 10) == 0)
                            {
                                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                            }
                        });
            ex::spawn(std::move(sndr), scope.get_token()); // NOLINT
            auto end = std::chrono::high_resolution_clock::now();
            auto duration =
                std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
            if (duration.count() > 1)
            {
                std::cout << "id: " << i << " 耗时超过 1 毫秒: " << duration.count()
                          << '\n';
            }
        }

        // NOTE: 此时 ex::spawn 类似 普通的函数执行。将占用 当前 main 线程
        auto start = std::chrono::high_resolution_clock::now();
        mcs::this_thread::sync_wait(scope.join());
        auto end = std::chrono::high_resolution_clock::now();
        auto duration =
            std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        std::cout << "sync_wait 前后耗时: " << duration.count() << " 毫秒" << '\n';

        EXPECT(duration.count() == 0);
    };

    TEST("on") = [] {
        TEST("on + sync_wait") = [] {
            std::cout << "\n test: [ on + sync_wait ]\n";
            ex::static_thread_pool<3> pool;
            auto on_out_id = std::this_thread::get_id();
            for (int i = 0; i < 100; ++i)
            {
                auto snd = ex::just() | ex::then([=] noexcept {
                               EXPECT(on_out_id != std::this_thread::get_id());
                               return i;
                           });
                auto task = ex::on(pool.get_scheduler(), std::move(snd)) | // NOLINT
                            ex::then([=](int ret) {
                                EXPECT(ret == i);
                                EXPECT(on_out_id == std::this_thread::get_id());
                            });
                mcs::this_thread::sync_wait(std::move(task)); // NOLINT
            }
        };
        TEST("on + option") = [] {
            std::cout << "\n test: [ on + option ]\n";
            ex::static_thread_pool<1> pool;
            int num = 1;
            auto called = false;
            auto snd = ex::just() | ex::then([=] noexcept {
                           std::cout << "just -> then -> on\n";
                           return num;
                       });
            auto task = ex::on(pool.get_scheduler(), std::move(snd)) | // NOLINT
                        ex::then([=, &called](int ret) {
                            EXPECT(ret == num);
                            called = true;
                            std::cout << "on -> then\n";
                        });

            // task.connect(recever_all_but_empty_env{});    //  编译错误
            // NOTE: 提供 queries::get_scheduler 的Env
            auto op = task.connect(recever_all_with_inline_scheduler{}); // OK
            EXPECT(not called);
            ex::opstate::start(op);
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            EXPECT(called);

            {
                called = false;
                auto op = task.connect(recever_all_with_inline_scheduler{}); // OK
                EXPECT(not called);
                op.start();
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                EXPECT(called);
            }
        };
    };
    TEST("spawn + on ") = [] {
        std::cout << "\n test: [ spawn + on ]\n";
        ex::static_thread_pool<1> pool;
        ex::counting_scope scope;

        int num = 1;
        auto called = false;
        auto snd = ex::just() | ex::then([=] noexcept {
                       std::cout << "just -> then -> on\n";
                       return num;
                   });
        auto task = ex::on(pool.get_scheduler(), std::move(snd)) | // NOLINT
                    ex::then([=, &called](int ret) {
                        EXPECT(ret == num);
                        called = true;
                        std::cout << "on -> then\n";
                    });
        ex::spawn(std::move(task), scope.get_token());

        auto start = std::chrono::high_resolution_clock::now();
        mcs::this_thread::sync_wait(scope.join());
        auto end = std::chrono::high_resolution_clock::now();
        auto duration =
            std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        std::cout << "sync_wait 前后耗时: " << duration.count() << " 毫秒" << '\n';
    };
    TEST("spawn + on with parallel work") = [] {
        std::cout << "\n test: [ spawn + on with parallel work]\n";
        ex::static_thread_pool<4> pool;
        auto sch = pool.get_scheduler();
        ex::counting_scope scope;

        constexpr int num = 100;
        std::array<bool, num> check_done;
        check_done.fill(false);

        auto get_work = [](int id) {
            return ex::just(id) | ex::then([](int id) noexcept {
                       std::this_thread::sleep_for(std::chrono::milliseconds(1));
                       return id;
                   });
        };

        for (int i = 0; i < num; ++i)
        {
            ex::spawn(ex::on(sch, get_work(i)) |
                          ex::then([&](auto id) noexcept { check_done[id] = true; }),
                      scope.get_token());
        }
        auto start = std::chrono::high_resolution_clock::now();
        mcs::this_thread::sync_wait(scope.join());
        auto end = std::chrono::high_resolution_clock::now();
        auto duration =
            std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        std::cout << "sync_wait 前后耗时: " << duration.count() << " 毫秒" << '\n';

        for (const auto &v : check_done)
        {
            EXPECT(v);
        }
    };

    TEST("spawn + starts_on with parallel work") = [] {
        std::cout << "\n test: [ spawn + starts_on with parallel work]\n";
        ex::static_thread_pool<4> pool;
        auto sch = pool.get_scheduler();
        ex::counting_scope scope;

        constexpr int num = 100;
        std::array<bool, num> check_done;
        check_done.fill(false);

        auto get_work = [](int id) {
            return ex::just(id) | ex::then([](int id) noexcept {
                       std::this_thread::sleep_for(std::chrono::milliseconds(1));
                       return id;
                   });
        };

        for (int i = 0; i < num; ++i)
        {
            ex::spawn(ex::starts_on(sch, get_work(i)) |
                          ex::then([&](auto id) noexcept { check_done[id] = true; }),
                      scope.get_token());
        }
        auto start = std::chrono::high_resolution_clock::now();
        mcs::this_thread::sync_wait(scope.join());
        auto end = std::chrono::high_resolution_clock::now();
        auto duration =
            std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        std::cout << "sync_wait 前后耗时: " << duration.count() << " 毫秒" << '\n';

        for (const auto &v : check_done)
        {
            EXPECT(v);
        }
    };

    TEST("spawn + continues_on with parallel work") = [] {
        std::cout << "\n test: [ spawn + continues_on with parallel work]\n";
        ex::static_thread_pool<4> pool;
        auto sch = pool.get_scheduler();
        ex::counting_scope scope;

        constexpr int num = 100;
        std::array<bool, num> check_done;
        check_done.fill(false);

        auto get_work = [](int id) {
            return ex::just(id) | ex::then([](int id) noexcept {
                       std::this_thread::sleep_for(std::chrono::milliseconds(1));
                       return id;
                   });
        };

        for (int i = 0; i < num; ++i)
        {
            ex::spawn(ex::continues_on(get_work(i), sch) |
                          ex::then([&](auto id) noexcept { check_done[id] = true; }),
                      scope.get_token());
        }
        auto start = std::chrono::high_resolution_clock::now();
        mcs::this_thread::sync_wait(scope.join());
        auto end = std::chrono::high_resolution_clock::now();
        auto duration =
            std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        std::cout << "sync_wait 前后耗时: " << duration.count() << " 毫秒" << '\n';

        for (const auto &v : check_done)
        {
            EXPECT(v);
        }
    };
    std::cout << "main done\n";
    return 0;
}
// NOLINTEND
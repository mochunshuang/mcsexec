#include "../../include/execution.hpp"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <thread>

#include "__cout_receiver.hpp"
#include "__my_class.hpp"

void base_then();
void base_schedule();
void base_schedule_from();
void base_schedule_from_2();

int main()
{
    // base_then();
    // base_schedule();
    base_schedule_from();
    base_schedule_from_2();
    std::cout << "hello world\n";
    return 0;
}
void base_then()
{
    std::cout << "\n base_then()\n";
    using MyClass = MyClassT<int>;
    using namespace mcs::execution;

    static_thread_pool<3> thread_pool;
    scheduler auto sched = thread_pool.get_scheduler();
    auto start = factories::schedule(sched) | then([] {
                     std::vector<MyClass> partials = {MyClass{0}, MyClass{0}, MyClass{0},
                                                      MyClass{0}, MyClass{0}};

                     std::cout << "\n auto task = then\n";
                     auto task =
                         just(std::move(partials)) | then([](std::vector<MyClass> p) {
                             std::cout << "thread_id: " << std::this_thread::get_id()
                                       << '\n';
                             for (std::size_t i = 0; i < p.size(); ++i)
                             {
                                 p[i].data = i;
                             }
                             return p;
                         });
                     std::cout << "\n mcs::this_thread::sync_wait\n";
                     auto [p] = mcs::this_thread::sync_wait(std::move(task)).value();
                     for (std::size_t i = 0; i < p.size(); ++i)
                     {
                         assert(p[i].data == static_cast<int>(i));
                     }
                     return 0;
                 });
    auto [ret] = mcs::this_thread::sync_wait(std::move(start)).value();
    assert(ret == 0);
    /**
     * @brief 仅仅初始化时调用构造函数：证明了: then => sync_wait 是零开销的
     *  std::vector<MyClass> 从just => then 是合法，符合预期的
     *
     */
}

void base_schedule()
{
    std::cout << "\n base_schedule()\n";
    using namespace mcs::execution;
    static_thread_pool<3> thread_pool;

    auto main_id = std::this_thread::get_id();
    for (int i = 0; i < 1; ++i)
    {
        auto start = factories::schedule(thread_pool.get_scheduler()) |
                     then([&] { assert(std::this_thread::get_id() != main_id); });
        mcs::this_thread::sync_wait(std::move(start));
    }
}

void base_schedule_from()
{
    std::cout << "\n base_schedule_from()\n";
    using MyClass = MyClassT<int>;
    using namespace mcs::execution;
    std::vector<MyClass> partials = {MyClass{0}, MyClass{0}, MyClass{0}, MyClass{0},
                                     MyClass{0}};

    auto task = just(std::move(partials)) | then([](std::vector<MyClass> p) {
                    std::cout << "schedule_from: thread_id: "
                              << std::this_thread::get_id() << '\n';
                    for (std::size_t i = 0; i < p.size(); ++i)
                    {
                        p[i].data = i;
                    }
                    return p;
                });

    // Note:
    static_thread_pool<3> thread_pool;
    thread_pool.printInfo();

    /**
     * @brief 当 std::move(task)在主线程执行完毕，然后调用 thread_pool.get_scheduler()
     *
     */
    // Note:  make-sender(schedule_from, sch, sndr)
    // Note: 可以看出： sch 依赖 sndr
    auto start = adapt::schedule_from(thread_pool.get_scheduler(), std::move(task));

    auto [p] = mcs::this_thread::sync_wait(std::move(start)).value();
    for (std::size_t i = 0; i < p.size(); ++i)
    {
        assert(p[i].data == static_cast<int>(i));
    }
}
void base_schedule_from_2()
{
    std::cout << "\n base_schedule_from_2()\n";
    using MyClass = MyClassT<int>;
    using namespace mcs::execution;
    std::vector<MyClass> partials = {MyClass{0}, MyClass{0}, MyClass{0}, MyClass{0},
                                     MyClass{0}};

    // Note:
    static_thread_pool<3> thread_pool;
    thread_pool.printInfo();

    auto main_id = std::this_thread::get_id();

    // Note: just() => sched.scheduler() => task
    auto start =
        adapt::schedule_from(thread_pool.get_scheduler(), just()) |
        then([&] { return std::move(partials); }) | then([&](std::vector<MyClass> p) {
            std::cout << "schedule_from: thread_id: " << std::this_thread::get_id()
                      << '\n';
            assert(std::this_thread::get_id() != main_id); // Note: id 一定不相等
            for (std::size_t i = 0; i < p.size(); ++i)
            {
                p[i].data = i;
            }
            return p;
        });

    std::cout << "\n sync_wait: \n";
    auto [p] = mcs::this_thread::sync_wait(std::move(start)).value();
    for (std::size_t i = 0; i < p.size(); ++i)
    {
        assert(p[i].data == static_cast<int>(i));
    }
    /**
     * @brief 从结果看出：sndr => schedule_from => next_sndr 满足0抽象开销
     *
     */
}
#include "../../include/execution.hpp"
#include "__my_class.hpp"

#include <iostream>

void base_continues_on();
int main()
{
    base_continues_on();
    std::cout << "hello world\n";
    return 0;
}
void base_continues_on()
{
    std::cout << "\n base_continues_on()\n";
    using MyClass = MyClassT<int>;
    using namespace mcs::execution;
    std::vector<MyClass> partials = {MyClass{0}, MyClass{0}, MyClass{0}, MyClass{0},
                                     MyClass{0}};

    // Note:
    static_thread_pool<3> thread_pool;
    thread_pool.printInfo();

    auto main_id = std::this_thread::get_id();

    // Note: just() => sched.scheduler() => task
    // 指定 just() 完成后  get_scheduler 启动开始start => then
    auto start =
        adapt::continues_on(just(), thread_pool.get_scheduler()) |
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
     * @brief 从结果看出：sndr => continues_on => next_sndr 满足0抽象开销
     *
     */
}
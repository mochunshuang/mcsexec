#include "../../include/execution.hpp"

#include "__cout_receiver.hpp"
#include "__my_class.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iostream>

using namespace mcs::execution;

// 第一个函数，接受一个 std::vector<double> 并返回它
auto first_function(std::vector<double> vec)
{
    std::cout << "first_function: ";
    for (std::size_t i = 0; i < vec.size(); ++i)
    {
        vec[i] += i;
    }
    return std::move(vec);
}

// 第二个函数，接受一个 std::vector<double> 并打印它
void second_function(std::vector<double> vec)
{
    std::cout << "second_function: ";
    for (const auto &val : vec)
    {
        std::cout << val << " ";
    }
    std::cout << std::endl;
}
void base()
{
    std::vector<double> partials = {0, 1, 2, 3, 4};
    second_function(first_function(std::move(partials)));
}
void base_test();
void base_test_2();
void base_test_3();
void base_test_4();
void base_test_5();
int main()
{
    base();
    base_test();
    base_test_2();
    base_test_3();
    base_test_4();
    base_test_5();
    std::cout << "hello world\n";
    return 0;
}

void base_test()
{
    //
    std::vector<double> partials = {0, 1, 2, 3, 4};
    auto task = then(just(std::move(partials)), [](std::vector<double> p) {
        // Note: 不修改不会异常
        std::cout << std::this_thread::get_id() << " I am running on cpu_ctx\n";
        return std::move(p);
    });
    using T = decltype(task.get_completion_signatures(empty_env{}));
    static_assert(
        std::is_same_v<
            completion_signatures<mcs::execution::recv::set_value_t(std::vector<double>)>,
            T>);
    std::cout << "\n connect\n";
    auto op = conn::connect(std::move(task), cout_receiver{}); // no copy
    opstate::start(op);
}

void base_test_2()
{
    std::vector<double> partials = {0, 1, 2, 3, 4};
    auto task = then(just(std::move(partials)), [](std::vector<double> p) {
        std::cout << std::this_thread::get_id() << " I am running on cpu_ctx\n";
        for (std::size_t i = 0; i < p.size(); ++i)
        {
            p[i] = i;
        }
        return std::move(p);
    });
    using T = decltype(task.get_completion_signatures(empty_env{}));
    static_assert(
        std::is_same_v<
            completion_signatures<mcs::execution::recv::set_value_t(std::vector<double>)>,
            T>);
    std::cout << "\n connect\n";
    auto op = conn::connect(std::move(task), cout_receiver{}); // no copy
    opstate::start(op);
}
void base_test_3()
{
    using MyClass = MyClassT<int>;
    using namespace mcs::execution;
    std::vector<MyClass> partials = {MyClass{0}, MyClass{0}, MyClass{0}, MyClass{0},
                                     MyClass{0}};

    std::cout << "\n auto task = then\n";
    auto task = just(std::move(partials)) | then([](std::vector<MyClass> p) {
                    std::cout << "thread_id: " << std::this_thread::get_id() << '\n';
                    for (std::size_t i = 0; i < p.size(); ++i)
                    {
                        p[i].data = i;
                    }
                    return p;
                });
    std::cout << "\n connect\n";
    auto op = conn::connect(std::move(task), cout_receiver{}); // no copy
    opstate::start(op);
    /**
     * @brief 仅仅初始化时调用构造函数：证明了。算法/框架 是零开销的。同时地址是对应上的
     *
     */
}
void base_test_4()
{
    using MyClass = MyClassT<int>;
    using namespace mcs::execution;
    std::vector<MyClass> partials = {MyClass{0}, MyClass{0}, MyClass{0}, MyClass{0},
                                     MyClass{0}};

    std::cout << "\n auto task = then\n";
    auto task = just(std::move(partials)) | then([](std::vector<MyClass> p) {
                    std::cout << "thread_id: " << std::this_thread::get_id() << '\n';
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
    /**
     * @brief 仅仅初始化时调用构造函数：证明了: then => sync_wait 是零开销的
     *  std::vector<MyClass> 从just => then 是合法，符合预期的
     *
     */
}
void base_test_5()
{
    std::cout << "\n base_test_5()\n";
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
}
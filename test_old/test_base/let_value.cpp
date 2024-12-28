#include "../../include/execution.hpp"
#include "__my_class.hpp"

#include <cassert>
#include <iostream>
#include <stdexcept>

namespace stdexec = mcs::execution;

stdexec::sender auto error_to_response(std::exception_ptr err)
{
    try
    {
        std::rethrow_exception(err);
    }
    catch (const std::invalid_argument &e)
    {
        return stdexec::just(404);
    }
    catch (const std::exception &e)
    {
        return stdexec::just(500);
    }
    catch (...)
    {
        // return stdexec::just(http_response{500, "Unknown server error"});
        return stdexec::just(-1);
    }
}

void base();
void base_test();
void base_test_2();

void base_let_error();

int main()
{
    base();
    base_test();
    base_test_2();
    base_let_error();
    std::cout << "hello world\n";
    return 0;
}
void base()
{
    std::cout << "\n base()\n";
    using namespace mcs::execution;
    std::vector<double> partials = {0, 0, 0, 0, 0};
    auto fun = [&](int i) -> sender auto {
        assert(i == 1);
        return just(std::move(partials)) | then([](std::vector<double> &&p) {
                   std::cout << std::this_thread::get_id()
                             << " I am running on cpu_ctx\n";
                   for (std::size_t i = 0; i < p.size(); ++i)
                   {
                       p[i] = i;
                   }
                   return std::move(p);
               });
        ;
    };
    sender auto task = let_value(just(1), fun);

    std::cout << "sync_wait: \n";
    auto [p] = mcs::this_thread::sync_wait(task).value();
    for (std::size_t i = 0; i < p.size(); ++i)
    {
        assert(p[i] == i);
    }
}
void base_test()
{
    std::cout << "\n base_test\n";

    using namespace mcs::execution;
    static_thread_pool<3> thread_pool;
    scheduler auto sch = thread_pool.get_scheduler();
    std::vector<double> partials = {0, 0, 0, 0, 0};
    auto fun = [&]() -> sender auto {
        return just(std::move(partials)) | then([](std::vector<double> &&p) {
                   std::cout << std::this_thread::get_id()
                             << " I am running on cpu_ctx\n";
                   for (std::size_t i = 0; i < p.size(); ++i)
                   {
                       p[i] = i;
                   }
                   return std::move(p);
               });
        ;
    };
    sender auto task = let_value(schedule(sch), fun);

    std::cout << "sync_wait: \n";
    auto [p] = mcs::this_thread::sync_wait(task).value();
    for (std::size_t i = 0; i < p.size(); ++i)
    {
        assert(p[i] == i);
    }
}

void base_test_2()
{
    std::cout << "\n base_test_2\n";

    using namespace mcs::execution;
    static_thread_pool<3> thread_pool;
    scheduler auto sch = thread_pool.get_scheduler();

    using MyClass = MyClassT<int>;

    std::vector<MyClass> partials = {MyClass{0}, MyClass{0}, MyClass{0}, MyClass{0},
                                     MyClass{0}};
    auto fun = [&]() -> sender auto {
        return just(std::move(partials)) | then([](std::vector<MyClass> &&p) {
                   std::cout << std::this_thread::get_id()
                             << " I am running on cpu_ctx\n";
                   for (std::size_t i = 0; i < p.size(); ++i)
                   {
                       p[i].data = i;
                   }
                   return std::move(p);
               });
        ;
    };
    sender auto task = let_value(factories::schedule(sch), fun);

    std::cout << "sync_wait: \n";
    auto [p] = mcs::this_thread::sync_wait(task).value();
    for (std::size_t i = 0; i < p.size(); ++i)
    {
        assert(p[i].data == i);
    }
    /**
     * @brief 从结果看出：schedule(sch) => call cun => then... 是 0抽象开销的
     *
     */
}

void base_let_error()
{
    std::cout << "\n base_let_error\n";

    using namespace mcs::execution;
    static_thread_pool<3> thread_pool;
    scheduler auto sch = thread_pool.get_scheduler();

    int count = 0;
    auto fun = [&]() -> sender auto {
        return factories::schedule(sch) //
               | then([&]() {
                     std::cout << std::this_thread::get_id() << " I am running on sch\n";
                     if (count == 1)
                         throw std::invalid_argument{"count ==1"};
                     return 200;
                 })                             //
               | let_error(&error_to_response); // Note: 变成业务统一返回值类型
    };
    sender auto task = let_value(factories::schedule(sch), fun);

    std::cout << "sync_wait: \n";
    for (int i = 0; i < 2; ++i)
    {
        auto [p] = mcs::this_thread::sync_wait(task).value();
        std::cout << "ret: " << p << '\n';
        count++;
    }
    /**
     * @brief Note: 需要 let_* 统一基类的返回值
     *
     */
}
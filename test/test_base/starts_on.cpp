#include "../../include/execution.hpp"

#include "__cout_receiver.hpp"
#include "__my_class.hpp"

#include <algorithm>
#include <iostream>
#include <utility>

void base();

void base_test();

int main()
{
    // base();
    base_test();

    std::cout << "\nhello world\n";
    return 0;
}
void base()
{
    std::cout << "\n base\n";

    using namespace mcs::execution;
    static_thread_pool<3> thread_pool;
    scheduler auto sch = thread_pool.get_scheduler();

    using MyClass = MyClassT<int>;

    std::cout << "copy\n";
    {
        std::vector<MyClass> partials = {MyClass{0}, MyClass{0}, MyClass{0}, MyClass{0},
                                         MyClass{0}};

        sender auto out_sndr =
            just(std::move(partials)) | then([](std::vector<MyClass> &&p) {
                std::cout << std::this_thread::get_id() << " I am running on cpu_ctx\n";
                for (std::size_t i = 0; i < p.size(); ++i)
                {
                    p[i].data = i;
                }
                return std::move(p);
            });
        // copy
        sender auto task =
            let_value(factories::schedule(sch), [sndr = out_sndr]() mutable {
                std::cout << "std::move(sndr) start\n";
                return std::move(sndr);
            });

        std::cout << "sync_wait: \n";
        auto [p] = mcs::this_thread::sync_wait(task).value();
        for (std::size_t i = 0; i < p.size(); ++i)
        {
            assert(p[i].data == i);
        }
        /**
         * @brief 从结果知道存在复制
         *
         */
    }
    std::cout << "\nmove\n";
    {
        std::vector<MyClass> partials = {MyClass{0}, MyClass{0}, MyClass{0}, MyClass{0},
                                         MyClass{0}};

        sender auto out_sndr =
            just(std::move(partials)) | then([](std::vector<MyClass> &&p) {
                std::cout << std::this_thread::get_id() << " I am running on cpu_ctx\n";
                for (std::size_t i = 0; i < p.size(); ++i)
                {
                    p[i].data = i;
                }
                return std::move(p);
            });
        // copy
        sender auto task =
            let_value(factories::schedule(sch), [sndr = std::move(out_sndr)]() mutable {
                std::cout << "std::move(sndr) start\n";
                return std::move(sndr);
            });

        std::cout << "sync_wait: \n";
        auto [p] = mcs::this_thread::sync_wait(task).value();
        for (std::size_t i = 0; i < p.size(); ++i)
        {
            assert(p[i].data == i);
        }
        /**
         * @brief 从结果知道存在复制 。 sndr = std::move(out_sndr) 又如何
         *
         */
    }

    std::cout << "\nmove decltype(auto)\n";
    {
        std::vector<MyClass> partials = {MyClass{0}, MyClass{0}, MyClass{0}, MyClass{0},
                                         MyClass{0}};

        sender auto out_sndr =
            just(std::move(partials)) | then([](std::vector<MyClass> &&p) {
                std::cout << std::this_thread::get_id() << " I am running on cpu_ctx\n";
                for (std::size_t i = 0; i < p.size(); ++i)
                {
                    p[i].data = i;
                }
                return std::move(p);
            });
        // copy
        sender auto task =
            let_value(factories::schedule(sch),
                      [sndr = std::move(out_sndr)]() mutable -> decltype(auto) {
                          std::cout << "std::move(sndr) start\n";
                          return std::move(sndr);
                      });

        std::cout << "sync_wait: \n";
        auto [p] = mcs::this_thread::sync_wait(task).value();
        for (std::size_t i = 0; i < p.size(); ++i)
        {
            assert(p[i].data == i);
        }
        /**
         * @brief 从结果知道存在复制 。 sndr = std::move(out_sndr) + decltype(auto) 又如何
         *
         */
    }
    /**
     * @brief 从结果看出：理论上，starts_on 转化成let_value; 不应该有问题
     *
     */
}

void base_test()
{
    std::cout << "\n base_test\n";

    using namespace mcs::execution;
    static_thread_pool<3> thread_pool;
    scheduler auto sch = thread_pool.get_scheduler();
    thread_pool.printInfo();

    using MyClass = MyClassT<int>;
    {
        //
        std::vector<MyClass> partials = {MyClass{0}, MyClass{0}, MyClass{0}, MyClass{0},
                                         MyClass{0}};

        sender auto out_sndr =
            just(std::move(partials)) | then([](std::vector<MyClass> &&p) {
                std::cout << std::this_thread::get_id() << " I am running on cpu_ctx\n";
                for (std::size_t i = 0; i < p.size(); ++i)
                {
                    p[i].data = i;
                }
                return std::move(p);
            });
        auto task = starts_on(sch, std::move(out_sndr));
        std::cout << "sync_wait: \n";
        auto [p] = mcs::this_thread::sync_wait(std::move(task)).value(); // Note: 一样复制
        for (std::size_t i = 0; i < p.size(); ++i)
        {
            assert(p[i].data == (int)i);
        }
    }
    /**
     * @brief 无法0抽象开销：存在复制。 原因是lambda 捕获是复制。= 右边是左值
     *
     */
    // Note: 标准是这么设计的，或许可以考虑优化。 可以优化，目前符合0抽象开销
    int &&a = 0;
    static_assert(std::is_same_v<decltype((a)), int &>); // Note: 原因
    [](int &&a) {
        static_assert(std::is_same_v<decltype((a)), int &>); // Note: 原因
    }(int{0});
}
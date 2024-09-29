#include "../../include/execution.hpp"
#include "__my_class.hpp"

#include <algorithm>
#include <iostream>

void base();
void base_test();
int main()
{
    base();
    base_test();
    std::cout << "hello world\n";
    return 0;
}
void base()
{
    std::cout << "\nbase\n";
    using namespace mcs::execution;

    static_thread_pool<3> cpu_ctx;
    {
        auto task = just() | then([]() {
                        std::cout << std::this_thread::get_id()
                                  << " I am running on cpu_ctx!\n";
                    });
        [[maybe_unused]] auto t = on(cpu_ctx.get_scheduler(), task) | then([]() {
                                      std::cout << std::this_thread::get_id()
                                                << " I am running on main thead!\n";
                                  });

        mcs::this_thread::sync_wait(t);
    }
    std::cout << std::this_thread::get_id() << " I am running on main()\n";
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

        sender auto task = just(std::move(partials)) | then([](std::vector<MyClass> &&p) {
                               std::cout << std::this_thread::get_id()
                                         << " I am running on cpu_ctx\n";
                               for (std::size_t i = 0; i < p.size(); ++i)
                               {
                                   p[i].data = static_cast<int>(i);
                               }
                               return std::move(p);
                           });
        [[maybe_unused]] auto t =
            on(sch, std::move(task)) | then([](std::vector<MyClass> &&p) {
                std::cout << std::this_thread::get_id()
                          << " I am running on main thead!\n";
                return std::move(p);
            });

        std::cout << "sync_wait: \n";
        auto [p] = mcs::this_thread::sync_wait(std::move(t)).value();
        for (std::size_t i = 0; i < p.size(); ++i)
        {
            assert(p[i].data == (int)i);
        }
        /**
         * @brief 存在大量的赋值，是否是有必要的呢？
         *
         */
        // Note:  确实能优化。目前从结构上来看,符合0抽象开销
    }
}
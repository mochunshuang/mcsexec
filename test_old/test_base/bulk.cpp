#include "../../include/execution.hpp"
#include "__my_class.hpp"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <utility>

void base();

int main()
{
    base();
    std::cout << "hello world\n";
    return 0;
}

void base()
{
    std::cout << "\n base\n";

    using namespace mcs::execution;
    static_thread_pool<3> thread_pool;
    scheduler auto sch = thread_pool.get_scheduler();
    thread_pool.printInfo();

    using MyClass = MyClassT<int>;
    std::vector<MyClass> partials = {MyClass{0}, MyClass{0}, MyClass{0}, MyClass{0},
                                     MyClass{0}};
    int n = static_cast<int>(partials.size());
    assert(n == 5);

    auto task = just(std::move(partials)) | then([](std::vector<MyClass> &&p) {
                    std::cout << "tid: " << std::this_thread::get_id()
                              << " , I am running on cpu_ctx\n";
                    return std::move(p);
                });

    auto start = starts_on(sch, std::move(task));

    {
        // Note: args is a pack of lvalue subexpressions referring to the value completion
        auto f = [](int i, std::vector<MyClass> &p) -> decltype(auto) {
            std::cout << "times:  " << i << " call:  \n";
            p[i].data = i;
            return p;
        };
        auto snd =
            bulk(std::move(start), n, std::move(f)) | then([](std::vector<MyClass> p) {
                for (auto &i : p)
                {
                    i.data += 1; // change one time
                }
                std::cout << "tid: " << std::this_thread::get_id()
                          << " , bulk-then : only once\n";
                return p;
            });
        std::cout << "sync_wait: \n";
        auto [p] = mcs::this_thread::sync_wait(std::move(snd)).value();
        for (std::size_t i = 0; i < p.size(); ++i)
        {
            assert(p[i].data == (int)(i + 1));
            std::cout << p[i].data << ' ';
        }
        std::cout << "\nsync_wait: end\n";
    }
    /**
     * @brief 重结果上看。重复执行了 5 次，且符合0抽象开销
     *
     */
}
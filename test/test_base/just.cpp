#include "../../include/execution.hpp"

#include "__cout_receiver.hpp"
#include "__my_class.hpp"
#include <cassert>
#include <exception>
#include <iostream>
#include <tuple>
#include <type_traits>

using MyClass = MyClassT<int>;

void base_test();
void connect_test();
void just_error_test();
void just_stopped_test();
void just_with_sched();
int main()
{
    base_test();
    connect_test();
    just_error_test();
    just_stopped_test();
    just_with_sched();
    std::cout << "hello world\n";
    return 0;
}
void base_test()
{
    using namespace mcs::execution;
    // Note: 发生了构造 + 拷贝
    std::vector<MyClass> partials = {MyClass{0}, MyClass{1}, MyClass{2}, MyClass{3}};
    std::cout << "call  just\n";
    // Note: 什么都没发生
    auto task = just(std::move(partials));
    {
        // Note: 什么都没发生
        std::cout << "  print\n";
        for (const auto &obj : partials)
        {
            obj.print();
        }
    }
    // Note: 什么都没发生
    auto &[_, data] = task;
    static_assert(std::is_same_v<
                  decltype(data),
                  mcs::execution::snd::__detail::product_type<std::vector<MyClass>>>);

    // Note: 什么都没发生
    auto &&[__, data2] = task;
    static_assert(std::is_same_v<
                  decltype(data2),
                  mcs::execution::snd::__detail::product_type<std::vector<MyClass>>>);

    // 测试tuple
    std::cout << "  tuple\n";
    {
        std::vector<MyClass> partials = {MyClass{0}, MyClass{1}, MyClass{2}, MyClass{3}};
        std::cout << "call  std::tuple\n";
        // auto tp = std::tuple<factories::__just_t<set_value_t>, std::vector<MyClass>>{
        //     {}, std::move(partials)}; // 发生复制
        auto tp = std::make_tuple(factories::__just_t<set_value_t>{},
                                  std::move(partials)); // 使用 std::make_tuple，避免复制

        // Note: 什么都没发生
        auto &[_, data] = tp;
        // Note: 什么都没发生
        auto &&[__, data2] = tp;
        static_assert(std::is_same_v<decltype(data), std::vector<MyClass>>);
        static_assert(std::is_same_v<decltype(data2), std::vector<MyClass>>);
    }
}
void connect_test()
{
    using namespace mcs::execution;
    std::vector<MyClass> partials = {MyClass{0}, MyClass{1}, MyClass{2}, MyClass{3}};
    auto task = just(std::move(partials));
    using T = decltype(task.get_completion_signatures(empty_env{}));
    static_assert(std::is_same_v<completion_signatures<mcs::execution::recv::set_value_t(
                                     std::vector<MyClass>)>,
                                 T>);

    std::cout << "\nbefore connect\n";
    auto &[_, data] = task;
    auto &[val] = data;
    for (const auto &obj : val)
    {
        obj.print();
    }
    std::cout << "\n connect\n";
    auto op = conn::connect(std::move(task), cout_receiver{}); // no copy
    // auto op = task.connect(cout_receiver{});

    static_assert(recv::receiver<cout_receiver>);

    op.start();

    {
        auto task = just();
        auto op = conn::connect(std::move(task), cout_receiver{});
        op.start();
    }
}

void just_error_test()
{
    using namespace mcs::execution;

    try
    {
        throw std::bad_exception{};
    }
    // catch (std::exception &e)
    catch (...) // Note: 唯一匹配 std::current_exception()
    {
        // Note: 需要转化为ptr
        // Note: 丢失具体的异常信息：bad_exception 上游是不会匹配的
        // auto task = just_error(std::make_exception_ptr(e));
        auto task = just_error(std::current_exception()); // Note: return exception_ptr

        using T = decltype(task.get_completion_signatures(empty_env{}));
        auto op = conn::connect(std::move(task), cout_receiver{});
        opstate::start(op);
    }
}
void just_stopped_test()
{
    using namespace mcs::execution;
    recv::set_stopped(cout_receiver{});
}

void just_with_sched()
{
    using namespace mcs::execution;
    static_thread_pool<3> thread_pool;
    scheduler auto sched = thread_pool.get_scheduler();
    std::vector<MyClass> partials = {MyClass{0}, MyClass{1}, MyClass{2}, MyClass{3}};

    // Note: 不匹配。没有依赖关系
    //  auto task =factories::schedule(sched)  just(std::move(partials));
}
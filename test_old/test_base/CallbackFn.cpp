#include <cassert>
#include <iostream>
#include <utility>
#include <thread>

// 基类仿函数
class BaseCallbackFn
{
  public:
    void operator()() const;
};

void BaseCallbackFn::operator()() const
{
    std::cout << "CallbackFn::operator() called" << std::endl;
}

// 派生类仿函数，没有重载 operator()
class CallbackFn : public BaseCallbackFn
{

    // CallbackFn a; // 智能使用指针
};

struct A
{
    int v{};
};

int main()
{
    CallbackFn callback_fn;

    // 调用基类的 operator()
    std::forward<CallbackFn>(callback_fn)();

    //
    [](const A &a) {
        // a.v = 1; // 不允许
        (void)a;
    }(A{});

    // id
    std::thread::id id{};
    std::cout << " std::thread::id: " << id << '\n';
    {
        std::thread::id id;
        std::cout << " std::thread::id: " << id << '\n';
    }

    std::jthread t([]() {
        std::jthread::id id{};
        std::cout << " std::thread::id: " << id << '\n';
    });

    {
        /**
         * @brief 必须 std::this_thread::get_id()初始化id 才是合法的
         *
         */
        std::thread::id id = std::this_thread::get_id();
        std::cout << " std::thread::id: " << id << '\n';

        std::jthread t([]() {
            std::jthread::id id = std::this_thread::get_id();
            std::cout << " std::thread::id: " << id << '\n';
        });
    }
    {
        std::atomic<bool> stopped{};
        assert(stopped.load() == false);

        // 原子地替换 stopped 的值为 true，并返回旧值
        bool old_value = stopped.exchange(true);

        std::cout << "Old value: " << std::boolalpha << old_value << std::endl;
        std::cout << "New value: " << stopped.load() << std::endl;
        {
            bool old_value = stopped.exchange(true);

            std::cout << "Old value: " << std::boolalpha << old_value << std::endl;
            std::cout << "New value: " << stopped.load() << std::endl;
        }
    }

    return 0;
}
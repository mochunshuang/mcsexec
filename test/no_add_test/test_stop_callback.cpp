#include <cassert>
#include <iostream>
#include <stop_token>

// NOLINTBEGIN

// 测试函数：验证 stop_callback 的行为
void test_stop_callback()
{
    std::stop_source stop_source;

    // 用于记录回调是否被调用
    bool callback1_called = false;
    bool callback2_called = false;
    bool callback3_called = false;

    // 注册回调（在 request_stop 前注册）
    std::stop_callback cb0(stop_source.get_token(), [&]() {
        callback1_called = true;
        std::cout << "Callback 0: Stop requested!" << std::endl;
    });
    std::stop_callback cb1(stop_source.get_token(), [&]() {
        callback1_called = true;
        std::cout << "Callback 1: Stop requested!" << std::endl;
    });

    // cb100 注册，然后取消注册
    {
        std::stop_callback cb100(stop_source.get_token(), [&]() {
            callback1_called = true;
            std::cout << "Callback 100: Stop requested!" << std::endl;
        });
    }

    // 第一次调用 request_stop
    assert(stop_source.request_stop());
    std::cout << "      stop_source.request_stop()" << std::endl;

    // （在 request_stop 后注册）
    std::stop_callback cb2(stop_source.get_token(), [&]() {
        callback2_called = true;
        std::cout << "Callback 2: Stop requested!" << std::endl;
    });

    // 第二次调用 request_stop
    assert(not stop_source.request_stop());
    std::cout << "      stop_source.request_stop()" << std::endl;

    // （在 request_stop 后注册）
    std::stop_callback cb3(stop_source.get_token(), [&]() {
        callback3_called = true;
        std::cout << "Callback 3: Stop requested!" << std::endl;
    });
    std::stop_callback cb4(stop_source.get_token(), [&]() {
        callback3_called = true;
        std::cout << "Callback 4: Stop requested!" << std::endl;
    });

    // 验证回调是否被调用
    assert(callback1_called); // 回调1应在 request_stop 后调用
    assert(callback2_called); // 回调2应在注册时立即调用
    assert(callback3_called); // 回调3应在注册时立即调用

    std::cout << "All callbacks executed successfully!" << std::endl;
}

void test_stop_callback_not_auto_call()
{
    bool called{false};
    {
        std::stop_source stop_source;
        std::stop_token stop_token = stop_source.get_token();
        std::stop_callback cb(stop_token, [&] { called = true; });
    }
    assert(not called);
}
int main()
{
    test_stop_callback();
    test_stop_callback_not_auto_call();
    return 0;
}
// NOLINTEND
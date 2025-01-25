#include <cassert>
#include <iostream>

#include "../test_base_head.hpp"

// NOLINTBEGIN

// 测试函数：验证 stop_callback 的行为
void test_stop_callback()
{
    ex::inplace_stop_source stop_source;

    // 用于记录回调是否被调用
    bool callback0_called = false;
    bool callback1_called = false;
    bool callback2_called = false;
    bool callback3_called = false;
    bool callback4_called = false;

    bool callback100_called = false;

    // 注册回调（在 request_stop 前注册）
    ex::inplace_stop_callback cb0(stop_source.get_token(), [&]() {
        callback0_called = true;
        std::cout << "Callback 0: Stop requested!" << '\n';
    });
    ex::inplace_stop_callback cb1(stop_source.get_token(), [&]() {
        callback1_called = true;
        std::cout << "Callback 1: Stop requested!" << '\n';
    });

    // cb100 注册，然后取消注册
    {
        ex::inplace_stop_callback cb100(stop_source.get_token(), [&]() {
            callback100_called = true;
            std::cout << "Callback 100: Stop requested!" << '\n';
        });
    }

    // 第一次调用 request_stop
    assert(stop_source.request_stop());
    std::cout << "      stop_source.request_stop()" << '\n';

    // （在 request_stop 后注册）
    ex::inplace_stop_callback cb2(stop_source.get_token(), [&]() {
        callback2_called = true;
        std::cout << "Callback 2: Stop requested!" << '\n';
    });

    // 第二次调用 request_stop
    assert(not stop_source.request_stop());
    std::cout << "      stop_source.request_stop()" << '\n';

    // （在 request_stop 后注册）
    ex::inplace_stop_callback cb3(stop_source.get_token(), [&]() {
        callback3_called = true;
        std::cout << "Callback 3: Stop requested!" << '\n';
    });
    ex::inplace_stop_callback cb4(stop_source.get_token(), [&]() {
        callback4_called = true;
        std::cout << "Callback 4: Stop requested!" << '\n';
    });

    EXPECT(callback0_called); // 回调0应在 request_stop 后调用
    EXPECT(callback1_called); // 回调1应在 request_stop 后调用
    EXPECT(callback2_called); // 回调2应在注册时立即调用
    EXPECT(callback3_called); // 回调3应在注册时立即调用
    EXPECT(callback4_called); // 回调3应在注册时立即调用

    EXPECT(not callback100_called); // 不会被调用
}

void test_stop_callback_not_auto_call()
{
    bool called{false};
    {
        ex::inplace_stop_source stop_source;
        ex::inplace_stop_token stop_token = stop_source.get_token();
        ex::inplace_stop_callback cb(stop_token, [&] { called = true; });
    }
    EXPECT(not called);
}
int main()
{
    TEST("test_stop_callback") = [] {
        test_stop_callback();
    };
    TEST("test_stop_callback_not_auto_call") = [] {
        test_stop_callback_not_auto_call();
    };
    return 0;
}
// NOLINTEND
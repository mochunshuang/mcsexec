#include <cassert>
#include <iostream>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <winsock2.h>
#include <mswsock.h>
#include <windows.h>

struct PER_IO_CONTEXT
{
    OVERLAPPED overlapped{}; // NOLINT
    double a;                // NOLINT
    std::string str;         // NOLINT
    // PER_IO_CONTEXT() : a{1}, str("string") {}
    explicit PER_IO_CONTEXT(double a = 1, std::string str = "string")
        : a{a}, str{std::move(str)}
    {
    }
};

struct RW_IO_CONTEXT : public PER_IO_CONTEXT
{
    int thread_id; // NOLINT

    RW_IO_CONTEXT() : PER_IO_CONTEXT(0, ""), thread_id{3} {}

    RW_IO_CONTEXT &operator=(const PER_IO_CONTEXT &per_io_context)
    {
        // 将 PER_IO_CONTEXT 的成员赋值给 RW_IO_CONTEXT
        *static_cast<PER_IO_CONTEXT *>(this) = per_io_context;
        // thread_id 不会被赋值，保持原值
        return *this;
    }
};

int main()
{
    PER_IO_CONTEXT ctx{2, "string"};
    RW_IO_CONTEXT io;
    io.thread_id = -2;
    assert(io.a == 0);
    assert(io.str == "");

    // 赋值操作
    io = ctx;

    assert(io.a == 2);
    assert(io.str == "string");
    assert(io.thread_id == -2);

    {
        auto *ctx = new PER_IO_CONTEXT{2, "string"};
        auto *io = new RW_IO_CONTEXT;
        io->thread_id = -2;
        assert(io->a == 0);
        assert(io->str == "");
        // io = *ctx; // 错误
        *io = *ctx; // 正确，调用 RW_IO_CONTEXT::operator=

        assert(io->a == 2);
        assert(io->str == "string");
        assert(io->thread_id == -2);
        // 单继承（无虚函数）
        {
            auto *parent =
                reinterpret_cast<PER_IO_CONTEXT *>(&io); // 可能正常工作 // NOLINT
            // Note: 不符合预期。 reinterpret_cast 仅仅用于处理 C 相关的代码，没有继承
            assert(parent->a != 2);
            assert(parent->str != "string");
        }
        {
            auto *parent = static_cast<PER_IO_CONTEXT *>(io); // 正确
            // Note: 符合预期。 有继承 不能使用  reinterpret_cast
            assert(parent->a == 2);
            assert(parent->str == "string");

            // 赋值,由父类赋值
            auto *child = new RW_IO_CONTEXT;
            parent->overlapped.Internal = 1;
            assert(child->overlapped.Internal == 0);

            *child = *parent; // 赋值

            assert(child->a == parent->a);
            assert(child->str == parent->str);
            assert(child->a == 2);
            assert(child->str == "string");

            // 测试 overlapped 是不是一样的复制
            assert(child->overlapped.Internal == parent->overlapped.Internal);
            assert(child->overlapped.Internal == 1);
            delete child;
        }
        {
            // parent -> overlp -> parent2 then *child = *parent2
            auto *parent = new PER_IO_CONTEXT{2, "string"};
            auto *child = new RW_IO_CONTEXT;
            // 转化 overlp
            auto *overlp = reinterpret_cast<OVERLAPPED *>(parent); // NOLINT
            // 转成 child
            auto *parent2 = reinterpret_cast<PER_IO_CONTEXT *>(overlp); // NOLINT

            *child = *parent2; // 赋值

            assert(parent == parent2);

            // 判断有没有影响
            assert(child->a == parent2->a);
            assert(child->str == parent2->str);
            assert(child->a == 2);
            assert(child->str == "string");
            assert(child->overlapped.Internal == parent2->overlapped.Internal);

            // 看看 parent -> overlp -> parent2 有没有丢失信息
            assert(parent->overlapped.Internal == parent2->overlapped.Internal);

            {
                auto *overlp = reinterpret_cast<OVERLAPPED *>(parent); // NOLINT
                // child => overlp => overlp2 => parent3 then *child = *parent3
                auto *overlp2 = reinterpret_cast<OVERLAPPED *>(child); // NOLINT

                // Note:
                // 从Accept到RW,通过OVERLAPPED来让前面的 PER_IO_CONTEXT 赋值 RW_IO_CONTEXT
                overlp2 = overlp;

                // 填充 child
                auto *child = new RW_IO_CONTEXT;
                auto *parent3 = reinterpret_cast<PER_IO_CONTEXT *>(overlp2); // NOLINT

                assert(parent == parent3); // 不会丢失

                *child = *parent3; // 赋值

                assert(child->a == parent3->a);
                assert(child->str == parent3->str);
                assert(child->a == 2);
                assert(child->str == "string");
                assert(child->overlapped.Internal == parent3->overlapped.Internal);
                {
                    // 一步到位
                    auto *overlp = reinterpret_cast<OVERLAPPED *>(parent); // NOLINT

                    // 被 window 赋值； 完成端口的部分
                    // 开始
                    RW_IO_CONTEXT *child = nullptr;
                    auto **rwOv = reinterpret_cast<OVERLAPPED **>(&child);
                    *rwOv = overlp;
                    // 结束

                    // 现在  child 应该是指向 parent 了
                    auto *new_child = new RW_IO_CONTEXT;
                    auto *p = static_cast<PER_IO_CONTEXT *>(child);
                    *new_child = *p;

                    // 应该是一样的
                    assert(new_child->a == parent->a);
                    assert(new_child->str == parent->str);
                    assert(new_child->a == 2);
                    assert(new_child->str == "string");

                    //
                    child = new_child; // 就完成了，上下文的上下文切换
                }
                delete child;
            }

            delete child;
            delete parent2;
        }
        delete ctx;
        delete io;
    }

    std::cout << "main done\n";
    return 0;
}
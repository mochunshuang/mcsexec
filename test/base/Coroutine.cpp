#include <algorithm>
#include <cassert>
#include <cerrno>
#include <coroutine>
#include <exception>
#include <iostream>
#include <utility>
#include <variant>

template <typename TaskType>
struct promise_base;
template <template <typename> typename Task, typename T>
struct promise_base<Task<T>>
{
    using handle_type = std::coroutine_handle<promise_base>;
    using value_type = T;
    // 使用 TaskType 作为返回类型
    Task<T> get_return_object() noexcept // NOLINT
    {
        return Task<T>{handle_type::from_promise(*this)};
    }

    constexpr std::suspend_always initial_suspend() noexcept // NOLINT
    {
        return {};
    }
    constexpr std::suspend_always final_suspend() noexcept // NOLINT
    {
        return {};
    }
    constexpr void unhandled_exception() noexcept // NOLINT
    {
        result_or_exception = std::current_exception();
    }
    constexpr void return_value(value_type &&t) noexcept // NOLINT
    {
        result_or_exception = std::move(t);
    }

    std::variant<value_type, std::exception_ptr> result_or_exception; // NOLINT
};
template <typename T>
struct task_base;

template <typename T>
struct task_base
{
    using promise_type = promise_base<task_base>;
    using handle_type = promise_type::handle_type;
    handle_type handle;
};

task_base<int> fun0() noexcept // NOLINT
{
    std::cout << "fun0 called\n";
    co_return 1;
};

task_base<int> fun1() noexcept // NOLINT
{
    std::cout << "fun1 called\n";
    throw std::runtime_error("An error occurred in fun1");
    co_return 0; // 这行代码不会执行
};

class track // NOLINT
{
  public:
    explicit track(std::string name) : m_name(std::move(name))
    {
        std::cout << "track: Constructor called for " << m_name << "\n";
    }
    ~track()
    {
        std::cout << "track: Destructor called for " << m_name << "\n";
    }

    track(const track &) = delete;
    track &operator=(const track &) = delete;

  private:
    std::string m_name;
};

template <typename T>
struct task_base_leaked // NOLINT
{
    using promise_type = promise_base<task_base_leaked<T>>;
    using handle_type = promise_type::handle_type;

    // 适配,转发: Task<T>{handle_type::from_promise(*this)}
    explicit task_base_leaked(handle_type h) : task_base_leaked(h, "task_base_leaked") {}
    explicit task_base_leaked(handle_type h, std::string name)
        : handle(h), t{std::move(name)}
    {
    }
    handle_type handle; // NOLINT
    track t;            // NOLINT
    ~task_base_leaked() noexcept
    {
        /**
         * Note: 注意
         * 如果 final_suspend 返回 std::suspend_always，协程句柄仍然有效，需要手动销毁。
         * 如果 final_suspend 返回 std::suspend_never，协程句柄会自动销毁。
         *
         */
        if (handle && not handle.done())
        {
            assert(false);
            std::cout << "handle 不会自动销毁 \n";
            handle.destroy();
        }
        if (handle)
        {
            std::cout << "handle 不为空 \n";
        }
        if (handle.done())
        {
            std::cout << "handle 已经 done \n";
        }
        else
        {
            assert(false);
            std::cout << "handle not done \n";
        }
        // Note: 无论如何,只要不move . 一定要move
        handle.destroy();
        assert(handle != nullptr);
        handle = {}; // Note: 移动销毁必须给默认的有效的状态,指针默认 nullptr
        assert(handle == nullptr);
    }
};

task_base_leaked<int> fun2() noexcept // NOLINT
{
    std::cout << "fun2 called\n";
    co_return 1;
};

void test_base(); // NOLINT

int main()
{
    test_base();
    return 0;
}
void test_base() // NOLINT
{
    auto test0 = [] {
        auto c = fun0();
        assert(not c.handle.done());
        // call fun0() now
        c.handle.resume();
        // Note: std::suspend_never final_suspend(). 下面的断言将false
        assert(c.handle.done());
    };
    test0();

    auto test1 = [] {
        auto c = fun0();
        c.handle.resume();
        // 如何拿到 co_return 的值?
        auto &result = c.handle.promise().result_or_exception;
        std::visit(
            [](auto &ret) {
                if constexpr (requires { assert(ret == 1); })
                {
                    std::cout << "co_return called\n";
                    assert(ret == 1);
                }
            },
            result);
    };
    test1();

    auto test2 = [] {
        auto c = fun1();
        c.handle.resume();
        auto &result = c.handle.promise().result_or_exception;
        std::visit(
            [&](auto &ret) {
                // 如何处理异常
                if constexpr (std::is_same_v<std::decay_t<decltype(ret)>,
                                             std::exception_ptr>)
                {
                    try
                    {
                        std::rethrow_exception(ret);
                    }
                    catch (const std::exception &e)
                    {
                        std::cout << "Caught exception: " << e.what() << "\n";
                    }
                }
                // 这里  result 还持有 异常吗? 是的
                assert(std::holds_alternative<std::exception_ptr>(
                    c.handle.promise().result_or_exception));
                ret = {};
                assert(std::holds_alternative<std::exception_ptr>(
                    c.handle.promise().result_or_exception));

                // 重新 emplace 才改变 holds_alternative
                c.handle.promise().result_or_exception.emplace<int>(1);
                assert(not std::holds_alternative<std::exception_ptr>(
                    c.handle.promise().result_or_exception));
            },
            result);
    };
    test2();

    auto test3 = [] {
        std::exception_ptr ret;
        try
        {
            throw std::runtime_error("Test exception");
        }
        catch (...)
        {
            ret = std::current_exception(); // 捕获异常并存储到 ret
        }

        try
        {
            std::rethrow_exception(ret); // 重新抛出异常
        }
        catch (const std::exception &e)
        {
            std::cout << "Caught exception: " << e.what() << '\n';
        }

        // ret 仍然有效
        if (ret)
        {
            std::cout << "ret is still valid" << '\n';
        }
    };
    test3();

    auto t = [] {
        auto c = fun2();
        c.handle.resume();
    };
    t();

    auto final_suspend = [] {
        struct task_suspend_never // NOLINT
        {
            struct promise_type
            {
                task_suspend_never get_return_object() // NOLINT
                {
                    return task_suspend_never{
                        std::coroutine_handle<promise_type>::from_promise(*this)};
                }
                std::suspend_always initial_suspend() // NOLINT
                {
                    return {};
                }
                // Note:  suspend_never
                std::suspend_never final_suspend() noexcept // NOLINT
                {
                    return {};
                }
                void unhandled_exception() {} // NOLINT
                void return_void() {}         // NOLINT
            };

            std::coroutine_handle<promise_type> handle; // NOLINT

            explicit task_suspend_never(std::coroutine_handle<promise_type> h) : handle(h)
            {
            }
            ~task_suspend_never()
            {
                if (handle)
                {
                    std::cout << "Task 析构函数: 协程句柄是否有效? " << handle.address()
                              << "\n";
                    // Note: 协程句柄，在协程外部操纵
                    // Note: 这是用于恢复协程执行或销毁协程帧的不带所有权句柄
                    // handle.destroy(); // Note: 异常会发生
                    // handle = {};
                    if (not handle.done())
                    {
                        std::cout << "not handle.done() ";
                        // handle.destroy();  // Note: 异常会发生
                    }
                    // Note: 得出结论： std::suspend_never final_suspend() 手动销毁
                }
            }
        };

        auto test = []() -> task_suspend_never {
            std::cout << "协程开始执行\n";
            co_return; // 协程结束
        }();
        static_assert(std::is_same_v<decltype(test), task_suspend_never>);

        // 必须要执行
        test.handle.resume();
    };
    final_suspend();
}
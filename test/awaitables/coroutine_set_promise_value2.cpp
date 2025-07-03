#include <iostream>
#include <coroutine>
#include <thread>
#include <future>
#include <chrono>
#include <memory>
#include <iomanip>
#include <queue>

// NOLINTBEGIN

// 时间戳辅助函数
std::string current_time()
{
    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now);
    auto ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) %
        1000;

    std::stringstream ss;
    ss << std::put_time(std::localtime(&now_c), "%H:%M:%S") << "." << std::setfill('0')
       << std::setw(3) << ms.count();
    return ss.str();
}

// 线程ID格式化
std::string thread_id()
{
    std::stringstream ss;
    ss << std::this_thread::get_id();
    return ss.str();
}

// 带颜色的输出
enum class Color
{
    RESET = 0,
    RED = 31,
    GREEN = 32,
    YELLOW = 33,
    BLUE = 34,
    MAGENTA = 35,
    CYAN = 36
};

std::string colored(const std::string &text, Color color)
{
    return "\033[" + std::to_string(static_cast<int>(color)) + "m" + text + "\033[" +
           std::to_string(static_cast<int>(Color::RESET)) + "m";
}

// 异步任务的awaitable
struct AsyncTask
{
    std::shared_ptr<std::future<void>> future;

    AsyncTask(std::future<void> f)
        : future(std::make_shared<std::future<void>>(std::move(f)))
    {
    }

    bool await_ready() const noexcept
    {
        return future->wait_for(std::chrono::seconds(0)) == std::future_status::ready;
    }

    void await_suspend(std::coroutine_handle<> h) const noexcept
    {
        std::thread([h, future_copy = future]() {
            future_copy->wait();
            h.resume();
        }).detach();
    }

    void await_resume() const noexcept {}
};

// 异步操作生成器
struct AsyncGenerator
{
    struct promise_type
    {
        int current_value{};
        bool value_ready = false;

        AsyncGenerator get_return_object()
        {
            return {std::coroutine_handle<promise_type>::from_promise(*this)};
        }

        std::suspend_always initial_suspend()
        {
            return {};
        }
        std::suspend_always final_suspend() noexcept
        {
            return {};
        }

        void return_void() {}

        struct YieldAwaitable
        {
            int &value;
            bool &value_ready;

            bool await_ready() const noexcept
            {
                return false;
            }
            void await_suspend(std::coroutine_handle<promise_type> h) const noexcept
            {
                h.promise().value_ready = true;
            }
            void await_resume() const noexcept {}
        };

        YieldAwaitable yield_value(int value)
        {
            current_value = value;
            return {current_value, value_ready};
        }

        void unhandled_exception() {}
    };

    std::coroutine_handle<promise_type> handle;

    AsyncGenerator(std::coroutine_handle<promise_type> h) : handle(h) {}
    ~AsyncGenerator()
    {
        if (handle)
            handle.destroy();
    }

    bool move_next()
    {
        if (!handle || handle.done())
            return false;

        handle.promise().value_ready = false;
        handle.resume();

        return !handle.done();
    }

    int current_value() const
    {
        return handle.promise().current_value;
    }

    bool is_value_ready() const
    {
        return handle.promise().value_ready;
    }
};

// 模拟网络请求的异步协程
AsyncGenerator fetch_data()
{
    std::cout << colored("[协程]", Color::CYAN) << " [" << current_time() << "] "
              << "开始数据获取 (线程ID: " << thread_id() << ")\n";

    for (int i = 0; i < 3; ++i)
    {
        std::cout << colored("[协程]", Color::CYAN) << " [" << current_time() << "] "
                  << "发起请求 #" << i + 1 << " (线程ID: " << thread_id() << ")\n";

        // NOTE:
        //  模拟网络请求（2秒延迟）
        auto future = std::async(std::launch::async, []() {
            std::this_thread::sleep_for(std::chrono::seconds(2));
        });

        co_await AsyncTask{std::move(future)};

        std::cout << colored("[协程]", Color::CYAN) << " [" << current_time() << "] "
                  << "请求 #" << i + 1 << " 完成 (线程ID: " << thread_id() << ")\n";

        co_yield i + 1;
    }

    std::cout << colored("[协程]", Color::CYAN) << " [" << current_time() << "] "
              << "所有数据获取完毕 (线程ID: " << thread_id() << ")\n";
}

// 模拟游戏状态
enum class GameState
{
    WAITING_FOR_DATA,
    PROCESSING_DATA,
    RENDERING,
    IDLE
};

int main()
{
    std::cout << colored("[主线程]", Color::GREEN) << " [" << current_time() << "] "
              << "游戏循环启动 (线程ID: " << thread_id() << ")\n";

    auto data_gen = fetch_data();
    GameState state = GameState::WAITING_FOR_DATA;
    std::queue<int> data_queue;

    bool game_running = true;
    while (game_running)
    {
        switch (state)
        {
        case GameState::WAITING_FOR_DATA: {
            std::cout << colored("[主线程]", Color::GREEN) << " [" << current_time()
                      << "] "
                      << "等待数据中... (线程ID: " << thread_id() << ")\n";

            // 尝试获取数据
            if (data_gen.move_next())
            {
                data_queue.push(data_gen.current_value());
                state = GameState::PROCESSING_DATA;
                std::cout << colored("[主线程]", Color::GREEN) << " [" << current_time()
                          << "] "
                          << "收到新数据: " << data_gen.current_value()
                          << " (线程ID: " << thread_id() << ")\n";
            }
            else
            {
                // 数据获取完毕
                if (data_queue.empty())
                {
                    std::cout << colored("[主线程]", Color::GREEN) << " ["
                              << current_time() << "] "
                              << "所有数据处理完毕，游戏结束 (线程ID: " << thread_id()
                              << ")\n";
                    game_running = false;
                }
                else
                {
                    state = GameState::RENDERING;
                }
            }

            // 模拟游戏其他操作（非阻塞）
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            break;
        }

        case GameState::PROCESSING_DATA: {
            std::cout << colored("[主线程]", Color::GREEN) << " [" << current_time()
                      << "] "
                      << "处理数据: " << data_queue.front() << " (线程ID: " << thread_id()
                      << ")\n";

            // 模拟数据处理
            std::this_thread::sleep_for(std::chrono::milliseconds(300));

            data_queue.pop();
            state = GameState::RENDERING;
            break;
        }

        case GameState::RENDERING: {
            std::cout << colored("[主线程]", Color::GREEN) << " [" << current_time()
                      << "] "
                      << "渲染游戏画面 (线程ID: " << thread_id() << ")\n";

            // 模拟渲染
            std::this_thread::sleep_for(std::chrono::milliseconds(200));

            if (data_queue.empty())
            {
                state = GameState::WAITING_FOR_DATA;
            }
            else
            {
                state = GameState::PROCESSING_DATA;
            }
            break;
        }

        case GameState::IDLE: {
            std::cout << colored("[主线程]", Color::GREEN) << " [" << current_time()
                      << "] "
                      << "空闲状态，等待用户输入 (线程ID: " << thread_id() << ")\n";

            // 模拟用户输入检查
            std::this_thread::sleep_for(std::chrono::milliseconds(500));

            // 这里可以添加用户输入检测逻辑
            break;
        }
        }
    }

    std::cout << colored("[主线程]", Color::GREEN) << " [" << current_time() << "] "
              << "游戏循环退出 (线程ID: " << thread_id() << ")\n";

    return 0;
}
// NOLINTEND
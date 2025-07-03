#include <iostream>
#include <stdexcept>
#include <system_error>
#include <utility>
#include <variant>

#include "../test_base_head.hpp"

template <typename R>
struct pomise_return
{
    using value_type = std::remove_cvref_t<R>;
    using result_type =
        std::variant<std::monostate, value_type, std::exception_ptr, std::error_code>;
    result_type result;
    template <typename T>
    void return_value(T &&value) noexcept // NOLINT
    {
        this->result.template emplace<value_type>(std::forward<T>(value));
    }
};
template <>
struct pomise_return<void>
{
    struct void_t
    {
    };
    using value_type = void_t;
    using result_type =
        std::variant<std::monostate, void_t, std::exception_ptr, std::error_code>;
    result_type result;         // NOLINT
    void return_void() noexcept // NOLINT
    {
        this->result.template emplace<value_type>(void_t{});
    }
};

template <typename R>
struct task_value_sig
{
    using type = ex::set_value_t(R);
};
template <>
struct task_value_sig<void>
{
    using type = ex::set_value_t();
};

// NOTE: 这个sndr 将生成 operation_state_task 协程
// NOTE: 通过 await_transform 连接生成统一的？ sender_awaitable？
// NOTE: operation_state_task 函数体统一由 connect_awaitable 定义
// NOTE: operation_state_task 内部将: co_await std::move(sndr);
// NOTE: std::move(sndr) -> await_transform -> task -> task::await_transform
// NOTE: -> sender_awaitable -> await_suspend(task::handle) -> opstate::start(task_state)
// NOTE: 一旦 task_state 完成 应该要 resume operation_state_task 进行 结果往下投递
// NOTE: awaitable_receiver 知道 task_state 的完成，然后 continuation 继续唤醒协程
// NOTE: 变成 sender_awaitable 然后 sndr 都固定和 awaitable_receiver 连接
// NOTE: 即：co_await -> await_transform -> sender_awaitable，
// NOTE: 当 sender_awaitable的操作完成，awaitable_receiver 继续 continuation8 恢复协程
template <typename T = void>
struct task
{
    struct state_base // NOLINT
    {
        using result_type = typename pomise_return<std::remove_cvref_t<T>>::result_type;
        using complete_callback_type = void(state_base *base,
                                            result_type &variant_result) noexcept;
        complete_callback_type *complete{nullptr};
    };

    struct promise_type : pomise_return<std::remove_cvref_t<T>>
    {
        using result_type = pomise_return<std::remove_cvref_t<T>>::result_type;

        struct final_awaiter
        {
            promise_type *promise;                       // NOLINT
            static constexpr bool await_ready() noexcept // NOLINT
            {
                return false;
            }
            void await_suspend(std::coroutine_handle<> /*unused*/) noexcept // NOLINT
            {
                // NOTE: this promise operation last
                promise->state->complete(promise->state, promise->result);
            }
            static constexpr void await_resume() noexcept {} // NOLINT
        };
        task get_return_object() noexcept // NOLINT
        {
            return task{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        static constexpr std::suspend_always initial_suspend() noexcept // NOLINT
        {
            return {};
        }
        constexpr final_awaiter final_suspend() noexcept // NOLINT
        {
            return {this};
        }
        void unhandled_exception() noexcept // NOLINT
        {
            // NOTE: 直接 让 recr 唤醒即可。交给 reciver
            this->result.template emplace<std::exception_ptr>(std::current_exception());
        }

        // NOTE: 变成 sender_awaitable 或者 它本身。
        // NOTE: 不要求是 Sndr 更通用
        // NOTE: 核心是理解这里。 将当前协程注入到 函数体传递是Sndr 中
        // NOTE: 其他协程如何让这stop? 依赖 unhandled_exception()
        template <typename Expr>
        auto await_transform(Expr &&expr) noexcept // NOLINT
        {
            // TODO(mcs): 调度器线程和当前线程不能够一样，否则死锁
            return ex::awaitables::as_awaitable(std::forward<Expr>(expr), *this);
        }

        // unhandled_stopped => awaitable_sender
        std::coroutine_handle<> unhandled_stopped() // NOLINT
        {
            this->state->complete(this->state, this->result);
            return std::noop_coroutine();
        }

        struct env
        {
            const promise_type *promise; // NOLINT

            // TODO(mcs): 要不改成。 传进来的。要是有办法，能抽取公共sndr还行的
            // auto query(ex::queries::get_stop_token_t /*unused*/) const noexcept
            // {
            //     return promise->state->get_stop_token();
            // }
            [[nodiscard]] ex::default_domain query(
                ex::queries::get_domain_t /*unused*/) const noexcept
            {
                return {};
            }
        };

        [[nodiscard]] auto get_env() const noexcept -> ex::queryable auto // NOLINT
        {
            return env{this};
        }

        state_base *state{}; // NOLINT
    };

    template <typename Rcvr>
    struct state : state_base
    {
        state(Rcvr &&r, std::coroutine_handle<promise_type> h) noexcept
            : state_base{.complete = &complete_impl}, rcvr{std::forward<Rcvr>(r)},
              handle{h}
        {
            handle.promise().state = this;
        }
        ~state() noexcept
        {
            if (this->handle)
            {
                this->handle.destroy();
            }
        }
        state(const state &) = delete;
        state(state &&) = delete;
        state &operator=(const state &) = delete;
        state &operator=(state &&) = delete;

        static constexpr void complete_impl( // NOLINT
            state_base *base, state_base::result_type &variant_result) noexcept
        {
            // NOTE: 正常路径是由 final_awaiter 设置
            auto *self = static_cast<state *>(base);
            switch (variant_result.index())
            {
            case 0: // set_stopped
                // this->reset_handle();
                ex::recv::set_stopped(std::move(self->rcvr));
                break;
            case 1: // set_value
                if constexpr (std::same_as<void, T>)
                {
                    // reset_handle();
                    ex::recv::set_value(std::move(self->rcvr));
                }
                else
                {
                    auto r(std::move(std::get<1>(variant_result)));
                    // this->reset_handle();
                    ex::recv::set_value(std::move(self->rcvr), std::move(r));
                }
                break;
            case 2: // NOTE: tow type error //TODO(mcs) 如果多个
                ex::set_error(std::move(self->rcvr), std::get<2>(variant_result));
                break;
            case 3:
                ex::set_error(std::move(self->rcvr), std::get<3>(variant_result));
                break;
            default:

                break;
            }
        }

        // NOTE: operation_concept
        using operation_state_concept = ex::operation_state_t;
        void start() & noexcept
        {
            handle.resume();
        }

        Rcvr rcvr;                                           // NOLINT
        std::coroutine_handle<promise_type> handle{nullptr}; // NOLINT
    };
    task(const task &) = delete;
    task &operator=(const task &) = delete;
    task &operator=(task &&) = delete;
    explicit task(std::coroutine_handle<promise_type> h) noexcept : handle_{h} {}
    ~task() noexcept
    {
        if (this->handle_)
            this->handle_.destroy();
    }
    task(task &&other) noexcept : handle_(std::exchange(other.handle_, nullptr)) {};

    // NOTE: sender: std::error_code 待续
    using sender_concept = ex::sender_t;
    using completion_signatures = ex::cmplsigs::completion_signatures<
        typename task_value_sig<T>::type, ex::set_error_t(std::exception_ptr),
        ex::set_error_t(std::error_code), ex::set_stopped_t()>;
    template <ex::recv::receiver Rcvr>
    state<Rcvr> connect(Rcvr rcvr) noexcept
    {
        return state<Rcvr>(std::forward<Rcvr>(rcvr), std::exchange(this->handle_, {}));
    }

  private:
    std::coroutine_handle<promise_type> handle_{}; // NOLINT
};

static_assert(ex::sender<task<int>>);

int main()
{

    TEST("task<int> ") = [] {
        auto rc = mcs::this_thread::sync_wait([] -> task<int> { // NOLINT
            co_return 17;                                       // NOLINT
        }());
        assert(rc);
        auto [value] = rc.value_or(std::tuple{0});
        EXPECT(value == 17);
    };
    TEST("task<int> ") = [] {
        auto rc = mcs::this_thread::sync_wait([] -> task<int> { // NOLINT
            co_return 17;                                       // NOLINT
        }() | ex::then([](int i) { return 1 + i; }));
        assert(rc);
        auto [value] = rc.value_or(std::tuple{0});
        EXPECT(value == 18);
    };
    TEST("co_await -> task<>") = [] {
        [[maybe_unused]] auto o = mcs::this_thread::sync_wait([]() -> task<> {
            co_await ex::just(); // void // NOLINT
            std::cout << "after co_await ex::just()\n";
            [[maybe_unused]] auto v = co_await ex::just(42); // int // NOLINT
            assert(v == 42);
            [[maybe_unused]] auto [i, b, c] =
                co_await ex::just(17, true, 'c'); // tuple<int, bool, char> // NOLINT
            assert(i == 17 && b == true && c == 'c');
            try
            {
                co_await ex::just_error(-1); // exception
                assert(nullptr == "never reached");
            }
            catch (int e)
            {
                assert(e == -1);
            }
            std::cout << "about to cancel\n";
            try
            {
                co_await ex::just_stopped(); // NOLINT
            }
            catch (...) // NOLINT
            {
                assert(false);
            } // cancel: never resumed
            assert(nullptr == "never reached");
        }());
        EXPECT(not o);
    };

    TEST("co_await 2 ") = [] {
        auto fun = [] -> task<int> {
            int i = 0;
            co_await ex::just(i);
            co_return -1;
        };
        {
            auto [ret] =
                mcs::this_thread::sync_wait(fun() | ex::then([](int i) {
                                                std::cout << "[co_await 2]: then call\n";
                                                std::cout << "i: " << i << '\n';
                                                return i;
                                            }))
                    .value();
            EXPECT(ret == -1);
        }
    };
    TEST("co_await 3 ") = [] {
        auto fun = [] -> task<int> {
            auto [a, b] = co_await ex::just(std::make_pair(-1, "name"));
            co_return a;
        };
        {
            auto [ret] =
                mcs::this_thread::sync_wait(fun() | ex::then([](int i) {
                                                std::cout << "[co_await 3]: then call\n";
                                                std::cout << "i: " << i << '\n';
                                                return i;
                                            }))
                    .value();
            EXPECT(ret == -1);
        }
    };

    TEST("co_await 4 ") = [] {
        auto fun = [] -> task<int> {
            auto [a, b] = co_await (ex::just(1) | ex::then([](auto p) noexcept {
                                        if (p > 0)
                                            return std::make_pair(-1, "name");
                                        return std::make_pair(p, "name");
                                    }));
            co_return a;
        };
        {
            auto [ret] =
                mcs::this_thread::sync_wait(fun() | ex::then([](int i) {
                                                std::cout << "[co_await 4]: then call\n";
                                                std::cout << "i: " << i << '\n';
                                                return i;
                                            }))
                    .value();
            EXPECT(ret == -1);
        }
    };

    TEST("with while(i-->0)") = [] {
        auto rc = mcs::this_thread::sync_wait([] -> task<int> { // NOLINT
            int i = 3;                                          // NOLINT
            while (i-- > 0)
            {
                [[maybe_unused]] auto ret = co_await (
                    ex::just(i) | ex::then([](int i) noexcept {
                        std::this_thread::sleep_for(std::chrono::milliseconds(i));
                        return i;
                    }));
                std::cout << "while + co_await: " << ret << '\n';
            }
            co_return 1;
        }());
        assert(rc);
        auto [value] = rc.value_or(std::tuple{0});
        EXPECT(value == 1);
    };

    ex::static_thread_pool<3> pool; // NOTE: 必须在外部。
    TEST("with while(i-->0) 2 ") = [&] {
        auto rc = mcs::this_thread::sync_wait([](auto &pool) -> task<int> { // NOLINT
            int i = 3;                                                      // NOLINT

            std::cout << ">>>> task enter thread_id: " << std::this_thread::get_id()
                      << '\n';
            while (i-- > 0)
            {
                std::cout << "before co_await thread_id: " << std::this_thread::get_id()
                          << '\n';
                [[maybe_unused]] auto ret = co_await (
                    ex::schedule(pool[i].get_scheduler()) | ex::then([=]() noexcept {
                        std::cout
                            << "inter co_await thread_id: " << std::this_thread::get_id()
                            << '\n';
                        std::this_thread::sleep_for(std::chrono::milliseconds(i));
                        return i;
                    }));
                std::cout << "after co_await thread_id: " << std::this_thread::get_id()
                          << '\n';
            }
            std::cout << ">>>> task co_return thread_id: " << std::this_thread::get_id()
                      << '\n';
            co_return 1;
        }(pool) | ex::then([](int ret) noexcept { return ret; })); // inject pool
        assert(rc);
        auto [value] = rc.value_or(std::tuple{0});
        EXPECT(value == 1);
    };

    TEST("co_yield only for error ") = [] {
        auto fun = [] -> task<int> {
            // 感觉没必要了
            // co_yield mcs::execution::task::with_error{-99}; // NOLINT

            // throw std::error_code{-99}; //NOTE: 不允许
            throw std::runtime_error{"error"};
            UNEXPECT("never reached");
            co_return -1;
        };
        {
            auto [ret] =
                mcs::this_thread::sync_wait(fun() | ex::then([](int i) {
                                                std::cout << "[co_yield]: then call\n";
                                                std::cout << "i: " << i << '\n';
                                                return i;
                                            }) |
                                            ex::upon_error([](auto e) {
                                                std::cout << "[upon_error]:  call\n";
                                                return -1;
                                            }))
                    .value();
            EXPECT(ret == -1);
        }
    };

    std::cout << "main done\n";
    return 0;
}
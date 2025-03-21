#include <cassert>
#include <iostream>
#include <atomic>
#include <thread>
#include <cstdint>
#include <string>
// NOLINTBEGIN
enum class State : std::uint8_t
{
    start,
    conflict_1,
    conflict_2,
    conflict_3,
    end
};

std::atomic<uint64_t> state_count{
    (static_cast<uint64_t>(0) << 8 | static_cast<uint8_t>(State::start))};

std::pair<State, int> unpack(uint64_t packed)
{
    State s = static_cast<State>(packed & 0xFF);
    int count = static_cast<int>(packed >> 8);
    return {s, count};
}

void print_state(const std::string &thread_name, State s, int count, bool success)
{
    std::cout << "[" << thread_name << "] "
              << "CAS " << (success ? "succeeded" : "failed")
              << " | State: " << static_cast<int>(s) << " | Count: " << count << "\n";
}

State next_state(State current)
{
    switch (current)
    {
    case State::start:
        return State::conflict_1;
    case State::conflict_1:
        return State::conflict_2;
    case State::conflict_2:
        return State::conflict_3;
    case State::conflict_3:
        return State::end;
    default:
        return State::end;
    }
}

template <typename Fn>
void thread_func(const std::string thread_name, Fn fun)
{
    while (true)
    {
        uint64_t current = state_count.load(std::memory_order_acquire);
        auto [current_state, current_count] = unpack(current);

        if (current_state == State::end)
        {
            print_state(thread_name, current_state, current_count, false);
            break;
        }

        int new_count = fun();
        uint64_t desired =
            (static_cast<uint64_t>(new_count) << 8 | static_cast<uint8_t>(current_state));

        bool success = state_count.compare_exchange_weak(
            current, desired, std::memory_order_release, std::memory_order_relaxed);

        if (success)
        {
            // NOTE: 不要做任何操作，这点时间。做其他事情就难冲突了
        }
        else
        {
            auto [failed_state, failed_count] = unpack(current);
            State updated_state = next_state(failed_state);
            uint64_t updated = (static_cast<uint64_t>(failed_count) << 8 |
                                static_cast<uint8_t>(updated_state));
            state_count.store(updated, std::memory_order_release);
            print_state(thread_name, updated_state, failed_count, false);
        }
    }
}

int main()
{
    auto fun = []() {
        static int v = 1;
        return v += 2; // 生成奇数：1,3, 5...
    };
    auto fun2 = []() {
        static int v = 0;
        return v += 2; // 生成偶数：2, 4, 6...
    };

    std::thread t1(thread_func<decltype(fun)>, "Thread1", fun);
    std::thread t2(thread_func<decltype(fun2)>, "Thread2", fun2);

    t1.join();
    t2.join();

    auto [final_state, final_count] = unpack(state_count);
    std::cout << "\nFinal State: " << static_cast<int>(final_state)
              << " | Final Count: " << final_count << " (State must be 'end')\n";
    assert(final_state == State::end);
    std::cout << "done\n";
}
// NOLINTEND
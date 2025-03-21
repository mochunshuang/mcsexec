#include <atomic>
#include <cassert>
#include <cstdint>
#include <tuple>

// NOLINTBEGIN
enum class State : uint8_t
{
    unused,
    open,
    open_and_joining,
    closed,
    unused_and_closed,
    closed_and_joining,
    joined
};

class StateCountManager
{
    using count_type = uint64_t;
    // using state_type = uint64_t;
    using state_type = State;

  private:
    // 低8位: state，高56位: count
    std::atomic<count_type> state_count_{0};

    // 将 state 和 count 组合为 64 位整数
    static count_type pack(state_type state, count_type count)
    {
        return (count << 8) | static_cast<count_type>(state);
    }

    // 从 64 位整数解包为 state 和 count
    static std::pair<state_type, count_type> unpack(count_type value)
    {
        return {static_cast<state_type>(value & 0xFF), value >> 8};
    }

  public:
    // 获取当前 state 和 count
    std::pair<state_type, count_type> get() const
    {
        return unpack(state_count_.load());
    }

    template <typename F>
    void update_transaction(F &&callback) noexcept // NOLINT
    {
        uint64_t old_value = state_count_.load();
        auto [current_state, current_count] = unpack(old_value);
        auto [new_state, new_count] = callback(current_state, current_count);
        auto new_value = pack(new_state, new_count);

        while (true)
        {
            if (state_count_.compare_exchange_weak(old_value, new_value))
            {
                return; // 修改成功
            }
            // CAS 失败时，old_value 已被更新为最新值，重新解包
            std::tie(current_state, current_count) = unpack(old_value);
            // 直接使用之前缓存的结果，而不是重新调用 callback
            new_value = pack(new_state, new_count);
        }
    }
};
// NOLINTEND
#include <iostream>

int main()
{
    StateCountManager manager;
    {
        auto [state, count] = manager.get();
        assert((state == State::unused && count == 0));
    }
    {
        manager.update_transaction([](State s, uint64_t c) -> std::pair<State, uint64_t> {
            switch (s)
            {
            case State::unused:
                return {State::open, c + 1}; // unused → open
            case State::open:
                return {State::closed, c}; // open → closed
            // ... 其他规则
            default:
                return {s, c}; // 不修改
            }
        });
        auto [state, count] = manager.get();
        assert((state == State::open && count == 1));
    }
    {
        auto new_state = State::closed;
        manager.update_transaction([&](State, uint64_t c) -> std::pair<State, uint64_t> {
            return {new_state, c}; // 不修改count
        });
        auto [state, count] = manager.get();
        assert((state == State::closed && count == 1));
    }
    // 验证无锁原子操作
    static_assert(std::atomic<uint64_t>::is_always_lock_free,
                  "StateAndCount must be lock-free!");

    std::cout << "main done\n";
    return 0;
}

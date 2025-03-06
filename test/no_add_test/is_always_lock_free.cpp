#include <iostream>
#include <cstdint>
#include <atomic>

// NOLINTBEGIN
enum state_type : std::uint8_t
{
    unused,
    open,
    open_and_joining,
    closed,
    unused_and_closed,
    closed_and_joining,
    joined
};

struct alignas(sizeof(uint64_t)) StateAndCount_2
{
    state_type state : 8;     // 8 bits
    std::uint64_t count : 56; // 56 bits
};
static_assert(not std::atomic<StateAndCount_2>::is_always_lock_free,
              "StateAndCount_2 must be lock-free!");

struct alignas(sizeof(uint64_t)) StateAndCount
{
  private:
    std::uint64_t state : 8;  // 8 bits
    std::uint64_t count : 56; // 56 bits

  public:
    // 获取 state 字段
    state_type get_state() const
    {
        return static_cast<state_type>(state);
    }

    // 设置 state 字段
    void set_state(state_type value)
    {
        state = static_cast<std::uint64_t>(value);
    }

    // 获取 count 字段
    std::uint64_t get_count() const
    {
        return count;
    }

    // 设置 count 字段
    void set_count(std::uint64_t value)
    {
        count = value;
    }
};

// 验证无锁原子操作
static_assert(std::atomic<StateAndCount>::is_always_lock_free,
              "StateAndCount must be lock-free!");

// 压缩state(8位) + count(56位)到64位原子变量
struct alignas(8) StateCount
{
    std::uint8_t state : 8;
    std::uint64_t count : 56;
};
static_assert(not std::atomic<StateCount>::is_always_lock_free,
              "StateAndCount must be lock-free!");

int main()
{
    StateAndCount sc;
    sc.set_state(state_type::open);
    sc.set_count(123456);

    std::cout << "State: " << static_cast<int>(sc.get_state()) << std::endl;
    std::cout << "Count: " << sc.get_count() << std::endl;

    return 0;
}
// NOLINTEND
#include "../test_base_head.hpp"

// 自定义查询标签
struct my_allocator_key
{
};
struct my_scheduler_key
{
};

// 自定义值类型
struct my_alloc
{
};
struct my_sched
{
};

// 自定义可查询对象
struct alloc_env
{
    constexpr my_alloc query(my_allocator_key) const noexcept // NOLINT
    {
        return my_alloc{};
    }
};

struct sched_env
{
    constexpr my_sched query(my_scheduler_key) const noexcept // NOLINT
    {
        return my_sched{};
    }
};

int main()
{

    TEST("base") = [] {
        // 创建可查询对象
        alloc_env alloc_env_obj;
        sched_env sched_env_obj;

        // 创建 env 对象
        auto e = mcs::execution::queries::env(alloc_env_obj, sched_env_obj);

        // 查询环境
        auto alloc = e.query(my_allocator_key{});
        auto sched = e.query(my_scheduler_key{});
        static_assert(std::is_same_v<decltype(sched), my_sched>);
        static_assert(std::is_same_v<decltype(alloc), my_alloc>);
    };

    TEST("base with prop") = [] {
        using namespace mcs::execution::queries; // NOLINT
        auto e = env{prop(my_allocator_key{}, my_alloc{}),
                     prop(my_scheduler_key{}, my_sched{})};

        // 查询环境
        auto alloc = e.query(my_allocator_key{});
        auto sched = e.query(my_scheduler_key{});
        static_assert(std::is_same_v<decltype(sched), my_sched>);
        static_assert(std::is_same_v<decltype(alloc), my_alloc>);
    };
    return 0;
}
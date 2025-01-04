#include <algorithm>
#include <cassert>
#include <iostream>
#include <utility>

struct move_only_type
{
    move_only_type() : val(0) {}
    explicit move_only_type(int v) : val(v) {}
    ~move_only_type() = default;

    move_only_type(const move_only_type &) = delete;
    move_only_type &operator=(const move_only_type &) = delete;

    move_only_type &operator=(move_only_type &&) = default;
    move_only_type(move_only_type &&) = default;
    int val; // NOLINT
};

template <typename Sndr>
auto test_fun(Sndr &&sndr) // NOLINT
{

    return std::forward<Sndr>(sndr);
};

template <typename Sndr>
auto test_fun2(Sndr &&sndr) // NOLINT
{

    return test_fun(std::forward<Sndr>(sndr));
};

int main()
{
    {
        move_only_type sndr{1};
        auto obj = test_fun(std::move(sndr));
        assert(obj.val == 1);
    }
    {
        move_only_type sndr{1};
        auto obj = test_fun2(std::move(sndr));
        assert(obj.val == 1);
    }
    return 0;
}
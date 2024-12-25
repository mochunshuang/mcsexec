

#include <concepts>
#include <iostream>

struct set_error_t
{
};
struct set_stopped_t
{
};
struct set_value_t
{
};

template <class Tag>
concept completion_tag = // exposition only
    std::same_as<Tag, set_value_t> || std::same_as<Tag, set_error_t> ||
    std::same_as<Tag, set_stopped_t>;

template <class CPO>
struct get_completion_scheduler_t;

struct forwarding_query_t
{
};

template <completion_tag CompletionTag>
struct get_completion_scheduler_t<CompletionTag> : forwarding_query_t
{
    template <typename T>
    constexpr auto operator()(T &&env) const noexcept -> auto
    {
        return env;
    }
};

template <class CPO>
inline constexpr get_completion_scheduler_t<CPO> get_completion_scheduler{}; // NOLINT

int main()
{
    static_assert(std::is_same_v<get_completion_scheduler_t<set_error_t>,
                                 get_completion_scheduler_t<set_error_t>>);

    constexpr int a = 0;
    static_assert(a == get_completion_scheduler<set_error_t>(a));

    std::cout << "hello world\n";
    return 0;
}
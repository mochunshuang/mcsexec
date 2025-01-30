#include <variant>

struct Recv
{
};

struct state_type
{
    Recv &recv; // NOLINT
    struct receiver_t
    {
        using receiver_concept = receiver_t;

        state_type *state; // exposition only // NOLINT
    };
    std::variant<int> async_result;
};

#include <iostream>

int main()
{
    Recv r;
    state_type a(r);
    {
        state_type a{r};
    }
    std::cout << "main done\n";
    return 0;
}
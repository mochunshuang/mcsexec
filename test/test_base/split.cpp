#include "../../include/execution.hpp"

#include "__cout_receiver.hpp"
#include "__my_class.hpp"

#include <iostream>
#include <type_traits>
#include <utility>

void base();

int main()
{
    base();
    std::cout << "hello world\n";
    return 0;
}

void base()
{
    using namespace mcs::execution;
    using mcs::execution::cmplsigs::completion_signatures;
    using mcs::execution::set_value_t;

    sender auto input = mcs::execution::just(1);
    static_assert(std::is_same_v<decltype(input.get_completion_signatures(empty_env{})),
                                 completion_signatures<set_value_t(int)>>);

    sender auto multi_shot = split(std::move(input));

    sender auto start =
        then(multi_shot, [](int i) { std::cout << i << " First continuation\n"; });
    mcs::this_thread::sync_wait(std::move(start));
    {
        sender auto start =
            then(multi_shot, [](int i) { std::cout << i << " First continuation\n"; });
        mcs::this_thread::sync_wait(std::move(start));
    }
}
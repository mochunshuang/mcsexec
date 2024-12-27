#include <algorithm>
#include <iostream>
#include "../include/execution.hpp"

#include <iostream>
#include <format>

#include <iostream>

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
    sender auto input = just();
    sender auto multi_shot = split(input);

    sender auto both =
        when_all(then(multi_shot, [] { std::cout << "First continuation\n"; }),
                 then(multi_shot, [] { std::cout << "Second continuation\n"; }));
    mcs::this_thread::sync_wait(std::move(both));
}
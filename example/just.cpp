#include <iostream>
#include "../include/execution.hpp"

int main()
{

    mcs::execution::sender auto snd2 = mcs::execution::just();
    mcs::this_thread::sync_wait(std::move(snd2));
    {
        mcs::execution::sender auto snd2 = mcs::execution::just(1, 2, 3);
        auto [a, b, c] = mcs::this_thread::sync_wait(std::move(snd2)).value();
        std::cout << a << " " << b << " " << c;
    }
    return 0;
}
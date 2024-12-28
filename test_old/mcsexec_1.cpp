#include "../include/execution.hpp"
#include <algorithm>
#include <cassert>
#include <iostream>

using namespace mcs::execution;

int main()
{
    // auto a = make_sender(1);

    mcs::execution::sender auto snd2 = mcs::execution::just(3.14, 1);
    auto i = snd2.get<1>();
    assert(i.get<1>() == 1);
    mcs::this_thread::sync_wait(snd2);
    // mcs::this_thread::sync_wait(std::move(snd2));

    [[maybe_unused]] mcs::execution::sender auto then2 = mcs::execution::then(
        snd2, [](double d, int i) { std::cout << "d: " << d << " i: " << i << "\n"; });

    {
        [[maybe_unused]] mcs::execution::sender auto then2 = //
            mcs::execution::just(3.14, 1)                    //
            | mcs::execution::then([](double d, int i) {
                  std::cout << "d: " << d << " i: " << i << "\n";
              });
        // mcs::this_thread::sync_wait(then2);
        // mcs::this_thread::sync_wait(std::move(then2));
    }

    // make_sender();
    std::cout << "hello world\n";
    return 0;
}
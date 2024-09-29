#include "../include/execution.hpp"

#include <algorithm>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <iostream>
#include <thread>
#include <tuple>
#include <utility>

using namespace mcs::execution;

int main()
{
    auto [val] =
        mcs::this_thread::sync_wait(split(just(42)) | then([](int v) { return v; }))
            .value();
    return 0;
}

module;

#include <iostream>

#include <utility>

export module mcs.execution.functional;

template <class Fn, class... Args>
using call_result_t = decltype(std::declval<Fn>()(std::declval<Args>()...));

export call_result_t<decltype([]() {})> hello()
{
    std::cout << "hello world\n";
}

export template <class Fn, class... Args>
concept callable = // exposition only
    requires(Fn &&fn, Args &&...args) {
        std::forward<Fn>(fn)(std::forward<Args>(args)...);
    };

export template <class Fn, class... Args>
concept nothrow_callable = // exposition only
    callable<Fn, Args...> && requires(Fn &&fn, Args &&...args) {
        { std::forward<Fn>(fn)(std::forward<Args>(args)...) } noexcept;
    };
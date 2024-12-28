
#include <algorithm>
#include <iostream>
#include "__my_class.hpp"

using A = MyClassT<int>;

template <typename T>
struct Operation
{
    T v;
};

template <typename Rcvr>
auto connect(Rcvr rcvr)
{
    std::cout << "start:\n";
    return Operation<Rcvr>(std::move(rcvr));
}

template <typename Rcvr>
auto connect_2(Rcvr &&rcvr)
{
    std::cout << "start:\n";
    return Operation<Rcvr>(std::forward<Rcvr>(rcvr));
}

int main()
{
    {
        std::cout << "\nconnect(A{1})\n";
        // Rcvr 是直接初始化的，也是0开销
        auto op = connect(A{1});
    }
    {
        std::cout << "\nconnect_2(A{1})\n";
        auto op = connect_2(A{1});
    }
    /**
     * @brief 总结直接初始化参数：T 和 T&& 是等价的
     *
     */
    std::cout << "\nhello world\n";
    return 0;
}
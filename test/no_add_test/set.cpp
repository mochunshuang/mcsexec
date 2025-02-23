#include <cassert>
#include <iostream>
#include <set>

int main()
{
    auto *key = new int;
    std::set<int *> s;
    s.insert(key);
    s.erase(key);
    assert(s.size() == 0);
    {
        s.insert(key);
        auto *k = key;
        s.erase(k);
        assert(s.size() == 0);
    }
    {
        s.insert(key);
        // 如何随机 pop set中的一个，也就是删除并返回一个
    }
    delete key;

    std::cout << "main done\n";
    return 0;
}
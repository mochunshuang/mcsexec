
#include <algorithm>
#include <cassert>
#include <iostream>
#include <memory>

int main()
{
    // 创建一个共享指针，引用计数 = 1
    std::shared_ptr<int> p1 = std::shared_ptr<int>(new int);
    std::cout << "p1 use_count after creation: " << p1.use_count() << std::endl;

    // 复制构造 p2，引用计数 = 2
    std::shared_ptr<int> p2(p1); // NOLINT
    std::cout << "p1 use_count after copy construction of p2: " << p1.use_count()
              << std::endl;
    std::cout << "p2 use_count after copy construction: " << p2.use_count() << std::endl;

    // 复制赋值 p3，引用计数 = 3
    std::shared_ptr<int> p3;
    p3 = p2;
    std::cout << "p1 use_count after copy assignment of p3: " << p1.use_count()
              << std::endl;
    std::cout << "p2 use_count after copy assignment of p3: " << p2.use_count()
              << std::endl;
    std::cout << "p3 use_count after copy assignment: " << p3.use_count() << std::endl;

    // 移动操作，才不更新count
    {
        std::shared_ptr<int> new_obj = std::move(p3);
        assert(new_obj.use_count() == 3);

        std::shared_ptr<int> obj{std::move(new_obj)};
        assert(obj.use_count() == 3);
    }

    return 0;
}
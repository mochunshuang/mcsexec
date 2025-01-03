#include <iostream>
#include <thread>

void thread_function()
{
    std::cout << "Thread ID: " << std::this_thread::get_id() << std::endl;
}

int main()
{
    std::thread t1(thread_function);
    std::thread t2(thread_function);

    std::thread::id id1 = std::this_thread::get_id();
    std::thread::id id2 = t2.get_id();

    if (id1 == id2)
    {
        std::cout << "Thread IDs are equal." << '\n';
    }
    else
    {
        std::cout << "Thread IDs are not equal." << '\n';
    }

    t1.join();
    t2.join();

    return 0;
}
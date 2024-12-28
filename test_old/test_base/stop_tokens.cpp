#include <stop_token>
#include <iostream>

void process(std::stop_token st)
{
    // register callbacks
    std::stop_callback scb{st, [] {
                               std::cout << "process: do callback\n";
                           }};
    std::cout << "process()\n";
} // deregister callbacks

int main()
{
    {
        std::stop_source ssrc;
        std::stop_token stok{ssrc.get_token()};
        // register callbacks
        std::stop_callback scb1{stok, [] {
                                    std::cout << "1st stop callback\n";
                                }};
        std::stop_callback scb2{stok, [] {
                                    std::cout << "1st stop callback\n";
                                }};
        // test
        process(stok); // no call

        // request stop
        ssrc.request_stop(); // call all registered callbacks

        // 立即调用
        std::stop_callback last{stok, [] {
                                    std::cout << "last stop callback\n";
                                }};

        // test
        process(stok); // immediately called
    }
    std::cout << "hello world\n";
    return 0;
}
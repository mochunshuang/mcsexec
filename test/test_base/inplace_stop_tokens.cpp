#include "../../include/execution.hpp"

#include <iostream>

using namespace mcs::execution;

void process(stoptoken::inplace_stop_token st)
{
    // register callbacks
    stoptoken::inplace_stop_callback scb{st, [] {
                                             std::cout << "process: do callback\n";
                                         }};
    std::cout << "process()\n";
} // deregister callbacks
void base();

int main()
{
    base();
    std::cout << "hello world\n";
    return 0;
}
void base()
{

    {
        stoptoken::inplace_stop_source ssrc;
        stoptoken::inplace_stop_token stok{ssrc.get_token()};
        // register callbacks
        stoptoken::inplace_stop_callback scb1{stok, [] {
                                                  std::cout << "1st stop callback\n";
                                              }};
        stoptoken::inplace_stop_callback scb2{stok, [] {
                                                  std::cout << "2st stop callback\n";
                                              }};
        // test
        process(stok); // no call

        // request stop
        ssrc.request_stop(); // call all registered callbacks

        // 立即调用
        stoptoken::inplace_stop_callback last{stok, [] {
                                                  std::cout << "last stop callback\n";
                                              }};

        // test
        process(stok); // immediately called
    }
}

#include <iostream>
/**
struct my_sender {
  using sender_concept = sender_t;
  using completion_signatures =
    execution::completion_signatures<
      set_value_t(),
      set_value_t(int, float),
      set_error_t(exception_ptr),
      set_error_t(error_code),
      set_stopped_t()>;
};

// Declares my_sender to be a sender that can complete by calling
// one of the following for a receiver expression rcvr:
//    set_value(rcvr)
//    set_value(rcvr, int{...}, float{...})
//    set_error(rcvr, exception_ptr{...})
//    set_error(rcvr, error_code{...})
//    set_stopped(rcvr)

//Note: 因此 completion_signatures 应该由所有可能的 recv::set_value(rcvr,args...)定义
 */
int main()
{
    std::cout << "hello world\n";
    return 0;
}
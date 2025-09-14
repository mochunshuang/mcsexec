#include <iostream>

// NOLINTBEGIN

// NOTE: 有状态的函数
class StateMachine
{
    int state_ = 0;

  public:
    void resume(int target_state)
    {
        switch (target_state)
        {
        case 1:
            execute_state1();
            break;
        case 2:
            execute_state2();
            break;
        case 3:
            execute_state3();
            break;
        case 4:
            execute_state4();
            break;
        }
    }

  private:
    void execute_state1()
    {
        std::cout << "执行状态 1\n";
    }
    void execute_state2()
    {
        std::cout << "执行状态 2\n";
    }
    void execute_state3()
    {
        std::cout << "执行状态 3\n";
    }
    void execute_state4()
    {
        std::cout << "执行状态 4\n";
    }
};

int main()
{
    StateMachine fsm;

    fsm.resume(1); // 执行状态1
    fsm.resume(2); // 执行状态2
    fsm.resume(3); // 执行状态3

    // 直接跳回状态2
    fsm.resume(2); // 执行状态2

    return 0;
}
// NOLINTEND
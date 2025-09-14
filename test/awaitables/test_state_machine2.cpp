#include <iostream>

// NOLINTBEGIN

class StateMachine
{
    int state_ = 0;
    int count_ = 0;

    using StateFunc = void (StateMachine::*)();
    StateFunc state_table[4] = {&StateMachine::state1, &StateMachine::state2,
                                &StateMachine::state3, &StateMachine::state4};

  public:
    void resume(int target_state)
    {
        // if (target_state >= 1 && target_state <= 4)
        // {
        //     state_ = target_state;
        //     (this->*state_table[state_ - 1])();
        // }
        (this->*state_table[target_state - 1])();
    }

  private:
    void state1()
    {
        std::cout << "执行状态 1\n";
        count_++;
        if (count_ == 3)
        {
            state_ = 2; // 切换到状态2
            state2();   // 直接调用，避免递归
            return;
        }
    }

    void state2()
    {
        std::cout << "执行状态 2\n";
    }

    void state3()
    {
        std::cout << "执行状态 3\n";
    }

    void state4()
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

    std::cout << "测试直接切换...\n";
    fsm.resume(1); // 执行状态1
    fsm.resume(1); // 执行状态1

    /*
    测试直接切换...
    执行状态 1
    执行状态 1
    执行状态 2
    */
    return 0;
}
// NOLINTEND
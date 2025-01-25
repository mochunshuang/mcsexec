#include <iostream>
// NOLINTBEGIN
// 基类
class callback_base
{
  protected:
    using execute_fn_t = void(callback_base *) noexcept;
    execute_fn_t *execute_;

  public:
    callback_base(execute_fn_t *e) : execute_(e) {}
    void __execute() noexcept
    {
        this->execute_(this);
    }
    callback_base *next{};
};

// 子类1：实现一个简单的打印回调
class callback_print : public callback_base
{
  public:
    callback_print(const std::string &message)
        : callback_base(&execute_impl), message_(message)
    {
    }

  private:
    std::string message_;

    static void execute_impl(callback_base *self) noexcept
    {
        auto derived = static_cast<callback_print *>(self);
        std::cout << "Printing message: " << derived->message_ << std::endl;
    }
};

// 子类2：实现一个计算平方的回调
class callback_square : public callback_base
{
  public:
    callback_square(int value) : callback_base(&execute_impl), value_(value) {}

  private:
    int value_;

    static void execute_impl(callback_base *self) noexcept
    {
        auto derived = static_cast<callback_square *>(self);
        int result = derived->value_ * derived->value_;
        std::cout << "Square of " << derived->value_ << " is " << result << std::endl;
    }
};

// 回调管理器
class callback_manager
{
  public:
    void register_callback(callback_base *callback)
    {
        if (!head_)
        {
            head_ = callback;
        }
        else
        {
            callback->next = head_;
            head_ = callback;
        }
    }

    void execute_all()
    {
        callback_base *current = head_;
        while (current)
        {
            current->__execute();
            current = static_cast<callback_base *>(current->next);
        }
    }

  private:
    callback_base *head_ = nullptr;
};

int main()
{
    // 创建回调管理器
    callback_manager manager;

    // 创建不同的回调对象
    callback_print print_callback("Hello, World!");
    callback_square square_callback(5);

    // 注册回调到管理器
    manager.register_callback(&print_callback);
    manager.register_callback(&square_callback);

    // 执行所有注册的回调
    manager.execute_all();

    return 0;
}
// NOLINTEND
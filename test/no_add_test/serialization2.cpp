#include <iostream>
#include <array>
#include <cstddef>
#include <utility>

// NOLINTBEGIN

// 定义 sender 概念
template <typename S>
concept SenderConcept = requires(S s) {
    { s.original_api() } -> std::same_as<void>;
};

// 基础概念类
struct sender_interface
{
    virtual ~sender_interface() = default;
    virtual void original_api() const = 0;
};

// 具体实现包装器 - 只接受符合 SenderConcept 的类型
template <SenderConcept Sender>
struct sender_model final : sender_interface
{
    Sender sender_;

    sender_model(Sender sender) : sender_(std::move(sender)) {}

    void original_api() const override
    {
        sender_.original_api();
    }
};

// 类型擦除容器，使用小对象优化
template <std::size_t Size = 4 * sizeof(void *)>
class any_sender
{
  private:
    // 存储缓冲区
    alignas(alignof(std::max_align_t)) std::array<std::byte, Size> buffer_;

    // 接口指针
    sender_interface *interface_ = nullptr;

    // 获取存储指针
    void *storage()
    {
        return buffer_.data();
    }
    const void *storage() const
    {
        return buffer_.data();
    }

  public:
    // 只接受符合 SenderConcept 的类型
    template <SenderConcept Sender>
    any_sender(Sender sender)
    {
        static_assert(sizeof(sender_model<Sender>) <= Size,
                      "Sender type is too large for internal buffer");

        // 在存储空间中构造对象
        interface_ = new (storage()) sender_model<Sender>(std::move(sender));
    }

    // 默认构造函数
    any_sender() = default;

    // 移动构造
    any_sender(any_sender &&other) noexcept
    {
        if (other.interface_)
        {
            interface_ = other.interface_;
            other.interface_ = nullptr;
        }
    }

    // 移动赋值
    any_sender &operator=(any_sender &&other) noexcept
    {
        if (this != &other)
        {
            if (interface_)
            {
                interface_->~sender_interface();
            }
            interface_ = other.interface_;
            other.interface_ = nullptr;
        }
        return *this;
    }

    // 禁止拷贝
    any_sender(const any_sender &) = delete;
    any_sender &operator=(const any_sender &) = delete;

    ~any_sender()
    {
        if (interface_)
        {
            interface_->~sender_interface();
        }
    }

    // 调用原始 API
    void original_api() const
    {
        if (interface_)
        {
            interface_->original_api();
        }
    }

    // 检查是否有效
    explicit operator bool() const
    {
        return interface_ != nullptr;
    }

    // 访问原始对象
    template <SenderConcept Sender>
    Sender *get_as()
    {
        if (auto model = dynamic_cast<sender_model<Sender> *>(interface_))
        {
            return &model->sender_;
        }
        return nullptr;
    }

    template <SenderConcept Sender>
    const Sender *get_as() const
    {
        if (auto model = dynamic_cast<const sender_model<Sender> *>(interface_))
        {
            return &model->sender_;
        }
        return nullptr;
    }
};

// 调度器概念
template <typename S>
concept SchedulerConcept = requires(S s) {
    { s.schedule() } -> SenderConcept;
};

// 调度器接口
struct scheduler_interface
{
    virtual ~scheduler_interface() = default;
    virtual any_sender<> schedule() = 0;
};

// 调度器实现
template <SchedulerConcept Scheduler>
struct scheduler_model final : scheduler_interface
{
    Scheduler scheduler_;

    scheduler_model(Scheduler scheduler) : scheduler_(std::move(scheduler)) {}

    any_sender<> schedule() override
    {
        return any_sender<>(scheduler_.schedule());
    }
};

// 简化的调度器包装器
class task_scheduler
{
  private:
    // 使用小对象优化存储
    static constexpr size_t SchedulerSize = 4 * sizeof(void *);
    alignas(
        alignof(std::max_align_t)) std::array<std::byte, SchedulerSize> scheduler_buffer_;
    scheduler_interface *scheduler_interface_ = nullptr;

    // 获取存储指针
    void *scheduler_storage()
    {
        return scheduler_buffer_.data();
    }

  public:
    template <SchedulerConcept Scheduler>
    task_scheduler(Scheduler scheduler)
    {
        static_assert(sizeof(scheduler_model<Scheduler>) <= SchedulerSize,
                      "Scheduler type is too large for internal buffer");

        // 在存储空间中构造对象
        scheduler_interface_ =
            new (scheduler_storage()) scheduler_model<Scheduler>(std::move(scheduler));
    }

    // 移动构造
    task_scheduler(task_scheduler &&other) noexcept
    {
        if (other.scheduler_interface_)
        {
            scheduler_interface_ = other.scheduler_interface_;
            other.scheduler_interface_ = nullptr;
        }
    }

    // 移动赋值
    task_scheduler &operator=(task_scheduler &&other) noexcept
    {
        if (this != &other)
        {
            if (scheduler_interface_)
            {
                scheduler_interface_->~scheduler_interface();
            }
            scheduler_interface_ = other.scheduler_interface_;
            other.scheduler_interface_ = nullptr;
        }
        return *this;
    }

    // 禁止拷贝
    task_scheduler(const task_scheduler &) = delete;
    task_scheduler &operator=(const task_scheduler &) = delete;

    ~task_scheduler()
    {
        if (scheduler_interface_)
        {
            scheduler_interface_->~scheduler_interface();
        }
    }

    any_sender<> schedule()
    {
        if (scheduler_interface_)
        {
            return scheduler_interface_->schedule();
        }
        return any_sender<>{}; // 返回空的 any_sender
    }
};

// 测试代码
struct run_loop
{
    int value{};
};

struct scheduler
{
    run_loop *run_loop_;
    struct sender
    {
        run_loop *run_loop_;
        void original_api() const
        {
            std::cout << "scheduler::sender original API called\n";
        }
    };
    auto schedule()
    {
        return sender{run_loop_};
    }
};

struct run_loop2
{
    int value{};
};

struct scheduler2
{
    run_loop2 *run_loop_;
    struct sender2
    {
        run_loop2 *run_loop_;
        void original_api() const
        {
            std::cout << "scheduler2::sender2 original API called\n";
        }
    };
    auto schedule()
    {
        return sender2{run_loop_};
    }
};

int main()
{
    run_loop loop{1};
    run_loop2 loop2{2};

    scheduler sched{&loop};
    scheduler2 sched2{&loop2};

    // 创建类型擦除的调度器
    task_scheduler ts1(sched);
    task_scheduler ts2(sched2);

    // 获取 sender 并调用原始 API
    auto sender1 = ts1.schedule();
    if (sender1)
    {
        sender1.original_api();

        // 访问原始 sender 对象
        if (auto *original_sender = sender1.get_as<scheduler::sender>())
        {
            std::cout << "Accessed original scheduler::sender\n";
        }
    }

    auto sender2 = ts2.schedule();
    if (sender2)
    {
        sender2.original_api();

        // 访问原始 sender 对象
        if (auto *original_sender = sender2.get_as<scheduler2::sender2>())
        {
            std::cout << "Accessed original scheduler2::sender2\n";
        }
    }

    std::cout << "main done\n";
    return 0;
}
// NOLINTEND
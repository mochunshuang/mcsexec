#include <cassert>
#include <iostream>
#include <memory>
#include <functional>

// NOLINTBEGIN
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
    };
    auto schedule()
    {
        return sender2{run_loop_};
    }
};

struct task_scheduler
{
    class sender
    {
        struct sender_concept
        {
            virtual ~sender_concept() = default;
        };
        template <typename Sender>
        struct model : sender_concept
        {
            model(Sender s) : sender_(std::move(s)) {}
            Sender sender_;
        };
        std::unique_ptr<sender_concept> ptr_;

      public:
        template <typename Sender>
        sender(Sender s) : ptr_(new model<Sender>(std::move(s)))
        {
        }
        sender(sender &&) = default;
        sender &operator=(sender &&) = default;
        sender(const sender &) = delete;
        sender &operator=(const sender &) = delete;
    };

    template <class Sch>
    explicit task_scheduler(Sch sch)
        : schedule_impl_([sch]() { return sender(sch->schedule()); })
    {
    }

    sender schedule()
    {
        return schedule_impl_();
    }

  private:
    std::function<sender()> schedule_impl_;
};

int main()
{
    run_loop loop{1};
    run_loop2 loop2{2};

    scheduler sched{&loop};
    scheduler2 sched2{&loop2};

    task_scheduler ts1(&sched);
    task_scheduler ts2(&sched2);

    auto sender1 = ts1.schedule();
    auto sender2 = ts2.schedule();

    std::cout << "main done\n";
    return 0;
}
// NOLINTEND
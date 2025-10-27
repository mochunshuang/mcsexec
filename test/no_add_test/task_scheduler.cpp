#include <iostream>
#include <memory>

// NOLINTBEGIN
namespace __detail
{
    template <std::size_t size = 3 * sizeof(void *),
              std::size_t align = alignof(std::max_align_t)>
    class storage_schedule
    {
      private:
        static constexpr std::size_t BUFFER_SIZE = size;
        static constexpr std::size_t BUFFER_ALIGN = align;

        union storage_union {
            alignas(BUFFER_ALIGN) std::byte stack_buffer[BUFFER_SIZE];
            void *heap_ptr;
        };

      public:
        storage_union storage_;

        // 分配器直接存储在栈上
        alignas(alignof(std::max_align_t)) std::byte
            alloc_storage_[sizeof(std::allocator<std::byte>)];

        storage_schedule() = default;

        template <typename Sch, typename Allocator>
        storage_schedule(Sch &&sch, Allocator alloc)
        {
            using sch_type = std::decay_t<Sch>;

            std::cout << "Constructing storage_schedule:\n";
            std::cout << "  sizeof(Sch) = " << sizeof(sch_type) << "\n";
            std::cout << "  BUFFER_SIZE = " << BUFFER_SIZE << "\n";

            // 1. 存储分配器
            using alloc_type = typename std::allocator_traits<
                Allocator>::template rebind_alloc<std::byte>;
            new (alloc_storage_) alloc_type(std::move(alloc));
            std::cout << "  Allocator stored in fixed buffer\n";

            // 2. 存储调度器对象
            if (sizeof(sch_type) <= BUFFER_SIZE)
            {
                std::cout << "  Using stack storage (small object optimization)\n";
                new (&storage_.stack_buffer) sch_type(std::forward<Sch>(sch));
            }
            else
            {
                std::cout << "  Using heap storage\n";
                // 使用存储的分配器来分配
                auto &alloc_obj = *reinterpret_cast<alloc_type *>(alloc_storage_);
                using rebound_alloc_type = typename std::allocator_traits<
                    alloc_type>::template rebind_alloc<sch_type>;
                rebound_alloc_type rebound_alloc(alloc_obj);

                sch_type *ptr = rebound_alloc.allocate(1);
                new (ptr) sch_type(std::forward<Sch>(sch));
                storage_.heap_ptr = ptr;
            }
        }

        ~storage_schedule()
        {
            std::cout << "Destroying storage_schedule\n";

            // 销毁分配器
            using alloc_type = std::allocator<std::byte>;
            reinterpret_cast<alloc_type *>(alloc_storage_)->~alloc_type();
        }
    };
}; // namespace __detail

class task_scheduler // NOLINT
{
  public:
    template <class Sch, class Allocator = std::allocator<std::byte>>
    explicit task_scheduler(Sch &&sch, Allocator alloc = {})
        : sch_(std::forward<Sch>(sch), std::move(alloc))
    {
        std::cout << "task_scheduler constructed\n";
    }

  private:
    __detail::storage_schedule<> sch_;
};

// 测试
struct small_scheduler
{
    int data[2]; // 8 bytes, fits in buffer
    small_scheduler()
    {
        std::cout << "  small_scheduler constructed\n";
    }
    ~small_scheduler()
    {
        std::cout << "  small_scheduler destroyed\n";
    }
};

struct large_scheduler
{
    int data[10]; // 40 bytes, too large for buffer
    large_scheduler()
    {
        std::cout << "  large_scheduler constructed\n";
    }
    ~large_scheduler()
    {
        std::cout << "  large_scheduler destroyed\n";
    }
};

int main()
{
    std::cout << "=== Testing small scheduler ===\n";
    {
        task_scheduler sched1(small_scheduler{});
    }

    std::cout << "\n=== Testing large scheduler ===\n";
    {
        task_scheduler sched2(large_scheduler{});
    }

    std::cout << "main done\n";
    return 0;
}
// NOLINTEND
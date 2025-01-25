#include <atomic>
#include <cassert>

// NOLINTBEGIN
void test_compare_exchange_weak_failure()
{
    std::atomic<int> shared_value{0}; // 共享变量
    int expect_status = 0;

    // 手动修改 shared_value，确保它与 expect_status 不同
    shared_value.store(1);

    int new_status = 2;
    bool success = shared_value.compare_exchange_weak(
        expect_status, new_status, std::memory_order_acq_rel, std::memory_order_relaxed);
    assert(not success);
    assert(expect_status == 1); // modify by not success
}

int main()
{
    test_compare_exchange_weak_failure();
    return 0;
}
// NOLINTEND
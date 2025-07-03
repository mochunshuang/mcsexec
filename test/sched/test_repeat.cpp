#include "../test_base_head.hpp"
#include <cassert>
#include <concepts>
#include <exception>
#include <thread>

int main()
{
    TEST("repeat starts_on") = []() {
        ex::static_thread_pool<1> pool;
        auto pool_id = pool[0].thread_id();
        auto main_id = std::this_thread::get_id();

        auto wait_start_snd = ex::just(0) | ex::then([&](int id) noexcept {
                                  EXPECT(main_id == std::this_thread::get_id());
                                  bool called = false;
                                  auto nest_repeat = ex::starts_on(
                                      pool.get_scheduler(),
                                      ex::just() | ex::then([&] {
                                          EXPECT(pool_id == std::this_thread::get_id());
                                          called = true;
                                      }));
                                  // NOTE: main_id -> pool_id -> main_id
                                  mcs::this_thread::sync_wait(std::move(nest_repeat));

                                  EXPECT(main_id == std::this_thread::get_id());
                                  assert(called);
                                  return id;
                              });
        mcs::this_thread::sync_wait(std::move(wait_start_snd));
    };

    TEST("repeat starts_on 2 ") = []() {
        constexpr auto k_test_dead_lock = false;
        // NOTE: 死锁 : 出现在 转义到自己线程的 任务部署过程中
        ex::static_thread_pool<1> pool;
        auto pool_id = pool[0].thread_id();
        auto wait_start_snd =
            ex::just(0) | ex::then([&](int id) noexcept {
                EXPECT(pool_id == std::this_thread::get_id());
                if constexpr (k_test_dead_lock)
                {
                    auto nest_repeat =
                        ex::starts_on(pool.get_scheduler(),
                                      ex::just() | ex::then([&] {
                                          EXPECT(pool_id == std::this_thread::get_id());
                                      }));
                    mcs::this_thread::sync_wait(std::move(nest_repeat));
                }
                return id;
            });
        mcs::this_thread::sync_wait(
            ex::starts_on(pool.get_scheduler(), std::move(wait_start_snd)));
    };

    TEST("repeat continues_on") = []() {
        ex::static_thread_pool<1> pool;
        auto pool_id = pool[0].thread_id();
        auto main_id = std::this_thread::get_id();
        // NOTE: continues_on 之后的Sndr的 线程id 才是 scheduler 的
        auto wait_start_snd =
            ex::just(0) | ex::then([&](int id) noexcept {
                EXPECT(main_id == std::this_thread::get_id());
                bool called = false;
                auto nest_repeat =
                    ex::just() |
                    ex::then([&] { EXPECT(main_id == std::this_thread::get_id()); }) |
                    ex::continues_on(pool.get_scheduler()) | ex::then([&] {
                        EXPECT(pool_id == std::this_thread::get_id());
                        called = true;
                    });
                mcs::this_thread::sync_wait(std::move(nest_repeat));

                EXPECT(main_id == std::this_thread::get_id());
                assert(called);
                return id;
            });
        mcs::this_thread::sync_wait(std::move(wait_start_snd) |
                                    ex::continues_on(pool.get_scheduler()));
    };
    TEST("repeat continues_on 2 ") = []() {
        ex::static_thread_pool<1> pool;
        auto pool_id = pool[0].thread_id();
        auto main_id = std::this_thread::get_id();
        // NOTE: continues_on 之后的 线程id 才保证是 scheduler 的
        auto wait_start_snd =
            ex::just(0) | ex::then([&](int id) noexcept {
                EXPECT(main_id == std::this_thread::get_id());
                bool called = false;
                auto nest_repeat =
                    ex::just() |
                    ex::then([&] { EXPECT(main_id == std::this_thread::get_id()); }) |
                    ex::continues_on(pool.get_scheduler()) | ex::then([&] {
                        EXPECT(pool_id == std::this_thread::get_id());

                        // NOTE: 一样死锁。 因为底层的 mutetx 是不能递归的
                        // TODO(mcs) 或许需要支持 同线程 可以 不锁自己？
                        constexpr auto k_test_dead_lock = false;
                        if constexpr (k_test_dead_lock)
                        {
                            bool nest_repeat_called = false;
                            auto nest_repeat_and_nest =
                                ex::just() | ex::continues_on(pool.get_scheduler()) |
                                ex::then([&] {
                                    EXPECT(pool_id == std::this_thread::get_id());
                                    nest_repeat_called = true;
                                });
                            mcs::this_thread::sync_wait(std::move(nest_repeat_and_nest));
                            assert(nest_repeat_called);
                        }

                        called = true;
                    });
                mcs::this_thread::sync_wait(std::move(nest_repeat));

                EXPECT(main_id == std::this_thread::get_id());
                assert(called);
                return id;
            });
        mcs::this_thread::sync_wait(std::move(wait_start_snd) |
                                    ex::continues_on(pool.get_scheduler()) |
                                    ex::then([&](auto) {
                                        EXPECT(main_id != std::this_thread::get_id());
                                        EXPECT(pool_id == std::this_thread::get_id());
                                    }));
    };

    TEST("repeat on  ") = []() {
        ex::static_thread_pool<1> pool;
        auto main_id = std::this_thread::get_id();
        auto pool_id = pool[0].thread_id();

        bool called = false;
        auto snd = ex::just() | ex::then([&] noexcept {
                       EXPECT(main_id != std::this_thread::get_id());
                       EXPECT(pool_id == std::this_thread::get_id());

                       // NOTE: 死锁，没意外
                       // NOTE: 因此 sndr 或 task_sndr 不要提前绑定 线程
                       constexpr auto k_test_dead_lock = false;
                       if constexpr (k_test_dead_lock)
                       {
                           bool nest_repeat_called = false;
                           auto task =
                               ex::on(pool.get_scheduler(),
                                      ex::just() | ex::then([&] {
                                          EXPECT(pool_id == std::this_thread::get_id());
                                          nest_repeat_called = true;
                                      }));
                           mcs::this_thread::sync_wait(std::move(task));
                           assert(nest_repeat_called);
                       }

                       called = true;
                       return 0;
                   });

        // NOTE: 从 pool 线程启动执行sndr 然后 then 回到 main 线程
        auto task = ex::on(pool.get_scheduler(), std::move(snd)) | // NOLINT
                    ex::then([=](int) { EXPECT(main_id == std::this_thread::get_id()); });
        mcs::this_thread::sync_wait(std::move(task)); // NOLINT

        assert(called);
    };

    return 0;
};
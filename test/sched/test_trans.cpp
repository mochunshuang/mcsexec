#include "../test_base_head.hpp"
#include <cassert>
#include <concepts>
#include <exception>
#include <thread>

int main()
{
    TEST("id + on") = []() {
        ex::static_thread_pool<1> pool;
        auto main_id = std::this_thread::get_id();
        auto pool_id = pool[0].thread_id();
        auto snd = ex::just() | ex::then([=] noexcept {
                       EXPECT(main_id != std::this_thread::get_id());
                       EXPECT(pool_id == std::this_thread::get_id());
                       return 0;
                   });

        // NOTE: 从 pool 线程启动执行sndr 然后 then 回到 main 线程
        auto task = ex::on(pool.get_scheduler(), std::move(snd)) | // NOLINT
                    ex::then([=](int) { EXPECT(main_id == std::this_thread::get_id()); });
        mcs::this_thread::sync_wait(std::move(task)); // NOLINT
    };
    TEST("id + on 2") = []() {
        ex::static_thread_pool<1> pool;
        ex::static_thread_pool<1> io_pool;
        auto main_id = std::this_thread::get_id();
        auto io_id = io_pool[0].thread_id();
        auto snd = ex::schedule(io_pool.get_scheduler()) | ex::then([=] noexcept {
                       EXPECT(main_id != std::this_thread::get_id());
                       EXPECT(io_id == std::this_thread::get_id());
                       return 0;
                   });

        // NOTE: 从 pool 线程启动执行sndr 然后 then 回到 main 线程
        auto task = ex::on(pool.get_scheduler(), std::move(snd)) | // NOLINT
                    ex::then([=](int) { EXPECT(main_id == std::this_thread::get_id()); });
        mcs::this_thread::sync_wait(std::move(task)); // NOLINT
    };
    TEST("id + on 3") = []() {
        ex::static_thread_pool<1> pool;
        ex::static_thread_pool<1> io_pool;
        auto main_id = std::this_thread::get_id();
        auto pool_id = pool[0].thread_id();
        auto io_id = io_pool[0].thread_id();
        bool called = false;
        auto snd = ex::schedule(io_pool.get_scheduler()) | ex::then([&] noexcept {
                       EXPECT(main_id != std::this_thread::get_id());

                       // NOTE: 将由 io 线程 启动 task 任务
                       EXPECT(io_id == std::this_thread::get_id());

                       auto task =
                           ex::on(pool.get_scheduler(),
                                  ex::just() | ex::then([&] {
                                      // NOTE: 当前任务会在 on 指定的 线程执行
                                      EXPECT(pool_id == std::this_thread::get_id());
                                      return 0;
                                  })) | // NOLINT
                           ex::then([=](int) {
                               // NOTE: 然后回到 启动 task 即 on 的线程中
                               EXPECT(io_id == std::this_thread::get_id());
                           });
                       mcs::this_thread::sync_wait(std::move(task)); // NOLINT
                       called = true;
                       return 0;
                   });
        mcs::this_thread::sync_wait(std::move(snd)); // NOLINT
        assert(called);
    };

    TEST("id + continues_on") = []() {
        ex::static_thread_pool<1> pool;
        ex::static_thread_pool<1> io_pool;
        auto main_id = std::this_thread::get_id();
        auto io_id = io_pool[0].thread_id();
        auto pool_id = pool[0].thread_id();

        auto start = ex::schedule(io_pool.get_scheduler()) | ex::then([=] noexcept {
                         EXPECT(main_id != std::this_thread::get_id());
                         EXPECT(io_id == std::this_thread::get_id());
                         return 0;
                     });

        // NOTE: io_pool -> pool
        mcs::this_thread::sync_wait(start | ex::continues_on(pool.get_scheduler()) |
                                    ex::then([&](int) {
                                        EXPECT(pool_id == std::this_thread::get_id());
                                        return 0;
                                    })); // NOLINT
    };

    TEST("id + continues_on 2") = []() {
        ex::static_thread_pool<1> pool;
        ex::static_thread_pool<1> io_pool;
        auto main_id = std::this_thread::get_id();
        auto io_id = io_pool[0].thread_id();
        auto pool_id = pool[0].thread_id();

        auto start = ex::schedule(io_pool.get_scheduler()) | ex::then([=] {
                         EXPECT(main_id != std::this_thread::get_id());
                         EXPECT(io_id == std::this_thread::get_id());
                         return 0;
                     });

        // NOTE: 这样组合，更清晰
        auto start2 = ex::just() | ex::let_value([&]() {
                          EXPECT(main_id == std::this_thread::get_id());
                          return start;
                      });

        // NOTE: io_pool -> pool
        mcs::this_thread::sync_wait(start2 | ex::continues_on(pool.get_scheduler()) |
                                    ex::then([&](int) {
                                        EXPECT(pool_id == std::this_thread::get_id());
                                        return 0;
                                    })); // NOLINT
    };
    TEST("id + continues_on 3") = []() {
        ex::static_thread_pool<1> pool;
        ex::static_thread_pool<1> io_pool;
        auto main_id = std::this_thread::get_id();
        auto io_id = io_pool[0].thread_id();
        auto pool_id = pool[0].thread_id();

        // NOTE: io_pool -> pool。 一次性
        bool called = false;
        mcs::this_thread::sync_wait(
            ex::just() | ex::let_value([&]() {
                EXPECT(main_id == std::this_thread::get_id());
                return ex::schedule(io_pool.get_scheduler()) | ex::then([=] {
                           EXPECT(main_id != std::this_thread::get_id());
                           EXPECT(io_id == std::this_thread::get_id());
                           return 0;
                       });
            }) |
            ex::continues_on(pool.get_scheduler()) | ex::then([&](int) {
                EXPECT(pool_id == std::this_thread::get_id());
                called = true;
                return 0;
            })); // NOLINT
        assert(called);
    };

    TEST("id + starts_on") = []() {
        ex::static_thread_pool<1> pool;
        ex::static_thread_pool<1> io_pool;
        auto main_id = std::this_thread::get_id();
        auto io_id = io_pool[0].thread_id();
        auto pool_id = pool[0].thread_id();

        // NOTE: thead_id ==  start_thead_id
        auto wait_start_snd = ex::just(0) | ex::then([&](int id) noexcept {
                                  EXPECT(pool_id == std::this_thread::get_id());
                                  return id;
                              });

        auto input_sndr = ex::schedule(io_pool.get_scheduler()) | ex::then([=] noexcept {
                              EXPECT(main_id != std::this_thread::get_id());
                              EXPECT(io_id == std::this_thread::get_id());
                              return 0;
                          });

        bool called = false;

        // NOTE: io_pool -> pool
        mcs::this_thread::sync_wait(ex::starts_on(
            pool.get_scheduler(),
            input_sndr | ex::then([&](int) {
                // NOTE: let_value 不会启动 而是当作 普通类型 用 then 返回
                return ex::let_value(wait_start_snd | ex::then([&](int) {
                                         // NOTE: no change thead_id
                                         EXPECT(pool_id == std::this_thread::get_id());
                                         called = true;
                                     }));
            }))); // NOLINT
        assert(not called);
    };
    TEST("id + starts_on 2 ") = []() {
        ex::static_thread_pool<1> pool;
        ex::static_thread_pool<1> io_pool;
        auto main_id = std::this_thread::get_id();
        auto io_id = io_pool[0].thread_id();

        // NOTE: thead_id ==  start_thead_id
        auto wait_start_snd = ex::just(0) | ex::then([&](int id) noexcept {
                                  //   EXPECT(pool_id == std::this_thread::get_id());
                                  EXPECT(io_id == std::this_thread::get_id());
                                  return id;
                              });

        auto input_sndr = ex::schedule(io_pool.get_scheduler()) | ex::then([=] noexcept {
                              EXPECT(main_id != std::this_thread::get_id());
                              EXPECT(io_id == std::this_thread::get_id());
                              return 0;
                          });

        bool called = false;

        // NOTE: io_pool -> pool
        constexpr auto k_exepct_value = 2;
        auto [ret] =
            mcs::this_thread::sync_wait(
                ex::starts_on(
                    pool.get_scheduler(),
                    input_sndr | ex::then([&](int) { return; }) | ex::let_value([&] {
                        return wait_start_snd | ex::then([&](int) {
                                   // NOTE: no change thead_id
                                   //    EXPECT(pool_id == std::this_thread::get_id());
                                   EXPECT(io_id == std::this_thread::get_id());
                                   called = true;
                                   return k_exepct_value;
                               });
                    })))
                .value(); // NOLINT

        assert(called);
        assert(ret == k_exepct_value);
    };

    return 0;
};
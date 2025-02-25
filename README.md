partial implementation of the *Senders* model of asynchronous programming proposed by [**P2300 - `std::execution`**](http://wg21.link/p2300)

For the sole purpose of learning c++ templates, without any suggestion or hint

![Build Status](https://github.com/mochunshuang/mcsexec/workflows/linux_gcc_14/badge.svg)
![Build Status](https://github.com/mochunshuang/mcsexec/workflows/windows_clang_19_1/badge.svg)
![Build Status](https://github.com/mochunshuang/mcsexec/workflows/macos_gcc_14/badge.svg)

## Features

- CMake 3.30
- G++ 14.2

## Implementation status

NOTE: Unless otherwise specified, it has already been implemented

All in execution.hpp

### Concepts

- #### Schedulers

- - [execution::scheduler](https://en.cppreference.com/w/cpp/execution/scheduler)

- #### Senders

- - [execution::sender](https://en.cppreference.com/mwiki/index.php?title=cpp/execution/sender&action=edit&redlink=1)
  - [execution::sender_in](https://en.cppreference.com/mwiki/index.php?title=cpp/execution/sender_in&action=edit&redlink=1)
  - [execution::sender_to](https://en.cppreference.com/mwiki/index.php?title=cpp/execution/sender_to&action=edit&redlink=1)

- #### Receivers

- - [execution::receiver](https://en.cppreference.com/mwiki/index.php?title=cpp/execution/receiver&action=edit&redlink=1)
  - [execution::receiver_of](https://en.cppreference.com/mwiki/index.php?title=cpp/execution/receiver_of&action=edit&redlink=1)

- #### Operation states

  - [execution::operation_state](https://en.cppreference.com/mwiki/index.php?title=cpp/execution/operation_state&action=edit&redlink=1)



### Utility components

- #### Execution contexts

  - [execution::run_loop](https://en.cppreference.com/mwiki/index.php?title=cpp/execution/run_loop&action=edit&redlink=1)

- #### Execution domains

  - [execution::default_domain](https://en.cppreference.com/mwiki/index.php?title=cpp/execution/default_domain&action=edit&redlink=1)
  - [execution::transform_sender](https://en.cppreference.com/mwiki/index.php?title=cpp/execution/transform_sender&action=edit&redlink=1)
  - [execution::transform_env](https://en.cppreference.com/mwiki/index.php?title=cpp/execution/transform_env&action=edit&redlink=1)
  - [execution::apply_sender](https://en.cppreference.com/mwiki/index.php?title=cpp/execution/apply_sender&action=edit&redlink=1)

#### Forward progress guarantee

- [execution::forward_progress_guarantee](https://en.cppreference.com/mwiki/index.php?title=cpp/execution/forward_progress_guarantee&action=edit&redlink=1)

#### Environments

- [execution::prop](https://en.cppreference.com/mwiki/index.php?title=cpp/execution/prop&action=edit&redlink=1)
- [execution::env](https://en.cppreference.com/mwiki/index.php?title=cpp/execution/env&action=edit&redlink=1)
- [execution::get_env](https://en.cppreference.com/mwiki/index.php?title=cpp/execution/get_env&action=edit&redlink=1)

#### Queries

- [forwarding_query](https://en.cppreference.com/mwiki/index.php?title=cpp/execution/forwarding_query&action=edit&redlink=1)[execution::forwarding_query]
- [get_allocator](https://en.cppreference.com/mwiki/index.php?title=cpp/execution/get_allocator&action=edit&redlink=1)[execution::get_allocator]
- [get_stop_token](https://en.cppreference.com/mwiki/index.php?title=cpp/execution/get_stop_token&action=edit&redlink=1)[execution::get_stop_token]
- [execution::get_domain](https://en.cppreference.com/mwiki/index.php?title=cpp/execution/get_domain&action=edit&redlink=1)
- [execution::get_scheduler](https://en.cppreference.com/mwiki/index.php?title=cpp/execution/get_scheduler&action=edit&redlink=1)
- [execution::get_delegation_scheduler](https://en.cppreference.com/mwiki/index.php?title=cpp/execution/get_delegation_scheduler&action=edit&redlink=1)
- [execution::get_completion_scheduler](https://en.cppreference.com/mwiki/index.php?title=cpp/execution/get_completion_scheduler&action=edit&redlink=1)
- [execution::get_forward_progress_guarantee](https://en.cppreference.com/mwiki/index.php?title=cpp/execution/get_forward_progress_guarantee&action=edit&redlink=1)

#### Completion signatures

- [execution::completion_signatures](https://en.cppreference.com/mwiki/index.php?title=cpp/execution/completion_signatures&action=edit&redlink=1)
- [execution::get_completion_signatures](https://en.cppreference.com/mwiki/index.php?title=cpp/execution/get_completion_signatures&action=edit&redlink=1)
- [execution::transform_completion_signatures](https://en.cppreference.com/mwiki/index.php?title=cpp/execution/transform_completion_signatures&action=edit&redlink=1)
- [execution::transform_completion_signatures_of](https://en.cppreference.com/mwiki/index.php?title=cpp/execution/transform_completion_signatures_of&action=edit&redlink=1)
- [execution::tag_of_t](https://en.cppreference.com/mwiki/index.php?title=cpp/execution/tag_of_t&action=edit&redlink=1)
- [execution::value_types_of_t](https://en.cppreference.com/mwiki/index.php?title=cpp/execution/value_types_of_t&action=edit&redlink=1)
- [execution::error_types_of_t](https://en.cppreference.com/mwiki/index.php?title=cpp/execution/error_types_of_t&action=edit&redlink=1)
- [execution::sends_stopped](https://en.cppreference.com/mwiki/index.php?title=cpp/execution/sends_stopped&action=edit&redlink=1)

#### Coroutine utility

- [execution::as_awaitable](https://en.cppreference.com/mwiki/index.php?title=cpp/execution/as_awaitable&action=edit&redlink=1)
- [execution::with_awaitable_senders](https://en.cppreference.com/mwiki/index.php?title=cpp/execution/with_awaitable_senders&action=edit&redlink=1)



### Core operations

#### Operation state

- [execution::connect](https://en.cppreference.com/mwiki/index.php?title=cpp/execution/connect&action=edit&redlink=1)
- [execution::start](https://en.cppreference.com/mwiki/index.php?title=cpp/execution/start&action=edit&redlink=1)

#### Completion functions

- [execution::set_value](https://en.cppreference.com/mwiki/index.php?title=cpp/execution/set_value&action=edit&redlink=1)
- [execution::set_error](https://en.cppreference.com/mwiki/index.php?title=cpp/execution/set_error&action=edit&redlink=1)
- [execution::set_stopped](https://en.cppreference.com/mwiki/index.php?title=cpp/execution/set_stopped&action=edit&redlink=1)



### Sender algorithms

#### Sender factories

- [execution::just](https://en.cppreference.com/w/cpp/execution/just)
- [execution::just_error](https://en.cppreference.com/w/cpp/execution/just_error)
- [execution::just_stopped](https://en.cppreference.com/w/cpp/execution/just_stopped)
- [execution::read_env](https://en.cppreference.com/w/cpp/execution/read_env)
- [execution::schedule](https://en.cppreference.com/w/cpp/execution/schedule)

#### Pipeable sender adaptors

- [execution::sender_adaptor_closure](https://en.cppreference.com/mwiki/index.php?title=cpp/execution/sender_adaptor_closure&action=edit&redlink=1)

#### Sender adaptors

- [execution::starts_on](https://en.cppreference.com/mwiki/index.php?title=cpp/execution/starts_on&action=edit&redlink=1)
- [execution::continues_on](https://en.cppreference.com/mwiki/index.php?title=cpp/execution/continues_on&action=edit&redlink=1)
- [execution::on](https://en.cppreference.com/w/cpp/execution/on)[==partial implementation==]
- [execution::schedule_from](https://en.cppreference.com/mwiki/index.php?title=cpp/execution/schedule_from&action=edit&redlink=1)
- [execution::then](https://en.cppreference.com/w/cpp/execution/then)
- [execution::upon_error](https://en.cppreference.com/w/cpp/execution/upon_error)
- [execution::upon_stopped](https://en.cppreference.com/w/cpp/execution/upon_stopped)
- [execution::let_value](https://en.cppreference.com/w/cpp/execution/let_value)
- [execution::let_error](https://en.cppreference.com/w/cpp/execution/let_error)
- [execution::let_stopped](https://en.cppreference.com/w/cpp/execution/let_stopped)
- [execution::bulk; execution::bulk_chunked; execution::bulk_unchunked](https://en.cppreference.com/w/cpp/execution/bulk)
- [execution::split](https://en.cppreference.com/mwiki/index.php?title=cpp/execution/split&action=edit&redlink=1)
- [execution::when_all](https://en.cppreference.com/w/cpp/execution/when_all)
- [execution::when_all_with_variant](https://en.cppreference.com/mwiki/index.php?title=cpp/execution/when_all_with_variant&action=edit&redlink=1)
- [execution::into_variant](https://en.cppreference.com/w/cpp/execution/into_variant)
- [execution::stopped_as_optional](https://en.cppreference.com/w/cpp/execution/stopped_as_optional)
- [execution::stopped_as_error](https://en.cppreference.com/w/cpp/execution/stopped_as_error)

#### Sender consumers

- [this_thread::sync_wait](https://en.cppreference.com/w/cpp/thread/this_thread/sync_wait)
- [this_thread::sync_wait_with_variant](https://en.cppreference.com/mwiki/index.php?title=cpp/thread/this_thread/sync_wait_with_variant&action=edit&redlink=1)



## **example**

~~~c++
#include <iostream>
#include "../include/execution.hpp"

int main()
{
    mcs::execution::sender auto j = mcs::execution::just(3.14, 1);
    mcs::execution::sender auto t =
        mcs::execution::then(std::move(j), [](double d, int i) {
            std::cout << "d: " << d << " i: " << i << "\n";
            return;
        });
    mcs::execution::sender auto t2 =
        mcs::execution::then(std::move(t), []() { std::cout << "then3: " << "\n"; });

    mcs::this_thread::sync_wait(std::move(t2));

    {
        using namespace mcs::execution;
        static_thread_pool<3> thread_pool;
        scheduler auto sched6 = thread_pool.get_scheduler();

        sender auto begin = schedule(sched6);

        auto t0 = then(begin, []() { return 42; });
        auto t1 = then(t0, [](int i) { return i + 1; });
        auto [v] = mcs::this_thread::sync_wait(t1).value();

        std::cout << v << '\n';
    }

    {
        using namespace mcs::execution;
        static_thread_pool<3> thread_pool;
        scheduler auto sched = thread_pool.get_scheduler();

        auto s = schedule(sched) //
                 | then([]() {
                       std::cout << "thread_id: " << std::this_thread::get_id()
                                 << std::endl;
                       return 42;
                   }) //
                 | then([](int i) { return i + 1; });

        auto [val] = mcs::this_thread::sync_wait(s).value();

        std::cout << val << ' ' << "thread_id: " << std::this_thread::get_id()
                  << std::endl;
    }
    return 0;
}
~~~


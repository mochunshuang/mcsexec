#include "../../include/execution.hpp"

#include "__cout_receiver.hpp"
#include "__my_class.hpp"

#include <iostream>
#include <type_traits>
#include <utility>

void base();

int main()
{
    base();
    std::cout << "hello world\n";
    return 0;
}

void base()
{
    using namespace mcs::execution;
    using mcs::execution::cmplsigs::completion_signatures;
    using mcs::execution::set_value_t;

    sender auto input = mcs::execution::just(1);
    static_assert(std::is_same_v<decltype(input.get_completion_signatures(empty_env{})),
                                 completion_signatures<set_value_t(int)>>);

    sender auto multi_shot = split(std::move(input));
    {
#if 1
        sender auto input = mcs::execution::just(1);
        sender auto multi_shot = split(std::move(input));
        // 上来就直接转换了
        auto &[tag, data] = multi_shot;
        using T = decltype(tag);
        static_assert(std::is_same_v<mcs::execution::adapt::split_impl_tag, T>);

        using Sig = decltype(multi_shot.get_completion_signatures(empty_env{}));
        // static_assert(std::is_same_v<Sig, int>);

        // Note: 变成sndr 才行
        static_assert(not sender<decltype(data)>);

        // decltype(sndr)::Sndr_t
        // static_assert(
        //     std::is_same_v<decltype(std::declval<typename decltype(sndr)::Sndr_t>()
        //                                 .get_completion_signatures(empty_env{})),
        //                    completion_signatures<set_value_t(int)>>);

        // using Sndr = std::declval<typename decltype(sndr)::Sndr_t>();
        using Then = mcs::execution::adapt::__split::shared_state<
            mcs::execution::snd::__detail::basic_sender<
                mcs::execution::factories::__just_t<mcs::execution::recv::set_value_t>,
                mcs::execution::snd::__detail::product_type<int>>>;

        using Sndr = mcs::execution::snd::__detail::basic_sender<
            mcs::execution::factories::__just_t<mcs::execution::recv::set_value_t>,
            mcs::execution::snd::__detail::product_type<int>>;
        static_assert(
            std::is_same_v<decltype(std::declval<Sndr>().get_completion_signatures(
                               empty_env{})),
                           completion_signatures<set_value_t(int)>>);

        using S = mcs::execution::adapt::__split::shared_wrapper<
            mcs::execution::adapt::__split::shared_state<
                mcs::execution::snd::__detail::basic_sender<
                    mcs::execution::factories::__just_t<
                        mcs::execution::recv::set_value_t>,
                    mcs::execution::snd::__detail::product_type<int>>>,
            mcs::execution::adapt::split_t>;
        static_assert(std::is_same_v<std::remove_cvref_t<decltype(data)>, S>);
        // static_assert(std::is_same_v<Sig, S>);
        static_assert(std::is_same_v<Sig, completion_signatures<set_value_t(int)>>);
#endif
    }
    sender auto start =
        then(multi_shot, [](int i) { std::cout << i << " First continuation\n"; });
    mcs::this_thread::sync_wait(std::move(start));
}
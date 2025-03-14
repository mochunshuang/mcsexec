#include "../test_base_head.hpp"
#include <iostream>
#include <memory_resource>
#include <vector>

// NOLINTBEGIN

template <typename T>
class simple_allocator
{
  public:
    using value_type = T;

    simple_allocator() : resource_(std::pmr::get_default_resource()) {}
    explicit simple_allocator(std::pmr::memory_resource *resource) : resource_(resource)
    {
    }

    template <typename U>
    simple_allocator(const simple_allocator<U> &other) noexcept
        : resource_(other.resource_)
    {
    }

    T *allocate(std::size_t n)
    {
        return static_cast<T *>(resource_->allocate(n * sizeof(T), alignof(T)));
    }

    void deallocate(T *p, std::size_t n)
    {
        resource_->deallocate(p, n * sizeof(T), alignof(T));
    }

    std::pmr::memory_resource *resource() const
    {
        return resource_;
    }

  private:
    std::pmr::memory_resource *resource_;

    template <typename U>
    friend class simple_allocator;
};

template <typename T, typename U>
bool operator==(const simple_allocator<T> &a, const simple_allocator<U> &b)
{
    return a.resource() == b.resource();
}

template <typename T, typename U>
bool operator!=(const simple_allocator<T> &a, const simple_allocator<U> &b)
{
    return !(a == b);
}

void base()
{
    std::pmr::monotonic_buffer_resource pool;
    simple_allocator<int> alloc(&pool);

    std::vector<int, simple_allocator<int>> vec(alloc);
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);

    for (int i : vec)
    {
        std::cout << i << " ";
    }
    std::cout << std::endl;

    static_assert(ex::simple_allocator<decltype(alloc)>);
}

int main()
{
    base();

    TEST("prop + env") = [] {
        auto q = ex::prop(ex::get_allocator, simple_allocator<int>{});
        auto env = ex::get_allocator(q);
        static_assert(std::is_same_v<decltype(env), simple_allocator<int>>);
    };

    TEST("JOIN_ENV") = [] {
        using E0 = decltype(ex::prop(ex::get_stop_token, ex::never_stop_token()));
        using E1 = decltype(ex::prop(ex::get_allocator, simple_allocator<int>{}));
        auto e2 = ex::snd::general::JOIN_ENV<E0, E1>(E0{}, E1{});
        static_assert(
            std::is_same_v<decltype(ex::get_stop_token(e2)), ex::never_stop_token>);
        static_assert(
            std::is_same_v<decltype(ex::get_allocator(e2)), simple_allocator<int>>);
    };
    TEST("FWD_ENV") = [] {
        using E0 = decltype(ex::prop(ex::get_stop_token, ex::never_stop_token()));
        using E1 = decltype(ex::prop(ex::get_allocator, simple_allocator<int>{}));
        auto e2 = ex::snd::general::JOIN_ENV<E0, E1>(E0{}, E1{});

        // NOLINT
        auto f0 = ex::snd::general::FWD_ENV(E0{});
        auto f1 = ex::snd::general::FWD_ENV(E1{});
        auto f2 = ex::snd::general::FWD_ENV(e2);
        static_assert(
            std::is_same_v<decltype(ex::get_stop_token(f0)), ex::never_stop_token>);
        static_assert(
            std::is_same_v<decltype(ex::get_allocator(f1)), simple_allocator<int>>);

        static_assert(
            std::is_same_v<decltype(ex::get_stop_token(f2)), ex::never_stop_token>);
        static_assert(
            std::is_same_v<decltype(ex::get_allocator(f2)), simple_allocator<int>>);
    };
    TEST("FWD_ENV env<>{}") = [] {
        auto f0 = ex::snd::general::FWD_ENV(ex::env{});
        // NOTE: 会调用 复制构造，因此下面的类型相等
        auto f1 = ex::snd::general::FWD_ENV(f0);
        auto f2 = ex::snd::general::FWD_ENV(f1);
        static_assert(std::is_same_v<decltype(f1), decltype(f0)>);
        static_assert(std::is_same_v<decltype(f2), decltype(f0)>);
    };
    // write-env is an exposition-only sender adaptor that, when connected with a
    // receiver rcvr, connects the adapted sender with a receiver whose execution
    // environment is the result of joining the queryable argument env to the result
    // of get_env(rcvr). NOTE: sndr.get_env() 结果要么是空，要么是 child.get_env()
    // NOTE: write-env 仅仅在 recv + get_env一起使用才有改变，get_env()不受到影响
    TEST("write-env test: get_env()") = [] {
        auto sndr = ex::just(1);
        // NOTE: 编译错误 因为 sndr.get_env() 是  env<>
        // ex::get_allocator(sndr.get_env());

        auto s = ex::write_env(std::move(sndr),
                               ex::prop(ex::get_stop_token, ex::never_stop_token()));
        auto e = ex::get_stop_token(s.get_env());
        static_assert(std::is_same_v<decltype(e), ex::never_stop_token>);

        auto s2 = ex::write_env(std::move(s),
                                ex::prop(ex::get_allocator, simple_allocator<int>{}));

        // NOTE: 编译错误 因为 s2.get_env()转发给 s.get_env() 因此没毛病
        // ex::get_allocator(s2.get_env());
        // NOTE: 成功是因为 s 确定能查到
        auto e2 = ex::get_stop_token(s2.get_env());
        static_assert(std::is_same_v<decltype(e2), decltype(e)>);

        // ::mcs::execution::snd::general::FWD_ENV<env<>>
        auto v0 = ex::just().get_env(); // env<>
        auto v1 = s.get_env();          //  env<>
        auto v2 = s2.get_env();
        static_assert(std::is_same_v<decltype(v0), ex::env<>>);
        static_assert(std::is_same_v<decltype(v1), ex::snd::general::FWD_ENV<ex::env<>>>);
        static_assert(std::is_same_v<decltype(v1), decltype(v2)>);
    };
    std::cout << "main done\n";
    return 0;
}
// NOLINTEND
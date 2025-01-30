#include "../test_base_head.hpp"

template <class Env = mcs::execution::empty_env>
class base_expect_receiver // NOLINT
{
    std::atomic<bool> m_called{false};
    Env m_env{};

  public:
    using receiver_concept = mcs::execution::receiver_t;
    base_expect_receiver() = default;

    ~base_expect_receiver()
    {
        EXPECT(m_called.load());
    }

    explicit base_expect_receiver(Env env) : m_env(std::move(env)) {}

    base_expect_receiver(base_expect_receiver &&other) noexcept
        : m_called(other.m_called.exchange(true)), m_env(std::move(other.m_env))
    {
    }

    base_expect_receiver &operator=(base_expect_receiver &&other) = delete;

    base_expect_receiver(const base_expect_receiver &other) noexcept
        : m_called(other.m_called.load()), m_env(std::move(other.m_env)) {};
    base_expect_receiver &operator=(const base_expect_receiver &other) = default;

    void set_called() // NOLINT
    {
        m_called.store(true);
    }

    Env get_env() const noexcept // NOLINT
    {
        return m_env;
    }
};
int main()
{
    // 测试左值引用
    static_assert(std::constructible_from<std::remove_cv_t<base_expect_receiver<> &>,
                                          base_expect_receiver<> &>);

    return 0;
}
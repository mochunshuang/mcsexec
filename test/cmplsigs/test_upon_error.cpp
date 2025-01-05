
#include "../test_base_head.hpp"

struct Error1
{
};

struct Error2
{
};

struct Error3
{
};

struct Error4
{
};
template <class... AdditionalCompletions>
struct many_error_sender
{
    using sender_concept = ex::sender_t;
    using completion_signatures =
        ex::completion_signatures<AdditionalCompletions..., ex::set_error_t(Error1),
                                  ex::set_error_t(Error2), ex::set_error_t(Error3)>;

    auto get_completion_signatures( // NOLINT
        const auto & /*env*/) noexcept -> completion_signatures
    {
        return {};
    }
    decltype(auto) get_env() const noexcept // NOLINT
    {
        return ex::empty_env{};
    }
};

int main()
{
    TEST("upon_error many input error types") = [] {
        auto pre_sndr = many_error_sender{};
        using CS = decltype(pre_sndr.get_completion_signatures(ex::empty_env{}));

        using P_CS =
            ex::snd::completion_signatures_of_t<many_error_sender<>, ex::empty_env>;
        static_assert(std::is_same_v<CS, P_CS>);

        auto sndr [[maybe_unused]] = many_error_sender{} | ex::upon_error([](auto e) {
                                         if constexpr (std::same_as<decltype(e), Error3>)
                                         {
                                             return Error4{};
                                         }
                                         else
                                         {
                                             return e;
                                         }
                                     });
        // Note: 就是这样的
        using T = ex::snd::completion_signatures_of_t<decltype(sndr), ex::empty_env>;
        static_assert(
            ex::tool::eq_set_sigs_v<T,
                                    ex::cmplsigs::completion_signatures<
                                        ex::set_error_t(std::exception_ptr), // 错误
                                        // set_error_t => set_value_t
                                        ex::set_value_t(Error1), ex::set_value_t(Error2),
                                        ex::set_value_t(Error4)>>);
        {
            auto sndr = many_error_sender<ex::set_value_t(int)>{} |
                        ex::upon_error([](auto) { return 0; });
            using T = ex::snd::completion_signatures_of_t<decltype(sndr), ex::empty_env>;
            static_assert(
                ex::tool::eq_set_sigs_v<T,
                                        ex::cmplsigs::completion_signatures<
                                            ex::set_error_t(std::exception_ptr), // 错误
                                            // set_error_t => set_value_t
                                            ex::set_value_t(int)>>);
        }
        {
            auto sndr = many_error_sender<ex::set_value_t(double)>{} |
                        ex::upon_error([](auto) { return 0; });
            using T = ex::snd::completion_signatures_of_t<decltype(sndr), ex::empty_env>;
            static_assert(ex::tool::eq_set_sigs_v<
                          T,
                          ex::cmplsigs::completion_signatures<
                              ex::set_error_t(std::exception_ptr), // 错误
                              // set_error_t => set_value_t
                              ex::set_value_t(int), ex::set_value_t(double)>>);
        }
    };

    return 0;
}
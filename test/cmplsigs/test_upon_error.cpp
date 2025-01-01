
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

struct many_error_sender
{
    using sender_concept = ex::sender_t;
    using completion_signatures =
        ex::completion_signatures<ex::set_error_t(Error1), ex::set_error_t(Error2),
                                  ex::set_error_t(Error3)>;

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
            ex::cmplsigs::get_completion_signatures<many_error_sender, ex::empty_env>;
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
        // TODO function_traits 对模板的lambda 失败
        // using T = ex::cmplsigs::get_completion_signatures<decltype(sndr),
        // ex::empty_env>;
    };

    return 0;
}
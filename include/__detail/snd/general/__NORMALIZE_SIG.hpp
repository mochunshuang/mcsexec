#pragma once
#include <type_traits>

namespace mcs::execution::snd::general
{
    namespace __detail
    {
        template <typename T>
        using remove_rvalue_reference_t =
            std::conditional_t<std::is_rvalue_reference_v<T>, // 如果是右值引用
                               std::remove_reference_t<T>,    // 移除引用
                               T                              // 否则保留原类型
                               >;
        template <typename T>
        struct _NORMALIZE_SIG;
        template <typename R, typename... Args>
        struct _NORMALIZE_SIG<R(Args...)>
        {
            using type = R(remove_rvalue_reference_t<Args>...);
        };

    }; // namespace __detail

    template <typename T>
    using NORMALIZE_SIG = typename __detail::_NORMALIZE_SIG<T>::type;

}; // namespace mcs::execution::snd::general
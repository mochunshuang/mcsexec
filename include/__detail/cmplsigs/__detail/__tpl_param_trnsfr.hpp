#pragma once

namespace mcs::execution::cmplsigs::__detail
{
    template <template <typename...> class TargetTemplate, typename Source>
    struct template_parameter_transfer;

    template <template <typename...> class TargetTemplate,
              template <typename...> class List, typename... Ts>
    struct template_parameter_transfer<TargetTemplate, List<Ts...>>
    {
        using type = TargetTemplate<Ts...>;
    };

    // 辅助类型别名
    template <template <typename...> class TargetTemplate, typename Source>
    using tpl_param_trnsfr_t =
        typename template_parameter_transfer<TargetTemplate, Source>::type;

}; // namespace mcs::execution::cmplsigs::__detail
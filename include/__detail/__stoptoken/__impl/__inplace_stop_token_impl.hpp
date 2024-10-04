#pragma once

#include <utility>

#include "../__inplace_stop_token.hpp"
#include "../__inplace_stop_source.hpp"

namespace mcs::execution::stoptoken
{
    /**
     * @brief As specified in [basic.life], the behavior of stop_requested() is undefined
     * unless the call strongly happens before the start of the destructor of the
     * associated inplace_stop_source, if any
     *
     * @return true
     * @return false
     */
    inline bool inplace_stop_token::stop_requested() const noexcept // NOLINT
    {
        // Note: must call before the start of the destructor
        return stop_source != nullptr && stop_source->stop_requested();
    }

    /**
     * @brief As specified in [basic.stc.general], the behavior of stop_possible() is
     * implementation-defined unless the call strongly happens before the end of the
     * storage duration of the associated inplace_stop_source object, if any
     *
     * @return true
     * @return false
     */
    inline bool inplace_stop_token::stop_possible() const noexcept
    {
        // Note: happens before the end of the associated inplace_stop_source
        return stop_source != nullptr;
    }

    inline void inplace_stop_token::swap(inplace_stop_token &rhs) noexcept
    {
        std::swap(this->stop_source, rhs.stop_source);
    }

}; // namespace mcs::execution::stoptoken
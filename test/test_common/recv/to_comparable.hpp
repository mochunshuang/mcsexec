#pragma once

#include <exception>
#include <string>
#include <typeinfo>
#include <utility>

namespace test
{

    inline std::pair<const std::type_info &, std::string> to_comparable(
        std::exception_ptr eptr)
    {
        try
        {
            std::rethrow_exception(std::move(eptr));
        }
        catch (const std::exception &e)
        {
            return {typeid(e), e.what()};
        }
        catch (...)
        {
            return {typeid(void), "<unknown>"};
        }
    }

    template <class T>
    inline const T &to_comparable(const T &value)
    {
        return value; // NOLINT
    }

}; // namespace test
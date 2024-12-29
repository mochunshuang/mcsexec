
#include <boost/ut.hpp>
#include <expected>
#include <iostream>

int main()
{
    using namespace boost::ut;

    "auto test"_test = [] {
        std::cout << " auto test\n";
    };

    "lazy log"_test = [] {
        std::cout << " lazy log\n";
        std::expected<bool, std::string> e = std::unexpected("lazy evaluated");
        expect(e.has_value()) << [&] {
            return e.error();
        } << fatal;
        expect(e.value());
    };
    return 0;
}
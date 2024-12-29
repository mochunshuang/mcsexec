#include <any>
#include <iostream>
#include <stdexcept>
#include <tuple>
#include <string>
#include <exception>

// 自定义结构体
struct MyStruct
{
    int value;
    std::string name;
};

// 打印 any 中存储的值
template <typename T> // NOLINTNEXTLINE
void printAnyValue(const std::any &anyValue, const std::string &typeName)
{
    try
    {
        T value = std::any_cast<T>(anyValue);
        std::cout << "Stored " << typeName << ": " << value << '\n';
    }
    catch (const std::bad_any_cast &e)
    {
        std::cout << "Failed to cast to " << typeName << ": " << e.what() << '\n';
    }
}

// 测试存储基本类型
void testBasicTypes() // NOLINT
{
    std::cout << "=== Testing Basic Types ===\n";
    std::any anyInt = 42;
    std::any anyDouble = 3.14;
    std::any anyString = std::string("Hello, std::any!");

    printAnyValue<int>(anyInt, "int");
    printAnyValue<double>(anyDouble, "double");
    printAnyValue<std::string>(anyString, "string");
    std::cout << '\n';
}

// 测试存储自定义结构体
void testCustomStruct() // NOLINT
{
    std::cout << "=== Testing Custom Struct ===\n";
    std::any anyStruct = MyStruct{.value = 10, .name = "Test"};
    try
    {
        auto myStruct = std::any_cast<MyStruct>(anyStruct);
        std::cout << "Stored struct: value=" << myStruct.value
                  << ", name=" << myStruct.name << '\n';
    }
    catch (const std::bad_any_cast &e)
    {
        std::cout << "Failed to cast to MyStruct: " << e.what() << '\n';
    }
    std::cout << '\n';
}

// 测试存储 std::tuple
void testTuple() // NOLINT
{
    std::cout << "=== Testing Tuple ===\n";
    std::any anyTuple = std::make_tuple(1, 2.5, "Tuple");
    try
    {
        auto tuple = std::any_cast<std::tuple<int, double, std::string>>(anyTuple);
        std::cout << "Stored tuple: " << std::get<0>(tuple) << ", " << std::get<1>(tuple)
                  << ", " << std::get<2>(tuple) << '\n';
    }
    catch (const std::bad_any_cast &e)
    {
        std::cout << "Failed to cast to tuple: " << e.what() << '\n';
    }
    std::cout << '\n';
}

// 测试存储异常对象
void testException() // NOLINT
{
    std::cout << "=== Testing Exception ===\n";
    try
    {
        throw std::runtime_error("This is a runtime error!");
    }
    catch (const std::exception &e)
    {
        std::any anyException = e;
        try
        {
            std::cout << "Stored exception: "
                      << std::any_cast<std::runtime_error>(anyException).what() << '\n';
        }
        catch (const std::bad_any_cast &e)
        {
            std::cout << "Failed to cast to runtime_error: " << e.what() << '\n';
        }
    }
    std::cout << '\n';
}

// 测试存储异常指针
void testExceptionPtr() // NOLINT
{
    std::cout << "=== Testing Exception Pointer ===\n";
    std::exception_ptr eptr;
    try
    {
        throw std::logic_error("This is a logic error!");
    }
    catch (...)
    {
        eptr = std::current_exception();
    }
    std::any anyExceptionPtr = eptr;
    try
    {
        if (std::any_cast<std::exception_ptr>(anyExceptionPtr))
        {
            std::rethrow_exception(std::any_cast<std::exception_ptr>(anyExceptionPtr));
        }
    }
    catch (const std::exception &e)
    {
        std::cout << "Stored exception_ptr: " << e.what() << '\n';
    }
    std::cout << '\n';
}

// 测试类型不匹配时的 bad_any_cast
void testBadAnyCast() // NOLINT
{
    std::cout << "=== Testing Bad Any Cast ===\n";
    std::any anyInt = 42;
    try
    {
        std::any_cast<float>(anyInt); // 尝试将 int 转换为 float
    }
    catch (const std::bad_any_cast &e)
    {
        std::cout << "Caught bad_any_cast: " << e.what() << '\n';
    }
    std::cout << '\n';
}

int main()
{
    testBasicTypes();
    testCustomStruct();
    testTuple();
    testException();
    testExceptionPtr();
    testBadAnyCast();

    std::cout << "All tests completed." << std::endl;
    return 0;
}
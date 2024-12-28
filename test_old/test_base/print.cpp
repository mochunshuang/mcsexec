#include <filesystem>
#include <iostream>

#include <print>

namespace fs = std::filesystem;

auto base(char *path)
{
    // 获取当前程序名
    std::string program_name = fs::path(path).stem().string();
    // 构建日志文件路径
    fs::path log_file_path = fs::current_path() / (program_name + "_log.txt");
    std::cout << "日志文件路径: " << log_file_path << std::endl;
    return log_file_path;
}
void test();
int main(int argc, char **argv)
{
    std::cout << argv[0] << '\n';
    base(argv[0]);

    test();
    return 0;
}
void test()
{
    std::print("{0} {2}{1}!\n", "Hello", 23, "C++"); // 重载 (1)
}

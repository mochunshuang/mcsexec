#include <cstdint>
#include <format>
#include <iomanip>
#include <iostream>
#include <source_location>
#include <print>
#include <string>
#include <string_view>
#include <ranges>
// NOLINTBEGIN
struct mcslog
{

    enum class LOG_LEVEL : std::uint8_t
    {
        TRACE = 0,
        DEBUG,
        INFO,
        WARN,
        ERROR,
        FATAL,
        END
    };
    // NOLINTBEGIN
    // 经典主题 (推荐)
    struct classic_theme
    {
        static constexpr const char *colors[] = {
            "\033[37m",     // TRACE: 白色
            "\033[36m",     // DEBUG: 青色
            "\033[32m",     // INFO:  绿色
            "\033[33m",     // WARN:  黄色
            "\033[31m",     // ERROR: 红色
            "\033[1;41;37m" // FATAL: 白字红底粗体 (最醒目)
        };
    };

    // 专业主题
    struct professional_theme
    {
        static constexpr const char *colors[] = {
            "\033[2;37m",   // TRACE: 淡灰色
            "\033[34m",     // DEBUG: 蓝色
            "\033[32m",     // INFO:  绿色
            "\033[33m",     // WARN:  黄色
            "\033[35m",     // ERROR: 紫色
            "\033[1;45;37m" // FATAL: 白字紫红底粗体
        };
    };

    // 明亮主题
    struct bright_theme
    {
        static constexpr const char *colors[] = {
            "\033[90m",      // TRACE: 灰色
            "\033[96m",      // DEBUG: 亮青色
            "\033[92m",      // INFO:  亮绿色
            "\033[93m",      // WARN:  亮黄色
            "\033[91m",      // ERROR: 亮红色
            "\033[1;101;97m" // FATAL: 亮白字亮红底粗体
        };
    };

    // 简约主题 (只突出重要级别)
    struct minimal_theme
    {
        static constexpr const char *colors[] = {
            "\033[0m",     // TRACE: 默认色
            "\033[0m",     // DEBUG: 默认色
            "\033[32m",    // INFO:  绿色
            "\033[33m",    // WARN:  黄色
            "\033[31m",    // ERROR: 红色
            "\033[1;5;31m" // FATAL: 红色粗体闪烁 (非常醒目)
        };
    };
    // 复古主题 (终端经典风格)
    struct retro_theme
    {
        static constexpr const char *colors[] = {
            "\033[0m",       // TRACE: 默认色
            "\033[0m",       // DEBUG: 默认色
            "\033[0m",       // INFO:  默认色
            "\033[33m",      // WARN:  黄色
            "\033[31m",      // ERROR: 红色
            "\033[1;5;7;31m" // FATAL: 反白闪烁红色粗体 (最经典醒目)
        };
    };
    // NOLINTEND
    static void print_color(std::source_location source = std::source_location::current())
    {
        constexpr char reset[] = {"\033[0m"}; // NOLINT

        constexpr auto array = classic_theme::colors; // NOLINT
        constexpr auto fun_color = "\033[38;2;255;165;0m";
        static constexpr std::string_view log_level[] = {
            "TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"}; // NOLINTNEXTLINE
        constexpr decltype(array) all_theme[] = {
            classic_theme::colors, professional_theme::colors, bright_theme::colors,
            minimal_theme::colors, retro_theme::colors};
        for (const auto &theme : all_theme)
        {
            for (int i = 0; i < static_cast<uint8_t>(LOG_LEVEL::END); ++i)
            {
                std::println("{}[{:^6}] [{}:{}:{}{}{}]: {}{}", theme[i], log_level[i],
                             source.file_name(), source.line(), fun_color,
                             source.function_name(), theme[i],
                             std::string{"hello msg..."}, reset);
            }
            std::println();
        }
    }

    template <typename... Args>
    static void debug(std::format_string<Args...> fmt, Args &&...args,
                      std::source_location source = std::source_location::current())
    {
        // std::println("{}[{:>9}]{} [{}:{}]: {}", AnsiColor::bold, "DEBUG",
        //              AnsiColor::reset, source.file_name(), source.line(),
        //              std::format(fmt, std::forward<Args>(args)...));
        // std::println("{}", std::format(fmt, std::forward<Args>(args)...));
    }
};

template <typename... As>
void print(As &&...as)
{
    mcslog::print_color();
}

// ANSI重置代码
const std::string RESET = "\033[0m";

// 演示16种基本颜色（30-37, 90-97 前景色）
void demo_16_colors()
{
    std::cout << "16 Basic Colors (Foreground):" << std::endl;
    // 标准色
    for (int i = 30; i <= 37; i++)
    {
        std::string color_code = "\033[" + std::to_string(i) + "m";
        std::cout << color_code << std::setw(4) << i << " Hello" << RESET << "  ";
    }
    std::cout << std::endl;
    // 亮色
    for (int i = 90; i <= 97; i++)
    {
        std::string color_code = "\033[" + std::to_string(i) + "m";
        std::cout << color_code << std::setw(4) << i << " Hello" << RESET << "  ";
    }
    std::cout << std::endl << std::endl;
}

// 演示256色（使用38;5;n序列）
void demo_256_colors()
{
    std::cout << "256 Colors (38;5;n):" << std::endl;
    // 只打印部分颜色示例，避免输出过长
    int count = 0;
    for (int color : std::views::iota(0u, 256u))
    {
        std::string color_code = "\033[38;5;" + std::to_string(color) + "m";
        std::cout << color_code << std::setw(4) << color << " Test" << RESET << "  ";
        count++;
        if (count % 8 == 0)
            std::cout << std::endl;
    }
    std::cout << std::endl << std::endl;
}

// 演示RGB真彩色（使用38;2;R;G;B序列）
void demo_rgb_colors()
{
    std::cout << "RGB True Colors (38;2;R;G;B):" << std::endl;
    // 定义一些RGB示例颜色
    int colors[][3] = {
        {255, 0, 0},     // 红
        {0, 255, 0},     // 绿
        {0, 0, 255},     // 蓝
        {255, 255, 0},   // 黄
        {255, 0, 255},   // 品红
        {0, 255, 255},   // 青
        {128, 0, 128},   // 紫
        {255, 165, 0},   // 橙
        {128, 128, 128}, // 灰
        {0, 128, 0},     // 深绿
        {75, 0, 130},    // 靛蓝
        {210, 180, 140}  // 棕褐色
    };
    int num_colors = sizeof(colors) / sizeof(colors[0]);
    for (int i = 0; i < num_colors; i++)
    {
        int r = colors[i][0];
        int g = colors[i][1];
        int b = colors[i][2];
        std::string color_code = "\033[38;2;" + std::to_string(r) + ";" +
                                 std::to_string(g) + ";" + std::to_string(b) + "m";
        std::cout << color_code << "RGB(" << std::setw(3) << r << "," << std::setw(3) << g
                  << "," << std::setw(3) << b << ") Hello" << RESET;
        if ((i + 1) % 3 == 0)
            std::cout << std::endl;
        else
            std::cout << "  ";
    }
    std::cout << std::endl;
}

template <size_t N>
constexpr auto make_format_string()
{
    std::string result;
    for (size_t i = 0; i < N; ++i)
    {
        if (i > 0)
            result += " "; // 可选分隔符
        result += "{}";
    }
    return result;
}

template <typename... As>
void print_pack(As &&...as)
{

    if constexpr (sizeof...(As) > 0)
    {
        std::string fmt_str;
        const char *sep = "";
        auto append = [&](const auto &) {
            fmt_str += sep;
            fmt_str += "{}";
            sep = " ";
        };
        (append(std::forward<As>(as)), ...);
        std::println("{}", std::vformat(fmt_str, std::make_format_args(as...)));
    }
}

int main()
{
    mcslog::print_color();
    // mcslog::debug("123");
    print(1);
    print("asd", 1, 2.0, std::string{});
    {
        const char *reset = "\033[0m";

        // 直接循环生成24种不同的256色
        for (int i = 0; i < 24; ++i)
        {
            int color_code = 16 + i * 10; // 从16开始，间隔10，获得明显不同的颜色
            std::cout << "\033[38;5;" << color_code << "mHello " << reset << "(Color "
                      << color_code << ")" << std::endl;
        }
    }
    {
        // 彩虹色系 + 扩展色
        const char *rainbow_colors[] = {
            // 红色系
            "\033[38;5;196m", "\033[38;5;202m", "\033[38;5;208m", "\033[38;5;214m",

            // 黄色系
            "\033[38;5;220m", "\033[38;5;226m", "\033[38;5;190m", "\033[38;5;154m",

            // 绿色系
            "\033[38;5;46m", "\033[38;5;48m", "\033[38;5;51m", "\033[38;5;45m",

            // 蓝色系
            "\033[38;5;39m", "\033[38;5;33m", "\033[38;5;27m", "\033[38;5;21m",

            // 紫色系
            "\033[38;5;57m", "\033[38;5;93m", "\033[38;5;129m", "\033[38;5;165m",

            // 粉色系
            "\033[38;5;201m", "\033[38;5;207m", "\033[38;5;213m", "\033[38;5;219m"};

        const char *reset = "\033[0m";

        for (int i = 0; i < 24; ++i)
        {
            std::cout << rainbow_colors[i] << "Hello World!" << reset << " - Color "
                      << (i + 1) << std::endl;
        }
    }
    {
        // 基础8色 + 亮色8色 + 扩展8色 = 24色
        const char *colors[] = {// 基础8色
                                "\033[30m", "\033[31m", "\033[32m", "\033[33m",
                                "\033[34m", "\033[35m", "\033[36m", "\033[37m",

                                // 亮色8色
                                "\033[90m", "\033[91m", "\033[92m", "\033[93m",
                                "\033[94m", "\033[95m", "\033[96m", "\033[97m",

                                // 扩展8色 (256色中的常用色)
                                "\033[38;5;196m", "\033[38;5;46m", "\033[38;5;51m",
                                "\033[38;5;226m", "\033[38;5;201m", "\033[38;5;214m",
                                "\033[38;5;123m", "\033[38;5;129m"};

        const char *reset = "\033[0m";

        for (int i = 0; i < 24; ++i)
        {
            std::cout << colors[i] << "Hello " << reset << "(Color " << (i + 1) << ")"
                      << std::endl;
        }
    }

    std::cout << "ANSI Color Support Demonstration" << std::endl;
    std::cout << "=================================" << std::endl << std::endl;

    demo_16_colors();
    demo_256_colors();
    demo_rgb_colors();

    {
        // 测试不同类型和数量的参数
        print_pack(10, 3.14, "hello", 'A', true);
        // 输出：10 3.14 hello A true

        print_pack("user", 42, std::string{"example"});
        // 输出：user 42 example
    }
    std::cout << "main done\n";
    return 0;
}
// NOLINTEND
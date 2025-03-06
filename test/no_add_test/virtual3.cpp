#include <iostream>
#include <chrono>
#include <vector>
#include <functional>
#include <memory>
#include <concepts>
#include <algorithm>
// NOLINTBEGIN
// 预热循环，避免冷启动误差
void warmup()
{
    volatile int64_t sum = 0;
    for (int i = 0; i < 1e6; ++i)
    {
        sum += i;
    }
}

// 虚函数版本
struct VirtualShape
{
    virtual void draw() const = 0;
    virtual ~VirtualShape() = default;
};

struct VirtualCircle : VirtualShape
{
    void draw() const override
    { /* 空操作，避免IO影响计时 */
    }
};

// 函数指针版本
struct FuncPtrShape
{
    using DrawFunc = void (*)(const FuncPtrShape *) noexcept;
    DrawFunc draw_func;
};

void circle_draw(const FuncPtrShape *self) noexcept
{ /* 空操作 */
}

// std::function版本
struct StdFunctionShape
{
    std::function<void()> draw;
};

// 类型擦除版本
class TypeErasedShape
{
    struct Concept
    {
        virtual void draw() const = 0;
        virtual ~Concept() = default;
    };

    template <typename T>
    struct Model : Concept
    {
        T obj;
        Model(T o) : obj(std::move(o)) {}
        void draw() const override
        {
            obj.draw();
        }
    };

    std::unique_ptr<Concept> pimpl;

  public:
    template <typename T>
    TypeErasedShape(T obj) : pimpl(std::make_unique<Model<T>>(std::move(obj)))
    {
    }

    void draw() const
    {
        pimpl->draw();
    }
};

struct ErasedCircle
{
    void draw() const
    { /* 空操作 */
    }
};

// 模板+Concepts版本
template <typename T>
concept Drawable = requires(const T &obj) {
    { obj.draw() } -> std::same_as<void>;
};

template <Drawable T>
void template_draw(const T &obj)
{
    obj.draw();
}

struct ConceptCircle
{
    void draw() const
    { /* 空操作 */
    }
};

// 性能测试函数
template <typename F>
double measure_perf(F &&func, int iterations = 10'000)
{
    warmup(); // 预热
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i)
    {
        func();
    }
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double, std::nano>(end - start).count() / iterations;
}

int main()
{
    constexpr int N = 10'000;

    // 虚函数测试
    VirtualCircle vcircle;
    VirtualShape *vshape = &vcircle;
    double t_virtual = measure_perf([&] { vshape->draw(); }, N);

    // 函数指针测试
    FuncPtrShape fshape;
    fshape.draw_func = circle_draw;
    double t_funcptr = measure_perf([&] { fshape.draw_func(&fshape); }, N);

    // std::function测试
    StdFunctionShape sfunc;
    ErasedCircle ecircle;
    sfunc.draw = [&] {
        ecircle.draw();
    };
    double t_std_function = measure_perf([&] { sfunc.draw(); }, N);

    // 类型擦除测试
    TypeErasedShape terased(ErasedCircle{});
    double t_type_erased = measure_perf([&] { terased.draw(); }, N);

    // 模板+Concepts测试
    ConceptCircle tcircle;
    double t_template = measure_perf([&] { template_draw(tcircle); }, N);

    // 收集结果并排序
    std::vector<std::pair<std::string, double>> results = {
        {"Virtual Function", t_virtual},
        {"Function Pointer", t_funcptr},
        {"std::function", t_std_function},
        {"Type Erasure", t_type_erased},
        {"Template+Concepts", t_template}};

    std::sort(results.begin(), results.end(),
              [](auto &a, auto &b) { return a.second < b.second; });

    // 输出排名
    std::cout << "性能排名（单次调用耗时，单位：纳秒）:\n";
    for (const auto &[name, time] : results)
    {
        std::cout << name << ": " << time << " ns\n";
    }
}
// NOLINTEND
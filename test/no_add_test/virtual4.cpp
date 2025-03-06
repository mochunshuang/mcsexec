#include <iostream>
#include <chrono>
#include <vector>
#include <functional>
#include <memory>
#include <concepts>
#include <algorithm>

// NOLINTBEGIN

// 全局volatile变量防止优化
volatile int64_t global_sum = 0;

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
    virtual void draw(int value) const = 0;
    virtual ~VirtualShape() = default;
};

struct VirtualCircle : VirtualShape
{
    mutable int64_t sum = 0;
    void draw(int value) const override
    {
        sum += value;
        // 阻止编译器优化
        asm volatile("" : "+g"(sum));
    }
};

// 函数指针版本
struct FuncPtrShape
{
    using DrawFunc = void (*)(FuncPtrShape *self, int value) noexcept;
    DrawFunc draw_func;
    mutable int64_t sum = 0;
};

void circle_draw(FuncPtrShape *self, int value) noexcept
{
    self->sum += value;
    asm volatile("" : "+g"(self->sum));
}

// std::function版本
struct StdFunctionShape
{
    std::function<void(int)> draw;
    mutable int64_t sum = 0;
};

// 类型擦除版本
class TypeErasedShape
{
    struct Concept
    {
        virtual void draw(int value) const = 0;
        virtual ~Concept() = default;
    };

    template <typename T>
    struct Model : Concept
    {
        T obj;
        Model(T o) : obj(std::move(o)) {}
        void draw(int value) const override
        {
            obj.draw(value);
        }
    };

    std::unique_ptr<Concept> pimpl;

  public:
    template <typename T>
    TypeErasedShape(T obj) : pimpl(std::make_unique<Model<T>>(std::move(obj)))
    {
    }

    void draw(int value) const
    {
        pimpl->draw(value);
    }
};

struct ErasedCircle
{
    mutable int64_t sum = 0;
    void draw(int value) const
    {
        sum += value;
        asm volatile("" : "+g"(sum));
    }
};

// 模板+Concepts版本
template <typename T>
concept Drawable = requires(const T &obj, int value) {
    { obj.draw(value) } -> std::same_as<void>;
};

template <Drawable T>
void template_draw(const T &obj, int value)
{
    obj.draw(value);
}

struct ConceptCircle
{
    mutable int64_t sum = 0;
    void draw(int value) const
    {
        sum += value;
        asm volatile("" : "+g"(sum));
    }
};

// 性能测试函数
template <typename F>
double measure_perf(F &&func, int iterations = 10'000)
{
    warmup();
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i)
    {
        func(i); // 传递动态参数
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
    double t_virtual = measure_perf([&](int i) { vshape->draw(i); }, N);

    // 函数指针测试
    FuncPtrShape fshape;
    fshape.draw_func = circle_draw;
    double t_funcptr = measure_perf([&](int i) { fshape.draw_func(&fshape, i); }, N);

    // std::function测试
    StdFunctionShape sfunc;
    ErasedCircle ecircle;
    sfunc.draw = [&](int i) {
        ecircle.draw(i);
        asm volatile("" : "+g"(ecircle.sum));
    };
    double t_std_function = measure_perf([&](int i) { sfunc.draw(i); }, N);

    // 类型擦除测试
    TypeErasedShape terased(ErasedCircle{});
    double t_type_erased = measure_perf([&](int i) { terased.draw(i); }, N);

    // 模板+Concepts测试
    ConceptCircle tcircle;
    double t_template = measure_perf([&](int i) { template_draw(tcircle, i); }, N);

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
        printf("%-18s: %.2f ns\n", name.c_str(), time);
    }

    // 阻止全局变量优化
    global_sum = vcircle.sum + fshape.sum + ecircle.sum + tcircle.sum;
}
// NOLINTEND
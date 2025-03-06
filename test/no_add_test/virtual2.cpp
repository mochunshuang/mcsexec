#include <cassert>
#include <iostream>
// NOLINTBEGIN
#if false
// 虚函数 => 函数指针版本
// 基类：用函数指针替换虚函数
struct Shape
{
    using DrawFunc = void (*)(Shape *) noexcept;
    DrawFunc draw_func; // 函数指针

    void draw() noexcept
    {
        draw_func(this);
    }
};

// 具体子类：Circle
struct Circle : Shape
{
    Circle()
        : Shape{.draw_func =
                    [](Shape *self) noexcept {
                        Circle *obj = static_cast<Circle *>(self);
                        std::cout << "Drawing Circle\n";
                        assert(obj->radius == 1.0);
                    }},
          radius(1.0)
    {
    }
    double radius;
};
/**
 * @brief 关键点：

手动绑定函数指针，需强制类型转换（风险高）。

适用于子类已知且简单的场景，但类型不安全
 * 
 * @return int 
 */
int main()
{
    Circle c;
    Shape &s = static_cast<Shape &>(c); // 注意类型安全风险
    s.draw();                           // 调用接口函数
}

#elif false
// 虚函数 => std::function 版本
#include <functional>
struct Shape
{
    std::function<void()> draw; // 更灵活的调用包装

    template <typename T>
    Shape(T &&obj)
    { // 构造时绑定具体类型
        draw = [&obj] {
            obj.draw_impl();
        };
    }
};

// 具体类型：不需要继承
struct Circle
{
    void draw_impl() const
    {
        std::cout << "Drawing Circle\n";
    }
};
/**
 * @brief 关键点：

使用std::function存储任意可调用对象。

无需继承，直接绑定对象方法，灵活性高，但有闭包开销。
 *
 * @return int
 */
int main()
{
    Circle c;
    Shape s(c);
    s.draw(); // 输出 "Drawing Circle"
}

#elif false
// 虚函数 => 类型擦除版本
#include <memory>

class Shape
{
    struct Concept // 内部抽象接口
    {
        virtual void draw() const = 0;
        virtual ~Concept() = default;
    };

    template <typename T>
    struct Model : Concept // 模板子类存储具体类型
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
    Shape(T obj) : pimpl(std::make_unique<Model<T>>(std::move(obj)))
    {
    }

    void draw() const
    {
        pimpl->draw();
    }
};

// 具体类型：无需继承基类
struct Circle
{
    void draw() const
    {
        std::cout << "Drawing Circle\n";
    }
};

int main()
{
    Shape s = Circle{};
    s.draw(); // 输出 "Drawing Circle"
}
/**
 * @brief 关键点：

通过内部Concept和Model实现类型擦除。

安全且无需继承，但需额外间接调用开销。
 *
 */
#elif true

#include <concepts>

// 定义概念约束：类型必须有 draw() 方法
template <typename T>
concept Drawable = requires(const T &obj) {
    { obj.draw() } -> std::same_as<void>;
};

// 模板函数直接操作具体类型
template <Drawable T>
void draw_all(const T &obj)
{
    obj.draw();
}

// 具体类型：无需继承
struct Circle
{
    void draw() const
    {
        std::cout << "Drawing Circle\n";
    }
};

/**
 * @brief 关键点：

使用C++20的Concepts约束类型必须实现draw()。

零运行时开销，但只能在编译时确定类型。
 *
 * @return int
 */
int main()
{
    Circle c;
    draw_all(c); // 输出 "Drawing Circle"
}

/**
 * @brief  总结
高频调用/极致性能 → 模板+Concepts。

动态类型/外部扩展 → 类型擦除。

简单回调/C兼容 → 函数指针。

灵活闭包 → std::function。
 *
 */

#endif

// NOLINTEND
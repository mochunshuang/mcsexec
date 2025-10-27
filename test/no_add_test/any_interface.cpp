#include <iostream>
#include <string>
#include <cstring>

// NOLINTBEGIN
// 1. 方法标签
struct method1_tag
{
};
struct method2_tag
{
};
struct method3_tag
{
};

constexpr method1_tag method1{};
constexpr method2_tag method2{};
constexpr method3_tag method3{};

// 2. 对象操作虚函数表
struct object_ops
{
    void (*destroy)(void *);
    void *(*get_object)(void *);

    template <typename T>
    static const object_ops *create()
    {
        static const object_ops ops = {+[](void *obj) -> void {
                                           T *t = static_cast<T *>(obj);
                                           t->~T();
                                       },
                                       +[](void *obj) -> void * {
                                           return obj;
                                       }};
        return &ops;
    }
};

// 3. 方法调用虚函数表
struct method_vtable
{
    void (*method1)(void *, int);
    int (*method2)(void *, double) noexcept;
    std::string (*method3)(void *, const char *);

    template <typename Tag, typename... Args>
    auto invoke(Tag tag, void *obj, Args &&...args) const
    {
        if constexpr (std::is_same_v<Tag, method1_tag>)
        {
            return method1(obj, std::forward<Args>(args)...);
        }
        else if constexpr (std::is_same_v<Tag, method2_tag>)
        {
            return method2(obj, std::forward<Args>(args)...);
        }
        else if constexpr (std::is_same_v<Tag, method3_tag>)
        {
            return method3(obj, std::forward<Args>(args)...);
        }
    }

    template <typename T>
    static const method_vtable *create()
    {
        static const method_vtable vtable = {
            +[](void *obj, int arg) -> void {
                T &t = *static_cast<T *>(obj);
                t.method1(arg);
            },
            +[](void *obj, double arg) noexcept -> int {
                T &t = *static_cast<T *>(obj);
                return t.method2(arg);
            },
            +[](void *obj, const char *arg) -> std::string {
                T &t = *static_cast<T *>(obj);
                return t.method3(arg);
            }};
        return &vtable;
    }
};

// 4. 内部存储封装
class storage
{
  private:
    static constexpr size_t BUFFER_SIZE = 64;
    alignas(8) char buffer_[BUFFER_SIZE];
    const object_ops *ops_;
    const method_vtable *vtable_;

  public:
    storage() : ops_(nullptr), vtable_(nullptr) {}

    template <typename T>
    void construct(T &&t)
    {
        static_assert(sizeof(T) <= BUFFER_SIZE, "Object too large");
        static_assert(std::is_nothrow_move_constructible_v<T>,
                      "Object must be nothrow move constructible");

        new (buffer_) T(std::forward<T>(t));
        ops_ = object_ops::create<T>();
        vtable_ = method_vtable::create<T>();
    }

    ~storage()
    {
        if (ops_)
        {
            ops_->destroy(buffer_);
        }
    }

    // 禁止拷贝
    storage(const storage &) = delete;
    storage &operator=(const storage &) = delete;

    // 移动构造
    storage(storage &&other) noexcept
    {
        ops_ = other.ops_;
        vtable_ = other.vtable_;

        if (ops_ && vtable_)
        {
            // 移动构造对象
            std::memcpy(buffer_, other.buffer_, BUFFER_SIZE);
        }

        other.ops_ = nullptr;
        other.vtable_ = nullptr;
    }

    // 移动赋值
    storage &operator=(storage &&other) noexcept
    {
        if (this != &other)
        {
            // 清理当前对象
            if (ops_)
            {
                ops_->destroy(buffer_);
            }

            ops_ = other.ops_;
            vtable_ = other.vtable_;

            if (ops_ && vtable_)
            {
                std::memcpy(buffer_, other.buffer_, BUFFER_SIZE);
            }

            other.ops_ = nullptr;
            other.vtable_ = nullptr;
        }
        return *this;
    }

    // 获取对象指针 - 移除 const
    void *get_object()
    {
        return ops_ ? ops_->get_object(buffer_) : nullptr;
    }

    // 获取虚函数表
    const method_vtable *get_vtable() const
    {
        return vtable_;
    }

    // 检查是否有效
    bool is_valid() const
    {
        return ops_ != nullptr && vtable_ != nullptr;
    }
};

// 5. 测试对象
struct test_object
{
    int value = 0;

    void method1(int x)
    {
        value += x;
        std::cout << "test_object::method1(" << x << ") -> value=" << value << std::endl;
    }

    int method2(double y) noexcept
    {
        int result = value + static_cast<int>(y);
        std::cout << "test_object::method2(" << y << ") -> " << result << std::endl;
        return result;
    }

    std::string method3(const char *str)
    {
        std::string result = std::string(str) + "_" + std::to_string(value);
        std::cout << "test_object::method3(" << str << ") -> " << result << std::endl;
        return result;
    }
};

// 6. 类型擦除包装器 - 使用封装的 storage
class any_interface
{
  private:
    storage storage_;

  public:
    template <typename T>
    any_interface(T &&t)
    {
        storage_.construct(std::forward<T>(t));
    }

    // 统一的调用接口
    template <typename Tag, typename... Args>
    auto invoke(Tag tag, Args &&...args)
    {
        if (!storage_.is_valid())
        {
            throw std::runtime_error("Invalid any_interface");
        }

        void *obj = storage_.get_object(); // 这里调用非const版本
        const method_vtable *vt = storage_.get_vtable();
        return vt->invoke(tag, obj, std::forward<Args>(args)...);
    }
};

// 7. 测试
int main()
{
    any_interface obj(test_object{100});

    // 使用统一的 invoke 接口
    obj.invoke(method1, 10);
    obj.invoke(method2, 5.5);
    obj.invoke(method3, "hello");

    return 0;
}
// NOLINTEND
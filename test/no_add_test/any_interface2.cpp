#include <iostream>
#include <string>
#include <cstring>
#include <type_traits>

// NOLINTBEGIN
// 1. 方法标签
struct method1_tag
{
    template <typename T, typename... Args>
    constexpr auto operator()(T &obj, Args &&...args) const noexcept
    {
        return obj.method1(std::forward<Args>(args)...);
    }
};
struct method2_tag
{
    template <typename T, typename... Args>
    constexpr auto operator()(T &obj, Args &&...args) const noexcept
    {
        return obj.method2(std::forward<Args>(args)...);
    }
};
struct method3_tag
{
    template <typename T, typename... Args>
    constexpr auto operator()(T &obj, Args &&...args) const noexcept
    {
        return obj.method3(std::forward<Args>(args)...);
    }
};

constexpr method1_tag method1{};
constexpr method2_tag method2{};
constexpr method3_tag method3{};

// 2. 虚函数表创建器 - 核心机制
struct create_vtable_t
{
    template <class VTable, class T>
    constexpr auto operator()(VTable *, T *) const noexcept -> const VTable *
    {
        return VTable::template create<T>();
    }
};

inline constexpr create_vtable_t create_vtable{};

// 3. 方法虚函数条目
template <class Tag, class Sig>
struct method_entry;

template <class Tag, class Ret, class... Args>
struct method_entry<Tag, Ret(Args...)>
{
    Ret (*fn)(void *, Args...);

    auto operator()(Tag, void *obj, Args &&...args) const -> Ret
    {
        return fn(obj, std::forward<Args>(args)...);
    }
};

template <class Tag, class Ret, class... Args>
struct method_entry<Tag, Ret(Args...) noexcept>
{
    Ret (*fn)(void *, Args...) noexcept;

    auto operator()(Tag, void *obj, Args &&...args) const noexcept -> Ret
    {
        return fn(obj, std::forward<Args>(args)...);
    }
};

// 4. 方法包装器生成器
template <class T, class Tag, class Sig>
struct method_wrapper_fn;

template <class T, class Tag, class Ret, class... Args>
struct method_wrapper_fn<T, Tag, Ret(Args...)>
{
    static constexpr auto create() -> Ret (*)(void *, Args...)
    {
        return +[](void *obj, Args... args) -> Ret {
            return Tag{}(*static_cast<T *>(obj), std::forward<Args>(args)...);
        };
    }
};

template <class T, class Tag, class Ret, class... Args>
struct method_wrapper_fn<T, Tag, Ret(Args...) noexcept>
{
    static constexpr auto create() -> Ret (*)(void *, Args...) noexcept
    {
        return +[](void *obj, Args... args) noexcept -> Ret {
            return Tag{}(*static_cast<T *>(obj), std::forward<Args>(args)...);
        };
    }
};

// 5. 动态虚函数表
template <class... Entries>
class dynamic_vtable : public Entries...
{
  public:
    using Entries::operator()...;

    template <class T>
    static const dynamic_vtable *create()
    {
        static const dynamic_vtable vtable = {
            method_wrapper_fn<T, typename Entries::tag_type,
                              typename Entries::signature_type>::create()...};
        return &vtable;
    }
};

// 6. 为每个方法条目定义标签和签名类型
template <class Tag, class Sig>
struct method_entry_with_traits : method_entry<Tag, Sig>
{
    using tag_type = Tag;
    using signature_type = Sig;
};

// 7. 定义具体的虚函数表类型
using my_vtable =
    dynamic_vtable<method_entry_with_traits<method1_tag, void(int)>,
                   method_entry_with_traits<method2_tag, int(double) noexcept>,
                   method_entry_with_traits<method3_tag, std::string(const char *)>>;

// 8. 对象操作虚函数表
struct object_ops
{
    void (*destroy)(void *);
    void *(*get_object)(void *);

    template <class T>
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

// 9. 内部存储封装
class storage
{
  private:
    static constexpr size_t BUFFER_SIZE = 64;
    alignas(8) char buffer_[BUFFER_SIZE];
    const object_ops *ops_;
    const my_vtable *vtable_;

  public:
    storage() : ops_(nullptr), vtable_(nullptr) {}

    template <class T>
    void construct(T &&t)
    {
        static_assert(sizeof(T) <= BUFFER_SIZE, "Object too large");
        static_assert(std::is_nothrow_move_constructible_v<T>,
                      "Object must be nothrow move constructible");

        new (buffer_) T(std::forward<T>(t));
        ops_ = object_ops::create<T>();
        vtable_ =
            create_vtable(static_cast<my_vtable *>(nullptr), static_cast<T *>(nullptr));
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

    void *get_object()
    {
        return ops_ ? ops_->get_object(buffer_) : nullptr;
    }

    const my_vtable *get_vtable() const
    {
        return vtable_;
    }

    bool is_valid() const
    {
        return ops_ != nullptr && vtable_ != nullptr;
    }
};

// 10. 测试对象
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

// 11. 类型擦除包装器
class any_interface
{
  private:
    storage storage_;

  public:
    template <class T>
    any_interface(T &&t)
    {
        storage_.construct(std::forward<T>(t));
    }

    // 统一的调用接口
    template <class Tag, class... Args>
    auto invoke(Tag tag, Args &&...args)
    {
        if (!storage_.is_valid())
        {
            throw std::runtime_error("Invalid any_interface");
        }

        void *obj = storage_.get_object();
        const my_vtable *vt = storage_.get_vtable();

        // 使用标签分发调用正确的虚函数
        return (*vt)(tag, obj, std::forward<Args>(args)...);
    }
};

// 12. 测试
int main()
{
    any_interface obj(test_object{100});

    obj.invoke(method1, 10);
    obj.invoke(method2, 5.5);
    obj.invoke(method3, "hello");

    return 0;
}
// NOLINTEND
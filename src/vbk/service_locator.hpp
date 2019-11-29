#ifndef BITCOIN_SRC_VBK_LOCATOR_HPP
#define BITCOIN_SRC_VBK_LOCATOR_HPP

#include <cassert>
#include <memory>
#include <type_traits>

namespace VeriBlock {

/// Singleton-container for service of type T, so that this service can be replaced in runtime (mocked).
/// @tparam T service type
/// @tparam Tag if it happens that multiple services of the same type T have to be used, just specify different tags for them
template <typename T, typename Tag = struct VbkDefaultTag>
class SingletonContainer
{
public:
    /// instance accessor
    static SingletonContainer<T, Tag>& instance()
    {
        static SingletonContainer<T, Tag> _instance;
        return _instance;
    }

    T& get()
    {
        assert(ptr != nullptr && "service is not bound");
        return *ptr;
    }

    void set(std::shared_ptr<T> instance) noexcept
    {
        this->ptr = std::move(instance);
    }

    bool has() const noexcept
    {
        return ptr != nullptr;
    }

private:
    SingletonContainer() = default;
    std::shared_ptr<T> ptr;
};

template <
    typename T,
    typename Tag = struct VbkDefaultTag,
    // this condition forbids using setService<Impl>(...). should be setService<Interface>(...)
    typename = typename std::enable_if<
        std::is_abstract<T>::value ||
        !std::is_polymorphic<T>::value>::type>
T& getService()
{
    return SingletonContainer<T, Tag>::instance().get();
}

template <
    typename T,
    typename Tag = struct VbkDefaultTag,
    // this condition forbids using setService<Impl>(...). should be setService<Interface>(...)
    typename = typename std::enable_if<
        std::is_abstract<T>::value ||
        !std::is_polymorphic<T>::value>::type>
void setService(std::shared_ptr<T> ptr)
{
    SingletonContainer<T, Tag>::instance().set(std::move(ptr));
}

template <
    typename T,
    typename Tag = struct VbkDefaultTag,
    // this condition forbids using setService<Impl>(...). should be setService<Interface>(...)
    typename = typename std::enable_if<
        std::is_abstract<T>::value ||
        !std::is_polymorphic<T>::value>::type>
void setService(T* ptr)
{
    std::shared_ptr<T> p(ptr);
    SingletonContainer<T, Tag>::instance().set(std::move(p));
}

template <
    typename T,
    typename Tag = struct VbkDefaultTag>
bool hasService()
{
    return SingletonContainer<T, Tag>::instance().has();
}

} // namespace VeriBlock

#endif //BITCOIN_SRC_VBK_LOCATOR_HPP

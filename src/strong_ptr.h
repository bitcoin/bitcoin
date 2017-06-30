// Copyright (c) 2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_STRONGPTR_H
#define BITCOIN_STRONGPTR_H

#include <memory>

/*
    strong_ptr and decay_ptr create a safe use pattern for moving shared_ptrs
    to a unique_ptr.

    While any such pattern would usually make no sense, it can be made to work
    with a new type that holds an extra reference (strong_ptr), and one that
    observes the lifetime of extra references (decay_ptr).

    The process is:
        1. Create a strong_ptr like any typical smart pointer. It cannot be
            copied, but it can "loan out" shared_ptrs using
            strong_ptr::get_shared().
        2. Use the "loaned" shared_ptrs as usual, and use the strong_ptr as
            though it were a unique_ptr.
        3. When it's time to coalesce, move the strong_ptr into a decay_ptr.
        4. Query the status with decay_ptr::decayed().

    Once moved into a decay_ptr, the original strong_ptr is reset, and the
    decay_ptr is unable to loan out any new shared_ptrs. Once the decay_ptr has
    decayed, it behaves just like a unique_ptr.

    If a loaned shared_ptr outlives the strong_ptr that owns it, the lifetime
    is extended until the last copy is deleted. This is accomplished by
    creating a shared_ptr to represent the lifetime of the strong_ptr, and
    storing a copy of it in the loaned shared_ptr's deleter.

    Thus, strong_ptr and decay_ptr ensure that allocated memory is always freed.

    Here's a quick example of their use:

        void func(std::shared_ptr<int>)
        {
            // Some long-lived operation
        }

        int main()
        {
            strong_ptr<int> str(make_strong<int>(100));
            std::thread(func, str.get_shared());
            decay_ptr<int> dec(std::move(strong));
            // The original strong_ptr is now reset
            while (!dec.decayed()) {
                // wait here
                continue;
            }
            // dec is guaranteed to be unique here. Its memory will be freed
            // when it goes out of scope.
            return 0;
        }
*/

template <typename T>
class decay_ptr;

template <typename T>
class strong_ptr
{
    struct shared_deleter {
        void operator()(T* /* unused */) const {}
        const std::shared_ptr<void> m_data;
    };

    friend class decay_ptr<T>;

    template <typename U, typename... Args>
    friend strong_ptr<U> make_strong(Args&&... args);

    template <typename U>
    explicit strong_ptr(std::shared_ptr<U>&& rhs) : m_data{std::move(rhs)}
    {
    }

public:
    using element_type = T;

    constexpr strong_ptr() : strong_ptr(nullptr) {}

    constexpr strong_ptr(std::nullptr_t) : m_shared{nullptr} {}

    template <typename U>
    explicit strong_ptr(U* ptr) : m_data(ptr)
    {
    }

    template <typename Deleter>
    strong_ptr(std::nullptr_t ptr, Deleter deleter) : m_data{ptr, std::move(deleter)}
    {
    }

    template <typename U, typename Deleter>
    strong_ptr(U* ptr, Deleter deleter) : m_data{ptr, std::move(deleter)}
    {
    }

    template <typename U, typename Deleter>
    strong_ptr(std::unique_ptr<U, Deleter>&& rhs) : m_data{std::move(rhs)}
    {
    }

    template <typename U>
    strong_ptr(strong_ptr<U>&& rhs) : m_data{std::move(rhs.m_data)}, m_shared{std::move(rhs.m_shared)}
    {
    }

    strong_ptr(strong_ptr&& rhs) noexcept : m_data{std::move(rhs.m_data)}, m_shared{std::move(rhs.m_shared)} {}

    ~strong_ptr() = default;

    strong_ptr(const strong_ptr& rhs) = delete;
    strong_ptr& operator=(const strong_ptr& rhs) = delete;

    strong_ptr& operator=(strong_ptr&& rhs) noexcept
    {
        m_data = std::move(rhs.m_data);
        m_shared = std::move(rhs.m_shared);
        return *this;
    }

    template <typename U>
    strong_ptr& operator=(strong_ptr<U>&& rhs)
    {
        m_data = std::move(rhs.m_data);
        m_shared = std::move(rhs.m_shared);
        return *this;
    }

    template <typename U>
    strong_ptr& operator=(U* rhs)
    {
        reset(rhs);
        return *this;
    }

    template <typename U, typename Deleter>
    strong_ptr& operator=(std::unique_ptr<U, Deleter>&& rhs)
    {
        m_data = std::move(rhs);
        m_shared.reset(nullptr, shared_deleter{m_data});
        return *this;
    }

    void reset()
    {
        m_data.reset();
        m_shared.reset();
    }
    void reset(std::nullptr_t)
    {
        reset();
    }
    template <typename U>
    void reset(U* ptr)
    {
        m_data.reset(ptr);
        m_shared.reset(nullptr, shared_deleter{m_data});
    }
    template <typename U, typename Deleter>
    void reset(U* ptr, Deleter deleter)
    {
        m_data.reset(ptr, std::move(deleter));
        m_shared.reset(nullptr, shared_deleter{m_data});
    }
    std::shared_ptr<T> get_shared() const
    {
        return std::shared_ptr<T>(m_shared, m_data.get());
    }
    T* operator*()
    {
        return m_data.operator*();
    }
    const T* operator*() const
    {
        return m_data.operator*();
    }
    T* operator->()
    {
        return m_data.operator->();
    }
    const T* operator->() const
    {
        return m_data.operator->();
    }
    const T* get() const
    {
        return m_data.get();
    }
    T* get()
    {
        return m_data.get();
    }
    explicit operator bool() const
    {
        return m_data.operator bool();
    }

private:
    std::shared_ptr<T> m_data;
    std::shared_ptr<void> m_shared{nullptr, shared_deleter{m_data}};
};

template <typename T>
class decay_ptr
{
public:
    constexpr decay_ptr() = default;
    constexpr decay_ptr(std::nullptr_t) : decay_ptr{} {}
    decay_ptr(decay_ptr&& rhs) noexcept : m_data{std::move(rhs.m_data)}, m_decaying{rhs.m_decaying}
    {
        // NOTE: Until c++14, weak_ptr lacked a move ctor
        rhs.m_decaying.reset();
    }

    template <typename U>
    decay_ptr(decay_ptr<U>&& rhs) : m_data{std::move(rhs.m_data)}, m_decaying{rhs.m_decaying}
    {
        // NOTE: Until c++14, weak_ptr lacked a move ctor
        rhs.m_decaying.reset();
    }

    decay_ptr(const decay_ptr&) = delete;
    decay_ptr& operator=(const decay_ptr&) = delete;

    template <typename U>
    decay_ptr(strong_ptr<U>&& ptr) : m_data{std::move(ptr.m_data)}, m_decaying{ptr.m_shared}
    {
        // NOTE: Until c++14, weak_ptr lacked a ctor which accepts an rvalue shared_ptr.
        ptr.m_shared.reset();
    }

    ~decay_ptr() = default;

    template <typename U>
    decay_ptr& operator=(decay_ptr<U>&& rhs)
    {
        // NOTE: Until c++14, weak_ptr lacked a move operator
        m_data = std::move(rhs.m_data);
        m_decaying = rhs.m_decaying;
        rhs.m_decaying.reset();
        return *this;
    }
    decay_ptr& operator=(decay_ptr&& rhs) noexcept
    {
        // NOTE: Until c++14, weak_ptr lacked a move operator
        m_data = std::move(rhs.m_data);
        m_decaying = rhs.m_decaying;
        rhs.m_decaying.reset();
        return *this;
    }
    template <typename U>
    decay_ptr& operator=(strong_ptr<U>&& rhs)
    {
        // NOTE: Until c++14, weak_ptr lacked a move operator which accepts an rvalue shared_ptr.
        m_data = std::move(rhs.m_data);
        m_decaying = rhs.m_shared;
        rhs.m_shared.reset();
        return *this;
    }
    bool decayed() const
    {
        return m_decaying.expired();
    }
    void reset()
    {
        m_decaying.reset();
        m_data.reset();
    }
    T* operator*()
    {
        return m_data.operator*();
    }
    const T* operator*() const
    {
        return m_data.operator*();
    }
    T* operator->()
    {
        return m_data.operator->();
    }
    const T* operator->() const
    {
        return m_data.operator->();
    }
    const T* get() const
    {
        return m_data.get();
    }
    T* get()
    {
        return m_data.get();
    }
    explicit operator bool() const
    {
        return m_data.operator bool();
    }

private:
    std::shared_ptr<T> m_data;
    std::weak_ptr<void> m_decaying;
};

template <typename T, typename... Args>
inline strong_ptr<T> make_strong(Args&&... args)
{
    return strong_ptr<T>(std::make_shared<T>(std::forward<Args>(args)...));
}


#endif // BITCOIN_STRONGPTR_H

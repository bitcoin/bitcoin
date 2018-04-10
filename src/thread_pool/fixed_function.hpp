#pragma once

#include <type_traits>
#include <cstring>
#include <stdexcept>
#include <utility>

namespace tp
{

/**
 * @brief The FixedFunction<R(ARGS...), STORAGE_SIZE> class implements
 * functional object.
 * This function is analog of 'std::function' with limited capabilities:
 *  - It supports only move semantics.
 *  - The size of functional objects is limited to storage size.
 * Due to limitations above it is much faster on creation and copying than
 * std::function.
 */
template <typename SIGNATURE, size_t STORAGE_SIZE = 128>
class FixedFunction;

template <typename R, typename... ARGS, size_t STORAGE_SIZE>
class FixedFunction<R(ARGS...), STORAGE_SIZE>
{

    typedef R (*func_ptr_type)(ARGS...);

public:
    FixedFunction()
        : m_function_ptr(nullptr), m_method_ptr(nullptr),
          m_alloc_ptr(nullptr)
    {
    }

    /**
     * @brief FixedFunction Constructor from functional object.
     * @param object Functor object will be stored in the internal storage
     * using move constructor. Unmovable objects are prohibited explicitly.
     */
    template <typename FUNC>
    FixedFunction(FUNC&& object)
        : FixedFunction()
    {
        typedef typename std::remove_reference<FUNC>::type unref_type;

        static_assert(sizeof(unref_type) < STORAGE_SIZE,
            "functional object doesn't fit into internal storage");
        static_assert(std::is_move_constructible<unref_type>::value,
            "Should be of movable type");

        m_method_ptr = [](
            void* object_ptr, func_ptr_type, ARGS... args) -> R
        {
            return static_cast<unref_type*>(object_ptr)
                ->
                operator()(args...);
        };

        m_alloc_ptr = [](void* storage_ptr, void* object_ptr)
        {
            if(object_ptr)
            {
                unref_type* x_object = static_cast<unref_type*>(object_ptr);
                new(storage_ptr) unref_type(std::move(*x_object));
            }
            else
            {
                static_cast<unref_type*>(storage_ptr)->~unref_type();
            }
        };

        m_alloc_ptr(&m_storage, &object);
    }

    /**
     * @brief FixedFunction Constructor from free function or static member.
     */
    template <typename RET, typename... PARAMS>
    FixedFunction(RET (*func_ptr)(PARAMS...))
        : FixedFunction()
    {
        m_function_ptr = func_ptr;
        m_method_ptr = [](void*, func_ptr_type f_ptr, ARGS... args) -> R
        {
            return static_cast<RET (*)(PARAMS...)>(f_ptr)(args...);
        };
    }

    FixedFunction(FixedFunction&& o) : FixedFunction()
    {
        moveFromOther(o);
    }

    FixedFunction& operator=(FixedFunction&& o)
    {
        moveFromOther(o);
        return *this;
    }

    ~FixedFunction()
    {
        if(m_alloc_ptr) m_alloc_ptr(&m_storage, nullptr);
    }

    /**
     * @brief operator () Execute stored functional object.
     * @throws std::runtime_error if no functional object is stored.
     */
    R operator()(ARGS... args)
    {
        if(!m_method_ptr) throw std::runtime_error("call of empty functor");
        return m_method_ptr(&m_storage, m_function_ptr, args...);
    }

private:
    FixedFunction& operator=(const FixedFunction&) = delete;
    FixedFunction(const FixedFunction&) = delete;

    union
    {
        typename std::aligned_storage<STORAGE_SIZE, sizeof(size_t)>::type
            m_storage;
        func_ptr_type m_function_ptr;
    };

    typedef R (*method_type)(
        void* object_ptr, func_ptr_type free_func_ptr, ARGS... args);
    method_type m_method_ptr;

    typedef void (*alloc_type)(void* storage_ptr, void* object_ptr);
    alloc_type m_alloc_ptr;

    void moveFromOther(FixedFunction& o)
    {
        if(this == &o) return;

        if(m_alloc_ptr)
        {
            m_alloc_ptr(&m_storage, nullptr);
            m_alloc_ptr = nullptr;
        }
        else
        {
            m_function_ptr = nullptr;
        }

        m_method_ptr = o.m_method_ptr;
        o.m_method_ptr = nullptr;

        if(o.m_alloc_ptr)
        {
            m_alloc_ptr = o.m_alloc_ptr;
            m_alloc_ptr(&m_storage, &o.m_storage);
        }
        else
        {
            m_function_ptr = o.m_function_ptr;
        }
    }
};

}

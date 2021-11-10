//---------------------------------------------------------------------------//
// Copyright (c) 2013-2014 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_MEMORY_SVM_PTR_HPP
#define BOOST_COMPUTE_MEMORY_SVM_PTR_HPP

#include <boost/type_traits.hpp>
#include <boost/static_assert.hpp>
#include <boost/assert.hpp>

#include <boost/compute/cl.hpp>
#include <boost/compute/kernel.hpp>
#include <boost/compute/context.hpp>
#include <boost/compute/command_queue.hpp>
#include <boost/compute/type_traits/is_device_iterator.hpp>

namespace boost {
namespace compute {

// forward declaration for svm_ptr<T>
template<class T>
class svm_ptr;

// svm functions require OpenCL 2.0
#if defined(BOOST_COMPUTE_CL_VERSION_2_0) || defined(BOOST_COMPUTE_DOXYGEN_INVOKED)
namespace detail {

template<class T, class IndexExpr>
struct svm_ptr_index_expr
{
    typedef T result_type;

    svm_ptr_index_expr(const svm_ptr<T> &svm_ptr,
                       const IndexExpr &expr)
        : m_svm_ptr(svm_ptr),
          m_expr(expr)
    {
    }

    operator T() const
    {
        BOOST_STATIC_ASSERT_MSG(boost::is_integral<IndexExpr>::value,
                                "Index expression must be integral");

        BOOST_ASSERT(m_svm_ptr.get());

        const context &context = m_svm_ptr.get_context();
        const device &device = context.get_device();
        command_queue queue(context, device);

        T value;
        T* ptr =
            static_cast<T*>(m_svm_ptr.get()) + static_cast<std::ptrdiff_t>(m_expr);
        queue.enqueue_svm_map(static_cast<void*>(ptr), sizeof(T), CL_MAP_READ);
        value = *(ptr);
        queue.enqueue_svm_unmap(static_cast<void*>(ptr)).wait();

        return value;
    }

    const svm_ptr<T> &m_svm_ptr;
    IndexExpr m_expr;
};

} // end detail namespace
#endif

template<class T>
class svm_ptr
{
public:
    typedef T value_type;
    typedef std::ptrdiff_t difference_type;
    typedef T* pointer;
    typedef T& reference;
    typedef std::random_access_iterator_tag iterator_category;

    svm_ptr()
        : m_ptr(0)
    {
    }

    svm_ptr(void *ptr, const context &context)
        : m_ptr(static_cast<T*>(ptr)),
          m_context(context)
    {
    }

    svm_ptr(const svm_ptr<T> &other)
        : m_ptr(other.m_ptr),
          m_context(other.m_context)
    {
    }

    svm_ptr<T>& operator=(const svm_ptr<T> &other)
    {
        m_ptr = other.m_ptr;
        m_context = other.m_context;
        return *this;
    }

    ~svm_ptr()
    {
    }

    void* get() const
    {
        return m_ptr;
    }

    svm_ptr<T> operator+(difference_type n)
    {
        return svm_ptr<T>(m_ptr + n, m_context);
    }

    difference_type operator-(svm_ptr<T> other)
    {
        BOOST_ASSERT(other.m_context == m_context);
        return m_ptr - other.m_ptr;
    }

    const context& get_context() const
    {
        return m_context;
    }

    bool operator==(const svm_ptr<T>& other) const
    {
        return (other.m_context == m_context) && (m_ptr == other.m_ptr);
    }

    bool operator!=(const svm_ptr<T>& other) const
    {
        return (other.m_context != m_context) || (m_ptr != other.m_ptr);
    }

    // svm functions require OpenCL 2.0
    #if defined(BOOST_COMPUTE_CL_VERSION_2_0) || defined(BOOST_COMPUTE_DOXYGEN_INVOKED)
    /// \internal_
    template<class Expr>
    detail::svm_ptr_index_expr<T, Expr>
    operator[](const Expr &expr) const
    {
        BOOST_ASSERT(m_ptr);

        return detail::svm_ptr_index_expr<T, Expr>(*this,
                                                   expr);
    }
    #endif

private:
    T *m_ptr;
    context m_context;
};

namespace detail {

/// \internal_
template<class T>
struct set_kernel_arg<svm_ptr<T> >
{
    void operator()(kernel &kernel_, size_t index, const svm_ptr<T> &ptr)
    {
        kernel_.set_arg_svm_ptr(index, ptr.get());
    }
};

} // end detail namespace

/// \internal_ (is_device_iterator specialization for svm_ptr)
template<class T>
struct is_device_iterator<svm_ptr<T> > : boost::true_type {};

} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_MEMORY_SVM_PTR_HPP

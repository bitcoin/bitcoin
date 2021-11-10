//---------------------------------------------------------------------------//
// Copyright (c) 2015 Jakub Szuppe <j.szuppe@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_ITERATOR_STRIDED_ITERATOR_HPP
#define BOOST_COMPUTE_ITERATOR_STRIDED_ITERATOR_HPP

#include <cstddef>
#include <iterator>

#include <boost/config.hpp>
#include <boost/iterator/iterator_adaptor.hpp>

#include <boost/compute/functional.hpp>
#include <boost/compute/detail/meta_kernel.hpp>
#include <boost/compute/detail/is_buffer_iterator.hpp>
#include <boost/compute/detail/read_write_single_value.hpp>
#include <boost/compute/iterator/detail/get_base_iterator_buffer.hpp>
#include <boost/compute/type_traits/is_device_iterator.hpp>
#include <boost/compute/type_traits/result_of.hpp>

namespace boost {
namespace compute {

// forward declaration for strided_iterator
template<class Iterator>
class strided_iterator;

namespace detail {

// helper class which defines the iterator_adaptor super-class
// type for strided_iterator
template<class Iterator>
class strided_iterator_base
{
public:
    typedef ::boost::iterator_adaptor<
        ::boost::compute::strided_iterator<Iterator>,
        Iterator,
        typename std::iterator_traits<Iterator>::value_type,
        typename std::iterator_traits<Iterator>::iterator_category
    > type;
};

// helper class for including stride value in index expression
template<class IndexExpr, class Stride>
struct stride_expr
{
    stride_expr(const IndexExpr &expr, const Stride &stride)
        : m_index_expr(expr),
          m_stride(stride)
    {
    }

    const IndexExpr m_index_expr;
    const Stride m_stride;
};

template<class IndexExpr, class Stride>
inline stride_expr<IndexExpr, Stride> make_stride_expr(const IndexExpr &expr,
                                                       const Stride &stride)
{
    return stride_expr<IndexExpr, Stride>(expr, stride);
}

template<class IndexExpr, class Stride>
inline meta_kernel& operator<<(meta_kernel &kernel,
                               const stride_expr<IndexExpr, Stride> &expr)
{
    // (expr.m_stride * (expr.m_index_expr))
    return kernel << "(" << static_cast<ulong_>(expr.m_stride)
                  << " * (" << expr.m_index_expr << "))";
}

template<class Iterator, class Stride, class IndexExpr>
struct strided_iterator_index_expr
{
    typedef typename std::iterator_traits<Iterator>::value_type result_type;

    strided_iterator_index_expr(const Iterator &input_iter,
                                const Stride &stride,
                                const IndexExpr &index_expr)
        : m_input_iter(input_iter),
          m_stride(stride),
          m_index_expr(index_expr)
    {
    }

    const Iterator m_input_iter;
    const Stride m_stride;
    const IndexExpr m_index_expr;
};

template<class Iterator, class Stride, class IndexExpr>
inline meta_kernel& operator<<(meta_kernel &kernel,
                               const strided_iterator_index_expr<Iterator,
                                                                 Stride,
                                                                 IndexExpr> &expr)
{
    return kernel << expr.m_input_iter[make_stride_expr(expr.m_index_expr, expr.m_stride)];
}

} // end detail namespace

/// \class strided_iterator
/// \brief An iterator adaptor with adjustable iteration step.
///
/// The strided iterator adaptor skips over multiple elements each time
/// it is incremented or decremented.
///
/// \see buffer_iterator, make_strided_iterator(), make_strided_iterator_end()
template<class Iterator>
class strided_iterator :
    public detail::strided_iterator_base<Iterator>::type
{
public:
    typedef typename
        detail::strided_iterator_base<Iterator>::type super_type;
    typedef typename super_type::value_type value_type;
    typedef typename super_type::reference reference;
    typedef typename super_type::base_type base_type;
    typedef typename super_type::difference_type difference_type;

    strided_iterator(Iterator iterator, difference_type stride)
        : super_type(iterator),
          m_stride(static_cast<difference_type>(stride))
    {
        // stride must be greater than zero
        BOOST_ASSERT_MSG(stride > 0, "Stride must be greater than zero");
    }

    strided_iterator(const strided_iterator<Iterator> &other)
        : super_type(other.base()),
          m_stride(other.m_stride)
    {
    }

    strided_iterator<Iterator>&
    operator=(const strided_iterator<Iterator> &other)
    {
        if(this != &other){
            super_type::operator=(other);

            m_stride = other.m_stride;
        }

        return *this;
    }

    ~strided_iterator()
    {
    }

    size_t get_index() const
    {
        return super_type::base().get_index();
    }

    const buffer& get_buffer() const
    {
        return detail::get_base_iterator_buffer(*this);
    }

    template<class IndexExpression>
    detail::strided_iterator_index_expr<Iterator, difference_type, IndexExpression>
    operator[](const IndexExpression &expr) const
    {
        typedef
            typename detail::strided_iterator_index_expr<Iterator,
                                                         difference_type,
                                                         IndexExpression>
            StridedIndexExprType;
        return StridedIndexExprType(super_type::base(),m_stride, expr);
    }

private:
    friend class ::boost::iterator_core_access;

    reference dereference() const
    {
        return reference();
    }

    bool equal(const strided_iterator<Iterator> &other) const
    {
        return (other.m_stride == m_stride)
                   && (other.base_reference() == this->base_reference());
    }

    void increment()
    {
        std::advance(super_type::base_reference(), m_stride);
    }

    void decrement()
    {
        std::advance(super_type::base_reference(),-m_stride);
    }

    void advance(typename super_type::difference_type n)
    {
        std::advance(super_type::base_reference(), n * m_stride);
    }

    difference_type distance_to(const strided_iterator<Iterator> &other) const
    {
        return std::distance(this->base_reference(), other.base_reference()) / m_stride;
    }

private:
    difference_type m_stride;
};

/// Returns a strided_iterator for \p iterator with \p stride.
///
/// \param iterator the underlying iterator
/// \param stride the iteration step for strided_iterator
///
/// \return a \c strided_iterator for \p iterator with \p stride.
///
/// For example, to create an iterator which iterates over every other
/// element in a \c vector<int>:
/// \code
/// auto strided_iterator = make_strided_iterator(vec.begin(), 2);
/// \endcode
template<class Iterator>
inline strided_iterator<Iterator>
make_strided_iterator(Iterator iterator,
                      typename std::iterator_traits<Iterator>::difference_type stride)
{
    return strided_iterator<Iterator>(iterator, stride);
}

/// Returns a strided_iterator which refers to element that would follow
/// the last element accessible through strided_iterator for \p first iterator
/// with \p stride.
///
/// Parameter \p stride must be greater than zero.
///
/// \param first the iterator referring to the first element accessible
///        through strided_iterator for \p first with \p stride
/// \param last the iterator referring to the last element that may be
////       accessible through strided_iterator for \p first with \p stride
/// \param stride the iteration step
///
/// \return a \c strided_iterator referring to element that would follow
///         the last element accessible through strided_iterator for \p first
///         iterator with \p stride.
///
/// It can be helpful when iterating over strided_iterator:
/// \code
/// // vec.size() may not be divisible by 3
/// auto strided_iterator_begin = make_strided_iterator(vec.begin(), 3);
/// auto strided_iterator_end = make_strided_iterator_end(vec.begin(), vec.end(), 3);
///
/// // copy every 3rd element to result
/// boost::compute::copy(
///         strided_iterator_begin,
///         strided_iterator_end,
///         result.begin(),
///         queue
/// );
/// \endcode
template<class Iterator>
strided_iterator<Iterator>
make_strided_iterator_end(Iterator first,
                          Iterator last,
                          typename std::iterator_traits<Iterator>::difference_type stride)
{
    typedef typename std::iterator_traits<Iterator>::difference_type difference_type;

    // calculate distance from end to the last element that would be
    // accessible through strided_iterator.
    difference_type range = std::distance(first, last);
    difference_type d = (range - 1) / stride;
    d *= stride;
    d -= range;
    // advance from end to the element that would follow the last
    // accessible element
    Iterator end_for_strided_iterator = last;
    std::advance(end_for_strided_iterator, d + stride);
    return strided_iterator<Iterator>(end_for_strided_iterator, stride);
}

/// \internal_ (is_device_iterator specialization for strided_iterator)
template<class Iterator>
struct is_device_iterator<strided_iterator<Iterator> > : boost::true_type {};

} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_ITERATOR_STRIDED_ITERATOR_HPP

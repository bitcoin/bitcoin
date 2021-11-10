//---------------------------------------------------------------------------//
// Copyright (c) 2013-2014 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_ITERATOR_DISCARD_ITERATOR_HPP
#define BOOST_COMPUTE_ITERATOR_DISCARD_ITERATOR_HPP

#include <string>
#include <cstddef>
#include <iterator>

#include <boost/config.hpp>
#include <boost/iterator/iterator_facade.hpp>

#include <boost/compute/detail/meta_kernel.hpp>
#include <boost/compute/type_traits/is_device_iterator.hpp>

namespace boost {
namespace compute {

// forward declaration for discard_iterator
class discard_iterator;

namespace detail {

// helper class which defines the iterator_facade super-class
// type for discard_iterator
struct discard_iterator_base
{
    typedef ::boost::iterator_facade<
        ::boost::compute::discard_iterator,
        void,
        ::std::random_access_iterator_tag,
        void *
    > type;
};

template<class IndexExpr>
struct discard_iterator_index_expr
{
    typedef void result_type;

    discard_iterator_index_expr(const IndexExpr &expr)
        : m_expr(expr)
    {
    }

    IndexExpr m_expr;
};

template<class IndexExpr>
inline meta_kernel& operator<<(meta_kernel &kernel,
                               const discard_iterator_index_expr<IndexExpr> &expr)
{
    (void) expr;

    return kernel;
}

} // end detail namespace

/// \class discard_iterator
/// \brief An iterator which discards all values written to it.
///
/// \see make_discard_iterator(), constant_iterator
class discard_iterator : public detail::discard_iterator_base::type
{
public:
    typedef detail::discard_iterator_base::type super_type;
    typedef super_type::reference reference;
    typedef super_type::difference_type difference_type;

    discard_iterator(size_t index = 0)
        : m_index(index)
    {
    }

    discard_iterator(const discard_iterator &other)
        : m_index(other.m_index)
    {
    }

    discard_iterator& operator=(const discard_iterator &other)
    {
        if(this != &other){
            m_index = other.m_index;
        }

        return *this;
    }

    ~discard_iterator()
    {
    }

    /// \internal_
    template<class Expr>
    detail::discard_iterator_index_expr<Expr>
    operator[](const Expr &expr) const
    {
        return detail::discard_iterator_index_expr<Expr>(expr);
    }

private:
    friend class ::boost::iterator_core_access;

    /// \internal_
    reference dereference() const
    {
        return 0;
    }

    /// \internal_
    bool equal(const discard_iterator &other) const
    {
        return m_index == other.m_index;
    }

    /// \internal_
    void increment()
    {
        m_index++;
    }

    /// \internal_
    void decrement()
    {
        m_index--;
    }

    /// \internal_
    void advance(difference_type n)
    {
        m_index = static_cast<size_t>(static_cast<difference_type>(m_index) + n);
    }

    /// \internal_
    difference_type distance_to(const discard_iterator &other) const
    {
        return static_cast<difference_type>(other.m_index - m_index);
    }

private:
    size_t m_index;
};

/// Returns a new discard_iterator with \p index.
///
/// \param index the index of the iterator
///
/// \return a \c discard_iterator at \p index
inline discard_iterator make_discard_iterator(size_t index = 0)
{
    return discard_iterator(index);
}

/// internal_ (is_device_iterator specialization for discard_iterator)
template<>
struct is_device_iterator<discard_iterator> : boost::true_type {};

} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_ITERATOR_DISCARD_ITERATOR_HPP

//---------------------------------------------------------------------------//
// Copyright (c) 2013 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_ITERATOR_ZIP_ITERATOR_HPP
#define BOOST_COMPUTE_ITERATOR_ZIP_ITERATOR_HPP

#include <cstddef>
#include <iterator>

#include <boost/config.hpp>
#include <boost/fusion/algorithm/iteration/for_each.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/mpl/back_inserter.hpp>
#include <boost/mpl/transform.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/preprocessor/repetition.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/tuple/tuple_comparison.hpp>

#include <boost/compute/config.hpp>
#include <boost/compute/functional.hpp>
#include <boost/compute/detail/meta_kernel.hpp>
#include <boost/compute/detail/mpl_vector_to_tuple.hpp>
#include <boost/compute/types/tuple.hpp>
#include <boost/compute/type_traits/is_device_iterator.hpp>
#include <boost/compute/type_traits/type_name.hpp>

namespace boost {
namespace compute {

// forward declaration for zip_iterator
template<class IteratorTuple>
class zip_iterator;

namespace detail {

namespace mpl = boost::mpl;

// meta-function returning the value_type for an iterator
template<class Iterator>
struct make_iterator_value_type
{
    typedef typename std::iterator_traits<Iterator>::value_type type;
};

// meta-function returning the value_type for a zip_iterator
template<class IteratorTuple>
struct make_zip_iterator_value_type
{
    typedef typename
        detail::mpl_vector_to_tuple<
            typename mpl::transform<
                IteratorTuple,
                make_iterator_value_type<mpl::_1>,
                mpl::back_inserter<mpl::vector<> >
            >::type
        >::type type;
};

// helper class which defines the iterator_facade super-class
// type for zip_iterator
template<class IteratorTuple>
class zip_iterator_base
{
public:
    typedef ::boost::iterator_facade<
        ::boost::compute::zip_iterator<IteratorTuple>,
        typename make_zip_iterator_value_type<IteratorTuple>::type,
        ::std::random_access_iterator_tag,
        typename make_zip_iterator_value_type<IteratorTuple>::type
    > type;
};

template<class IteratorTuple, class IndexExpr>
struct zip_iterator_index_expr
{
    typedef typename
        make_zip_iterator_value_type<IteratorTuple>::type
        result_type;

    zip_iterator_index_expr(const IteratorTuple &iterators,
                            const IndexExpr &index_expr)
        : m_iterators(iterators),
          m_index_expr(index_expr)
    {
    }

    const IteratorTuple m_iterators;
    const IndexExpr m_index_expr;
};

/// \internal_
#define BOOST_COMPUTE_PRINT_ELEM(z, n, unused)                                 \
        BOOST_PP_EXPR_IF(n, << ", ")                                           \
        << boost::get<n>(expr.m_iterators)[expr.m_index_expr]

/// \internal_
#define BOOST_COMPUTE_PRINT_ZIP_IDX(z, n, unused)                              \
template<BOOST_PP_ENUM_PARAMS(n, class Iterator), class IndexExpr>             \
inline meta_kernel& operator<<(                                                \
        meta_kernel &kernel,                                                   \
        const zip_iterator_index_expr<                                         \
                  boost::tuple<BOOST_PP_ENUM_PARAMS(n, Iterator)>,             \
                  IndexExpr                                                    \
              > &expr)                                                         \
{                                                                              \
    typedef typename                                                           \
        boost::tuple<BOOST_PP_ENUM_PARAMS(n, Iterator)>                        \
        tuple_type;                                                            \
    typedef typename                                                           \
        make_zip_iterator_value_type<tuple_type>::type                         \
        value_type;                                                            \
    kernel.inject_type<value_type>();                                          \
    return kernel                                                              \
        << "(" << type_name<value_type>() << ")"                               \
        << "{ "                                                                \
        BOOST_PP_REPEAT(n, BOOST_COMPUTE_PRINT_ELEM, ~)                        \
        << "}";                                                                \
}

BOOST_PP_REPEAT_FROM_TO(1, BOOST_COMPUTE_MAX_ARITY, BOOST_COMPUTE_PRINT_ZIP_IDX, ~)

#undef BOOST_COMPUTE_PRINT_ZIP_IDX
#undef BOOST_COMPUTE_PRINT_ELEM

struct iterator_advancer
{
    iterator_advancer(size_t n)
        : m_distance(n)
    {
    }

    template<class Iterator>
    void operator()(Iterator &i) const
    {
        std::advance(i, m_distance);
    }

    size_t m_distance;
};

template<class Iterator>
void increment_iterator(Iterator &i)
{
    i++;
}

template<class Iterator>
void decrement_iterator(Iterator &i)
{
    i--;
}

} // end detail namespace

/// \class zip_iterator
/// \brief A zip iterator adaptor.
///
/// The zip_iterator class combines values from multiple input iterators. When
/// dereferenced it returns a tuple containing each value at the current
/// position in each input range.
///
/// \see make_zip_iterator()
template<class IteratorTuple>
class zip_iterator : public detail::zip_iterator_base<IteratorTuple>::type
{
public:
    typedef typename
        detail::zip_iterator_base<IteratorTuple>::type
        super_type;
    typedef typename super_type::value_type value_type;
    typedef typename super_type::reference reference;
    typedef typename super_type::difference_type difference_type;
    typedef IteratorTuple iterator_tuple;

    zip_iterator(IteratorTuple iterators)
        : m_iterators(iterators)
    {
    }

    zip_iterator(const zip_iterator<IteratorTuple> &other)
        : m_iterators(other.m_iterators)
    {
    }

    zip_iterator<IteratorTuple>&
    operator=(const zip_iterator<IteratorTuple> &other)
    {
        if(this != &other){
            super_type::operator=(other);

            m_iterators = other.m_iterators;
        }

        return *this;
    }

    ~zip_iterator()
    {
    }

    const IteratorTuple& get_iterator_tuple() const
    {
        return m_iterators;
    }

    template<class IndexExpression>
    detail::zip_iterator_index_expr<IteratorTuple, IndexExpression>
    operator[](const IndexExpression &expr) const
    {
        return detail::zip_iterator_index_expr<IteratorTuple,
                                               IndexExpression>(m_iterators,
                                                                expr);
    }

private:
    friend class ::boost::iterator_core_access;

    reference dereference() const
    {
        return reference();
    }

    bool equal(const zip_iterator<IteratorTuple> &other) const
    {
        return m_iterators == other.m_iterators;
    }

    void increment()
    {
        boost::fusion::for_each(m_iterators, detail::increment_iterator);
    }

    void decrement()
    {
        boost::fusion::for_each(m_iterators, detail::decrement_iterator);
    }

    void advance(difference_type n)
    {
        boost::fusion::for_each(m_iterators, detail::iterator_advancer(n));
    }

    difference_type distance_to(const zip_iterator<IteratorTuple> &other) const
    {
        return std::distance(boost::get<0>(m_iterators),
                             boost::get<0>(other.m_iterators));
    }

private:
    IteratorTuple m_iterators;
};

/// Creates a zip_iterator for \p iterators.
///
/// \param iterators a tuple of input iterators to zip together
///
/// \return a \c zip_iterator for \p iterators
///
/// For example, to zip together iterators from three vectors (\c a, \c b, and
/// \p c):
/// \code
/// auto zipped = boost::compute::make_zip_iterator(
///     boost::make_tuple(a.begin(), b.begin(), c.begin())
/// );
/// \endcode
template<class IteratorTuple>
inline zip_iterator<IteratorTuple>
make_zip_iterator(IteratorTuple iterators)
{
    return zip_iterator<IteratorTuple>(iterators);
}

/// \internal_ (is_device_iterator specialization for zip_iterator)
template<class IteratorTuple>
struct is_device_iterator<zip_iterator<IteratorTuple> > : boost::true_type {};

namespace detail {

// get<N>() specialization for zip_iterator
/// \internal_
#define BOOST_COMPUTE_ZIP_GET_N(z, n, unused)                                  \
template<size_t N, class IteratorTuple, class IndexExpr,                       \
    BOOST_PP_ENUM_PARAMS(n, class T)>                                          \
inline meta_kernel&                                                            \
operator<<(meta_kernel &kernel,                                                \
           const invoked_get<                                                  \
               N,                                                              \
               zip_iterator_index_expr<IteratorTuple, IndexExpr>,              \
               boost::tuple<BOOST_PP_ENUM_PARAMS(n, T)>                        \
            > &expr)                                                           \
{                                                                              \
    typedef typename boost::tuple<BOOST_PP_ENUM_PARAMS(n, T)> Tuple;           \
    typedef typename boost::tuples::element<N, Tuple>::type T;                 \
    BOOST_STATIC_ASSERT(N < size_t(boost::tuples::length<Tuple>::value));      \
    kernel.inject_type<T>();                                                   \
    return kernel                                                              \
        << boost::get<N>(expr.m_arg.m_iterators)[expr.m_arg.m_index_expr];     \
}

BOOST_PP_REPEAT_FROM_TO(1, BOOST_COMPUTE_MAX_ARITY, BOOST_COMPUTE_ZIP_GET_N, ~)

#undef BOOST_COMPUTE_ZIP_GET_N

} // end detail namespace
} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_ITERATOR_ZIP_ITERATOR_HPP

// Boost.Range library
//
//  Copyright Neil Groves 2014. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org/libs/range/
//
#ifndef BOOST_RANGE_DETAIL_DEFAULT_CONSTRUCTIBLE_UNARY_FN_HPP_INCLUDED
#define BOOST_RANGE_DETAIL_DEFAULT_CONSTRUCTIBLE_UNARY_FN_HPP_INCLUDED

#include <boost/optional/optional.hpp>
#include <boost/mpl/if.hpp>
#include <boost/type_traits/has_trivial_constructor.hpp>

namespace boost
{
    namespace range_detail
    {

template<typename F, typename R>
class default_constructible_unary_fn_wrapper
{
public:
    typedef R result_type;

    default_constructible_unary_fn_wrapper()
    {
    }
    default_constructible_unary_fn_wrapper(const F& source)
        : m_impl(source)
    {
    }
    default_constructible_unary_fn_wrapper(const default_constructible_unary_fn_wrapper& source)
        : m_impl(source.m_impl)
    {
    }
    default_constructible_unary_fn_wrapper& operator=(const default_constructible_unary_fn_wrapper& source)
    {
        if (source.m_impl)
        {
            // Lambda are not copy/move assignable.
            m_impl.emplace(*source.m_impl);
        }
        else
        {
            m_impl.reset();
        }
        return *this;
    }
    template<typename Arg>
    R operator()(const Arg& arg) const
    {
        BOOST_ASSERT(m_impl);
        return (*m_impl)(arg);
    }
    template<typename Arg>
    R operator()(Arg& arg) const
    {
        BOOST_ASSERT(m_impl);
        return (*m_impl)(arg);
    }
private:
    boost::optional<F> m_impl;
};

template<typename F, typename R>
struct default_constructible_unary_fn_gen
{
    typedef typename boost::mpl::if_<
        boost::has_trivial_default_constructor<F>,
        F,
        default_constructible_unary_fn_wrapper<F,R>
    >::type type;
};

    } // namespace range_detail
} // namespace boost

#endif // include guard

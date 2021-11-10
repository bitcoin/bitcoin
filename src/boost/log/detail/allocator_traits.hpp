/*
 *             Copyright Andrey Semashev 2018.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   allocator_traits.hpp
 * \author Andrey Semashev
 * \date   03.01.2018
 *
 * \brief  This header is the Boost.Log library implementation, see the library documentation
 *         at http://www.boost.org/doc/libs/release/libs/log/doc/html/index.html.
 */

#ifndef BOOST_LOG_ALLOCATOR_TRAITS_HPP_INCLUDED_
#define BOOST_LOG_ALLOCATOR_TRAITS_HPP_INCLUDED_

#include <memory>
#include <boost/log/detail/config.hpp>
#if defined(BOOST_NO_CXX11_ALLOCATOR)
#include <boost/container/allocator_traits.hpp>
#endif
#include <boost/log/utility/use_std_allocator.hpp>
#include <boost/log/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace aux {

// A portable name for allocator traits
#if !defined(BOOST_NO_CXX11_ALLOCATOR)
using std::allocator_traits;
#else
using boost::container::allocator_traits;
#endif

/*!
 * \brief A standalone trait to rebind an allocator to another type.
 *
 * The important difference from <tt>std::allocator_traits&lt;Alloc&gt;::rebind_alloc&lt;U&gt;</tt> is that this
 * trait does not require template aliases and thus is compatible with C++03. There is
 * <tt>boost::container::allocator_traits&lt;Alloc&gt;::portable_rebind_alloc&lt;U&gt;</tt>, but it is not present in <tt>std::allocator_traits</tt>.
 */
template< typename Allocator, typename U >
struct rebind_alloc
{
#if !defined(BOOST_NO_CXX11_ALLOCATOR)
    typedef typename std::allocator_traits< Allocator >::BOOST_NESTED_TEMPLATE rebind_alloc< U > type;
#else
    typedef typename boost::container::allocator_traits< Allocator >::BOOST_NESTED_TEMPLATE portable_rebind_alloc< U >::type type;
#endif
};

/*!
 * This specialization mostly exists to keep <tt>std::allocator&lt;void&gt;</tt> working.
 * The default template will attempt to instantiate the allocator type to test if it provides the nested <tt>rebind</tt> template.
 * We don't want that to happen because it prohibits using <tt>std::allocator&lt;void&gt;</tt> in C++17 and later, which deprecated
 * this allocator specialization. This specialization does not use the nested <tt>rebind</tt> template in this case.
 */
template< typename T, typename U >
struct rebind_alloc< std::allocator< T >, U >
{
    typedef std::allocator< U > type;
};

template< typename U >
struct rebind_alloc< use_std_allocator, U >
{
    typedef std::allocator< U > type;
};

} // namespace aux

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_ALLOCATOR_TRAITS_HPP_INCLUDED_

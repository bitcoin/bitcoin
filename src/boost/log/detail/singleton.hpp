/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   singleton.hpp
 * \author Andrey Semashev
 * \date   20.04.2008
 *
 * \brief  This header is the Boost.Log library implementation, see the library documentation
 *         at http://www.boost.org/doc/libs/release/libs/log/doc/html/index.html.
 */

#ifndef BOOST_LOG_DETAIL_SINGLETON_HPP_INCLUDED_
#define BOOST_LOG_DETAIL_SINGLETON_HPP_INCLUDED_

#include <boost/log/detail/config.hpp>
#include <boost/log/utility/once_block.hpp>
#include <boost/log/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace aux {

//! A base class for singletons, constructed on-demand
template< typename DerivedT, typename StorageT = DerivedT >
class lazy_singleton
{
public:
    BOOST_DEFAULTED_FUNCTION(lazy_singleton(), {})

    //! Returns the singleton instance
    static StorageT& get()
    {
        BOOST_LOG_ONCE_BLOCK()
        {
            DerivedT::init_instance();
        }
        return get_instance();
    }

    //! Initializes the singleton instance
    static void init_instance()
    {
        get_instance();
    }

    BOOST_DELETED_FUNCTION(lazy_singleton(lazy_singleton const&))
    BOOST_DELETED_FUNCTION(lazy_singleton& operator= (lazy_singleton const&))

protected:
    //! Returns the singleton instance (not thread-safe)
    static StorageT& get_instance()
    {
        static StorageT instance;
        return instance;
    }
};

//! A base class for singletons, constructed on namespace scope initialization stage
template< typename DerivedT, typename StorageT = DerivedT >
class singleton :
    public lazy_singleton< DerivedT, StorageT >
{
public:
    static StorageT& instance;
};

template< typename DerivedT, typename StorageT >
StorageT& singleton< DerivedT, StorageT >::instance =
    lazy_singleton< DerivedT, StorageT >::get();

} // namespace aux

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_DETAIL_SINGLETON_HPP_INCLUDED_

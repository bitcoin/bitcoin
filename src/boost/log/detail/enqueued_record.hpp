/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   enqueued_record.hpp
 * \author Andrey Semashev
 * \date   01.04.2014
 *
 * \brief  This header is the Boost.Log library implementation, see the library documentation
 *         at http://www.boost.org/doc/libs/release/libs/log/doc/html/index.html. In this file
 *         internal configuration macros are defined.
 */

#ifndef BOOST_LOG_DETAIL_ENQUEUED_RECORD_HPP_INCLUDED_
#define BOOST_LOG_DETAIL_ENQUEUED_RECORD_HPP_INCLUDED_

#include <boost/move/core.hpp>
#include <boost/move/utility_core.hpp>
#include <boost/log/detail/config.hpp>
#include <boost/log/detail/timestamp.hpp>
#include <boost/log/core/record_view.hpp>
#include <boost/log/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace sinks {

namespace aux {

//! Log record with enqueueing timestamp
class enqueued_record
{
    BOOST_COPYABLE_AND_MOVABLE(enqueued_record)

public:
    //! Ordering predicate
    template< typename OrderT >
    struct order :
        public OrderT
    {
        typedef typename OrderT::result_type result_type;

        order() {}
        order(order const& that) : OrderT(static_cast< OrderT const& >(that)) {}
        order(OrderT const& that) : OrderT(that) {}

        result_type operator() (enqueued_record const& left, enqueued_record const& right) const
        {
            // std::priority_queue requires ordering with semantics of std::greater, so we swap arguments
            return OrderT::operator() (right.m_record, left.m_record);
        }
    };

    boost::log::aux::timestamp m_timestamp;
    record_view m_record;

    enqueued_record(enqueued_record const& that) BOOST_NOEXCEPT : m_timestamp(that.m_timestamp), m_record(that.m_record)
    {
    }
    enqueued_record(BOOST_RV_REF(enqueued_record) that) BOOST_NOEXCEPT :
        m_timestamp(that.m_timestamp),
        m_record(boost::move(that.m_record))
    {
    }
    explicit enqueued_record(record_view const& rec) :
        m_timestamp(boost::log::aux::get_timestamp()),
        m_record(rec)
    {
    }
    enqueued_record& operator= (BOOST_COPY_ASSIGN_REF(enqueued_record) that) BOOST_NOEXCEPT
    {
        m_timestamp = that.m_timestamp;
        m_record = that.m_record;
        return *this;
    }
    enqueued_record& operator= (BOOST_RV_REF(enqueued_record) that) BOOST_NOEXCEPT
    {
        m_timestamp = that.m_timestamp;
        m_record = boost::move(that.m_record);
        return *this;
    }
};

} // namespace aux

} // namespace sinks

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_DETAIL_ENQUEUED_RECORD_HPP_INCLUDED_

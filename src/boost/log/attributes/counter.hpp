/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   counter.hpp
 * \author Andrey Semashev
 * \date   01.05.2007
 *
 * The header contains implementation of the counter attribute.
 */

#ifndef BOOST_LOG_ATTRIBUTES_COUNTER_HPP_INCLUDED_
#define BOOST_LOG_ATTRIBUTES_COUNTER_HPP_INCLUDED_

#include <boost/static_assert.hpp>
#include <boost/type_traits/is_integral.hpp>
#include <boost/log/detail/config.hpp>
#include <boost/log/attributes/attribute.hpp>
#include <boost/log/attributes/attribute_cast.hpp>
#include <boost/log/attributes/attribute_value_impl.hpp>
#ifndef BOOST_LOG_NO_THREADS
#include <boost/memory_order.hpp>
#include <boost/atomic/atomic.hpp>
#endif // BOOST_LOG_NO_THREADS
#include <boost/log/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace attributes {

/*!
 * \brief A class of an attribute that counts an integral value
 *
 * This attribute acts as a counter - it returns a monotonously
 * changing value each time requested. The attribute value type can be specified
 * as a template parameter. The type must be an integral type.
 */
template< typename T >
class counter :
    public attribute
{
    BOOST_STATIC_ASSERT_MSG(is_integral< T >::value, "Boost.Log: Only integral types are supported by the counter attribute");

public:
    //! A counter value type
    typedef T value_type;

protected:
    //! Factory implementation
    class BOOST_SYMBOL_VISIBLE impl :
        public attribute::impl
    {
    private:
#ifndef BOOST_LOG_NO_THREADS
        boost::atomic< value_type > m_counter;
#else
        value_type m_counter;
#endif
        const value_type m_step;

    public:
        impl(value_type initial, value_type step) BOOST_NOEXCEPT :
            m_counter(initial), m_step(step)
        {
        }

        attribute_value get_value()
        {
#ifndef BOOST_LOG_NO_THREADS
            value_type value = m_counter.fetch_add(m_step, boost::memory_order_relaxed);
#else
            value_type value = m_counter;
            m_counter += m_step;
#endif
            return make_attribute_value(value);
        }
    };

public:
    /*!
     * Constructor
     *
     * \param initial Initial value of the counter
     * \param step Changing step of the counter. Each value acquired from the attribute
     *        will be greater than the previous one by this amount.
     */
    explicit counter(value_type initial = (value_type)0, value_type step = (value_type)1) :
        attribute(new impl(initial, step))
    {
    }

    /*!
     * Constructor for casting support
     */
    explicit counter(cast_source const& source) :
        attribute(source.as< impl >())
    {
    }
};

} // namespace attributes

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_ATTRIBUTES_COUNTER_HPP_INCLUDED_

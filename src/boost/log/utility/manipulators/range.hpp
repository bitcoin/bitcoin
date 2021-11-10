/*
 *             Copyright Andrey Semashev 2020.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          https://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   utility/manipulators/range.hpp
 * \author Andrey Semashev
 * \date   11.05.2020
 *
 * The header contains implementation of a stream manipulator for inserting a range of elements, optionally separated with a delimiter.
 */

#ifndef BOOST_LOG_UTILITY_MANIPULATORS_RANGE_HPP_INCLUDED_
#define BOOST_LOG_UTILITY_MANIPULATORS_RANGE_HPP_INCLUDED_

#include <cstddef>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/range/const_iterator.hpp>
#include <boost/core/enable_if.hpp>
#include <boost/type_traits/is_scalar.hpp>
#include <boost/type_traits/conditional.hpp>
#include <boost/log/detail/config.hpp>
#include <boost/log/detail/is_ostream.hpp>
#include <boost/log/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

/*!
 * Stream manipulator for inserting a range of elements, optionally separated with a delimiter.
 */
template< typename RangeT, typename DelimiterT >
class range_manipulator
{
private:
    typedef typename conditional<
        is_scalar< DelimiterT >::value,
        DelimiterT,
        DelimiterT const&
    >::type stored_delimiter_type;

private:
    RangeT const& m_range;
    stored_delimiter_type m_delimiter;

public:
    //! Initializing constructor
    range_manipulator(RangeT const& range, stored_delimiter_type delimiter) BOOST_NOEXCEPT :
        m_range(range),
        m_delimiter(delimiter)
    {
    }

    //! The method outputs elements of the range separated with delimiter
    template< typename StreamT >
    void output(StreamT& stream) const
    {
        typedef typename boost::range_const_iterator< RangeT >::type const_iterator;
        const_iterator it = boost::begin(m_range);
        const const_iterator end = boost::end(m_range);
        if (BOOST_LIKELY(it != end))
        {
            stream << *it;

            for (++it; it != end; ++it)
            {
                stream << m_delimiter;
                stream << *it;
            }
        }
    }
};

/*!
 * Stream manipulator for inserting a range of elements. Specialization for when there is no delimiter.
 */
template< typename RangeT >
class range_manipulator< RangeT, void >
{
private:
    RangeT const& m_range;

public:
    //! Initializing constructor
    explicit range_manipulator(RangeT const& range) BOOST_NOEXCEPT :
        m_range(range)
    {
    }

    //! The method outputs elements of the range
    template< typename StreamT >
    void output(StreamT& stream) const
    {
        typedef typename boost::range_const_iterator< RangeT >::type const_iterator;
        const_iterator it = boost::begin(m_range);
        const const_iterator end = boost::end(m_range);
        for (; it != end; ++it)
        {
            stream << *it;
        }
    }
};

/*!
 * Stream output operator for \c range_manipulator. Outputs every element of the range, separated with a delimiter, if one was specified on manipulator construction.
 */
template< typename StreamT, typename RangeT, typename DelimiterT >
inline typename boost::enable_if_c< log::aux::is_ostream< StreamT >::value, StreamT& >::type operator<< (StreamT& strm, range_manipulator< RangeT, DelimiterT > const& manip)
{
    if (BOOST_LIKELY(strm.good()))
        manip.output(strm);

    return strm;
}

/*!
 * Range manipulator generator function.
 *
 * \param range Range of elements to output. The range must support begin and end iterators, and its elements must support stream output.
 * \param delimiter Delimiter to separate elements in the output. Optional. If not specified, elements are output without separation.
 * \returns Manipulator to be inserted into the stream.
 *
 * \note Both \a range and \a delimiter objects must outlive the created manipulator object.
 */
template< typename RangeT, typename DelimiterT >
inline typename boost::enable_if_c<
    is_scalar< DelimiterT >::value,
    range_manipulator< RangeT, DelimiterT >
>::type range_manip(RangeT const& range, DelimiterT delimiter) BOOST_NOEXCEPT
{
    return range_manipulator< RangeT, DelimiterT >(range, delimiter);
}

/*!
 * Range manipulator generator function.
 *
 * \param range Range of elements to output. The range must support begin and end iterators, and its elements must support stream output.
 * \param delimiter Delimiter to separate elements in the output. Optional. If not specified, elements are output without separation.
 * \returns Manipulator to be inserted into the stream.
 *
 * \note Both \a range and \a delimiter objects must outlive the created manipulator object.
 */
template< typename RangeT, typename DelimiterT >
inline typename boost::disable_if_c<
    is_scalar< DelimiterT >::value,
    range_manipulator< RangeT, DelimiterT >
>::type range_manip(RangeT const& range, DelimiterT const& delimiter) BOOST_NOEXCEPT
{
    return range_manipulator< RangeT, DelimiterT >(range, delimiter);
}

/*!
 * Range manipulator generator function.
 *
 * \param range Range of elements to output. The range must support begin and end iterators, and its elements must support stream output.
 * \param delimiter Delimiter to separate elements in the output. Optional. If not specified, elements are output without separation.
 * \returns Manipulator to be inserted into the stream.
 *
 * \note Both \a range and \a delimiter objects must outlive the created manipulator object.
 */
template< typename RangeT, typename DelimiterElementT, std::size_t N >
inline range_manipulator< RangeT, DelimiterElementT* > range_manip(RangeT const& range, DelimiterElementT (&delimiter)[N]) BOOST_NOEXCEPT
{
    return range_manipulator< RangeT, DelimiterElementT* >(range, delimiter);
}

/*!
 * Range manipulator generator function.
 *
 * \param range Range of elements to output. The range must support begin and end iterators, and its elements must support stream output.
 * \returns Manipulator to be inserted into the stream.
 *
 * \note \a delimiter object must outlive the created manipulator object.
 */
template< typename RangeT >
inline range_manipulator< RangeT, void > range_manip(RangeT const& range) BOOST_NOEXCEPT
{
    return range_manipulator< RangeT, void >(range);
}

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_UTILITY_MANIPULATORS_RANGE_HPP_INCLUDED_

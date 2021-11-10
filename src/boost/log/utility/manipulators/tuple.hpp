/*
 *             Copyright Andrey Semashev 2020.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          https://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   utility/manipulators/tuple.hpp
 * \author Andrey Semashev
 * \date   11.05.2020
 *
 * The header contains implementation of a stream manipulator for inserting a tuple or any heterogeneous sequence of elements, optionally separated with a delimiter.
 */

#ifndef BOOST_LOG_UTILITY_MANIPULATORS_TUPLE_HPP_INCLUDED_
#define BOOST_LOG_UTILITY_MANIPULATORS_TUPLE_HPP_INCLUDED_

#include <cstddef>
#include <boost/core/enable_if.hpp>
#include <boost/type_traits/is_scalar.hpp>
#include <boost/type_traits/conditional.hpp>
#include <boost/type_traits/integral_constant.hpp>
#include <boost/fusion/include/fold.hpp>
#include <boost/fusion/include/for_each.hpp>
#include <boost/log/detail/config.hpp>
#include <boost/log/detail/is_ostream.hpp>
#include <boost/log/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

/*!
 * Stream manipulator for inserting a heterogeneous sequence of elements, optionally separated with a delimiter.
 */
template< typename TupleT, typename DelimiterT >
class tuple_manipulator
{
private:
    typedef typename conditional<
        is_scalar< DelimiterT >::value,
        DelimiterT,
        DelimiterT const&
    >::type stored_delimiter_type;

    template< typename StreamT >
    struct output_visitor
    {
        typedef boost::true_type result_type;

        output_visitor(StreamT& stream, stored_delimiter_type delimiter) BOOST_NOEXCEPT :
            m_stream(stream),
            m_delimiter(delimiter)
        {
        }

        template< typename T >
        result_type operator() (boost::true_type, T const& elem) const
        {
            m_stream << m_delimiter;
            return operator()(boost::false_type(), elem);
        }

        template< typename T >
        result_type operator() (boost::false_type, T const& elem) const
        {
            m_stream << elem;
            return result_type();
        }

    private:
        StreamT& m_stream;
        stored_delimiter_type m_delimiter;
    };

private:
    TupleT const& m_tuple;
    stored_delimiter_type m_delimiter;

public:
    //! Initializing constructor
    tuple_manipulator(TupleT const& tuple, stored_delimiter_type delimiter) BOOST_NOEXCEPT :
        m_tuple(tuple),
        m_delimiter(delimiter)
    {
    }

    //! The method outputs elements of the sequence separated with delimiter
    template< typename StreamT >
    void output(StreamT& stream) const
    {
        boost::fusion::fold(m_tuple, boost::false_type(), output_visitor< StreamT >(stream, m_delimiter));
    }
};

/*!
 * Stream manipulator for inserting a heterogeneous sequence of elements. Specialization for when there is no delimiter.
 */
template< typename TupleT >
class tuple_manipulator< TupleT, void >
{
private:
    template< typename StreamT >
    struct output_visitor
    {
        typedef void result_type;

        explicit output_visitor(StreamT& stream) BOOST_NOEXCEPT :
            m_stream(stream)
        {
        }

        template< typename T >
        result_type operator() (T const& elem) const
        {
            m_stream << elem;
        }

    private:
        StreamT& m_stream;
    };

private:
    TupleT const& m_tuple;

public:
    //! Initializing constructor
    explicit tuple_manipulator(TupleT const& tuple) BOOST_NOEXCEPT :
        m_tuple(tuple)
    {
    }

    //! The method outputs elements of the sequence
    template< typename StreamT >
    void output(StreamT& stream) const
    {
        boost::fusion::for_each(m_tuple, output_visitor< StreamT >(stream));
    }
};

/*!
 * Stream output operator for \c tuple_manipulator. Outputs every element of the sequence, separated with a delimiter, if one was specified on manipulator construction.
 */
template< typename StreamT, typename TupleT, typename DelimiterT >
inline typename boost::enable_if_c< log::aux::is_ostream< StreamT >::value, StreamT& >::type operator<< (StreamT& strm, tuple_manipulator< TupleT, DelimiterT > const& manip)
{
    if (BOOST_LIKELY(strm.good()))
        manip.output(strm);

    return strm;
}

/*!
 * Tuple manipulator generator function.
 *
 * \param tuple Heterogeneous sequence of elements to output. The sequence must be supported by Boost.Fusion, and its elements must support stream output.
 * \param delimiter Delimiter to separate elements in the output. Optional. If not specified, elements are output without separation.
 * \returns Manipulator to be inserted into the stream.
 *
 * \note Both \a tuple and \a delimiter objects must outlive the created manipulator object.
 */
template< typename TupleT, typename DelimiterT >
inline typename boost::enable_if_c<
    is_scalar< DelimiterT >::value,
    tuple_manipulator< TupleT, DelimiterT >
>::type tuple_manip(TupleT const& tuple, DelimiterT delimiter) BOOST_NOEXCEPT
{
    return tuple_manipulator< TupleT, DelimiterT >(tuple, delimiter);
}

/*!
 * Tuple manipulator generator function.
 *
 * \param tuple Heterogeneous sequence of elements to output. The sequence must be supported by Boost.Fusion, and its elements must support stream output.
 * \param delimiter Delimiter to separate elements in the output. Optional. If not specified, elements are output without separation.
 * \returns Manipulator to be inserted into the stream.
 *
 * \note Both \a tuple and \a delimiter objects must outlive the created manipulator object.
 */
template< typename TupleT, typename DelimiterT >
inline typename boost::disable_if_c<
    is_scalar< DelimiterT >::value,
    tuple_manipulator< TupleT, DelimiterT >
>::type tuple_manip(TupleT const& tuple, DelimiterT const& delimiter) BOOST_NOEXCEPT
{
    return tuple_manipulator< TupleT, DelimiterT >(tuple, delimiter);
}

/*!
 * Tuple manipulator generator function.
 *
 * \param tuple Heterogeneous sequence of elements to output. The sequence must be supported by Boost.Fusion, and its elements must support stream output.
 * \param delimiter Delimiter to separate elements in the output. Optional. If not specified, elements are output without separation.
 * \returns Manipulator to be inserted into the stream.
 *
 * \note Both \a tuple and \a delimiter objects must outlive the created manipulator object.
 */
template< typename TupleT, typename DelimiterElementT, std::size_t N >
inline tuple_manipulator< TupleT, DelimiterElementT* > tuple_manip(TupleT const& tuple, DelimiterElementT (&delimiter)[N]) BOOST_NOEXCEPT
{
    return tuple_manipulator< TupleT, DelimiterElementT* >(tuple, delimiter);
}

/*!
 * Tuple manipulator generator function.
 *
 * \param tuple Heterogeneous sequence of elements to output. The sequence must be supported by Boost.Fusion, and its elements must support stream output.
 * \returns Manipulator to be inserted into the stream.
 *
 * \note \a tuple object must outlive the created manipulator object.
 */
template< typename TupleT >
inline tuple_manipulator< TupleT, void > tuple_manip(TupleT const& tuple) BOOST_NOEXCEPT
{
    return tuple_manipulator< TupleT, void >(tuple);
}

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_UTILITY_MANIPULATORS_TUPLE_HPP_INCLUDED_

/*
 *             Copyright Andrey Semashev 2020.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          https://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   utility/manipulators/optional.hpp
 * \author Andrey Semashev
 * \date   12.05.2020
 *
 * The header contains implementation of a stream manipulator for inserting an optional value.
 */

#ifndef BOOST_LOG_UTILITY_MANIPULATORS_OPTIONAL_HPP_INCLUDED_
#define BOOST_LOG_UTILITY_MANIPULATORS_OPTIONAL_HPP_INCLUDED_

#include <cstddef>
#include <boost/core/enable_if.hpp>
#include <boost/type_traits/is_array.hpp>
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
 * Stream manipulator for inserting an optional value.
 */
template< typename OptionalT, typename NoneT >
class optional_manipulator
{
private:
    typedef typename conditional<
        is_scalar< OptionalT >::value,
        OptionalT,
        OptionalT const&
    >::type stored_optional_type;

    typedef typename conditional<
        is_scalar< NoneT >::value,
        NoneT,
        NoneT const&
    >::type stored_none_type;

private:
    stored_optional_type m_optional;
    stored_none_type m_none;

public:
    //! Initializing constructor
    optional_manipulator(stored_optional_type opt, stored_none_type none) BOOST_NOEXCEPT :
        m_optional(opt),
        m_none(none)
    {
    }

    //! The method outputs the value, if it is present, otherwise outputs the "none" marker
    template< typename StreamT >
    void output(StreamT& stream) const
    {
        if (!!m_optional)
            stream << *m_optional;
        else
            stream << m_none;
    }
};

/*!
 * Stream manipulator for inserting an optional value. Specialization for no "none" marker.
 */
template< typename OptionalT >
class optional_manipulator< OptionalT, void >
{
private:
    typedef typename conditional<
        is_scalar< OptionalT >::value,
        OptionalT,
        OptionalT const&
    >::type stored_optional_type;

private:
    stored_optional_type m_optional;

public:
    //! Initializing constructor
    optional_manipulator(stored_optional_type opt) BOOST_NOEXCEPT :
        m_optional(opt)
    {
    }

    //! The method outputs the value, if it is present
    template< typename StreamT >
    void output(StreamT& stream) const
    {
        if (!!m_optional)
            stream << *m_optional;
    }
};

/*!
 * Stream output operator for \c optional_manipulator. Outputs the optional value or the "none" marker, if one was specified on manipulator construction.
 */
template< typename StreamT, typename OptionalT, typename NoneT >
inline typename boost::enable_if_c< log::aux::is_ostream< StreamT >::value, StreamT& >::type operator<< (StreamT& strm, optional_manipulator< OptionalT, NoneT > const& manip)
{
    manip.output(strm);
    return strm;
}

/*!
 * Optional manipulator generator function.
 *
 * \param opt Optional value to output. The optional value must support contextual conversion to \c bool and dereferencing, and its dereferencing result must support stream output.
 * \param none Marker used to indicate when the value is not present. Optional. If not specified, nothing is output if the value is not present.
 * \returns Manipulator to be inserted into the stream.
 *
 * \note Both \a opt and \a none objects must outlive the created manipulator object.
 */
template< typename OptionalT, typename NoneT >
inline typename boost::enable_if_c<
    is_scalar< OptionalT >::value && is_scalar< NoneT >::value,
    optional_manipulator< OptionalT, NoneT >
>::type optional_manip(OptionalT opt, NoneT none) BOOST_NOEXCEPT
{
    return optional_manipulator< OptionalT, NoneT >(opt, none);
}

/*!
 * Optional manipulator generator function.
 *
 * \param opt Optional value to output. The optional value must support contextual conversion to \c bool and dereferencing, and its dereferencing result must support stream output.
 * \param none Marker used to indicate when the value is not present. Optional. If not specified, nothing is output if the value is not present.
 * \returns Manipulator to be inserted into the stream.
 *
 * \note Both \a opt and \a none objects must outlive the created manipulator object.
 */
template< typename OptionalT, typename NoneT >
inline typename boost::enable_if_c<
    is_scalar< OptionalT >::value && !is_scalar< NoneT >::value,
    optional_manipulator< OptionalT, NoneT >
>::type optional_manip(OptionalT opt, NoneT const& none) BOOST_NOEXCEPT
{
    return optional_manipulator< OptionalT, NoneT >(opt, none);
}

/*!
 * Optional manipulator generator function.
 *
 * \param opt Optional value to output. The optional value must support contextual conversion to \c bool and dereferencing, and its dereferencing result must support stream output.
 * \param none Marker used to indicate when the value is not present. Optional. If not specified, nothing is output if the value is not present.
 * \returns Manipulator to be inserted into the stream.
 *
 * \note Both \a opt and \a none objects must outlive the created manipulator object.
 */
template< typename OptionalT, typename NoneElementT, std::size_t N >
inline typename boost::enable_if_c<
    is_scalar< OptionalT >::value,
    optional_manipulator< OptionalT, NoneElementT* >
>::type optional_manip(OptionalT opt, NoneElementT (&none)[N]) BOOST_NOEXCEPT
{
    return optional_manipulator< OptionalT, NoneElementT* >(opt, none);
}

/*!
 * Optional manipulator generator function.
 *
 * \param opt Optional value to output. The optional value must support contextual conversion to \c bool and dereferencing, and its dereferencing result must support stream output.
 * \param none Marker used to indicate when the value is not present. Optional. If not specified, nothing is output if the value is not present.
 * \returns Manipulator to be inserted into the stream.
 *
 * \note Both \a opt and \a none objects must outlive the created manipulator object.
 */
template< typename OptionalT, typename NoneT >
inline typename boost::enable_if_c<
    !is_scalar< OptionalT >::value && !is_array< OptionalT >::value && is_scalar< NoneT >::value,
    optional_manipulator< OptionalT, NoneT >
>::type optional_manip(OptionalT const& opt, NoneT none) BOOST_NOEXCEPT
{
    return optional_manipulator< OptionalT, NoneT >(opt, none);
}

/*!
 * Optional manipulator generator function.
 *
 * \param opt Optional value to output. The optional value must support contextual conversion to \c bool and dereferencing, and its dereferencing result must support stream output.
 * \param none Marker used to indicate when the value is not present. Optional. If not specified, nothing is output if the value is not present.
 * \returns Manipulator to be inserted into the stream.
 *
 * \note Both \a opt and \a none objects must outlive the created manipulator object.
 */
template< typename OptionalT, typename NoneT >
inline typename boost::enable_if_c<
    !is_scalar< OptionalT >::value && !is_array< OptionalT >::value && !is_scalar< NoneT >::value,
    optional_manipulator< OptionalT, NoneT >
>::type optional_manip(OptionalT const& opt, NoneT const& none) BOOST_NOEXCEPT
{
    return optional_manipulator< OptionalT, NoneT >(opt, none);
}

/*!
 * Optional manipulator generator function.
 *
 * \param opt Optional value to output. The optional value must support contextual conversion to \c bool and dereferencing, and its dereferencing result must support stream output.
 * \param none Marker used to indicate when the value is not present. Optional. If not specified, nothing is output if the value is not present.
 * \returns Manipulator to be inserted into the stream.
 *
 * \note Both \a opt and \a none objects must outlive the created manipulator object.
 */
template< typename OptionalT, typename NoneElementT, std::size_t N >
inline typename boost::enable_if_c<
    !is_scalar< OptionalT >::value && !is_array< OptionalT >::value,
    optional_manipulator< OptionalT, NoneElementT* >
>::type optional_manip(OptionalT const& opt, NoneElementT (&none)[N]) BOOST_NOEXCEPT
{
    return optional_manipulator< OptionalT, NoneElementT* >(opt, none);
}

/*!
 * Optional manipulator generator function.
 *
 * \param opt Optional value to output. The optional value must support contextual conversion to \c bool and dereferencing, and its dereferencing result must support stream output.
 * \returns Manipulator to be inserted into the stream.
 *
 * \note \a opt object must outlive the created manipulator object.
 */
template< typename OptionalT >
inline typename boost::enable_if_c<
    is_scalar< OptionalT >::value,
    optional_manipulator< OptionalT, void >
>::type optional_manip(OptionalT opt) BOOST_NOEXCEPT
{
    return optional_manipulator< OptionalT, void >(opt);
}

/*!
 * Optional manipulator generator function.
 *
 * \param opt Optional value to output. The optional value must support contextual conversion to \c bool and dereferencing, and its dereferencing result must support stream output.
 * \returns Manipulator to be inserted into the stream.
 *
 * \note \a opt object must outlive the created manipulator object.
 */
template< typename OptionalT >
inline typename boost::enable_if_c<
    !is_scalar< OptionalT >::value && !is_array< OptionalT >::value,
    optional_manipulator< OptionalT, void >
>::type optional_manip(OptionalT const& opt) BOOST_NOEXCEPT
{
    return optional_manipulator< OptionalT, void >(opt);
}

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_UTILITY_MANIPULATORS_OPTIONAL_HPP_INCLUDED_

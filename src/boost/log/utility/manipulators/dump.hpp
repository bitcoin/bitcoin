/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   dump.hpp
 * \author Andrey Semashev
 * \date   03.05.2013
 *
 * This header contains the \c dump output manipulator.
 */

#ifndef BOOST_LOG_UTILITY_MANIPULATORS_DUMP_HPP_INCLUDED_
#define BOOST_LOG_UTILITY_MANIPULATORS_DUMP_HPP_INCLUDED_

#include <cstddef>
#include <iosfwd>
#include <boost/log/detail/config.hpp>
#include <boost/log/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace aux {

typedef void dump_data_char_t(const void* data, std::size_t size, std::basic_ostream< char >& strm);
extern BOOST_LOG_API dump_data_char_t* dump_data_char;
BOOST_FORCEINLINE void dump_data(const void* data, std::size_t size, std::basic_ostream< char >& strm)
{
    (dump_data_char)(data, size, strm);
}

typedef void dump_data_wchar_t(const void* data, std::size_t size, std::basic_ostream< wchar_t >& strm);
extern BOOST_LOG_API dump_data_wchar_t* dump_data_wchar;
BOOST_FORCEINLINE void dump_data(const void* data, std::size_t size, std::basic_ostream< wchar_t >& strm)
{
    (dump_data_wchar)(data, size, strm);
}

#if !defined(BOOST_NO_CXX11_CHAR16_T)
typedef void dump_data_char16_t(const void* data, std::size_t size, std::basic_ostream< char16_t >& strm);
extern BOOST_LOG_API dump_data_char16_t* dump_data_char16;
BOOST_FORCEINLINE void dump_data(const void* data, std::size_t size, std::basic_ostream< char16_t >& strm)
{
    (dump_data_char16)(data, size, strm);
}
#endif

#if !defined(BOOST_NO_CXX11_CHAR32_T)
typedef void dump_data_char32_t(const void* data, std::size_t size, std::basic_ostream< char32_t >& strm);
extern BOOST_LOG_API dump_data_char32_t* dump_data_char32;
BOOST_FORCEINLINE void dump_data(const void* data, std::size_t size, std::basic_ostream< char32_t >& strm)
{
    (dump_data_char32)(data, size, strm);
}
#endif

template< std::size_t SizeV, typename R >
struct enable_dump_size_based
{
};

template< typename R >
struct enable_dump_size_based< 1u, R >
{
    typedef R type;
};

template< typename T, typename R >
struct enable_dump :
    public enable_dump_size_based< sizeof(T), R >
{
};

template< typename R >
struct enable_dump< void, R >
{
    typedef R type;
};

template< typename R >
struct enable_dump< const void, R >
{
    typedef R type;
};

template< typename R >
struct enable_dump< volatile void, R >
{
    typedef R type;
};

template< typename R >
struct enable_dump< const volatile void, R >
{
    typedef R type;
};

} // namespace aux

/*!
 * \brief Manipulator for printing binary representation of the data
 */
class dump_manip
{
private:
    //! Beginning of the data
    const void* m_data;
    //! Size of the data, in bytes
    std::size_t m_size;

public:
    dump_manip(const void* data, std::size_t size) BOOST_NOEXCEPT : m_data(data), m_size(size) {}
    dump_manip(dump_manip const& that) BOOST_NOEXCEPT : m_data(that.m_data), m_size(that.m_size) {}

    const void* get_data() const BOOST_NOEXCEPT { return m_data; }
    std::size_t get_size() const BOOST_NOEXCEPT { return m_size; }
};

//! The operator outputs binary data to a stream
template< typename CharT, typename TraitsT >
inline std::basic_ostream< CharT, TraitsT >& operator<< (std::basic_ostream< CharT, TraitsT >& strm, dump_manip const& manip)
{
    if (BOOST_LIKELY(strm.good()))
        aux::dump_data(manip.get_data(), manip.get_size(), strm);

    return strm;
}

/*!
 * \brief Manipulator for printing binary representation of the data with a size limit
 */
class bounded_dump_manip :
    public dump_manip
{
private:
    //! Maximum size to output, in bytes
    std::size_t m_max_size;

public:
    bounded_dump_manip(const void* data, std::size_t size, std::size_t max_size) BOOST_NOEXCEPT : dump_manip(data, size), m_max_size(max_size) {}
    bounded_dump_manip(bounded_dump_manip const& that) BOOST_NOEXCEPT : dump_manip(static_cast< dump_manip const& >(that)), m_max_size(that.m_max_size) {}

    std::size_t get_max_size() const BOOST_NOEXCEPT { return m_max_size; }
};

//! The operator outputs binary data to a stream
template< typename CharT, typename TraitsT >
inline std::basic_ostream< CharT, TraitsT >& operator<< (std::basic_ostream< CharT, TraitsT >& strm, bounded_dump_manip const& manip)
{
    if (BOOST_LIKELY(strm.good()))
    {
        const std::size_t size = manip.get_size(), max_size = manip.get_max_size();
        if (max_size >= size)
        {
            aux::dump_data(manip.get_data(), size, strm);
        }
        else
        {
            aux::dump_data(manip.get_data(), max_size, strm);
            strm << " and " << (size - max_size) << " bytes more";
        }
    }

    return strm;
}

/*!
 * \brief Creates a stream manipulator that will output contents of a memory region in hexadecimal form
 * \param data The pointer to the beginning of the region
 * \param size The size of the region, in bytes
 * \return The manipulator that is to be put to a stream
 */
template< typename T >
inline typename aux::enable_dump< T, dump_manip >::type dump(T* data, std::size_t size) BOOST_NOEXCEPT
{
    return dump_manip((const void*)data, size);
}

/*!
 * \brief Creates a stream manipulator that will dump elements of an array in hexadecimal form
 * \param data The pointer to the beginning of the array
 * \param count The size of the region, in number of \c T elements
 * \return The manipulator that is to be put to a stream
 */
template< typename T >
inline dump_manip dump_elements(T* data, std::size_t count) BOOST_NOEXCEPT
{
    return dump_manip((const void*)data, count * sizeof(T));
}

/*!
 * \brief Creates a stream manipulator that will output contents of a memory region in hexadecimal form
 * \param data The pointer to the beginning of the region
 * \param size The size of the region, in bytes
 * \param max_size The maximum number of bytes of the region to output
 * \return The manipulator that is to be put to a stream
 */
template< typename T >
inline typename aux::enable_dump< T, bounded_dump_manip >::type dump(T* data, std::size_t size, std::size_t max_size) BOOST_NOEXCEPT
{
    return bounded_dump_manip((const void*)data, size, max_size);
}

/*!
 * \brief Creates a stream manipulator that will dump elements of an array in hexadecimal form
 * \param data The pointer to the beginning of the array
 * \param count The size of the region, in number of \c T elements
 * \param max_count The maximum number of elements to output
 * \return The manipulator that is to be put to a stream
 */
template< typename T >
inline bounded_dump_manip dump_elements(T* data, std::size_t count, std::size_t max_count) BOOST_NOEXCEPT
{
    return bounded_dump_manip((const void*)data, count * sizeof(T), max_count * sizeof(T));
}

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_UTILITY_MANIPULATORS_DUMP_HPP_INCLUDED_

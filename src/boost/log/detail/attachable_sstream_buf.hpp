/*
 *          Copyright Andrey Semashev 2007 - 2016.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   attachable_sstream_buf.hpp
 * \author Andrey Semashev
 * \date   29.07.2007
 *
 * \brief  This header is the Boost.Log library implementation, see the library documentation
 *         at http://www.boost.org/doc/libs/release/libs/log/doc/html/index.html.
 */

#ifndef BOOST_LOG_ATTACHABLE_SSTREAM_BUF_HPP_INCLUDED_
#define BOOST_LOG_ATTACHABLE_SSTREAM_BUF_HPP_INCLUDED_

#include <cstddef>
#include <memory>
#include <locale>
#include <string>
#include <streambuf>
#include <boost/assert.hpp>
#include <boost/cstdint.hpp>
#include <boost/type_traits/integral_constant.hpp>
#include <boost/log/detail/config.hpp>
#include <boost/log/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace aux {

//! A streambuf that puts the formatted data to an external string
template<
    typename CharT,
    typename TraitsT = std::char_traits< CharT >,
    typename AllocatorT = std::allocator< CharT >
>
class basic_ostringstreambuf :
    public std::basic_streambuf< CharT, TraitsT >
{
    //! Self type
    typedef basic_ostringstreambuf< CharT, TraitsT, AllocatorT > this_type;
    //! Base type
    typedef std::basic_streambuf< CharT, TraitsT > base_type;

    //! Buffer size
    enum { buffer_size = 16 };

public:
    //! Character type
    typedef typename base_type::char_type char_type;
    //! Traits type
    typedef typename base_type::traits_type traits_type;
    //! String type
    typedef std::basic_string< char_type, traits_type, AllocatorT > string_type;
    //! Size type
    typedef typename string_type::size_type size_type;
    //! Int type
    typedef typename base_type::int_type int_type;

    struct storage_state
    {
        //! A reference to the string that will be filled
        string_type* storage;
        //! Max size of the storage, in characters
        size_type max_size;
        //! Indicates that storage overflow happened
        bool overflow;

        BOOST_CONSTEXPR storage_state() BOOST_NOEXCEPT : storage(NULL), max_size(0u), overflow(false)
        {
        }
    };

private:
    //! Buffer storage state
    storage_state m_storage_state;
    //! A buffer used to temporarily store output
    char_type m_buffer[buffer_size];

public:
    //! Constructor
    basic_ostringstreambuf() BOOST_NOEXCEPT
    {
        base_type::setp(m_buffer, m_buffer + (sizeof(m_buffer) / sizeof(*m_buffer)));
    }
    //! Constructor
    explicit basic_ostringstreambuf(string_type& storage) BOOST_NOEXCEPT
    {
        base_type::setp(m_buffer, m_buffer + (sizeof(m_buffer) / sizeof(*m_buffer)));
        attach(storage);
    }

    storage_state const& get_storage_state() const BOOST_NOEXCEPT { return m_storage_state; }
    void set_storage_state(storage_state const& st) BOOST_NOEXCEPT { m_storage_state = st; }

    //! Detaches the buffer from the string
    void detach()
    {
        if (m_storage_state.storage)
        {
            this_type::sync();
            m_storage_state.storage = NULL;
            m_storage_state.max_size = 0u;
            m_storage_state.overflow = false;
        }
    }

    //! Attaches the buffer to another string
    void attach(string_type& storage)
    {
        attach(storage, storage.max_size());
    }

    //! Attaches the buffer to another string
    void attach(string_type& storage, size_type max_size)
    {
        detach();
        m_storage_state.storage = &storage;
        this->max_size(max_size);
    }

    //! Returns a pointer to the attached string
    string_type* storage() const BOOST_NOEXCEPT { return m_storage_state.storage; }

    //! Returns the maximum size of the storage
    size_type max_size() const BOOST_NOEXCEPT { return m_storage_state.max_size; }
    //! Sets the maximum size of the storage
    void max_size(size_type size)
    {
        if (m_storage_state.storage)
        {
            const size_type storage_max_size = m_storage_state.storage->max_size();
            size = size > storage_max_size ? storage_max_size : size;
        }

        m_storage_state.max_size = size;
        ensure_max_size();
    }
    //! Makes sure the storage does not exceed the max size limit. Should be called after the storage is modified externally.
    void ensure_max_size()
    {
        if (m_storage_state.storage && m_storage_state.storage->size() > m_storage_state.max_size)
        {
            const size_type len = length_until_boundary(m_storage_state.storage->c_str(), m_storage_state.storage->size(), m_storage_state.max_size);
            m_storage_state.storage->resize(len);
            m_storage_state.overflow = true;
        }
    }

    //! Returns true if the max size limit has been exceeded
    bool storage_overflow() const BOOST_NOEXCEPT { return m_storage_state.overflow; }
    //! Sets the overflow flag
    void storage_overflow(bool f) BOOST_NOEXCEPT { m_storage_state.overflow = f; }

    //! Returns the size left in the storage
    size_type size_left() const BOOST_NOEXCEPT
    {
        BOOST_ASSERT(m_storage_state.storage != NULL);

        const size_type size = m_storage_state.storage->size();
        return size < m_storage_state.max_size ? m_storage_state.max_size - size : static_cast< size_type >(0u);
    }

    //! Appends a string to the storage and returns the number of written characters
    size_type append(const char_type* s, size_type n)
    {
        if (!m_storage_state.overflow)
        {
            BOOST_ASSERT(m_storage_state.storage != NULL);

            size_type left = size_left();
            BOOST_LOG_ASSUME(left <= m_storage_state.storage->max_size());
            if (BOOST_LIKELY(n <= left))
            {
                m_storage_state.storage->append(s, n);
                return n;
            }
            else
            {
                // We have to find out where the last character that fits before the limit ends
                left = length_until_boundary(s, n, left);
                m_storage_state.storage->append(s, left);
                m_storage_state.overflow = true;
                return left;
            }
        }
        return 0u;
    }

    //! Appends the specified number of characters to the storage and returns the number of written characters
    size_type append(size_type n, char_type c)
    {
        if (!m_storage_state.overflow)
        {
            BOOST_ASSERT(m_storage_state.storage != NULL);

            const size_type left = size_left();
            BOOST_LOG_ASSUME(left <= m_storage_state.storage->max_size());
            if (BOOST_LIKELY(n <= left))
            {
                m_storage_state.storage->append(n, c);
                return n;
            }
            else
            {
                m_storage_state.storage->append(left, c);
                m_storage_state.overflow = true;
                return left;
            }
        }
        return 0u;
    }

    //! Appends a character to the storage and returns the number of written characters
    size_type push_back(char_type c)
    {
        if (!m_storage_state.overflow)
        {
            BOOST_ASSERT(m_storage_state.storage != NULL);

            BOOST_LOG_ASSUME(m_storage_state.max_size <= m_storage_state.storage->max_size());
            if (BOOST_LIKELY(m_storage_state.storage->size() < m_storage_state.max_size))
            {
                m_storage_state.storage->push_back(c);
                return 1u;
            }
            else
            {
                m_storage_state.overflow = true;
                return 0u;
            }
        }
        return 0u;
    }

protected:
    //! Puts all buffered data to the string
    int sync() BOOST_OVERRIDE
    {
        char_type* pBase = this->pbase();
        char_type* pPtr = this->pptr();
        if (pBase != pPtr)
        {
            this->append(pBase, static_cast< size_type >(pPtr - pBase));
            this->pbump(static_cast< int >(pBase - pPtr));
        }
        return 0;
    }
    //! Puts an unbuffered character to the string
    int_type overflow(int_type c) BOOST_OVERRIDE
    {
        this_type::sync();
        if (!traits_type::eq_int_type(c, traits_type::eof()))
        {
            this->push_back(traits_type::to_char_type(c));
            return c;
        }
        else
            return traits_type::not_eof(c);
    }
    //! Puts a character sequence to the string
    std::streamsize xsputn(const char_type* s, std::streamsize n) BOOST_OVERRIDE
    {
        this_type::sync();
        return static_cast< std::streamsize >(this->append(s, static_cast< size_type >(n)));
    }

    //! Finds the string length so that it includes only complete characters, and does not exceed \a max_size
    size_type length_until_boundary(const char_type* s, size_type n, size_type max_size) const
    {
        BOOST_ASSERT(max_size <= n);
        return length_until_boundary(s, n, max_size, boost::integral_constant< std::size_t, sizeof(char_type) >());
    }

private:
    //! Finds the string length so that it includes only complete characters, and does not exceed \a max_size
    size_type length_until_boundary(const char_type* s, size_type, size_type max_size, boost::integral_constant< std::size_t, 1u >) const
    {
        std::locale loc = this->getloc();
        std::codecvt< wchar_t, char, std::mbstate_t > const& fac = std::use_facet< std::codecvt< wchar_t, char, std::mbstate_t > >(loc);
        std::mbstate_t mbs = std::mbstate_t();
        return static_cast< size_type >(fac.length(mbs, s, s + max_size, ~static_cast< std::size_t >(0u)));
    }

    //! Finds the string length so that it includes only complete characters, and does not exceed \a max_size
    static size_type length_until_boundary(const char_type* s, size_type n, size_type max_size, boost::integral_constant< std::size_t, 2u >)
    {
        // Note: Although it's not required to be true for wchar_t, here we assume that the string has Unicode encoding (UTF-16 or UCS-2).
        // Compilers use some version of Unicode for wchar_t on all tested platforms, and std::locale doesn't offer a way
        // to find the character boundary for character types other than char anyway.
        size_type pos = max_size;
        while (pos > 0u)
        {
            --pos;
            uint_fast16_t c = static_cast< uint_fast16_t >(s[pos]);
            // Check if this is a leading surrogate
            if ((c & 0xFC00u) != 0xD800u)
                return pos + 1u;
        }

        return 0u;
    }

    //! Finds the string length so that it includes only complete characters, and does not exceed \a max_size
    static size_type length_until_boundary(const char_type* s, size_type n, size_type max_size, boost::integral_constant< std::size_t, 4u >)
    {
        // In UTF-32 and UCS-4 one code point is encoded as one code unit
        return max_size;
    }

    //! Copy constructor (closed)
    BOOST_DELETED_FUNCTION(basic_ostringstreambuf(basic_ostringstreambuf const& that))
    //! Assignment (closed)
    BOOST_DELETED_FUNCTION(basic_ostringstreambuf& operator= (basic_ostringstreambuf const& that))
};

} // namespace aux

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_ATTACHABLE_SSTREAM_BUF_HPP_INCLUDED_

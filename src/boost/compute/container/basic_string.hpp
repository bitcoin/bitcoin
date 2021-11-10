//---------------------------------------------------------------------------//
// Copyright (c) 2013 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_CONTAINER_BASIC_STRING_HPP
#define BOOST_COMPUTE_CONTAINER_BASIC_STRING_HPP

#include <string>
#include <cstring>

#include <boost/compute/cl.hpp>
#include <boost/compute/algorithm/find.hpp>
#include <boost/compute/algorithm/search.hpp>
#include <boost/compute/container/vector.hpp>
#include <boost/compute/system.hpp>
#include <boost/compute/command_queue.hpp>
#include <iosfwd>

namespace boost {
namespace compute {

/// \class basic_string
/// \brief A template for a dynamically-sized character sequence.
///
/// The \c basic_string class provides a generic template for a dynamically-
/// sized character sequence. This is most commonly used through the \c string
/// typedef (for \c basic_string<char>).
///
/// For example, to create a string on the device with its contents copied
/// from a C-string on the host:
/// \code
/// boost::compute::string str("hello, world!");
/// \endcode
///
/// \see \ref vector "vector<T>"
template<class CharT, class Traits = std::char_traits<CharT> >
class basic_string
{
public:
    typedef Traits traits_type;
    typedef typename Traits::char_type value_type;
    typedef size_t size_type;
    static const size_type npos = size_type(-1);
    typedef typename ::boost::compute::vector<CharT>::reference reference;
    typedef typename ::boost::compute::vector<CharT>::const_reference const_reference;
    typedef typename ::boost::compute::vector<CharT>::iterator iterator;
    typedef typename ::boost::compute::vector<CharT>::const_iterator const_iterator;
    typedef typename ::boost::compute::vector<CharT>::reverse_iterator reverse_iterator;
    typedef typename ::boost::compute::vector<CharT>::const_reverse_iterator const_reverse_iterator;

    basic_string()
    {
    }

    basic_string(size_type count, CharT ch)
        : m_data(count)
    {
        std::fill(m_data.begin(), m_data.end(), ch);
    }

    basic_string(const basic_string &other,
                 size_type pos,
                 size_type count = npos)
        : m_data(other.begin() + pos,
                 other.begin() + (std::min)(other.size(), count))
    {
    }

    basic_string(const char *s, size_type count)
        : m_data(s, s + count)
    {
    }

    basic_string(const char *s)
        : m_data(s, s + std::strlen(s))
    {
    }

    template<class InputIterator>
    basic_string(InputIterator first, InputIterator last)
        : m_data(first, last)
    {
    }

    basic_string(const basic_string<CharT, Traits> &other)
        : m_data(other.m_data)
    {
    }

    basic_string<CharT, Traits>& operator=(const basic_string<CharT, Traits> &other)
    {
        if(this != &other){
            m_data = other.m_data;
        }

        return *this;
    }

    ~basic_string()
    {
    }

    reference at(size_type pos)
    {
        return m_data.at(pos);
    }

    const_reference at(size_type pos) const
    {
        return m_data.at(pos);
    }

    reference operator[](size_type pos)
    {
        return m_data[pos];
    }

    const_reference operator[](size_type pos) const
    {
        return m_data[pos];
    }

    reference front()
    {
        return m_data.front();
    }

    const_reference front() const
    {
        return m_data.front();
    }

    reference back()
    {
        return m_data.back();
    }

    const_reference back() const
    {
        return m_data.back();
    }

    iterator begin()
    {
        return m_data.begin();
    }

    const_iterator begin() const
    {
        return m_data.begin();
    }

    const_iterator cbegin() const
    {
        return m_data.cbegin();
    }

    iterator end()
    {
        return m_data.end();
    }

    const_iterator end() const
    {
        return m_data.end();
    }

    const_iterator cend() const
    {
        return m_data.cend();
    }

    reverse_iterator rbegin()
    {
        return m_data.rbegin();
    }

    const_reverse_iterator rbegin() const
    {
        return m_data.rbegin();
    }

    const_reverse_iterator crbegin() const
    {
        return m_data.crbegin();
    }

    reverse_iterator rend()
    {
        return m_data.rend();
    }

    const_reverse_iterator rend() const
    {
        return m_data.rend();
    }

    const_reverse_iterator crend() const
    {
        return m_data.crend();
    }

    bool empty() const
    {
        return m_data.empty();
    }

    size_type size() const
    {
        return m_data.size();
    }

    size_type length() const
    {
        return m_data.size();
    }

    size_type max_size() const
    {
        return m_data.max_size();
    }

    void reserve(size_type size)
    {
        m_data.reserve(size);
    }

    size_type capacity() const
    {
        return m_data.capacity();
    }

    void shrink_to_fit()
    {
        m_data.shrink_to_fit();
    }

    void clear()
    {
        m_data.clear();
    }

    void swap(basic_string<CharT, Traits> &other)
    {
        if(this != &other)
        {
            ::boost::compute::vector<CharT> temp_data(other.m_data);
            other.m_data = m_data;
            m_data = temp_data;
        }
    }

    basic_string<CharT, Traits> substr(size_type pos = 0,
                                       size_type count = npos) const
    {
        return basic_string<CharT, Traits>(*this, pos, count);
    }

    /// Finds the first character \p ch
    size_type find(CharT ch, size_type pos = 0) const
    {
        const_iterator iter = ::boost::compute::find(begin() + pos, end(), ch);
        if(iter == end()){
            return npos;
        }
        else {
            return static_cast<size_type>(std::distance(begin(), iter));
        }
    }

    /// Finds the first substring equal to \p str
    size_type find(basic_string& str, size_type pos = 0) const
    {
        const_iterator iter = ::boost::compute::search(begin() + pos, end(),
                                                       str.begin(), str.end());
        if(iter == end()){
            return npos;
        }
        else {
            return static_cast<size_type>(std::distance(begin(), iter));
        }
    }

    /// Finds the first substring equal to the character string
    /// pointed to by \p s.
    /// The length of the string is determined by the first null character.
    ///
    /// For example, the following code
    /// \snippet test/test_string.cpp string_find
    ///
    /// will return 5 as position.
    size_type find(const char* s, size_type pos = 0) const
    {
        basic_string str(s);
        const_iterator iter = ::boost::compute::search(begin() + pos, end(),
                                                       str.begin(), str.end());
        if(iter == end()){
            return npos;
        }
        else {
            return static_cast<size_type>(std::distance(begin(), iter));
        }
    }

private:
    ::boost::compute::vector<CharT> m_data;
};

template<class CharT, class Traits>
std::ostream&
operator<<(std::ostream& stream,
           boost::compute::basic_string<CharT, Traits>const& outStr)
{
    command_queue queue = ::boost::compute::system::default_queue();
    boost::compute::copy(outStr.begin(),
                        outStr.end(),
                        std::ostream_iterator<CharT>(stream),
                        queue);
    return stream;
}

} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_CONTAINER_BASIC_STRING_HPP

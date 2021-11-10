// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2005-2007 Jonathan Turkanis
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// See http://www.boost.org/libs/iostreams for documentation.

#ifndef BOOST_IOSTREAMS_COUNTER_HPP_INCLUDED
#define BOOST_IOSTREAMS_COUNTER_HPP_INCLUDED

#if defined(_MSC_VER)
# pragma once
#endif

#include <algorithm>  // count.
#include <boost/iostreams/categories.hpp>
#include <boost/iostreams/char_traits.hpp>
#include <boost/iostreams/operations.hpp>
#include <boost/iostreams/pipeline.hpp>

// Must come last.
#include <boost/iostreams/detail/config/disable_warnings.hpp> // VC7.1 C4244.

namespace boost { namespace iostreams {

//
// Template name: basic_counter.
// Template parameters:
//      Ch - The character type.
// Description: Filter which counts lines and characters.
//
template<typename Ch>
class basic_counter  {
public:
    typedef Ch char_type;
    struct category
        : dual_use,
          filter_tag,
          multichar_tag,
          optimally_buffered_tag
        { };
    explicit basic_counter(int first_line = 0, int first_char = 0)
        : lines_(first_line), chars_(first_char)
        { }
    int lines() const { return lines_; }
    int characters() const { return chars_; }
    std::streamsize optimal_buffer_size() const { return 0; }

    template<typename Source>
    std::streamsize read(Source& src, char_type* s, std::streamsize n)
    {
        std::streamsize result = iostreams::read(src, s, n);
        if (result == -1)
            return -1;
        lines_ += std::count(s, s + result, char_traits<Ch>::newline());
        chars_ += result;
        return result;
    }

    template<typename Sink>
    std::streamsize write(Sink& snk, const char_type* s, std::streamsize n)
    {
        std::streamsize result = iostreams::write(snk, s, n);
        lines_ += std::count(s, s + result, char_traits<Ch>::newline());
        chars_ += result;
        return result;
    }
private:
    int lines_;
    int chars_;
};
BOOST_IOSTREAMS_PIPABLE(basic_counter, 1)


typedef basic_counter<char>     counter;
typedef basic_counter<wchar_t>  wcounter;

} } // End namespaces iostreams, boost.

#include <boost/iostreams/detail/config/enable_warnings.hpp>

#endif // #ifndef BOOST_IOSTREAMS_COUNTER_HPP_INCLUDED

/*
 * Distributed under the Boost Software License, Version 1.0.(See accompanying 
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)
 * 
 * See http://www.boost.org/libs/iostreams for documentation.

 * File:        boost/iostreams/filter/grep.hpp
 * Date:        Mon May 26 17:48:45 MDT 2008
 * Copyright:   2008 CodeRage, LLC
 * Author:      Jonathan Turkanis
 * Contact:     turkanis at coderage dot com
 *
 * Defines the class template basic_grep_filter and its specializations
 * grep_filter and wgrep_filter.
 */

#ifndef BOOST_IOSTREAMS_GREP_FILTER_HPP_INCLUDED
#define BOOST_IOSTREAMS_GREP_FILTER_HPP_INCLUDED

#if defined(_MSC_VER)
# pragma once
#endif              

#include <iostream>

#include <memory>  // allocator.
#include <boost/iostreams/char_traits.hpp>   
#include <boost/iostreams/filter/line.hpp>              
#include <boost/iostreams/pipeline.hpp>
#include <boost/regex.hpp>

namespace boost { namespace iostreams {

namespace grep {

const int invert      = 1;
const int whole_line  = invert << 1;

} // End namespace grep.

template< typename Ch,
          typename Tr = regex_traits<Ch>,
          typename Alloc = std::allocator<Ch> >
class basic_grep_filter : public basic_line_filter<Ch, Alloc> {
private:
    typedef basic_line_filter<Ch, Alloc>               base_type;
public:
    typedef typename base_type::char_type              char_type;
    typedef typename base_type::category               category;
    typedef char_traits<char_type>                     traits_type;
    typedef typename base_type::string_type            string_type;
    typedef basic_regex<Ch, Tr>                        regex_type;
    typedef regex_constants::match_flag_type           match_flag_type;
    basic_grep_filter( const regex_type& re,
                       match_flag_type match_flags = 
                           regex_constants::match_default,
                       int options = 0 );
    int count() const { return count_; }

    template<typename Sink>
    void close(Sink& snk, BOOST_IOS::openmode which)
    {
        base_type::close(snk, which);
        options_ &= ~f_initialized;
    }
private:
    virtual string_type do_filter(const string_type& line)
    {
        if ((options_ & f_initialized) == 0) {
            options_ |= f_initialized;
            count_ = 0;
        }
        bool matches = (options_ & grep::whole_line) ?
            regex_match(line, re_, match_flags_) :
            regex_search(line, re_, match_flags_);
        if (options_ & grep::invert)
            matches = !matches;
        if (matches)
            ++count_;
        return matches ? line + traits_type::newline() : string_type();
    }

    // Private flags bitwise OR'd with constants from namespace grep
    enum flags_ {
        f_initialized = 65536
    };

    regex_type       re_;
    match_flag_type  match_flags_;
    int              options_;
    int              count_;
};
BOOST_IOSTREAMS_PIPABLE(basic_grep_filter, 3)

typedef basic_grep_filter<char>     grep_filter;
typedef basic_grep_filter<wchar_t>  wgrep_filter;
                    
//------------------Implementation of basic_grep_filter-----------------------//

template<typename Ch, typename Tr, typename Alloc>
basic_grep_filter<Ch, Tr, Alloc>::basic_grep_filter
    (const regex_type& re, match_flag_type match_flags, int options)
    : base_type(true), re_(re), match_flags_(match_flags), 
      options_(options), count_(0)
    { }

} } // End namespaces iostreams, boost.

#endif      // #ifndef BOOST_IOSTREAMS_REGEX_FILTER_HPP_INCLUDED

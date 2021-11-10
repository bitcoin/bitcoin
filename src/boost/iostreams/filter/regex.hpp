// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2003-2007 Jonathan Turkanis
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// See http://www.boost.org/libs/iostreams for documentation.

#ifndef BOOST_IOSTREAMS_REGEX_FILTER_HPP_INCLUDED
#define BOOST_IOSTREAMS_REGEX_FILTER_HPP_INCLUDED

#if defined(_MSC_VER)
# pragma once
#endif              

#include <memory>                         // allocator.
#include <boost/function.hpp>        
#include <boost/iostreams/filter/aggregate.hpp>              
#include <boost/iostreams/pipeline.hpp>                
#include <boost/regex.hpp>

namespace boost { namespace iostreams {

template< typename Ch,
          typename Tr = regex_traits<Ch>,
          typename Alloc = std::allocator<Ch> >
class basic_regex_filter : public aggregate_filter<Ch, Alloc> {
private:
    typedef aggregate_filter<Ch, Alloc>                 base_type;
public:
    typedef typename base_type::char_type              char_type;
    typedef typename base_type::category               category;
    typedef std::basic_string<Ch>                      string_type;
    typedef basic_regex<Ch, Tr>                        regex_type;
    typedef regex_constants::match_flag_type           flag_type;
    typedef match_results<const Ch*>                   match_type;
    typedef function1<string_type, const match_type&>  formatter;

    basic_regex_filter( const regex_type& re,
                        const formatter& replace,
                        flag_type flags = regex_constants::match_default )
        : re_(re), replace_(replace), flags_(flags) { }
    basic_regex_filter( const regex_type& re,
                        const string_type& fmt,
                        flag_type flags = regex_constants::match_default,
                        flag_type fmt_flags = regex_constants::format_default )
        : re_(re), replace_(simple_formatter(fmt, fmt_flags)), flags_(flags) { }
    basic_regex_filter( const regex_type& re,
                        const char_type* fmt,
                        flag_type flags = regex_constants::match_default,
                        flag_type fmt_flags = regex_constants::format_default )
        : re_(re), replace_(simple_formatter(fmt, fmt_flags)), flags_(flags) { }
private:
    typedef typename base_type::vector_type       vector_type;
    void do_filter(const vector_type& src, vector_type& dest)
        {
            typedef regex_iterator<const Ch*, Ch, Tr> iterator;
            if (src.empty())
                return;
            iterator first(&src[0], &src[0] + src.size(), re_, flags_);
            iterator last;
            const Ch* suffix = 0;
            for (; first != last; ++first) {
                dest.insert( dest.end(), 
                             first->prefix().first,
                             first->prefix().second );
                string_type replacement = replace_(*first);
                dest.insert( dest.end(), 
                             replacement.begin(),
                             replacement.end() );
                suffix = first->suffix().first;
            }
            if (suffix) {
                dest.insert(dest.end(), suffix, &src[0] + src.size());
            } else {
                dest.insert(dest.end(), &src[0], &src[0] + src.size());
            }
        }
    struct simple_formatter {
        simple_formatter(const string_type& fmt, flag_type fmt_flags) 
            : fmt_(fmt), fmt_flags_(fmt_flags) { }
        string_type operator() (const match_type& match) const
        { return match.format(fmt_, fmt_flags_); }
        string_type  fmt_;
        flag_type    fmt_flags_;
    };
    regex_type  re_;
    formatter   replace_;
    flag_type   flags_;
};
BOOST_IOSTREAMS_PIPABLE(basic_regex_filter, 3)

typedef basic_regex_filter<char>     regex_filter;
typedef basic_regex_filter<wchar_t>  wregex_filter;


} } // End namespaces iostreams, boost.

#endif      // #ifndef BOOST_IOSTREAMS_REGEX_FILTER_HPP_INCLUDED

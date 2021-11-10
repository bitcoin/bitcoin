// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2003-2007 Jonathan Turkanis
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// See http://www.boost.org/libs/iostreams for documentation.

#ifndef BOOST_IOSTREAMS_DETAIL_OUTPUT_ITERATOR_ADAPTER_HPP_INCLUDED
#define BOOST_IOSTREAMS_DETAIL_OUTPUT_ITERATOR_ADAPTER_HPP_INCLUDED

#if defined(_MSC_VER)
# pragma once
#endif              

#include <algorithm>                      // copy.
#include <iosfwd>                         // streamsize.
#include <boost/iostreams/categories.hpp> // tags.
#include <boost/static_assert.hpp>
#include <boost/type_traits/is_convertible.hpp>

namespace boost { namespace iostreams { namespace detail {

template<typename Mode, typename Ch, typename OutIt>
class output_iterator_adapter {
public:
    BOOST_STATIC_ASSERT((is_convertible<Mode, output>::value));
    typedef Ch        char_type;
    typedef sink_tag  category;
    explicit output_iterator_adapter(OutIt out) : out_(out) { }
    std::streamsize write(const char_type* s, std::streamsize n) 
    { 
        std::copy(s, s + n, out_); 
        return n; 
    }
private:
    OutIt out_;
};

} } } // End namespaces detail, iostreams, boost.

#endif // #ifndef BOOST_IOSTREAMS_DETAIL_OUTPUT_ITERATOR_ADAPTER_HPP_INCLUDED //-----//

// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2003-2007 Jonathan Turkanis
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// See http://www.boost.org/libs/iostreams for documentation.

#ifndef BOOST_IOSTREAMS_DETAIL_TRANSLATE_INT_TYPE_HPP_INCLUDED
#define BOOST_IOSTREAMS_DETAIL_TRANSLATE_INT_TYPE_HPP_INCLUDED

#if defined(_MSC_VER)
# pragma once
#endif              

#include <boost/type_traits/is_same.hpp>

namespace boost { namespace iostreams { namespace detail {

template<bool IsSame>
struct translate_int_type_impl;

//
// Template name: translate_char.
// Description: Translates a character or an end-of-file indicator from the 
//      int_type of one character traits type to the int_type of another.
//
template<typename SourceTr, typename TargetTr>
typename TargetTr::int_type 
translate_int_type(typename SourceTr::int_type c) 
{ 
    typedef translate_int_type_impl<is_same<SourceTr, TargetTr>::value> impl;
    return impl::template inner<SourceTr, TargetTr>::translate(c);
}

//----------------------------------------------------------------------------//

template<>
struct translate_int_type_impl<true> {
    template<typename SourceTr, typename TargetTr>
    struct inner {
        static typename TargetTr::int_type 
        translate(typename SourceTr::int_type c) { return c; }
    };
};

template<>
struct translate_int_type_impl<false> {
    template<typename SourceTr, typename TargetTr>
    struct inner {
        static typename TargetTr::int_type 
        translate(typename SourceTr::int_type c)
            { 
                return SourceTr::eq_int_type(SourceTr::eof()) ?
                           TargetTr::eof() :
                           TargetTr::to_int_type(SourceTr::to_char_type(c));
            }
    };
};

} } } // End namespaces detail, iostreams, boost.

#endif // #ifndef BOOST_IOSTREAMS_DETAIL_TRANSLATE_INT_TYPE_HPP_INCLUDED

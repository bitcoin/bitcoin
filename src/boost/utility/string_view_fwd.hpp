/*
   Copyright (c) Marshall Clow 2012-2012.

   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

    For more information, see http://www.boost.org

    Based on the StringRef implementation in LLVM (http://llvm.org) and
    N3422 by Jeffrey Yasskin
        http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2012/n3442.html
    Updated July 2015 to reflect the Library Fundamentals TS
        http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2015/n4480.html

*/

#ifndef BOOST_STRING_VIEW_FWD_HPP
#define BOOST_STRING_VIEW_FWD_HPP

#include <boost/config.hpp>
#include <string>

namespace boost {

    template<typename charT, typename traits = std::char_traits<charT> > class basic_string_view;
    typedef basic_string_view<char,     std::char_traits<char> >        string_view;
    typedef basic_string_view<wchar_t,  std::char_traits<wchar_t> >    wstring_view;

#ifndef BOOST_NO_CXX11_CHAR16_T
    typedef basic_string_view<char16_t, std::char_traits<char16_t> > u16string_view;
#endif

#ifndef BOOST_NO_CXX11_CHAR32_T
    typedef basic_string_view<char32_t, std::char_traits<char32_t> > u32string_view;
#endif

}

#endif

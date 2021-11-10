//
//  Copyright (c) 2020 Alexander Grund
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef BOOST_NOWIDE_DETAIL_CONVERT_HPP_INCLUDED
#define BOOST_NOWIDE_DETAIL_CONVERT_HPP_INCLUDED

#include <boost/nowide/utf/convert.hpp>

// Legacy compatibility header only. Include <boost/nowide/utf/convert.hpp> instead

namespace boost {
namespace nowide {
    namespace detail {
        using boost::nowide::utf::convert_buffer;
        using boost::nowide::utf::convert_string;
        using boost::nowide::utf::strlen;
    } // namespace detail
} // namespace nowide
} // namespace boost

#endif

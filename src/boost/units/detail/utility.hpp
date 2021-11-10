// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2003-2008 Matthias Christian Schabel
// Copyright (C) 2008 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UNITS_UTILITY_HPP
#define BOOST_UNITS_UTILITY_HPP

#include <typeinfo>
#include <string>
#include <boost/core/demangle.hpp>

namespace boost {

namespace units {

namespace detail {

inline
std::string
demangle(const char* name)
{
    std::string demangled = core::demangle(name);

    const std::string::size_type prefix_len = sizeof("boost::units::") - 1;
    std::string::size_type i = 0;
    for (;;)
    {
        std::string::size_type pos = demangled.find("boost::units::", i, prefix_len);
        if (pos == std::string::npos)
            break;

        demangled.erase(pos, prefix_len);
        i = pos;
    }

    return demangled;
}

} // namespace detail

template<class L>
inline std::string simplify_typename(const L& /*source*/)
{
    return detail::demangle(typeid(L).name());
}

} // namespace units

} // namespace boost

#endif // BOOST_UNITS_UTILITY_HPP

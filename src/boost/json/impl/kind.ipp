//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/json
//

#ifndef BOOST_JSON_IMPL_KIND_IPP
#define BOOST_JSON_IMPL_KIND_IPP

#include <boost/json/kind.hpp>
#include <ostream>

BOOST_JSON_NS_BEGIN

string_view
to_string(kind k) noexcept
{
    switch(k)
    {
    case kind::array:   return "array";
    case kind::object:  return "object";
    case kind::string:  return "string";
    case kind::int64:   return "int64";
    case kind::uint64:  return "uint64";
    case kind::double_: return "double";
    case kind::bool_:   return "bool";
    default: // satisfy warnings
    case kind::null:    return "null";
    }
}

std::ostream&
operator<<(std::ostream& os, kind k)
{
    os << to_string(k);
    return os;
}

BOOST_JSON_NS_END

#endif

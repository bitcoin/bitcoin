//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/json
//

#ifndef BOOST_JSON_IMPL_VISIT_HPP
#define BOOST_JSON_IMPL_VISIT_HPP

BOOST_JSON_NS_BEGIN

template<class Visitor>
auto
visit(
    Visitor&& v,
    value& jv) -> decltype(
        std::declval<Visitor>()(nullptr))
{
    switch(jv.kind())
    {
    default: // unreachable()?
    case kind::null:    return std::forward<Visitor>(v)(nullptr);
    case kind::bool_:   return std::forward<Visitor>(v)(jv.get_bool());
    case kind::int64:   return std::forward<Visitor>(v)(jv.get_int64());
    case kind::uint64:  return std::forward<Visitor>(v)(jv.get_uint64());
    case kind::double_: return std::forward<Visitor>(v)(jv.get_double());
    case kind::string:  return std::forward<Visitor>(v)(jv.get_string());
    case kind::array:   return std::forward<Visitor>(v)(jv.get_array());
    case kind::object:  return std::forward<Visitor>(v)(jv.get_object());
    }
}

template<class Visitor>
auto
visit(
    Visitor&& v,
    value const& jv) -> decltype(
        std::declval<Visitor>()(nullptr))
{
    switch (jv.kind())
    {
    default: // unreachable()?
    case kind::null:    return std::forward<Visitor>(v)(nullptr);
    case kind::bool_:   return std::forward<Visitor>(v)(jv.get_bool());
    case kind::int64:   return std::forward<Visitor>(v)(jv.get_int64());
    case kind::uint64:  return std::forward<Visitor>(v)(jv.get_uint64());
    case kind::double_: return std::forward<Visitor>(v)(jv.get_double());
    case kind::string:  return std::forward<Visitor>(v)(jv.get_string());
    case kind::array:   return std::forward<Visitor>(v)(jv.get_array());
    case kind::object:  return std::forward<Visitor>(v)(jv.get_object());
    }
}

BOOST_JSON_NS_END

#endif

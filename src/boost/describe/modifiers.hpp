#ifndef BOOST_DESCRIBE_MODIFIERS_HPP_INCLUDED
#define BOOST_DESCRIBE_MODIFIERS_HPP_INCLUDED

// Copyright 2020 Peter Dimov
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/describe/enum.hpp>

namespace boost
{
namespace describe
{

enum modifiers
{
    mod_public = 1,
    mod_protected = 2,
    mod_private = 4,
    mod_virtual = 8,
    mod_static = 16,
    mod_function = 32,
    mod_any_member = 64,
    mod_inherited = 128,
    mod_hidden = 256,
};

BOOST_DESCRIBE_ENUM(modifiers,
    mod_public,
    mod_protected,
    mod_private,
    mod_virtual,
    mod_static,
    mod_function,
    mod_any_member,
    mod_inherited,
    mod_hidden);

BOOST_DESCRIBE_CONSTEXPR_OR_CONST modifiers mod_any_access = static_cast<modifiers>( mod_public | mod_protected | mod_private );

} // namespace describe
} // namespace boost

#endif // #ifndef BOOST_DESCRIBE_MODIFIERS_HPP_INCLUDED

///////////////////////////////////////////////////////////////////////////////
/// \file regex_traits.hpp
/// Includes the C regex traits or the CPP regex traits header file depending on the
/// BOOST_XPRESSIVE_USE_C_TRAITS macro.
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_XPRESSIVE_REGEX_TRAITS_HPP_EAN_10_04_2005
#define BOOST_XPRESSIVE_REGEX_TRAITS_HPP_EAN_10_04_2005

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

#include <boost/type_traits/is_convertible.hpp>
#include <boost/xpressive/detail/detail_fwd.hpp>

#ifdef BOOST_XPRESSIVE_USE_C_TRAITS
# include <boost/xpressive/traits/c_regex_traits.hpp>
#else
# include <boost/xpressive/traits/cpp_regex_traits.hpp>
#endif

namespace boost { namespace xpressive
{

///////////////////////////////////////////////////////////////////////////////
// regex_traits_version_1_tag
/// Tag used to denote that a traits class conforms to the version 1 traits
/// interface.
struct regex_traits_version_1_tag
{
};

///////////////////////////////////////////////////////////////////////////////
// regex_traits_version_2_tag
/// Tag used to denote that a traits class conforms to the version 2 traits
/// interface.
struct regex_traits_version_2_tag
  : regex_traits_version_1_tag
{
};

///////////////////////////////////////////////////////////////////////////////
// regex_traits_version_1_case_fold_tag DEPRECATED use has_fold_case trait
/// INTERNAL ONLY
///
struct regex_traits_version_1_case_fold_tag
  : regex_traits_version_1_tag
{
};

///////////////////////////////////////////////////////////////////////////////
// has_fold_case
/// Trait used to denote that a traits class has the fold_case member function.
template<typename Traits>
struct has_fold_case
  : is_convertible<
        typename Traits::version_tag *
      , regex_traits_version_1_case_fold_tag *
    >
{
};

///////////////////////////////////////////////////////////////////////////////
// regex_traits
/// Thin wrapper around the default regex_traits implementation, either
/// cpp_regex_traits or c_regex_traits
///
template<typename Char, typename Impl>
struct regex_traits
  : Impl
{
    typedef typename Impl::locale_type locale_type;

    regex_traits()
      : Impl()
    {
    }

    explicit regex_traits(locale_type const &loc)
      : Impl(loc)
    {
    }
};

///////////////////////////////////////////////////////////////////////////////
// lookup_classname
/// INTERNAL ONLY
template<typename Traits, std::size_t N>
inline typename Traits::char_class_type
lookup_classname(Traits const &traits, char const (&cname)[N], bool icase)
{
    typename Traits::char_type name[N] = {0};
    for(std::size_t j = 0; j < N-1; ++j)
    {
        name[j] = traits.widen(cname[j]);
    }
    return traits.lookup_classname(name, name + N - 1, icase);
}

}}

#endif

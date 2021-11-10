//  is_evenly_divisible_by.hpp  --------------------------------------------------------------//

//  Copyright 2009-2010 Vicente J. Botet Escriba

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_CHRONO_DETAIL_NO_WARNING_SIGNED_UNSIGNED_CMP_HPP
#define BOOST_CHRONO_DETAIL_NO_WARNING_SIGNED_UNSIGNED_CMP_HPP

//
// We simply cannot include this header on gcc without getting copious warnings of the kind:
//
//../../../boost/chrono/detail/no_warning/signed_unsigned_cmp.hpp:37: warning: comparison between signed and unsigned integer expressions
//
// And yet there is no other reasonable implementation, so we declare this a system header
// to suppress these warnings.
//

#if defined(__GNUC__) && (__GNUC__ >= 4)
#pragma GCC system_header
#elif defined __SUNPRO_CC
#pragma disable_warn
#elif defined _MSC_VER
#pragma warning(push, 1)
#endif

namespace boost {
namespace chrono {
namespace detail {

  template <class T, class U>
  bool lt(T t, U u)
  {
    return t < u;
  }

  template <class T, class U>
  bool gt(T t, U u)
  {
    return t > u;
  }

} // namespace detail
} // namespace detail
} // namespace chrono

#if defined __SUNPRO_CC
#pragma enable_warn
#elif defined _MSC_VER
#pragma warning(pop)
#endif

#endif // BOOST_CHRONO_DETAIL_NO_WARNING_SIGNED_UNSIGNED_CMP_HPP

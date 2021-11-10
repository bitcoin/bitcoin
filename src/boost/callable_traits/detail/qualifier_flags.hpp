/*
Defines `qualifier_flags`

@Copyright Barrett Adair 2015-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)

*/

#ifndef BOOST_CLBL_TRTS_QUALIFIER_FLAGS_HPP
#define BOOST_CLBL_TRTS_QUALIFIER_FLAGS_HPP

#include <boost/callable_traits/detail/config.hpp>

namespace boost { namespace callable_traits { namespace detail {
    
//bit qualifier_flags used to signify cv/ref qualifiers
using qualifier_flags = std::uint32_t;

/*
    | && &  V  C |
--------------------------------------------
0   | 0  0  0  0 | default
1   | 0  0  0  1 | const
2   | 0  0  1  0 | volatile
3   | 0  0  1  1 | const volatile
--------------------------------------------
4   | 0  1  0  0 | &
5   | 0  1  0  1 | const &
6   | 0  1  1  0 | volatile &
7   | 0  1  1  1 | const volatile &
--------------------------------------------
8   | 1  0  0  0 | &&
9   | 1  0  0  1 | const &&
10  | 1  0  1  0 | volatile &&
11  | 1  0  1  1 | const volatile &&

*/

// Flag representing the default qualifiers on a type 
// or member function overload.
constexpr qualifier_flags default_ = 0;

// Flag representing a const qualifier on a type or
// member function overload.
constexpr qualifier_flags const_ = 1;

// Flag representing a volatile qualifier on a type
// or member function overload.
constexpr qualifier_flags volatile_ = 2;

#ifdef BOOST_CLBL_TRTS_DISABLE_REFERENCE_QUALIFIERS

constexpr qualifier_flags lref_ = default_;
constexpr qualifier_flags rref_ = default_;
#else

// Flag representing an lvalue reference type, or
// an lvalue-reference-qualified member function
// overload.
constexpr qualifier_flags lref_ = 4;

// Flag representing an lvalue reference type, or
// an rvalue-reference-qualified member function
// overload.
constexpr qualifier_flags rref_ = 8;

#endif //#ifdef BOOST_CLBL_TRTS_DISABLE_REFERENCE_QUALIFIERS

constexpr qualifier_flags cv_ = 3;

template<qualifier_flags Flags>
using remove_const_flag = std::integral_constant<
    qualifier_flags, Flags & ~const_>;

template<qualifier_flags Flags>
using is_const = std::integral_constant<bool,
    (Flags & const_) != 0>;

template<qualifier_flags Flags>
using remove_volatile_flag = std::integral_constant<
    qualifier_flags, Flags & ~volatile_>;

template<typename U, typename T = typename std::remove_reference<U>::type>
using cv_of = std::integral_constant<qualifier_flags,
    (std::is_const<T>::value ? const_ : default_)
    | (std::is_volatile<T>::value ? volatile_ : default_)>;

template<typename T>
using ref_of = std::integral_constant<qualifier_flags,
    std::is_rvalue_reference<T>::value ? rref_
    : (std::is_lvalue_reference<T>::value ? lref_
        : default_)>;

//bit-flag implementation of C++11 reference collapsing rules
template<qualifier_flags Existing,
         qualifier_flags Other,
         bool AlreadyHasRef = (Existing & (lref_ | rref_)) != 0,
         bool AlreadyHasLRef = (Existing & lref_) == lref_,
         bool IsAddingLRef = (Other & lref_) == lref_
>
using collapse_flags = std::integral_constant<qualifier_flags,
    !AlreadyHasRef ? (Existing | Other)
        : (AlreadyHasLRef ? (Existing | (Other & ~rref_))
            : (IsAddingLRef ? ((Existing & ~rref_) | Other )
                : (Existing | Other)))>;

template<typename T> struct flag_map { static constexpr qualifier_flags value = default_; };
template<typename T> struct flag_map<T &> { static constexpr qualifier_flags value = lref_; };
template<typename T> struct flag_map<T &&> { static constexpr qualifier_flags value = rref_; };
template<typename T> struct flag_map<T const> { static constexpr qualifier_flags value = const_; };
template<typename T> struct flag_map<T const &> { static constexpr qualifier_flags value = const_ | lref_; };
template<typename T> struct flag_map<T const &&> { static constexpr qualifier_flags value = const_ | rref_; };
template<typename T> struct flag_map<T volatile> { static constexpr qualifier_flags value = volatile_; };
template<typename T> struct flag_map<T volatile &> { static constexpr qualifier_flags value = volatile_ | lref_; };
template<typename T> struct flag_map<T volatile &&> { static constexpr qualifier_flags value = volatile_ | rref_; };
template<typename T> struct flag_map<T const volatile> { static constexpr qualifier_flags value = const_ | volatile_; };
template<typename T> struct flag_map<T const volatile &> { static constexpr qualifier_flags value = const_ | volatile_ | lref_; };
template<typename T> struct flag_map<T const volatile &&> { static constexpr qualifier_flags value = const_ | volatile_ | rref_; };

}}} // namespace boost::callable_traits::detail

#endif // #ifndef BOOST_CLBL_TRTS_QUALIFIER_FLAGS_HPP

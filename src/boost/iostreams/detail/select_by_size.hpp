// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2004-2007 Jonathan Turkanis
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// See http://www.boost.org/libs/iostreams for documentation.

//
// Intended as an alternative to type_traits::yes_type and type_traits::no_type.
// Provides an arbitrary number of types (case_<0>, case_<1>, ...) for
// determining the results of overload resultion using 'sizeof', plus a uniform
// means of using the result. yes_type and no_type are typedefs for case_<1>
// and case_<0>. A single case with negative argument, case_<-1>, is also 
// provided, for convenience.
//
// This header may be included any number of times, with
// BOOST_SELECT_BY_SIZE_MAX_CASE defined to be the largest N such that case_<N>
// is needed for a particular application. It defaults to 20.
//
// This header depends only on Boost.Config and Boost.Preprocessor. Dependence
// on Type Traits or MPL was intentionally avoided, to leave open the 
// possibility that select_by_size could be used by these libraries.
//
// Example usage:
//
//    #define BOOST_SELECT_BY_SIZE_MAX_CASE 7   // (Needed when default was 2)
//    #include <boost/utility/select_by_size.hpp>
//
//    using namespace boost::utility;
//
//    case_<0> helper(bool);
//    case_<1> helper(int);
//    case_<2> helper(unsigned);
//    case_<3> helper(long);
//    case_<4> helper(unsigned long);
//    case_<5> helper(float);
//    case_<6> helper(double);
//    case_<7> helper(const char*);
//
//    struct test {
//        static const int value =
//            select_by_size< sizeof(helper(9876UL)) >::value;
//        BOOST_STATIC_ASSERT(value == 4);
//    };
//
// For compilers with integral constant expression problems, e.g. Borland 5.x,
// one can also write
//
//    struct test {
//        BOOST_SELECT_BY_SIZE(int, value, helper(9876UL));
//    };
//
// to define a static integral constant 'value' equal to
//
//    select_by_size< sizeof(helper(9876UL)) >::value.
//

// Include guards surround all contents of this header except for explicit
// specializations of select_by_size for case_<N> with N > 2.

#ifndef BOOST_IOSTREAMS_DETAIL_SELECT_BY_SIZE_HPP_INCLUDED
#define BOOST_IOSTREAMS_DETAIL_SELECT_BY_SIZE_HPP_INCLUDED

// The lowest N for which select_by_size< sizeof(case_<N>) > has not been
// specialized.
#define SELECT_BY_SIZE_MAX_SPECIALIZED 20

#include <boost/config.hpp>    // BOOST_STATIC_CONSTANT.
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/iteration/local.hpp>

/* Alternative implementation using max_align. 

#include <boost/type_traits/alignment_of.hpp>
#include <boost/type_traits/type_with_alignment.hpp>

namespace boost { namespace utility {

template<int N>
struct case_ { char c[(N + 1) * alignment_of<detail::max_align>::value]; };

template<unsigned Size>
struct select_by_size {
    BOOST_STATIC_CONSTANT(int, value = 
        (Size / alignment_of<detail::max_align>::value - 1));
};

} } // End namespaces utility, boost.

*/              // End alternate implementation.

namespace boost { namespace iostreams { namespace detail {

//--------------Definition of case_-------------------------------------------//

template<int N> struct case_ { char c1; case_<N - 1> c2; };
template<> struct case_<-1> { char c; };
typedef case_<true> yes_type;
typedef case_<false> no_type;

//--------------Declaration of select_by_size---------------------------------//

template<unsigned Size> struct select_by_size;

} } } // End namespaces detail, iostreams, boost.

//--------------Definition of SELECT_BY_SIZE_SPEC-----------------------------//

// Sepecializes select_by_size for sizeof(case<n-1>). The decrement is used
// here because the preprocessor library doesn't handle negative integers.
#define SELECT_BY_SIZE_SPEC(n) \
    namespace boost { namespace iostreams { namespace detail { \
      static const int BOOST_PP_CAT(sizeof_case_, n) = sizeof(case_<n - 1>); \
      template<> \
      struct select_by_size< BOOST_PP_CAT(sizeof_case_, n) > { \
          struct type { BOOST_STATIC_CONSTANT(int, value = n - 1); }; \
          BOOST_STATIC_CONSTANT(int, value = type::value); \
      }; \
    } } } \
    /**/

//--------------Default specializations of select_by_size---------------------//

#define BOOST_PP_LOCAL_MACRO(n) SELECT_BY_SIZE_SPEC(n)
#define BOOST_PP_LOCAL_LIMITS (0, 20)
#include BOOST_PP_LOCAL_ITERATE()
#undef BOOST_PP_LOCAL_MACRO

//--------------Definition of SELECT_BY_SIZE----------------------------------//

#define BOOST_SELECT_BY_SIZE(type_, name, expr) \
    BOOST_STATIC_CONSTANT( \
        unsigned, \
        BOOST_PP_CAT(boost_select_by_size_temp_, name) = sizeof(expr) \
    ); \
    BOOST_STATIC_CONSTANT( \
        type_, \
        name = \
            ( ::boost::iostreams::detail::select_by_size< \
                BOOST_PP_CAT(boost_select_by_size_temp_, name) \
              >::value ) \
    ) \
    /**/

#endif // #ifndef BOOST_IOSTREAMS_DETAIL_SELECT_BY_SIZE_HPP_INCLUDED

//----------Specializations of SELECT_BY_SIZE (outside main inclued guards)---//

#if defined(BOOST_SELECT_BY_SIZE_MAX_CASE) && \
    BOOST_SELECT_BY_SIZE_MAX_CASE > SELECT_BY_SIZE_MAX_SPECIALIZED

#define BOOST_PP_LOCAL_MACRO(n) SELECT_BY_SIZE_SPEC(n)
#define BOOST_PP_LOCAL_LIMITS \
    (SELECT_BY_SIZE_MAX_SPECIALIZED, BOOST_SELECT_BY_SIZE_MAX_CASE) \
    /**/
#include BOOST_PP_LOCAL_ITERATE()
#undef BOOST_PP_LOCAL_MACRO
#undef SELECT_BY_SIZE_MAX_SPECIALIZED
#define SELECT_BY_SIZE_MAX_SPECIALIZED BOOST_SELECT_BY_SIZE_MAX_CASE

#endif

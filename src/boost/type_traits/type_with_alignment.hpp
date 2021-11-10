//  (C) Copyright John Maddock 2000.
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).
//
//  See http://www.boost.org/libs/type_traits for most recent version including documentation.

#ifndef BOOST_TT_TYPE_WITH_ALIGNMENT_INCLUDED
#define BOOST_TT_TYPE_WITH_ALIGNMENT_INCLUDED

#include <boost/type_traits/alignment_of.hpp>
#include <boost/type_traits/is_pod.hpp>
#include <boost/static_assert.hpp>
#include <boost/config.hpp>
#include <cstddef> // size_t
#include <boost/detail/workaround.hpp>

#ifdef BOOST_MSVC
#   pragma warning(push)
#   pragma warning(disable: 4121) // alignment is sensitive to packing
#endif

#ifdef _MSC_VER
#include <boost/type_traits/conditional.hpp>
#endif

namespace boost {
#ifndef BOOST_BORLANDC
   namespace detail{

      union max_align
      {
         char c;
         short s;
         int i;
         long l;
#ifndef BOOST_NO_LONG_LONG
         boost::long_long_type ll;
#endif
#ifdef BOOST_HAS_INT128
         boost::int128_type i128;
#endif
         float f;
         double d;
         long double ld;
#ifdef BOOST_HAS_FLOAT128
         __float128 f128;
#endif
      };

template <std::size_t Target, bool check> struct long_double_alignment{ typedef long double type; };
template <std::size_t Target> struct long_double_alignment<Target, false>{ typedef boost::detail::max_align type; };

template <std::size_t Target, bool check> struct double_alignment{ typedef double type; };
template <std::size_t Target> struct double_alignment<Target, false>{ typedef typename long_double_alignment<Target, boost::alignment_of<long double>::value >= Target>::type type; };

#ifndef BOOST_NO_LONG_LONG
template <std::size_t Target, bool check> struct long_long_alignment{ typedef boost::long_long_type type; };
template <std::size_t Target> struct long_long_alignment<Target, false>{ typedef typename double_alignment<Target, boost::alignment_of<double>::value >= Target>::type type; };
#endif

template <std::size_t Target, bool check> struct long_alignment{ typedef long type; };
#ifndef BOOST_NO_LONG_LONG
template <std::size_t Target> struct long_alignment<Target, false>{ typedef typename long_long_alignment<Target, boost::alignment_of<boost::long_long_type>::value >= Target>::type type; };
#else
template <std::size_t Target> struct long_alignment<Target, false>{ typedef typename double_alignment<Target, boost::alignment_of<double>::value >= Target>::type type; };
#endif

template <std::size_t Target, bool check> struct int_alignment{ typedef int type; };
template <std::size_t Target> struct int_alignment<Target, false>{ typedef typename long_alignment<Target, boost::alignment_of<long>::value >= Target>::type type; };

template <std::size_t Target, bool check> struct short_alignment{ typedef short type; };
template <std::size_t Target> struct short_alignment<Target, false>{ typedef typename int_alignment<Target, boost::alignment_of<int>::value >= Target>::type type; };

template <std::size_t Target, bool check> struct char_alignment{ typedef char type; };
template <std::size_t Target> struct char_alignment<Target, false>{ typedef typename short_alignment<Target, boost::alignment_of<short>::value >= Target>::type type; };

} // namespace detail

template <std::size_t Align>
struct type_with_alignment 
{
   typedef typename boost::detail::char_alignment<Align, boost::alignment_of<char>::value >= Align>::type type;
};

#if (defined(__GNUC__) || (defined (__SUNPRO_CC) &&  (__SUNPRO_CC >= 0x5130)) || defined(__clang__)) && !defined(BOOST_TT_DISABLE_INTRINSICS)
namespace tt_align_ns {
struct __attribute__((__aligned__(2))) a2 {};
struct __attribute__((__aligned__(4))) a4 {};
struct __attribute__((__aligned__(8))) a8 {};
struct __attribute__((__aligned__(16))) a16 {};
struct __attribute__((__aligned__(32))) a32 {};
struct __attribute__((__aligned__(64))) a64 {};
struct __attribute__((__aligned__(128))) a128 {};
}

template<> struct type_with_alignment<1>  { public: typedef char type; };
template<> struct type_with_alignment<2>  { public: typedef tt_align_ns::a2 type; };
template<> struct type_with_alignment<4>  { public: typedef tt_align_ns::a4 type; };
template<> struct type_with_alignment<8>  { public: typedef tt_align_ns::a8 type; };
template<> struct type_with_alignment<16> { public: typedef tt_align_ns::a16 type; };
template<> struct type_with_alignment<32> { public: typedef tt_align_ns::a32 type; };
template<> struct type_with_alignment<64> { public: typedef tt_align_ns::a64 type; };
template<> struct type_with_alignment<128> { public: typedef tt_align_ns::a128 type; };

template<> struct is_pod< ::boost::tt_align_ns::a2> : public true_type{};
template<> struct is_pod< ::boost::tt_align_ns::a4> : public true_type{};
template<> struct is_pod< ::boost::tt_align_ns::a8> : public true_type{};
template<> struct is_pod< ::boost::tt_align_ns::a16> : public true_type{};
template<> struct is_pod< ::boost::tt_align_ns::a32> : public true_type{};
template<> struct is_pod< ::boost::tt_align_ns::a64> : public true_type{};
template<> struct is_pod< ::boost::tt_align_ns::a128> : public true_type{};

#endif
#if (defined(BOOST_MSVC) || (defined(BOOST_INTEL) && defined(_MSC_VER))) && !defined(BOOST_TT_DISABLE_INTRINSICS)
//
// MSVC supports types which have alignments greater than the normal
// maximum: these are used for example in the types __m64 and __m128
// to provide types with alignment requirements which match the SSE
// registers.  Therefore we extend type_with_alignment<> to support
// such types, however, we have to be careful to use a builtin type
// whenever possible otherwise we break previously working code:
// see https://lists.boost.org/Archives/boost/2014/03/212391.php
// for an example and test case.  Thus types like a8 below will
// be used *only* if the existing implementation can't provide a type
// with suitable alignment.  This does mean however, that type_with_alignment<>
// may return a type which cannot be passed through a function call
// by value (and neither can any type containing such a type like
// Boost.Optional).  However, this only happens when we have no choice 
// in the matter because no other "ordinary" type is available.
//
namespace tt_align_ns {
struct __declspec(align(8)) a8 { 
   char m[8]; 
   typedef a8 type;
};
struct __declspec(align(16)) a16 { 
   char m[16]; 
   typedef a16 type;
};
struct __declspec(align(32)) a32 { 
   char m[32]; 
   typedef a32 type;
};
struct __declspec(align(64)) a64 
{ 
   char m[64]; 
   typedef a64 type;
};
struct __declspec(align(128)) a128 { 
   char m[128]; 
   typedef a128 type;
};
}

template<> struct type_with_alignment<8>  
{ 
   typedef boost::conditional<
      ::boost::alignment_of<boost::detail::max_align>::value < 8,
      tt_align_ns::a8,
      boost::detail::char_alignment<8, false> >::type t1;
public: 
   typedef t1::type type;
};
template<> struct type_with_alignment<16> 
{ 
   typedef boost::conditional<
      ::boost::alignment_of<boost::detail::max_align>::value < 16,
      tt_align_ns::a16,
      boost::detail::char_alignment<16, false> >::type t1;
public: 
   typedef t1::type type;
};
template<> struct type_with_alignment<32> 
{ 
   typedef boost::conditional<
      ::boost::alignment_of<boost::detail::max_align>::value < 32,
      tt_align_ns::a32,
      boost::detail::char_alignment<32, false> >::type t1;
public: 
   typedef t1::type type;
};
template<> struct type_with_alignment<64> {
   typedef boost::conditional<
      ::boost::alignment_of<boost::detail::max_align>::value < 64,
      tt_align_ns::a64,
      boost::detail::char_alignment<64, false> >::type t1;
public: 
   typedef t1::type type;
};
template<> struct type_with_alignment<128> {
   typedef boost::conditional<
      ::boost::alignment_of<boost::detail::max_align>::value < 128,
      tt_align_ns::a128,
      boost::detail::char_alignment<128, false> >::type t1;
public: 
   typedef t1::type type;
};

template<> struct is_pod< ::boost::tt_align_ns::a8> : public true_type{};
template<> struct is_pod< ::boost::tt_align_ns::a16> : public true_type{};
template<> struct is_pod< ::boost::tt_align_ns::a32> : public true_type{};
template<> struct is_pod< ::boost::tt_align_ns::a64> : public true_type{};
template<> struct is_pod< ::boost::tt_align_ns::a128> : public true_type{};

#endif

#else

//
// Borland specific version, we have this for two reasons:
// 1) The version above doesn't always compile (with the new test cases for example)
// 2) Because of Borlands #pragma option we can create types with alignments that are
//    greater that the largest aligned builtin type.

namespace tt_align_ns{
#pragma option push -a16
struct a2{ short s; };
struct a4{ int s; };
struct a8{ double s; };
struct a16{ long double s; };
#pragma option pop
}

namespace detail {

typedef ::boost::tt_align_ns::a16 max_align;

}
//#if ! BOOST_WORKAROUND(BOOST_CODEGEARC, BOOST_TESTED_AT(0x610))
template <> struct is_pod< ::boost::tt_align_ns::a2> : public true_type{};
template <> struct is_pod< ::boost::tt_align_ns::a4> : public true_type{};
template <> struct is_pod< ::boost::tt_align_ns::a8> : public true_type{};
template <> struct is_pod< ::boost::tt_align_ns::a16> : public true_type{};
//#endif

template <std::size_t N> struct type_with_alignment
{
   // We should never get to here, but if we do use the maximally
   // aligned type:
   // BOOST_STATIC_ASSERT(0);
   typedef tt_align_ns::a16 type;
};
template <> struct type_with_alignment<1>{ typedef char type; };
template <> struct type_with_alignment<2>{ typedef tt_align_ns::a2 type; };
template <> struct type_with_alignment<4>{ typedef tt_align_ns::a4 type; };
template <> struct type_with_alignment<8>{ typedef tt_align_ns::a8 type; };
template <> struct type_with_alignment<16>{ typedef tt_align_ns::a16 type; };

#endif

} // namespace boost

#ifdef BOOST_MSVC
#   pragma warning(pop)
#endif

#endif // BOOST_TT_TYPE_WITH_ALIGNMENT_INCLUDED



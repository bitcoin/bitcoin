///////////////////////////////////////////////////////////////////////////////
//  Copyright 2011 John Maddock. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_BIG_NUM_BASE_HPP
#define BOOST_MATH_BIG_NUM_BASE_HPP

#include <limits>
#include <type_traits>
#include <boost/core/nvp.hpp>
#include <boost/math/tools/complex.hpp>
#include <boost/multiprecision/traits/transcendental_reduction_type.hpp>
#include <boost/multiprecision/traits/std_integer_traits.hpp>
#ifdef BOOST_MSVC
#pragma warning(push)
#pragma warning(disable : 4307)
#endif
#include <boost/lexical_cast.hpp>
#ifdef BOOST_MSVC
#pragma warning(pop)
#endif
//
// We now require C++11, if something we use is not supported, then error and say why:
//
#ifdef BOOST_NO_CXX11_RVALUE_REFERENCES
#error "This library now requires a C++11 or later compiler - this message was generated as a result of BOOST_NO_CXX11_RVALUE_REFERENCES being set"
#endif
#ifdef BOOST_NO_CXX11_TEMPLATE_ALIASES
#error "This library now requires a C++11 or later compiler - this message was generated as a result of BOOST_NO_CXX11_TEMPLATE_ALIASES being set"
#endif
#ifdef BOOST_NO_CXX11_HDR_ARRAY
#error "This library now requires a C++11 or later compiler - this message was generated as a result of BOOST_NO_CXX11_HDR_ARRAY being set"
#endif
#ifdef BOOST_NO_CXX11_HDR_TYPE_TRAITS
#error "This library now requires a C++11 or later compiler - this message was generated as a result of BOOST_NO_CXX11_HDR_TYPE_TRAITS being set"
#endif
#ifdef BOOST_NO_CXX11_ALLOCATOR
#error "This library now requires a C++11 or later compiler - this message was generated as a result of BOOST_NO_CXX11_ALLOCATOR being set"
#endif
#ifdef BOOST_NO_CXX11_CONSTEXPR
#error "This library now requires a C++11 or later compiler - this message was generated as a result of BOOST_NO_CXX11_CONSTEXPR being set"
#endif
#ifdef BOOST_MP_NO_CXX11_EXPLICIT_CONVERSION_OPERATORS
#error "This library now requires a C++11 or later compiler - this message was generated as a result of BOOST_MP_NO_CXX11_EXPLICIT_CONVERSION_OPERATORS being set"
#endif
#ifdef BOOST_NO_CXX11_REF_QUALIFIERS
#error "This library now requires a C++11 or later compiler - this message was generated as a result of BOOST_NO_CXX11_REF_QUALIFIERS being set"
#endif
#ifdef BOOST_NO_CXX11_HDR_FUNCTIONAL
#error "This library now requires a C++11 or later compiler - this message was generated as a result of BOOST_NO_CXX11_HDR_FUNCTIONAL being set"
#endif
#ifdef BOOST_NO_CXX11_VARIADIC_TEMPLATES
#error "This library now requires a C++11 or later compiler - this message was generated as a result of BOOST_NO_CXX11_VARIADIC_TEMPLATES being set"
#endif
#ifdef BOOST_NO_CXX11_USER_DEFINED_LITERALS
#error "This library now requires a C++11 or later compiler - this message was generated as a result of BOOST_NO_CXX11_USER_DEFINED_LITERALS being set"
#endif
#ifdef BOOST_NO_CXX11_DECLTYPE
#error "This library now requires a C++11 or later compiler - this message was generated as a result of BOOST_NO_CXX11_DECLTYPE being set"
#endif
#ifdef BOOST_NO_CXX11_STATIC_ASSERT
#error "This library now requires a C++11 or later compiler - this message was generated as a result of BOOST_NO_CXX11_STATIC_ASSERT being set"
#endif
#ifdef BOOST_NO_CXX11_DEFAULTED_FUNCTIONS
#error "This library now requires a C++11 or later compiler - this message was generated as a result of BOOST_NO_CXX11_DEFAULTED_FUNCTIONS being set"
#endif
#ifdef BOOST_NO_CXX11_NOEXCEPT
#error "This library now requires a C++11 or later compiler - this message was generated as a result of BOOST_NO_CXX11_NOEXCEPT being set"
#endif
#ifdef BOOST_NO_CXX11_REF_QUALIFIERS
#error "This library now requires a C++11 or later compiler - this message was generated as a result of BOOST_NO_CXX11_REF_QUALIFIERS being set"
#endif
#ifdef BOOST_NO_CXX11_USER_DEFINED_LITERALS
#error "This library now requires a C++11 or later compiler - this message was generated as a result of BOOST_NO_CXX11_USER_DEFINED_LITERALS being set"
#endif

#if defined(NDEBUG) && !defined(_DEBUG)
#define BOOST_MP_FORCEINLINE BOOST_FORCEINLINE
#else
#define BOOST_MP_FORCEINLINE inline
#endif

//
// Thread local storage:
// Note fails on Mingw, see https://sourceforge.net/p/mingw-w64/bugs/527/
//
#if !defined(BOOST_NO_CXX11_THREAD_LOCAL) && !(defined(__MINGW32__) && (__GNUC__ < 9) && !defined(__clang__))
#define BOOST_MP_THREAD_LOCAL thread_local
#define BOOST_MP_USING_THREAD_LOCAL
#else
#pragma GCC warning "thread_local on mingw is broken, please use MSys mingw gcc-9 or later, see https://sourceforge.net/p/mingw-w64/bugs/527/"
#define BOOST_MP_THREAD_LOCAL
#endif

#ifdef __has_include
# if __has_include(<version>)
#  include <version>
#  ifdef __cpp_lib_is_constant_evaluated
#   include <type_traits>
#   define BOOST_MP_HAS_IS_CONSTANT_EVALUATED
#  endif
# endif
#endif

#ifdef __has_builtin
#if __has_builtin(__builtin_is_constant_evaluated) && !defined(BOOST_NO_CXX14_CONSTEXPR) && !defined(BOOST_NO_CXX11_UNIFIED_INITIALIZATION_SYNTAX)
#define BOOST_MP_HAS_BUILTIN_IS_CONSTANT_EVALUATED
#endif
#endif
//
// MSVC also supports __builtin_is_constant_evaluated if it's recent enough:
//
#if defined(_MSC_FULL_VER) && (_MSC_FULL_VER >= 192528326)
#  define BOOST_MP_HAS_BUILTIN_IS_CONSTANT_EVALUATED
#endif
//
// As does GCC-9:
//
#if defined(BOOST_GCC) && !defined(BOOST_NO_CXX14_CONSTEXPR) && (__GNUC__ >= 9) && !defined(BOOST_MP_HAS_BUILTIN_IS_CONSTANT_EVALUATED)
#  define BOOST_MP_HAS_BUILTIN_IS_CONSTANT_EVALUATED
#endif

#if defined(BOOST_MP_HAS_IS_CONSTANT_EVALUATED) && !defined(BOOST_NO_CXX14_CONSTEXPR)
#  define BOOST_MP_IS_CONST_EVALUATED(x) std::is_constant_evaluated()
#elif defined(BOOST_MP_HAS_BUILTIN_IS_CONSTANT_EVALUATED)
#  define BOOST_MP_IS_CONST_EVALUATED(x) __builtin_is_constant_evaluated()
#elif !defined(BOOST_NO_CXX14_CONSTEXPR) && defined(BOOST_GCC) && (__GNUC__ >= 6)
#  define BOOST_MP_IS_CONST_EVALUATED(x) __builtin_constant_p(x)
#else
#  define BOOST_MP_NO_CONSTEXPR_DETECTION
#endif

#define BOOST_MP_CXX14_CONSTEXPR BOOST_CXX14_CONSTEXPR
//
// Early compiler versions trip over the constexpr code:
//
#if defined(__clang__) && (__clang_major__ < 5)
#undef BOOST_MP_CXX14_CONSTEXPR
#define BOOST_MP_CXX14_CONSTEXPR
#endif
#if defined(__apple_build_version__) && (__clang_major__ < 9)
#undef BOOST_MP_CXX14_CONSTEXPR
#define BOOST_MP_CXX14_CONSTEXPR
#endif
#if defined(BOOST_GCC) && (__GNUC__ < 6)
#undef BOOST_MP_CXX14_CONSTEXPR
#define BOOST_MP_CXX14_CONSTEXPR
#endif
#if defined(BOOST_INTEL)
#undef BOOST_MP_CXX14_CONSTEXPR
#define BOOST_MP_CXX14_CONSTEXPR
#define BOOST_MP_NO_CONSTEXPR_DETECTION
#endif

#ifdef BOOST_MP_NO_CONSTEXPR_DETECTION
#  define BOOST_CXX14_CONSTEXPR_IF_DETECTION
#else
#  define BOOST_CXX14_CONSTEXPR_IF_DETECTION constexpr
#endif

#ifdef BOOST_MSVC
#pragma warning(push)
#pragma warning(disable : 6326)
#endif

namespace boost {
namespace multiprecision {

enum expression_template_option
{
   et_off = 0,
   et_on  = 1
};

enum struct variable_precision_options : signed char
{
   assume_uniform_precision = -1,
   preserve_target_precision = 0,
   preserve_source_precision = 1,
   preserve_component_precision = 2,
   preserve_related_precision = 3,
   preserve_all_precision = 4,
};

inline constexpr bool operator==(variable_precision_options a, variable_precision_options b)
{
   return static_cast<unsigned>(a) == static_cast<unsigned>(b);
}

template <class Backend>
struct expression_template_default
{
   static constexpr const expression_template_option value = et_on;
};

template <class Backend, expression_template_option ExpressionTemplates = expression_template_default<Backend>::value>
class number;

template <class T>
struct is_number : public std::integral_constant<bool, false>
{};

template <class Backend, expression_template_option ExpressionTemplates>
struct is_number<number<Backend, ExpressionTemplates> > : public std::integral_constant<bool, true>
{};

template <class T>
struct is_et_number : public std::integral_constant<bool, false>
{};

template <class Backend>
struct is_et_number<number<Backend, et_on> > : public std::integral_constant<bool, true>
{};

template <class T>
struct is_no_et_number : public std::integral_constant<bool, false>
{};

template <class Backend>
struct is_no_et_number<number<Backend, et_off> > : public std::integral_constant<bool, true>
{};

namespace detail {

// Forward-declare an expression wrapper
template <class tag, class Arg1 = void, class Arg2 = void, class Arg3 = void, class Arg4 = void>
struct expression;

} // namespace detail

template <class T>
struct is_number_expression : public std::integral_constant<bool, false>
{};

template <class tag, class Arg1, class Arg2, class Arg3, class Arg4>
struct is_number_expression<detail::expression<tag, Arg1, Arg2, Arg3, Arg4> > : public std::integral_constant<bool, true>
{};

template <class T, class Num>
struct is_compatible_arithmetic_type
    : public std::integral_constant<bool, 
          std::is_convertible<T, Num>::value && !std::is_same<T, Num>::value && !is_number_expression<T>::value>
{};

namespace detail {
//
// Workaround for missing abs(boost::long_long_type) and abs(__int128) on some compilers:
//
template <class T>
constexpr typename std::enable_if<(boost::multiprecision::detail::is_signed<T>::value || std::is_floating_point<T>::value), T>::type abs(T t) noexcept
{
   // This strange expression avoids a hardware trap in the corner case
   // that val is the most negative value permitted in boost::long_long_type.
   // See https://svn.boost.org/trac/boost/ticket/9740.
   return t < 0 ? T(1u) + T(-(t + 1)) : t;
}
template <class T>
constexpr typename std::enable_if<boost::multiprecision::detail::is_unsigned<T>::value, T>::type abs(T t) noexcept
{
   return t;
}

#define BOOST_MP_USING_ABS using boost::multiprecision::detail::abs;

template <class T>
constexpr typename std::enable_if<(boost::multiprecision::detail::is_signed<T>::value || std::is_floating_point<T>::value), typename boost::multiprecision::detail::make_unsigned<T>::type>::type unsigned_abs(T t) noexcept
{
   // This strange expression avoids a hardware trap in the corner case
   // that val is the most negative value permitted in boost::long_long_type.
   // See https://svn.boost.org/trac/boost/ticket/9740.
   return t < 0 ? static_cast<typename boost::multiprecision::detail::make_unsigned<T>::type>(1u) + static_cast<typename boost::multiprecision::detail::make_unsigned<T>::type>(-(t + 1)) : static_cast<typename boost::multiprecision::detail::make_unsigned<T>::type>(t);
}
template <class T>
constexpr typename std::enable_if<boost::multiprecision::detail::is_unsigned<T>::value, T>::type unsigned_abs(T t) noexcept
{
   return t;
}

template <class T>
struct bits_of
{
   static_assert(boost::multiprecision::detail::is_integral<T>::value || std::is_enum<T>::value || std::numeric_limits<T>::is_specialized, "Failed integer size check");
   static constexpr const unsigned value =
       std::numeric_limits<T>::is_specialized ? std::numeric_limits<T>::digits
                                              : sizeof(T) * CHAR_BIT - (boost::multiprecision::detail::is_signed<T>::value ? 1 : 0);
};

#if defined(_GLIBCXX_USE_FLOAT128) && defined(BOOST_GCC) && !defined(__STRICT_ANSI__)
#define BOOST_MP_BITS_OF_FLOAT128_DEFINED
template <>
struct bits_of<__float128>
{
   static constexpr const unsigned value = 113;
};
#endif

template <int b>
struct has_enough_bits
{
   template <class T>
   struct type : public std::integral_constant<bool, bits_of<T>::value >= b>
   {};
};

template <class Tuple, int i, int digits, bool = (i >= std::tuple_size<Tuple>::value)>
struct find_index_of_large_enough_type
{
   static constexpr int value = bits_of<typename std::tuple_element<i, Tuple>::type>::value >= digits ? i : find_index_of_large_enough_type<Tuple, i + 1, digits>::value;
};
template <class Tuple, int i, int digits>
struct find_index_of_large_enough_type<Tuple, i, digits, true>
{
   static constexpr int value = INT_MAX;
};

template <int index, class Tuple, class Fallback, bool = (std::tuple_size<Tuple>::value <= index)>
struct dereference_tuple
{
   using type = typename std::tuple_element<index, Tuple>::type;
};
template <int index, class Tuple, class Fallback>
struct dereference_tuple<index, Tuple, Fallback, true>
{
   using type = Fallback;
};

template <class Val, class Backend, class Tag>
struct canonical_imp
{
   using type = typename std::remove_cv<typename std::decay<const Val>::type>::type;
};
template <class B, class Backend, class Tag>
struct canonical_imp<number<B, et_on>, Backend, Tag>
{
   using type = B;
};
template <class B, class Backend, class Tag>
struct canonical_imp<number<B, et_off>, Backend, Tag>
{
   using type = B;
};
#ifdef __SUNPRO_CC
template <class B, class Backend>
struct canonical_imp<number<B, et_on>, Backend, std::integral_constant<int, 3> >
{
   using type = B;
};
template <class B, class Backend>
struct canonical_imp<number<B, et_off>, Backend, std::integral_constant<int, 3> >
{
   using type = B;
};
#endif
template <class Val, class Backend>
struct canonical_imp<Val, Backend, std::integral_constant<int, 0> >
{
   static constexpr int index = find_index_of_large_enough_type<typename Backend::signed_types, 0, bits_of<Val>::value>::value;
   using type = typename dereference_tuple<index, typename Backend::signed_types, Val>::type;
};
template <class Val, class Backend>
struct canonical_imp<Val, Backend, std::integral_constant<int, 1> >
{
   static constexpr int index = find_index_of_large_enough_type<typename Backend::unsigned_types, 0, bits_of<Val>::value>::value;
   using type = typename dereference_tuple<index, typename Backend::unsigned_types, Val>::type;
};
template <class Val, class Backend>
struct canonical_imp<Val, Backend, std::integral_constant<int, 2> >
{
   static constexpr int index = find_index_of_large_enough_type<typename Backend::float_types, 0, bits_of<Val>::value>::value;
   using type = typename dereference_tuple<index, typename Backend::float_types, Val>::type;
};
template <class Val, class Backend>
struct canonical_imp<Val, Backend, std::integral_constant<int, 3> >
{
   using type = const char*;
};
template <class Val, class Backend>
struct canonical_imp<Val, Backend, std::integral_constant<int, 4> >
{
   using underlying = typename std::underlying_type<Val>::type;
   using tag = typename std::conditional<boost::multiprecision::detail::is_signed<Val>::value, std::integral_constant<int, 0>, std::integral_constant<int, 1>>::type;
   using type = typename canonical_imp<underlying, Backend, tag>::type;
};

template <class Val, class Backend>
struct canonical
{
   using tag_type = typename std::conditional<
       boost::multiprecision::detail::is_signed<Val>::value && boost::multiprecision::detail::is_integral<Val>::value,
       std::integral_constant<int, 0>,
       typename std::conditional<
           boost::multiprecision::detail::is_unsigned<Val>::value,
           std::integral_constant<int, 1>,
           typename std::conditional<
               std::is_floating_point<Val>::value,
               std::integral_constant<int, 2>,
               typename std::conditional<
                   (std::is_convertible<Val, const char*>::value || std::is_same<Val, std::string>::value),
                   std::integral_constant<int, 3>,
                   typename std::conditional<
                     std::is_enum<Val>::value,
                     std::integral_constant<int, 4>,
                     std::integral_constant<int, 5> >::type>::type>::type>::type>::type;

   using type = typename canonical_imp<Val, Backend, tag_type>::type;
};

struct terminal
{};
struct negate
{};
struct plus
{};
struct minus
{};
struct multiplies
{};
struct divides
{};
struct modulus
{};
struct shift_left
{};
struct shift_right
{};
struct bitwise_and
{};
struct bitwise_or
{};
struct bitwise_xor
{};
struct bitwise_complement
{};
struct add_immediates
{};
struct subtract_immediates
{};
struct multiply_immediates
{};
struct divide_immediates
{};
struct modulus_immediates
{};
struct bitwise_and_immediates
{};
struct bitwise_or_immediates
{};
struct bitwise_xor_immediates
{};
struct complement_immediates
{};
struct function
{};
struct multiply_add
{};
struct multiply_subtract
{};

template <class T>
struct backend_type;

template <class T, expression_template_option ExpressionTemplates>
struct backend_type<number<T, ExpressionTemplates> >
{
   using type = T;
};

template <class tag, class A1, class A2, class A3, class A4>
struct backend_type<expression<tag, A1, A2, A3, A4> >
{
   using type = typename backend_type<typename expression<tag, A1, A2, A3, A4>::result_type>::type;
};

template <class T1, class T2>
struct combine_expression
{
   using type = decltype(T1() + T2());
};

template <class T1, expression_template_option ExpressionTemplates, class T2>
struct combine_expression<number<T1, ExpressionTemplates>, T2>
{
   using type = number<T1, ExpressionTemplates>;
};

template <class T1, class T2, expression_template_option ExpressionTemplates>
struct combine_expression<T1, number<T2, ExpressionTemplates> >
{
   using type = number<T2, ExpressionTemplates>;
};

template <class T, expression_template_option ExpressionTemplates>
struct combine_expression<number<T, ExpressionTemplates>, number<T, ExpressionTemplates> >
{
   using type = number<T, ExpressionTemplates>;
};

template <class T1, expression_template_option ExpressionTemplates1, class T2, expression_template_option ExpressionTemplates2>
struct combine_expression<number<T1, ExpressionTemplates1>, number<T2, ExpressionTemplates2> >
{
   using type = typename std::conditional<
       std::is_convertible<number<T2, ExpressionTemplates2>, number<T1, ExpressionTemplates2> >::value,
       number<T1, ExpressionTemplates1>,
       number<T2, ExpressionTemplates2> >::type;
};

template <class T>
struct arg_type
{
   using type = expression<terminal, T>;
};

template <class Tag, class Arg1, class Arg2, class Arg3, class Arg4>
struct arg_type<expression<Tag, Arg1, Arg2, Arg3, Arg4> >
{
   using type = expression<Tag, Arg1, Arg2, Arg3, Arg4>;
};

struct unmentionable
{
   unmentionable* proc() { return 0; }
};

typedef unmentionable* (unmentionable::*unmentionable_type)();

template <class T, bool b>
struct expression_storage_base
{
   using type = const T&;
};

template <class T>
struct expression_storage_base<T, true>
{
   using type = T;
};

template <class T>
struct expression_storage : public expression_storage_base<T, boost::multiprecision::detail::is_arithmetic<T>::value>
{};

template <class T>
struct expression_storage<T*>
{
   using type = T*;
};

template <class T>
struct expression_storage<const T*>
{
   using type = const T*;
};

template <class tag, class A1, class A2, class A3, class A4>
struct expression_storage<expression<tag, A1, A2, A3, A4> >
{
   using type = expression<tag, A1, A2, A3, A4>;
};

template <class tag, class Arg1>
struct expression<tag, Arg1, void, void, void>
{
   using arity = std::integral_constant<int, 1>                   ;
   using left_type = typename arg_type<Arg1>::type  ;
   using left_result_type = typename left_type::result_type;
   using result_type = typename left_type::result_type;
   using tag_type = tag                            ;

   explicit BOOST_MP_CXX14_CONSTEXPR expression(const Arg1& a) : arg(a) {}
   BOOST_MP_CXX14_CONSTEXPR expression(const expression& e) : arg(e.arg) {}

   //
   // If we have static_assert we can give a more useful error message
   // than if we simply have no operator defined at all:
   //
   template <class Other>
   BOOST_MP_CXX14_CONSTEXPR expression& operator=(const Other&)
   {
      // This should always fail:
      static_assert(sizeof(Other) == INT_MAX, "You can not assign to a Boost.Multiprecision expression template: did you inadvertantly store an expression template in a \"auto\" variable?  Or pass an expression to a template function with deduced temnplate arguments?");
      return *this;
   }
   BOOST_MP_CXX14_CONSTEXPR expression& operator++()
   {
      // This should always fail:
      static_assert(sizeof(*this) == INT_MAX, "You can not increment a Boost.Multiprecision expression template: did you inadvertantly store an expression template in a \"auto\" variable?  Or pass an expression to a template function with deduced temnplate arguments?");
      return *this;
   }
   BOOST_MP_CXX14_CONSTEXPR expression& operator++(int)
   {
      // This should always fail:
      static_assert(sizeof(*this) == INT_MAX, "You can not increment a Boost.Multiprecision expression template: did you inadvertantly store an expression template in a \"auto\" variable?  Or pass an expression to a template function with deduced temnplate arguments?");
      return *this;
   }
   BOOST_MP_CXX14_CONSTEXPR expression& operator--()
   {
      // This should always fail:
      static_assert(sizeof(*this) == INT_MAX, "You can not decrement a Boost.Multiprecision expression template: did you inadvertantly store an expression template in a \"auto\" variable?  Or pass an expression to a template function with deduced temnplate arguments?");
      return *this;
   }
   BOOST_MP_CXX14_CONSTEXPR expression& operator--(int)
   {
      // This should always fail:
      static_assert(sizeof(*this) == INT_MAX, "You can not decrement a Boost.Multiprecision expression template: did you inadvertantly store an expression template in a \"auto\" variable?  Or pass an expression to a template function with deduced temnplate arguments?");
      return *this;
   }
   template <class Other>
   BOOST_MP_CXX14_CONSTEXPR expression& operator+=(const Other&)
   {
      // This should always fail:
      static_assert(sizeof(Other) == INT_MAX, "You can not use operator+= on a Boost.Multiprecision expression template: did you inadvertantly store an expression template in a \"auto\" variable?  Or pass an expression to a template function with deduced temnplate arguments?");
      return *this;
   }
   template <class Other>
   BOOST_MP_CXX14_CONSTEXPR expression& operator-=(const Other&)
   {
      // This should always fail:
      static_assert(sizeof(Other) == INT_MAX, "You can not use operator-= on a Boost.Multiprecision expression template: did you inadvertantly store an expression template in a \"auto\" variable?  Or pass an expression to a template function with deduced temnplate arguments?");
      return *this;
   }
   template <class Other>
   BOOST_MP_CXX14_CONSTEXPR expression& operator*=(const Other&)
   {
      // This should always fail:
      static_assert(sizeof(Other) == INT_MAX, "You can not use operator*= on a Boost.Multiprecision expression template: did you inadvertantly store an expression template in a \"auto\" variable?  Or pass an expression to a template function with deduced temnplate arguments?");
      return *this;
   }
   template <class Other>
   BOOST_MP_CXX14_CONSTEXPR expression& operator/=(const Other&)
   {
      // This should always fail:
      static_assert(sizeof(Other) == INT_MAX, "You can not use operator/= on a Boost.Multiprecision expression template: did you inadvertantly store an expression template in a \"auto\" variable?  Or pass an expression to a template function with deduced temnplate arguments?");
      return *this;
   }
   template <class Other>
   BOOST_MP_CXX14_CONSTEXPR expression& operator%=(const Other&)
   {
      // This should always fail:
      static_assert(sizeof(Other) == INT_MAX, "You can not use operator%= on a Boost.Multiprecision expression template: did you inadvertantly store an expression template in a \"auto\" variable?  Or pass an expression to a template function with deduced temnplate arguments?");
      return *this;
   }
   template <class Other>
   BOOST_MP_CXX14_CONSTEXPR expression& operator|=(const Other&)
   {
      // This should always fail:
      static_assert(sizeof(Other) == INT_MAX, "You can not use operator|= on a Boost.Multiprecision expression template: did you inadvertantly store an expression template in a \"auto\" variable?  Or pass an expression to a template function with deduced temnplate arguments?");
      return *this;
   }
   template <class Other>
   BOOST_MP_CXX14_CONSTEXPR expression& operator&=(const Other&)
   {
      // This should always fail:
      static_assert(sizeof(Other) == INT_MAX, "You can not use operator&= on a Boost.Multiprecision expression template: did you inadvertantly store an expression template in a \"auto\" variable?  Or pass an expression to a template function with deduced temnplate arguments?");
      return *this;
   }
   template <class Other>
   BOOST_MP_CXX14_CONSTEXPR expression& operator^=(const Other&)
   {
      // This should always fail:
      static_assert(sizeof(Other) == INT_MAX, "You can not use operator^= on a Boost.Multiprecision expression template: did you inadvertantly store an expression template in a \"auto\" variable?  Or pass an expression to a template function with deduced temnplate arguments?");
      return *this;
   }
   template <class Other>
   BOOST_MP_CXX14_CONSTEXPR expression& operator<<=(const Other&)
   {
      // This should always fail:
      static_assert(sizeof(Other) == INT_MAX, "You can not use operator<<= on a Boost.Multiprecision expression template: did you inadvertantly store an expression template in a \"auto\" variable?  Or pass an expression to a template function with deduced temnplate arguments?");
      return *this;
   }
   template <class Other>
   BOOST_MP_CXX14_CONSTEXPR expression& operator>>=(const Other&)
   {
      // This should always fail:
      static_assert(sizeof(Other) == INT_MAX, "You can not use operator>>= on a Boost.Multiprecision expression template: did you inadvertantly store an expression template in a \"auto\" variable?  Or pass an expression to a template function with deduced temnplate arguments?");
      return *this;
   }

   BOOST_MP_CXX14_CONSTEXPR left_type left() const
   {
      return left_type(arg);
   }

   BOOST_MP_CXX14_CONSTEXPR const Arg1& left_ref() const noexcept { return arg; }

   static constexpr const unsigned depth = left_type::depth + 1;
   template <class T
#ifndef __SUNPRO_CC
             ,
             typename std::enable_if<!is_number<T>::value && !std::is_convertible<result_type, T const&>::value && std::is_constructible<T, result_type>::value, int>::type = 0
#endif
             >
   explicit BOOST_MP_CXX14_CONSTEXPR operator T() const
   {
      return static_cast<T>(static_cast<result_type>(*this));
   }
   BOOST_MP_FORCEINLINE explicit BOOST_MP_CXX14_CONSTEXPR operator bool() const
   {
      result_type r(*this);
      return static_cast<bool>(r);
   }

   template <class T>
   BOOST_MP_CXX14_CONSTEXPR T convert_to()
   {
      result_type r(*this);
      return r.template convert_to<T>();
   }

 private:
   typename expression_storage<Arg1>::type arg;
   expression&                             operator=(const expression&);
};

template <class Arg1>
struct expression<terminal, Arg1, void, void, void>
{
   using arity = std::integral_constant<int, 0>;
   using result_type = Arg1        ;
   using tag_type = terminal    ;

   explicit BOOST_MP_CXX14_CONSTEXPR expression(const Arg1& a) : arg(a) {}
   BOOST_MP_CXX14_CONSTEXPR expression(const expression& e) : arg(e.arg) {}

   //
   // If we have static_assert we can give a more useful error message
   // than if we simply have no operator defined at all:
   //
   template <class Other>
   BOOST_MP_CXX14_CONSTEXPR expression& operator=(const Other&)
   {
      // This should always fail:
      static_assert(sizeof(Other) == INT_MAX, "You can not assign to a Boost.Multiprecision expression template: did you inadvertantly store an expression template in a \"auto\" variable?  Or pass an expression to a template function with deduced temnplate arguments?");
      return *this;
   }
   BOOST_MP_CXX14_CONSTEXPR expression& operator++()
   {
      // This should always fail:
      static_assert(sizeof(*this) == INT_MAX, "You can not increment a Boost.Multiprecision expression template: did you inadvertantly store an expression template in a \"auto\" variable?  Or pass an expression to a template function with deduced temnplate arguments?");
      return *this;
   }
   BOOST_MP_CXX14_CONSTEXPR expression& operator++(int)
   {
      // This should always fail:
      static_assert(sizeof(*this) == INT_MAX, "You can not increment a Boost.Multiprecision expression template: did you inadvertantly store an expression template in a \"auto\" variable?  Or pass an expression to a template function with deduced temnplate arguments?");
      return *this;
   }
   BOOST_MP_CXX14_CONSTEXPR expression& operator--()
   {
      // This should always fail:
      static_assert(sizeof(*this) == INT_MAX, "You can not decrement a Boost.Multiprecision expression template: did you inadvertantly store an expression template in a \"auto\" variable?  Or pass an expression to a template function with deduced temnplate arguments?");
      return *this;
   }
   BOOST_MP_CXX14_CONSTEXPR expression& operator--(int)
   {
      // This should always fail:
      static_assert(sizeof(*this) == INT_MAX, "You can not decrement a Boost.Multiprecision expression template: did you inadvertantly store an expression template in a \"auto\" variable?  Or pass an expression to a template function with deduced temnplate arguments?");
      return *this;
   }
   template <class Other>
   BOOST_MP_CXX14_CONSTEXPR expression& operator+=(const Other&)
   {
      // This should always fail:
      static_assert(sizeof(Other) == INT_MAX, "You can not use operator+= on a Boost.Multiprecision expression template: did you inadvertantly store an expression template in a \"auto\" variable?  Or pass an expression to a template function with deduced temnplate arguments?");
      return *this;
   }
   template <class Other>
   BOOST_MP_CXX14_CONSTEXPR expression& operator-=(const Other&)
   {
      // This should always fail:
      static_assert(sizeof(Other) == INT_MAX, "You can not use operator-= on a Boost.Multiprecision expression template: did you inadvertantly store an expression template in a \"auto\" variable?  Or pass an expression to a template function with deduced temnplate arguments?");
      return *this;
   }
   template <class Other>
   BOOST_MP_CXX14_CONSTEXPR expression& operator*=(const Other&)
   {
      // This should always fail:
      static_assert(sizeof(Other) == INT_MAX, "You can not use operator*= on a Boost.Multiprecision expression template: did you inadvertantly store an expression template in a \"auto\" variable?  Or pass an expression to a template function with deduced temnplate arguments?");
      return *this;
   }
   template <class Other>
   BOOST_MP_CXX14_CONSTEXPR expression& operator/=(const Other&)
   {
      // This should always fail:
      static_assert(sizeof(Other) == INT_MAX, "You can not use operator/= on a Boost.Multiprecision expression template: did you inadvertantly store an expression template in a \"auto\" variable?  Or pass an expression to a template function with deduced temnplate arguments?");
      return *this;
   }
   template <class Other>
   BOOST_MP_CXX14_CONSTEXPR expression& operator%=(const Other&)
   {
      // This should always fail:
      static_assert(sizeof(Other) == INT_MAX, "You can not use operator%= on a Boost.Multiprecision expression template: did you inadvertantly store an expression template in a \"auto\" variable?  Or pass an expression to a template function with deduced temnplate arguments?");
      return *this;
   }
   template <class Other>
   BOOST_MP_CXX14_CONSTEXPR expression& operator|=(const Other&)
   {
      // This should always fail:
      static_assert(sizeof(Other) == INT_MAX, "You can not use operator|= on a Boost.Multiprecision expression template: did you inadvertantly store an expression template in a \"auto\" variable?  Or pass an expression to a template function with deduced temnplate arguments?");
      return *this;
   }
   template <class Other>
   BOOST_MP_CXX14_CONSTEXPR expression& operator&=(const Other&)
   {
      // This should always fail:
      static_assert(sizeof(Other) == INT_MAX, "You can not use operator&= on a Boost.Multiprecision expression template: did you inadvertantly store an expression template in a \"auto\" variable?  Or pass an expression to a template function with deduced temnplate arguments?");
      return *this;
   }
   template <class Other>
   BOOST_MP_CXX14_CONSTEXPR expression& operator^=(const Other&)
   {
      // This should always fail:
      static_assert(sizeof(Other) == INT_MAX, "You can not use operator^= on a Boost.Multiprecision expression template: did you inadvertantly store an expression template in a \"auto\" variable?  Or pass an expression to a template function with deduced temnplate arguments?");
      return *this;
   }
   template <class Other>
   BOOST_MP_CXX14_CONSTEXPR expression& operator<<=(const Other&)
   {
      // This should always fail:
      static_assert(sizeof(Other) == INT_MAX, "You can not use operator<<= on a Boost.Multiprecision expression template: did you inadvertantly store an expression template in a \"auto\" variable?  Or pass an expression to a template function with deduced temnplate arguments?");
      return *this;
   }
   template <class Other>
   BOOST_MP_CXX14_CONSTEXPR expression& operator>>=(const Other&)
   {
      // This should always fail:
      static_assert(sizeof(Other) == INT_MAX, "You can not use operator>>= on a Boost.Multiprecision expression template: did you inadvertantly store an expression template in a \"auto\" variable?  Or pass an expression to a template function with deduced temnplate arguments?");
      return *this;
   }

   BOOST_MP_CXX14_CONSTEXPR const Arg1& value() const noexcept
   {
      return arg;
   }

   static constexpr const unsigned depth = 0;

   template <class T
#ifndef __SUNPRO_CC
             ,
             typename std::enable_if<!is_number<T>::value && !std::is_convertible<result_type, T const&>::value && std::is_constructible<T, result_type>::value, int>::type = 0
#endif
             >
   explicit BOOST_MP_CXX14_CONSTEXPR operator T() const
   {
      return static_cast<T>(static_cast<result_type>(*this));
   }
   BOOST_MP_FORCEINLINE explicit BOOST_MP_CXX14_CONSTEXPR operator bool() const
   {
      result_type r(*this);
      return static_cast<bool>(r);
   }

   template <class T>
   BOOST_MP_CXX14_CONSTEXPR T convert_to()
   {
      result_type r(*this);
      return r.template convert_to<T>();
   }

 private:
   typename expression_storage<Arg1>::type arg;
   expression&                             operator=(const expression&);
};

template <class tag, class Arg1, class Arg2>
struct expression<tag, Arg1, Arg2, void, void>
{
   using arity = std::integral_constant<int, 2>                                                          ;
   using left_type = typename arg_type<Arg1>::type                                         ;
   using right_type = typename arg_type<Arg2>::type                                         ;
   using left_result_type = typename left_type::result_type                                       ;
   using right_result_type = typename right_type::result_type                                      ;
   using result_type = typename combine_expression<left_result_type, right_result_type>::type;
   using tag_type = tag                                                                   ;

   BOOST_MP_CXX14_CONSTEXPR expression(const Arg1& a1, const Arg2& a2) : arg1(a1), arg2(a2) {}
   BOOST_MP_CXX14_CONSTEXPR expression(const expression& e) : arg1(e.arg1), arg2(e.arg2) {}

   //
   // If we have static_assert we can give a more useful error message
   // than if we simply have no operator defined at all:
   //
   template <class Other>
   BOOST_MP_CXX14_CONSTEXPR expression& operator=(const Other&)
   {
      // This should always fail:
      static_assert(sizeof(Other) == INT_MAX, "You can not assign to a Boost.Multiprecision expression template: did you inadvertantly store an expression template in a \"auto\" variable?  Or pass an expression to a template function with deduced temnplate arguments?");
      return *this;
   }
   BOOST_MP_CXX14_CONSTEXPR expression& operator++()
   {
      // This should always fail:
      static_assert(sizeof(*this) == INT_MAX, "You can not increment a Boost.Multiprecision expression template: did you inadvertantly store an expression template in a \"auto\" variable?  Or pass an expression to a template function with deduced temnplate arguments?");
      return *this;
   }
   BOOST_MP_CXX14_CONSTEXPR expression& operator++(int)
   {
      // This should always fail:
      static_assert(sizeof(*this) == INT_MAX, "You can not increment a Boost.Multiprecision expression template: did you inadvertantly store an expression template in a \"auto\" variable?  Or pass an expression to a template function with deduced temnplate arguments?");
      return *this;
   }
   BOOST_MP_CXX14_CONSTEXPR expression& operator--()
   {
      // This should always fail:
      static_assert(sizeof(*this) == INT_MAX, "You can not decrement a Boost.Multiprecision expression template: did you inadvertantly store an expression template in a \"auto\" variable?  Or pass an expression to a template function with deduced temnplate arguments?");
      return *this;
   }
   BOOST_MP_CXX14_CONSTEXPR expression& operator--(int)
   {
      // This should always fail:
      static_assert(sizeof(*this) == INT_MAX, "You can not decrement a Boost.Multiprecision expression template: did you inadvertantly store an expression template in a \"auto\" variable?  Or pass an expression to a template function with deduced temnplate arguments?");
      return *this;
   }
   template <class Other>
   BOOST_MP_CXX14_CONSTEXPR expression& operator+=(const Other&)
   {
      // This should always fail:
      static_assert(sizeof(Other) == INT_MAX, "You can not use operator+= on a Boost.Multiprecision expression template: did you inadvertantly store an expression template in a \"auto\" variable?  Or pass an expression to a template function with deduced temnplate arguments?");
      return *this;
   }
   template <class Other>
   BOOST_MP_CXX14_CONSTEXPR expression& operator-=(const Other&)
   {
      // This should always fail:
      static_assert(sizeof(Other) == INT_MAX, "You can not use operator-= on a Boost.Multiprecision expression template: did you inadvertantly store an expression template in a \"auto\" variable?  Or pass an expression to a template function with deduced temnplate arguments?");
      return *this;
   }
   template <class Other>
   BOOST_MP_CXX14_CONSTEXPR expression& operator*=(const Other&)
   {
      // This should always fail:
      static_assert(sizeof(Other) == INT_MAX, "You can not use operator*= on a Boost.Multiprecision expression template: did you inadvertantly store an expression template in a \"auto\" variable?  Or pass an expression to a template function with deduced temnplate arguments?");
      return *this;
   }
   template <class Other>
   BOOST_MP_CXX14_CONSTEXPR expression& operator/=(const Other&)
   {
      // This should always fail:
      static_assert(sizeof(Other) == INT_MAX, "You can not use operator/= on a Boost.Multiprecision expression template: did you inadvertantly store an expression template in a \"auto\" variable?  Or pass an expression to a template function with deduced temnplate arguments?");
      return *this;
   }
   template <class Other>
   BOOST_MP_CXX14_CONSTEXPR expression& operator%=(const Other&)
   {
      // This should always fail:
      static_assert(sizeof(Other) == INT_MAX, "You can not use operator%= on a Boost.Multiprecision expression template: did you inadvertantly store an expression template in a \"auto\" variable?  Or pass an expression to a template function with deduced temnplate arguments?");
      return *this;
   }
   template <class Other>
   BOOST_MP_CXX14_CONSTEXPR expression& operator|=(const Other&)
   {
      // This should always fail:
      static_assert(sizeof(Other) == INT_MAX, "You can not use operator|= on a Boost.Multiprecision expression template: did you inadvertantly store an expression template in a \"auto\" variable?  Or pass an expression to a template function with deduced temnplate arguments?");
      return *this;
   }
   template <class Other>
   BOOST_MP_CXX14_CONSTEXPR expression& operator&=(const Other&)
   {
      // This should always fail:
      static_assert(sizeof(Other) == INT_MAX, "You can not use operator&= on a Boost.Multiprecision expression template: did you inadvertantly store an expression template in a \"auto\" variable?  Or pass an expression to a template function with deduced temnplate arguments?");
      return *this;
   }
   template <class Other>
   BOOST_MP_CXX14_CONSTEXPR expression& operator^=(const Other&)
   {
      // This should always fail:
      static_assert(sizeof(Other) == INT_MAX, "You can not use operator^= on a Boost.Multiprecision expression template: did you inadvertantly store an expression template in a \"auto\" variable?  Or pass an expression to a template function with deduced temnplate arguments?");
      return *this;
   }
   template <class Other>
   BOOST_MP_CXX14_CONSTEXPR expression& operator<<=(const Other&)
   {
      // This should always fail:
      static_assert(sizeof(Other) == INT_MAX, "You can not use operator<<= on a Boost.Multiprecision expression template: did you inadvertantly store an expression template in a \"auto\" variable?  Or pass an expression to a template function with deduced temnplate arguments?");
      return *this;
   }
   template <class Other>
   BOOST_MP_CXX14_CONSTEXPR expression& operator>>=(const Other&)
   {
      // This should always fail:
      static_assert(sizeof(Other) == INT_MAX, "You can not use operator>>= on a Boost.Multiprecision expression template: did you inadvertantly store an expression template in a \"auto\" variable?  Or pass an expression to a template function with deduced temnplate arguments?");
      return *this;
   }

   BOOST_MP_CXX14_CONSTEXPR left_type left() const
   {
      return left_type(arg1);
   }
   BOOST_MP_CXX14_CONSTEXPR right_type  right() const { return right_type(arg2); }
   BOOST_MP_CXX14_CONSTEXPR const Arg1& left_ref() const noexcept { return arg1; }
   BOOST_MP_CXX14_CONSTEXPR const Arg2& right_ref() const noexcept { return arg2; }

   template <class T
#ifndef __SUNPRO_CC
             ,
             typename std::enable_if<!is_number<T>::value && !std::is_convertible<result_type, T const&>::value && std::is_constructible<T, result_type>::value, int>::type = 0
#endif
             >
   explicit BOOST_MP_CXX14_CONSTEXPR operator T() const
   {
      return static_cast<T>(static_cast<result_type>(*this));
   }
   BOOST_MP_FORCEINLINE explicit BOOST_MP_CXX14_CONSTEXPR operator bool() const
   {
      result_type r(*this);
      return static_cast<bool>(r);
   }
   template <class T>
   BOOST_MP_CXX14_CONSTEXPR T convert_to()
   {
      result_type r(*this);
      return r.template convert_to<T>();
   }

   static const constexpr unsigned                left_depth  = left_type::depth + 1;
   static const constexpr unsigned                right_depth = right_type::depth + 1;
   static const constexpr unsigned                depth       = left_depth > right_depth ? left_depth : right_depth;

 private:
   typename expression_storage<Arg1>::type arg1;
   typename expression_storage<Arg2>::type arg2;
   expression&                             operator=(const expression&);
};

template <class tag, class Arg1, class Arg2, class Arg3>
struct expression<tag, Arg1, Arg2, Arg3, void>
{
   using arity = std::integral_constant<int, 3>                     ;
   using left_type = typename arg_type<Arg1>::type    ;
   using middle_type = typename arg_type<Arg2>::type    ;
   using right_type = typename arg_type<Arg3>::type    ;
   using left_result_type = typename left_type::result_type  ;
   using middle_result_type = typename middle_type::result_type;
   using right_result_type = typename right_type::result_type ;
   using result_type = typename combine_expression<
       left_result_type,
       typename combine_expression<right_result_type, middle_result_type>::type>::type;
   using tag_type = tag                                                                        ;

   BOOST_MP_CXX14_CONSTEXPR expression(const Arg1& a1, const Arg2& a2, const Arg3& a3) : arg1(a1), arg2(a2), arg3(a3) {}
   BOOST_MP_CXX14_CONSTEXPR expression(const expression& e) : arg1(e.arg1), arg2(e.arg2), arg3(e.arg3) {}

   //
   // If we have static_assert we can give a more useful error message
   // than if we simply have no operator defined at all:
   //
   template <class Other>
   BOOST_MP_CXX14_CONSTEXPR expression& operator=(const Other&)
   {
      // This should always fail:
      static_assert(sizeof(Other) == INT_MAX, "You can not assign to a Boost.Multiprecision expression template: did you inadvertantly store an expression template in a \"auto\" variable?  Or pass an expression to a template function with deduced temnplate arguments?");
      return *this;
   }
   BOOST_MP_CXX14_CONSTEXPR expression& operator++()
   {
      // This should always fail:
      static_assert(sizeof(*this) == INT_MAX, "You can not increment a Boost.Multiprecision expression template: did you inadvertantly store an expression template in a \"auto\" variable?  Or pass an expression to a template function with deduced temnplate arguments?");
      return *this;
   }
   BOOST_MP_CXX14_CONSTEXPR expression& operator++(int)
   {
      // This should always fail:
      static_assert(sizeof(*this) == INT_MAX, "You can not increment a Boost.Multiprecision expression template: did you inadvertantly store an expression template in a \"auto\" variable?  Or pass an expression to a template function with deduced temnplate arguments?");
      return *this;
   }
   BOOST_MP_CXX14_CONSTEXPR expression& operator--()
   {
      // This should always fail:
      static_assert(sizeof(*this) == INT_MAX, "You can not decrement a Boost.Multiprecision expression template: did you inadvertantly store an expression template in a \"auto\" variable?  Or pass an expression to a template function with deduced temnplate arguments?");
      return *this;
   }
   BOOST_MP_CXX14_CONSTEXPR expression& operator--(int)
   {
      // This should always fail:
      static_assert(sizeof(*this) == INT_MAX, "You can not decrement a Boost.Multiprecision expression template: did you inadvertantly store an expression template in a \"auto\" variable?  Or pass an expression to a template function with deduced temnplate arguments?");
      return *this;
   }
   template <class Other>
   BOOST_MP_CXX14_CONSTEXPR expression& operator+=(const Other&)
   {
      // This should always fail:
      static_assert(sizeof(Other) == INT_MAX, "You can not use operator+= on a Boost.Multiprecision expression template: did you inadvertantly store an expression template in a \"auto\" variable?  Or pass an expression to a template function with deduced temnplate arguments?");
      return *this;
   }
   template <class Other>
   BOOST_MP_CXX14_CONSTEXPR expression& operator-=(const Other&)
   {
      // This should always fail:
      static_assert(sizeof(Other) == INT_MAX, "You can not use operator-= on a Boost.Multiprecision expression template: did you inadvertantly store an expression template in a \"auto\" variable?  Or pass an expression to a template function with deduced temnplate arguments?");
      return *this;
   }
   template <class Other>
   BOOST_MP_CXX14_CONSTEXPR expression& operator*=(const Other&)
   {
      // This should always fail:
      static_assert(sizeof(Other) == INT_MAX, "You can not use operator*= on a Boost.Multiprecision expression template: did you inadvertantly store an expression template in a \"auto\" variable?  Or pass an expression to a template function with deduced temnplate arguments?");
      return *this;
   }
   template <class Other>
   BOOST_MP_CXX14_CONSTEXPR expression& operator/=(const Other&)
   {
      // This should always fail:
      static_assert(sizeof(Other) == INT_MAX, "You can not use operator/= on a Boost.Multiprecision expression template: did you inadvertantly store an expression template in a \"auto\" variable?  Or pass an expression to a template function with deduced temnplate arguments?");
      return *this;
   }
   template <class Other>
   BOOST_MP_CXX14_CONSTEXPR expression& operator%=(const Other&)
   {
      // This should always fail:
      static_assert(sizeof(Other) == INT_MAX, "You can not use operator%= on a Boost.Multiprecision expression template: did you inadvertantly store an expression template in a \"auto\" variable?  Or pass an expression to a template function with deduced temnplate arguments?");
      return *this;
   }
   template <class Other>
   BOOST_MP_CXX14_CONSTEXPR expression& operator|=(const Other&)
   {
      // This should always fail:
      static_assert(sizeof(Other) == INT_MAX, "You can not use operator|= on a Boost.Multiprecision expression template: did you inadvertantly store an expression template in a \"auto\" variable?  Or pass an expression to a template function with deduced temnplate arguments?");
      return *this;
   }
   template <class Other>
   BOOST_MP_CXX14_CONSTEXPR expression& operator&=(const Other&)
   {
      // This should always fail:
      static_assert(sizeof(Other) == INT_MAX, "You can not use operator&= on a Boost.Multiprecision expression template: did you inadvertantly store an expression template in a \"auto\" variable?  Or pass an expression to a template function with deduced temnplate arguments?");
      return *this;
   }
   template <class Other>
   BOOST_MP_CXX14_CONSTEXPR expression& operator^=(const Other&)
   {
      // This should always fail:
      static_assert(sizeof(Other) == INT_MAX, "You can not use operator^= on a Boost.Multiprecision expression template: did you inadvertantly store an expression template in a \"auto\" variable?  Or pass an expression to a template function with deduced temnplate arguments?");
      return *this;
   }
   template <class Other>
   BOOST_MP_CXX14_CONSTEXPR expression& operator<<=(const Other&)
   {
      // This should always fail:
      static_assert(sizeof(Other) == INT_MAX, "You can not use operator<<= on a Boost.Multiprecision expression template: did you inadvertantly store an expression template in a \"auto\" variable?  Or pass an expression to a template function with deduced temnplate arguments?");
      return *this;
   }
   template <class Other>
   BOOST_MP_CXX14_CONSTEXPR expression& operator>>=(const Other&)
   {
      // This should always fail:
      static_assert(sizeof(Other) == INT_MAX, "You can not use operator>>= on a Boost.Multiprecision expression template: did you inadvertantly store an expression template in a \"auto\" variable?  Or pass an expression to a template function with deduced temnplate arguments?");
      return *this;
   }

   BOOST_MP_CXX14_CONSTEXPR left_type left() const
   {
      return left_type(arg1);
   }
   BOOST_MP_CXX14_CONSTEXPR middle_type middle() const { return middle_type(arg2); }
   BOOST_MP_CXX14_CONSTEXPR right_type  right() const { return right_type(arg3); }
   BOOST_MP_CXX14_CONSTEXPR const Arg1& left_ref() const noexcept { return arg1; }
   BOOST_MP_CXX14_CONSTEXPR const Arg2& middle_ref() const noexcept { return arg2; }
   BOOST_MP_CXX14_CONSTEXPR const Arg3& right_ref() const noexcept { return arg3; }

   template <class T
#ifndef __SUNPRO_CC
             ,
             typename std::enable_if<!is_number<T>::value && !std::is_convertible<result_type, T const&>::value && std::is_constructible<T, result_type>::value, int>::type = 0
#endif
             >
   explicit BOOST_MP_CXX14_CONSTEXPR operator T() const
   {
      return static_cast<T>(static_cast<result_type>(*this));
   }
   BOOST_MP_FORCEINLINE explicit BOOST_MP_CXX14_CONSTEXPR operator bool() const
   {
      result_type r(*this);
      return static_cast<bool>(r);
   }
   template <class T>
   BOOST_MP_CXX14_CONSTEXPR T convert_to()
   {
      result_type r(*this);
      return r.template convert_to<T>();
   }

   static constexpr const unsigned left_depth   = left_type::depth + 1;
   static constexpr const unsigned middle_depth = middle_type::depth + 1;
   static constexpr const unsigned right_depth  = right_type::depth + 1;
   static constexpr const unsigned depth        = left_depth > right_depth ? (left_depth > middle_depth ? left_depth : middle_depth) : (right_depth > middle_depth ? right_depth : middle_depth);

 private:
   typename expression_storage<Arg1>::type arg1;
   typename expression_storage<Arg2>::type arg2;
   typename expression_storage<Arg3>::type arg3;
   expression&                             operator=(const expression&);
};

template <class tag, class Arg1, class Arg2, class Arg3, class Arg4>
struct expression
{
   using arity = std::integral_constant<int, 4>                           ;
   using left_type = typename arg_type<Arg1>::type          ;
   using left_middle_type = typename arg_type<Arg2>::type          ;
   using right_middle_type = typename arg_type<Arg3>::type          ;
   using right_type = typename arg_type<Arg4>::type          ;
   using left_result_type = typename left_type::result_type        ;
   using left_middle_result_type = typename left_middle_type::result_type ;
   using right_middle_result_type = typename right_middle_type::result_type;
   using right_result_type = typename right_type::result_type       ;
   using result_type = typename combine_expression<
       left_result_type,
       typename combine_expression<
           left_middle_result_type,
           typename combine_expression<right_middle_result_type, right_result_type>::type>::type>::type;
   using tag_type = tag                                                                                         ;

   BOOST_MP_CXX14_CONSTEXPR expression(const Arg1& a1, const Arg2& a2, const Arg3& a3, const Arg4& a4) : arg1(a1), arg2(a2), arg3(a3), arg4(a4) {}
   BOOST_MP_CXX14_CONSTEXPR expression(const expression& e) : arg1(e.arg1), arg2(e.arg2), arg3(e.arg3), arg4(e.arg4) {}

   //
   // If we have static_assert we can give a more useful error message
   // than if we simply have no operator defined at all:
   //
   template <class Other>
   BOOST_MP_CXX14_CONSTEXPR expression& operator=(const Other&)
   {
      // This should always fail:
      static_assert(sizeof(Other) == INT_MAX, "You can not assign to a Boost.Multiprecision expression template: did you inadvertantly store an expression template in a \"auto\" variable?  Or pass an expression to a template function with deduced temnplate arguments?");
      return *this;
   }
   BOOST_MP_CXX14_CONSTEXPR expression& operator++()
   {
      // This should always fail:
      static_assert(sizeof(*this) == INT_MAX, "You can not increment a Boost.Multiprecision expression template: did you inadvertantly store an expression template in a \"auto\" variable?  Or pass an expression to a template function with deduced temnplate arguments?");
      return *this;
   }
   BOOST_MP_CXX14_CONSTEXPR expression& operator++(int)
   {
      // This should always fail:
      static_assert(sizeof(*this) == INT_MAX, "You can not increment a Boost.Multiprecision expression template: did you inadvertantly store an expression template in a \"auto\" variable?  Or pass an expression to a template function with deduced temnplate arguments?");
      return *this;
   }
   BOOST_MP_CXX14_CONSTEXPR expression& operator--()
   {
      // This should always fail:
      static_assert(sizeof(*this) == INT_MAX, "You can not decrement a Boost.Multiprecision expression template: did you inadvertantly store an expression template in a \"auto\" variable?  Or pass an expression to a template function with deduced temnplate arguments?");
      return *this;
   }
   BOOST_MP_CXX14_CONSTEXPR expression& operator--(int)
   {
      // This should always fail:
      static_assert(sizeof(*this) == INT_MAX, "You can not decrement a Boost.Multiprecision expression template: did you inadvertantly store an expression template in a \"auto\" variable?  Or pass an expression to a template function with deduced temnplate arguments?");
      return *this;
   }
   template <class Other>
   BOOST_MP_CXX14_CONSTEXPR expression& operator+=(const Other&)
   {
      // This should always fail:
      static_assert(sizeof(Other) == INT_MAX, "You can not use operator+= on a Boost.Multiprecision expression template: did you inadvertantly store an expression template in a \"auto\" variable?  Or pass an expression to a template function with deduced temnplate arguments?");
      return *this;
   }
   template <class Other>
   BOOST_MP_CXX14_CONSTEXPR expression& operator-=(const Other&)
   {
      // This should always fail:
      static_assert(sizeof(Other) == INT_MAX, "You can not use operator-= on a Boost.Multiprecision expression template: did you inadvertantly store an expression template in a \"auto\" variable?  Or pass an expression to a template function with deduced temnplate arguments?");
      return *this;
   }
   template <class Other>
   BOOST_MP_CXX14_CONSTEXPR expression& operator*=(const Other&)
   {
      // This should always fail:
      static_assert(sizeof(Other) == INT_MAX, "You can not use operator*= on a Boost.Multiprecision expression template: did you inadvertantly store an expression template in a \"auto\" variable?  Or pass an expression to a template function with deduced temnplate arguments?");
      return *this;
   }
   template <class Other>
   BOOST_MP_CXX14_CONSTEXPR expression& operator/=(const Other&)
   {
      // This should always fail:
      static_assert(sizeof(Other) == INT_MAX, "You can not use operator/= on a Boost.Multiprecision expression template: did you inadvertantly store an expression template in a \"auto\" variable?  Or pass an expression to a template function with deduced temnplate arguments?");
      return *this;
   }
   template <class Other>
   BOOST_MP_CXX14_CONSTEXPR expression& operator%=(const Other&)
   {
      // This should always fail:
      static_assert(sizeof(Other) == INT_MAX, "You can not use operator%= on a Boost.Multiprecision expression template: did you inadvertantly store an expression template in a \"auto\" variable?  Or pass an expression to a template function with deduced temnplate arguments?");
      return *this;
   }
   template <class Other>
   BOOST_MP_CXX14_CONSTEXPR expression& operator|=(const Other&)
   {
      // This should always fail:
      static_assert(sizeof(Other) == INT_MAX, "You can not use operator|= on a Boost.Multiprecision expression template: did you inadvertantly store an expression template in a \"auto\" variable?  Or pass an expression to a template function with deduced temnplate arguments?");
      return *this;
   }
   template <class Other>
   BOOST_MP_CXX14_CONSTEXPR expression& operator&=(const Other&)
   {
      // This should always fail:
      static_assert(sizeof(Other) == INT_MAX, "You can not use operator&= on a Boost.Multiprecision expression template: did you inadvertantly store an expression template in a \"auto\" variable?  Or pass an expression to a template function with deduced temnplate arguments?");
      return *this;
   }
   template <class Other>
   BOOST_MP_CXX14_CONSTEXPR expression& operator^=(const Other&)
   {
      // This should always fail:
      static_assert(sizeof(Other) == INT_MAX, "You can not use operator^= on a Boost.Multiprecision expression template: did you inadvertantly store an expression template in a \"auto\" variable?  Or pass an expression to a template function with deduced temnplate arguments?");
      return *this;
   }
   template <class Other>
   BOOST_MP_CXX14_CONSTEXPR expression& operator<<=(const Other&)
   {
      // This should always fail:
      static_assert(sizeof(Other) == INT_MAX, "You can not use operator<<= on a Boost.Multiprecision expression template: did you inadvertantly store an expression template in a \"auto\" variable?  Or pass an expression to a template function with deduced temnplate arguments?");
      return *this;
   }
   template <class Other>
   BOOST_MP_CXX14_CONSTEXPR expression& operator>>=(const Other&)
   {
      // This should always fail:
      static_assert(sizeof(Other) == INT_MAX, "You can not use operator>>= on a Boost.Multiprecision expression template: did you inadvertantly store an expression template in a \"auto\" variable?  Or pass an expression to a template function with deduced temnplate arguments?");
      return *this;
   }

   BOOST_MP_CXX14_CONSTEXPR left_type left() const
   {
      return left_type(arg1);
   }
   BOOST_MP_CXX14_CONSTEXPR left_middle_type  left_middle() const { return left_middle_type(arg2); }
   BOOST_MP_CXX14_CONSTEXPR right_middle_type right_middle() const { return right_middle_type(arg3); }
   BOOST_MP_CXX14_CONSTEXPR right_type        right() const { return right_type(arg4); }
   BOOST_MP_CXX14_CONSTEXPR const Arg1&       left_ref() const noexcept { return arg1; }
   BOOST_MP_CXX14_CONSTEXPR const Arg2&       left_middle_ref() const noexcept { return arg2; }
   BOOST_MP_CXX14_CONSTEXPR const Arg3&       right_middle_ref() const noexcept { return arg3; }
   BOOST_MP_CXX14_CONSTEXPR const Arg4&       right_ref() const noexcept { return arg4; }

   template <class T
#ifndef __SUNPRO_CC
             ,
             typename std::enable_if<!is_number<T>::value && !std::is_convertible<result_type, T const&>::value && std::is_constructible<T, result_type>::value, int>::type = 0
#endif
             >
   explicit BOOST_MP_CXX14_CONSTEXPR operator T() const
   {
      return static_cast<T>(static_cast<result_type>(*this));
   }
   BOOST_MP_FORCEINLINE explicit BOOST_MP_CXX14_CONSTEXPR operator bool() const
   {
      result_type r(*this);
      return static_cast<bool>(r);
   }
   template <class T>
   BOOST_MP_CXX14_CONSTEXPR T convert_to()
   {
      result_type r(*this);
      return r.template convert_to<T>();
   }

   static constexpr const unsigned left_depth         = left_type::depth + 1;
   static constexpr const unsigned left_middle_depth  = left_middle_type::depth + 1;
   static constexpr const unsigned right_middle_depth = right_middle_type::depth + 1;
   static constexpr const unsigned right_depth        = right_type::depth + 1;

   static constexpr const unsigned left_max_depth  = left_depth > left_middle_depth ? left_depth : left_middle_depth;
   static constexpr const unsigned right_max_depth = right_depth > right_middle_depth ? right_depth : right_middle_depth;

   static constexpr const unsigned depth = left_max_depth > right_max_depth ? left_max_depth : right_max_depth;

 private:
   typename expression_storage<Arg1>::type arg1;
   typename expression_storage<Arg2>::type arg2;
   typename expression_storage<Arg3>::type arg3;
   typename expression_storage<Arg4>::type arg4;
   expression&                             operator=(const expression&);
};

template <class T>
struct digits2
{
   static_assert(std::numeric_limits<T>::is_specialized, "numeric_limits must be specialized here");
   static_assert((std::numeric_limits<T>::radix == 2) || (std::numeric_limits<T>::radix == 10), "Failed radix check");
   // If we really have so many digits that this fails, then we're probably going to hit other problems anyway:
   static_assert(LONG_MAX / 1000 > (std::numeric_limits<T>::digits + 1), "Too many digits to cope with here");
   static constexpr const long  m_value = std::numeric_limits<T>::radix == 10 ? (((std::numeric_limits<T>::digits + 1) * 1000L) / 301L) : std::numeric_limits<T>::digits;
   static inline constexpr long value() noexcept { return m_value; }
};

#ifndef BOOST_MP_MIN_EXPONENT_DIGITS
#ifdef _MSC_VER
#define BOOST_MP_MIN_EXPONENT_DIGITS 2
#else
#define BOOST_MP_MIN_EXPONENT_DIGITS 2
#endif
#endif

template <class S>
void format_float_string(S& str, std::intmax_t my_exp, std::intmax_t digits, std::ios_base::fmtflags f, bool iszero)
{
   using size_type = typename S::size_type;
   bool                          scientific = (f & std::ios_base::scientific) == std::ios_base::scientific;
   bool                          fixed      = (f & std::ios_base::fixed) == std::ios_base::fixed;
   bool                          showpoint  = (f & std::ios_base::showpoint) == std::ios_base::showpoint;
   bool                          showpos    = (f & std::ios_base::showpos) == std::ios_base::showpos;

   bool neg = str.size() && (str[0] == '-');

   if (neg)
      str.erase(0, 1);

   if (digits == 0)
   {
      digits = (std::max)(str.size(), size_type(16));
   }

   if (iszero || str.empty() || (str.find_first_not_of('0') == S::npos))
   {
      // We will be printing zero, even though the value might not
      // actually be zero (it just may have been rounded to zero).
      str = "0";
      if (scientific || fixed)
      {
         str.append(1, '.');
         str.append(size_type(digits), '0');
         if (scientific)
            str.append("e+00");
      }
      else
      {
         if (showpoint)
         {
            str.append(1, '.');
            if (digits > 1)
               str.append(size_type(digits - 1), '0');
         }
      }
      if (neg)
         str.insert(static_cast<std::string::size_type>(0), 1, '-');
      else if (showpos)
         str.insert(static_cast<std::string::size_type>(0), 1, '+');
      return;
   }

   if (!fixed && !scientific && !showpoint)
   {
      //
      // Suppress trailing zeros:
      //
      std::string::iterator pos = str.end();
      while (pos != str.begin() && *--pos == '0')
      {
      }
      if (pos != str.end())
         ++pos;
      str.erase(pos, str.end());
      if (str.empty())
         str = '0';
   }
   else if (!fixed || (my_exp >= 0))
   {
      //
      // Pad out the end with zero's if we need to:
      //
      std::intmax_t chars = str.size();
      chars                 = digits - chars;
      if (scientific)
         ++chars;
      if (chars > 0)
      {
         str.append(static_cast<std::string::size_type>(chars), '0');
      }
   }

   if (fixed || (!scientific && (my_exp >= -4) && (my_exp < digits)))
   {
      if (1 + my_exp > static_cast<std::intmax_t>(str.size()))
      {
         // Just pad out the end with zeros:
         str.append(static_cast<std::string::size_type>(1 + my_exp - str.size()), '0');
         if (showpoint || fixed)
            str.append(".");
      }
      else if (my_exp + 1 < static_cast<std::intmax_t>(str.size()))
      {
         if (my_exp < 0)
         {
            str.insert(static_cast<std::string::size_type>(0), static_cast<std::string::size_type>(-1 - my_exp), '0');
            str.insert(static_cast<std::string::size_type>(0), "0.");
         }
         else
         {
            // Insert the decimal point:
            str.insert(static_cast<std::string::size_type>(my_exp + 1), 1, '.');
         }
      }
      else if (showpoint || fixed) // we have exactly the digits we require to left of the point
         str += ".";

      if (fixed)
      {
         // We may need to add trailing zeros:
         std::intmax_t l = str.find('.') + 1;
         l                 = digits - (str.size() - l);
         if (l > 0)
            str.append(size_type(l), '0');
      }
   }
   else
   {
      BOOST_MP_USING_ABS
      // Scientific format:
      if (showpoint || (str.size() > 1))
         str.insert(static_cast<std::string::size_type>(1u), 1, '.');
      str.append(static_cast<std::string::size_type>(1u), 'e');
      S e = boost::lexical_cast<S>(abs(my_exp));
      if (e.size() < BOOST_MP_MIN_EXPONENT_DIGITS)
         e.insert(static_cast<std::string::size_type>(0), BOOST_MP_MIN_EXPONENT_DIGITS - e.size(), '0');
      if (my_exp < 0)
         e.insert(static_cast<std::string::size_type>(0), 1, '-');
      else
         e.insert(static_cast<std::string::size_type>(0), 1, '+');
      str.append(e);
   }
   if (neg)
      str.insert(static_cast<std::string::size_type>(0), 1, '-');
   else if (showpos)
      str.insert(static_cast<std::string::size_type>(0), 1, '+');
}

template <class V>
BOOST_MP_CXX14_CONSTEXPR void check_shift_range(V val, const std::integral_constant<bool, true>&, const std::integral_constant<bool, true>&)
{
   if (val > (std::numeric_limits<std::size_t>::max)())
      BOOST_THROW_EXCEPTION(std::out_of_range("Can not shift by a value greater than std::numeric_limits<std::size_t>::max()."));
   if (val < 0)
      BOOST_THROW_EXCEPTION(std::out_of_range("Can not shift by a negative value."));
}
template <class V>
BOOST_MP_CXX14_CONSTEXPR void check_shift_range(V val, const std::integral_constant<bool, false>&, const std::integral_constant<bool, true>&)
{
   if (val < 0)
      BOOST_THROW_EXCEPTION(std::out_of_range("Can not shift by a negative value."));
}
template <class V>
BOOST_MP_CXX14_CONSTEXPR void check_shift_range(V val, const std::integral_constant<bool, true>&, const std::integral_constant<bool, false>&)
{
   if (val > (std::numeric_limits<std::size_t>::max)())
      BOOST_THROW_EXCEPTION(std::out_of_range("Can not shift by a value greater than std::numeric_limits<std::size_t>::max()."));
}
template <class V>
BOOST_MP_CXX14_CONSTEXPR void check_shift_range(V, const std::integral_constant<bool, false>&, const std::integral_constant<bool, false>&) noexcept {}

template <class T>
BOOST_MP_CXX14_CONSTEXPR const T& evaluate_if_expression(const T& val) { return val; }
template <class tag, class Arg1, class Arg2, class Arg3, class Arg4>
BOOST_MP_CXX14_CONSTEXPR typename expression<tag, Arg1, Arg2, Arg3, Arg4>::result_type evaluate_if_expression(const expression<tag, Arg1, Arg2, Arg3, Arg4>& val) { return val; }

} // namespace detail

//
// Traits class, lets us know what kind of number we have, defaults to a floating point type:
//
enum number_category_type
{
   number_kind_unknown        = -1,
   number_kind_integer        = 0,
   number_kind_floating_point = 1,
   number_kind_rational       = 2,
   number_kind_fixed_point    = 3,
   number_kind_complex        = 4
};

template <class Num, bool, bool>
struct number_category_base : public std::integral_constant<int, number_kind_unknown>
{};
template <class Num>
struct number_category_base<Num, true, false> : public std::integral_constant<int, std::numeric_limits<Num>::is_integer ? number_kind_integer : (std::numeric_limits<Num>::max_exponent ? number_kind_floating_point : number_kind_unknown)>
{};
template <class Num>
struct number_category : public number_category_base<Num, std::is_class<Num>::value || boost::multiprecision::detail::is_arithmetic<Num>::value, std::is_abstract<Num>::value>
{};
template <class Backend, expression_template_option ExpressionTemplates>
struct number_category<number<Backend, ExpressionTemplates> > : public number_category<Backend>
{};
template <class tag, class A1, class A2, class A3, class A4>
struct number_category<detail::expression<tag, A1, A2, A3, A4> > : public number_category<typename detail::expression<tag, A1, A2, A3, A4>::result_type>
{};
//
// Specializations for types which do not always have numberic_limits specializations:
//
#ifdef BOOST_HAS_INT128
template <>
struct number_category<boost::int128_type> : public std::integral_constant<int, number_kind_integer>
{};
template <>
struct number_category<boost::uint128_type> : public std::integral_constant<int, number_kind_integer>
{};
#endif
#ifdef BOOST_HAS_FLOAT128
template <>
struct number_category<__float128> : public std::integral_constant<int, number_kind_floating_point>
{};
#endif

template <class T>
struct component_type
{
   using type = T;
};
template <class tag, class A1, class A2, class A3, class A4>
struct component_type<detail::expression<tag, A1, A2, A3, A4> > : public component_type<typename detail::expression<tag, A1, A2, A3, A4>::result_type>
{};

template <class T>
struct scalar_result_from_possible_complex
{
   using type = typename std::conditional<number_category<T>::value == number_kind_complex, typename component_type<T>::type, T>::type;
};

template <class T>
struct complex_result_from_scalar; // individual backends must specialize this trait.

template <class T>
struct is_unsigned_number : public std::integral_constant<bool, false>
{};
template <class Backend, expression_template_option ExpressionTemplates>
struct is_unsigned_number<number<Backend, ExpressionTemplates> > : public is_unsigned_number<Backend>
{};
template <class T>
struct is_signed_number : public std::integral_constant<bool, !is_unsigned_number<T>::value>
{};
template <class T>
struct is_interval_number : public std::integral_constant<bool, false>
{};
template <class Backend, expression_template_option ExpressionTemplates>
struct is_interval_number<number<Backend, ExpressionTemplates> > : public is_interval_number<Backend>
{};

template <class T, class U>
struct is_equivalent_number_type : public std::is_same<T, U>
{};

template <class Backend, expression_template_option ExpressionTemplates, class T2>
struct is_equivalent_number_type<number<Backend, ExpressionTemplates>, T2> : public is_equivalent_number_type<Backend, T2>
{};
template <class T1, class Backend, expression_template_option ExpressionTemplates>
struct is_equivalent_number_type<T1, number<Backend, ExpressionTemplates> > : public is_equivalent_number_type<Backend, T1>
{};
template <class Backend, expression_template_option ExpressionTemplates, class Backend2, expression_template_option ExpressionTemplates2>
struct is_equivalent_number_type<number<Backend, ExpressionTemplates>, number<Backend2, ExpressionTemplates2> > : public is_equivalent_number_type<Backend, Backend2>
{};

}
} // namespace boost

namespace boost { namespace math {
   namespace tools {

      template <class T>
      struct promote_arg;

      template <class tag, class A1, class A2, class A3, class A4>
      struct promote_arg<boost::multiprecision::detail::expression<tag, A1, A2, A3, A4> >
      {
         using type = typename boost::multiprecision::detail::expression<tag, A1, A2, A3, A4>::result_type;
      };

      template <class R, class B, boost::multiprecision::expression_template_option ET>
      inline R real_cast(const boost::multiprecision::number<B, ET>& val)
      {
         return val.template convert_to<R>();
      }

      template <class R, class tag, class A1, class A2, class A3, class A4>
      inline R real_cast(const boost::multiprecision::detail::expression<tag, A1, A2, A3, A4>& val)
      {
         using val_type = typename boost::multiprecision::detail::expression<tag, A1, A2, A3, A4>::result_type;
         return val_type(val).template convert_to<R>();
      }

      template <class B, boost::multiprecision::expression_template_option ET>
      struct is_complex_type<boost::multiprecision::number<B, ET> > : public std::integral_constant<bool, boost::multiprecision::number_category<B>::value == boost::multiprecision::number_kind_complex> {};

} // namespace tools

namespace constants {

template <class T>
struct is_explicitly_convertible_from_string;

template <class B, boost::multiprecision::expression_template_option ET>
struct is_explicitly_convertible_from_string<boost::multiprecision::number<B, ET> >
{
   static constexpr const bool value = true;
};

} // namespace constants

}} // namespace boost::math

#ifdef BOOST_MSVC
#pragma warning(pop)
#endif

#endif // BOOST_MATH_BIG_NUM_BASE_HPP

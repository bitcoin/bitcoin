/*=============================================================================
    Phoenix V1.2.1
    Copyright (c) 2001-2002 Joel de Guzman

  Distributed under the Boost Software License, Version 1.0. (See accompanying
  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef BOOST_SPIRIT_CLASSIC_PHOENIX_OPERATORS_HPP
#define BOOST_SPIRIT_CLASSIC_PHOENIX_OPERATORS_HPP

///////////////////////////////////////////////////////////////////////////////
#if !defined(BOOST_NO_CWCTYPE)
    #include <cwctype>
#endif

#if (defined(__BORLANDC__) && !defined(__clang__)) || (defined(__ICL) && __ICL >= 700)
#define CREF const&
#else
#define CREF
#endif

#include <climits>
#include <boost/spirit/home/classic/phoenix/actor.hpp>
#include <boost/spirit/home/classic/phoenix/composite.hpp>
#include <boost/config.hpp>
#include <boost/mpl/if.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace phoenix {

///////////////////////////////////////////////////////////////////////////////
//
//  Operators
//
//      Lazy operators
//
//      This class provides a mechanism for lazily evaluating operators.
//      Syntactically, a lazy operator looks like an ordinary C/C++
//      infix, prefix or postfix operator. The operator application
//      looks the same. However, unlike ordinary operators, the actual
//      operator execution is deferred. (see actor.hpp, primitives.hpp
//      and composite.hpp for an overview). Samples:
//
//          arg1 + arg2
//          1 + arg1 * arg2
//          1 / -arg1
//          arg1 < 150
//
//      T1 set of classes implement all the C++ free operators. Like
//      lazy functions (see functions.hpp), lazy operators are not
//      immediately executed when invoked. Instead, a composite (see
//      composite.hpp) object is created and returned to the caller.
//      Example:
//
//          (arg1 + arg2) * arg3
//
//      does nothing more than return a composite. T1 second function
//      call will evaluate the actual operators. Example:
//
//          int i = 4, j = 5, k = 6;
//          cout << ((arg1 + arg2) * arg3)(i, j, k);
//
//      will print out "54".
//
//      Arbitrarily complex expressions can be lazily evaluated
//      following three simple rules:
//
//          1) Lazy evaluated binary operators apply when at least one
//          of the operands is an actor object (see actor.hpp and
//          primitives.hpp). Consequently, if an operand is not an actor
//          object, it is implicitly converted to an object of type
//          actor<value<T> > (where T is the original type of the
//          operand).
//
//          2) Lazy evaluated unary operators apply only to operands
//          which are actor objects.
//
//          3) The result of a lazy operator is a composite actor object
//          that can in turn apply to rule 1.
//
//      Example:
//
//          arg1 + 3
//
//      is a lazy expression involving the operator+. Following rule 1,
//      lazy evaluation is triggered since arg1 is an instance of an
//      actor<argument<N> > class (see primitives.hpp). The right
//      operand <3> is implicitly converted to an actor<value<int> >.
//      The result of this binary + expression is a composite object,
//      following rule 3.
//
//      Take note that although at least one of the operands must be a
//      valid actor class in order for lazy evaluation to take effect,
//      if this is not the case and we still want to lazily evaluate an
//      expression, we can use var(x), val(x) or cref(x) to transform
//      the operand into a valid action object (see primitives.hpp).
//      Example:
//
//          val(1) << 3;
//
//      Supported operators:
//
//          Unary operators:
//
//              prefix:   ~, !, -, +, ++, --, & (reference), * (dereference)
//              postfix:  ++, --
//
//          Binary operators:
//
//              =, [], +=, -=, *=, /=, %=, &=, |=, ^=, <<=, >>=
//              +, -, *, /, %, &, |, ^, <<, >>
//              ==, !=, <, >, <=, >=
//              &&, ||
//
//      Each operator has a special tag type associated with it. For
//      example the binary + operator has a plus_op tag type associated
//      with it. This is used to specialize either the unary_operator or
//      binary_operator template classes (see unary_operator and
//      binary_operator below). Specializations of these unary_operator
//      and binary_operator are the actual workhorses that implement the
//      operations. The behavior of each lazy operator depends on these
//      unary_operator and binary_operator specializations. 'preset'
//      specializations conform to the canonical operator rules modeled
//      by the behavior of integers and pointers:
//
//          Prefix -, + and ~ accept constant arguments and return an
//          object by value.
//
//          The ! accept constant arguments and returns a boolean
//          result.
//
//          The & (address-of), * (dereference) both return a reference
//          to an object.
//
//          Prefix ++ returns a reference to its mutable argument after
//          it is incremented.
//
//          Postfix ++ returns the mutable argument by value before it
//          is incremented.
//
//          The += and its family accept mutable right hand side (rhs)
//          operand and return a reference to the rhs operand.
//
//          Infix + and its family accept constant arguments and return
//          an object by value.
//
//          The == and its family accept constant arguments and return a
//          boolean result.
//
//          Operators && and || accept constant arguments and return a
//          boolean result and are short circuit evaluated as expected.
//
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//
//  Operator tags
//
//      Each C++ operator has a corresponding tag type. This is
//      used as a means for specializing the unary_operator and
//      binary_operator (see below). The tag also serves as the
//      lazy operator type compatible as a composite operation
//      see (composite.hpp).
//
///////////////////////////////////////////////////////////////////////////////

//  Unary operator tags

struct negative_op;         struct positive_op;
struct logical_not_op;      struct invert_op;
struct reference_op;        struct dereference_op;
struct pre_incr_op;         struct pre_decr_op;
struct post_incr_op;        struct post_decr_op;

//  Binary operator tags

struct assign_op;           struct index_op;
struct plus_assign_op;      struct minus_assign_op;
struct times_assign_op;     struct divide_assign_op;    struct mod_assign_op;
struct and_assign_op;       struct or_assign_op;        struct xor_assign_op;
struct shift_l_assign_op;   struct shift_r_assign_op;

struct plus_op;             struct minus_op;
struct times_op;            struct divide_op;           struct mod_op;
struct and_op;              struct or_op;               struct xor_op;
struct shift_l_op;          struct shift_r_op;

struct eq_op;               struct not_eq_op;
struct lt_op;               struct lt_eq_op;
struct gt_op;               struct gt_eq_op;
struct logical_and_op;      struct logical_or_op;

///////////////////////////////////////////////////////////////////////////////
//
//  unary_operator<TagT, T>
//
//      The unary_operator class implements most of the C++ unary
//      operators. Each specialization is basically a simple static eval
//      function plus a result_type typedef that determines the return
//      type of the eval function.
//
//      TagT is one of the unary operator tags above and T is the data
//      type (argument) involved in the operation.
//
//      Only the behavior of C/C++ built-in types are taken into account
//      in the specializations provided below. For user-defined types,
//      these specializations may still be used provided that the
//      operator overloads of such types adhere to the standard behavior
//      of built-in types.
//
//      T1 separate special_ops.hpp file implements more stl savvy
//      specializations. Other more specialized unary_operator
//      implementations may be defined by the client for specific
//      unary operator tags/data types.
//
///////////////////////////////////////////////////////////////////////////////
template <typename TagT, typename T>
struct unary_operator;

//////////////////////////////////
template <typename T>
struct unary_operator<negative_op, T> {

    typedef T const result_type;
    static result_type eval(T const& v)
    { return -v; }
};

//////////////////////////////////
template <typename T>
struct unary_operator<positive_op, T> {

    typedef T const result_type;
    static result_type eval(T const& v)
    { return +v; }
};

//////////////////////////////////
template <typename T>
struct unary_operator<logical_not_op, T> {

    typedef T const result_type;
    static result_type eval(T const& v)
    { return !v; }
};

//////////////////////////////////
template <typename T>
struct unary_operator<invert_op, T> {

    typedef T const result_type;
    static result_type eval(T const& v)
    { return ~v; }
};

//////////////////////////////////
template <typename T>
struct unary_operator<reference_op, T> {

    typedef T* result_type;
    static result_type eval(T& v)
    { return &v; }
};

//////////////////////////////////
template <typename T>
struct unary_operator<dereference_op, T*> {

    typedef T& result_type;
    static result_type eval(T* v)
    { return *v; }
};

//////////////////////////////////
template <typename T>
struct unary_operator<dereference_op, T* const> {

    typedef T& result_type;
    static result_type eval(T* const v)
    { return *v; }
};

//////////////////////////////////
template <>
struct unary_operator<dereference_op, nil_t> {

    //  G++ eager template instantiation
    //  somehow requires this.
    typedef nil_t result_type;
};

//////////////////////////////////
#ifndef BOOST_BORLANDC
template <>
struct unary_operator<dereference_op, nil_t const> {

    //  G++ eager template instantiation
    //  somehow requires this.
    typedef nil_t result_type;
};
#endif

//////////////////////////////////
template <typename T>
struct unary_operator<pre_incr_op, T> {

    typedef T& result_type;
    static result_type eval(T& v)
    { return ++v; }
};

//////////////////////////////////
template <typename T>
struct unary_operator<pre_decr_op, T> {

    typedef T& result_type;
    static result_type eval(T& v)
    { return --v; }
};

//////////////////////////////////
template <typename T>
struct unary_operator<post_incr_op, T> {

    typedef T const result_type;
    static result_type eval(T& v)
    { T t(v); ++v; return t; }
};

//////////////////////////////////
template <typename T>
struct unary_operator<post_decr_op, T> {

    typedef T const result_type;
    static result_type eval(T& v)
    { T t(v); --v; return t; }
};

///////////////////////////////////////////////////////////////////////////////
//
//  rank<T>
//
//      rank<T> class has a static int constant 'value' that defines the
//      absolute rank of a type. rank<T> is used to choose the result
//      type of binary operators such as +. The type with the higher
//      rank wins and is used as the operator's return type. T1 generic
//      user defined type has a very high rank and always wins when
//      compared against a user defined type. If this is not desirable,
//      one can write a rank specialization for the type.
//
//      Take note that ranks 0..9999 are reserved for the framework.
//
///////////////////////////////////////////////////////////////////////////////
template <typename T>
struct rank { static int const value = INT_MAX; };

template <> struct rank<void>               { static int const value = 0; };
template <> struct rank<bool>               { static int const value = 10; };

template <> struct rank<char>               { static int const value = 20; };
template <> struct rank<signed char>        { static int const value = 20; };
template <> struct rank<unsigned char>      { static int const value = 30; };
#if !defined(BOOST_NO_INTRINSIC_WCHAR_T)
template <> struct rank<wchar_t>            { static int const value = 40; };
#endif // !defined(BOOST_NO_INTRINSIC_WCHAR_T)

template <> struct rank<short>              { static int const value = 50; };
template <> struct rank<unsigned short>     { static int const value = 60; };

template <> struct rank<int>                { static int const value = 70; };
template <> struct rank<unsigned int>       { static int const value = 80; };

template <> struct rank<long>               { static int const value = 90; };
template <> struct rank<unsigned long>      { static int const value = 100; };

#ifdef BOOST_HAS_LONG_LONG
template <> struct rank< ::boost::long_long_type>          { static int const value = 110; };
template <> struct rank< ::boost::ulong_long_type> { static int const value = 120; };
#endif

template <> struct rank<float>              { static int const value = 130; };
template <> struct rank<double>             { static int const value = 140; };
template <> struct rank<long double>        { static int const value = 150; };

template <typename T> struct rank<T*>
{ static int const value = 160; };

template <typename T> struct rank<T* const>
{ static int const value = 160; };

template <typename T, int N> struct rank<T[N]>
{ static int const value = 160; };

///////////////////////////////////////////////////////////////////////////////
//
//  higher_rank<T0, T1>
//
//      Chooses the type (T0 or T1) with the higher rank.
//
///////////////////////////////////////////////////////////////////////////////
template <typename T0, typename T1>
struct higher_rank {
    typedef typename boost::mpl::if_c<
        rank<T0>::value < rank<T1>::value,
        T1, T0>::type type;
};

///////////////////////////////////////////////////////////////////////////////
//
//  binary_operator<TagT, T0, T1>
//
//      The binary_operator class implements most of the C++ binary
//      operators. Each specialization is basically a simple static eval
//      function plus a result_type typedef that determines the return
//      type of the eval function.
//
//      TagT is one of the binary operator tags above T0 and T1 are the
//      (arguments') data types involved in the operation.
//
//      Only the behavior of C/C++ built-in types are taken into account
//      in the specializations provided below. For user-defined types,
//      these specializations may still be used provided that the
//      operator overloads of such types adhere to the standard behavior
//      of built-in types.
//
//      T1 separate special_ops.hpp file implements more stl savvy
//      specializations. Other more specialized unary_operator
//      implementations may be defined by the client for specific
//      unary operator tags/data types.
//
//      All binary_operator except the logical_and_op and logical_or_op
//      have an eval static function that carries out the actual operation.
//      The logical_and_op and logical_or_op d are special because these
//      two operators are short-circuit evaluated.
//
///////////////////////////////////////////////////////////////////////////////
template <typename TagT, typename T0, typename T1>
struct binary_operator;

//////////////////////////////////
template <typename T0, typename T1>
struct binary_operator<assign_op, T0, T1> {

    typedef T0& result_type;
    static result_type eval(T0& lhs, T1 const& rhs)
    { return lhs = rhs; }
};

//////////////////////////////////
template <typename T1>
struct binary_operator<index_op, nil_t, T1> {

    //  G++ eager template instantiation
    //  somehow requires this.
    typedef nil_t result_type;
};

//////////////////////////////////
template <typename T0, typename T1>
struct binary_operator<index_op, T0*, T1> {

    typedef T0& result_type;
    static result_type eval(T0* ptr, T1 const& index)
    { return ptr[index]; }
};

//////////////////////////////////
template <typename T0, typename T1>
struct binary_operator<index_op, T0* const, T1> {

    typedef T0& result_type;
    static result_type eval(T0* const ptr, T1 const& index)
    { return ptr[index]; }
};

//////////////////////////////////
template <typename T0, int N, typename T1>
struct binary_operator<index_op, T0[N], T1> {

    typedef T0& result_type;
    static result_type eval(T0* ptr, T1 const& index)
    { return ptr[index]; }
};

//////////////////////////////////
template <typename T0, typename T1>
struct binary_operator<plus_assign_op, T0, T1> {

    typedef T0& result_type;
    static result_type eval(T0& lhs, T1 const& rhs)
    { return lhs += rhs; }
};

//////////////////////////////////
template <typename T0, typename T1>
struct binary_operator<minus_assign_op, T0, T1> {

    typedef T0& result_type;
    static result_type eval(T0& lhs, T1 const& rhs)
    { return lhs -= rhs; }
};

//////////////////////////////////
template <typename T0, typename T1>
struct binary_operator<times_assign_op, T0, T1> {

    typedef T0& result_type;
    static result_type eval(T0& lhs, T1 const& rhs)
    { return lhs *= rhs; }
};

//////////////////////////////////
template <typename T0, typename T1>
struct binary_operator<divide_assign_op, T0, T1> {

    typedef T0& result_type;
    static result_type eval(T0& lhs, T1 const& rhs)
    { return lhs /= rhs; }
};

//////////////////////////////////
template <typename T0, typename T1>
struct binary_operator<mod_assign_op, T0, T1> {

    typedef T0& result_type;
    static result_type eval(T0& lhs, T1 const& rhs)
    { return lhs %= rhs; }
};

//////////////////////////////////
template <typename T0, typename T1>
struct binary_operator<and_assign_op, T0, T1> {

    typedef T0& result_type;
    static result_type eval(T0& lhs, T1 const& rhs)
    { return lhs &= rhs; }
};

//////////////////////////////////
template <typename T0, typename T1>
struct binary_operator<or_assign_op, T0, T1> {

    typedef T0& result_type;
    static result_type eval(T0& lhs, T1 const& rhs)
    { return lhs |= rhs; }
};

//////////////////////////////////
template <typename T0, typename T1>
struct binary_operator<xor_assign_op, T0, T1> {

    typedef T0& result_type;
    static result_type eval(T0& lhs, T1 const& rhs)
    { return lhs ^= rhs; }
};

//////////////////////////////////
template <typename T0, typename T1>
struct binary_operator<shift_l_assign_op, T0, T1> {

    typedef T0& result_type;
    static result_type eval(T0& lhs, T1 const& rhs)
    { return lhs <<= rhs; }
};

//////////////////////////////////
template <typename T0, typename T1>
struct binary_operator<shift_r_assign_op, T0, T1> {

    typedef T0& result_type;
    static result_type eval(T0& lhs, T1 const& rhs)
    { return lhs >>= rhs; }
};

//////////////////////////////////
template <typename T0, typename T1>
struct binary_operator<plus_op, T0, T1> {

    typedef typename higher_rank<T0, T1>::type const result_type;
    static result_type eval(T0 const& lhs, T1 const& rhs)
    { return lhs + rhs; }
};

//////////////////////////////////
template <typename T0, typename T1>
struct binary_operator<minus_op, T0, T1> {

    typedef typename higher_rank<T0, T1>::type const result_type;
    static result_type eval(T0 const& lhs, T1 const& rhs)
    { return lhs - rhs; }
};

//////////////////////////////////
template <typename T0, typename T1>
struct binary_operator<times_op, T0, T1> {

    typedef typename higher_rank<T0, T1>::type const result_type;
    static result_type eval(T0 const& lhs, T1 const& rhs)
    { return lhs * rhs; }
};

//////////////////////////////////
template <typename T0, typename T1>
struct binary_operator<divide_op, T0, T1> {

    typedef typename higher_rank<T0, T1>::type const result_type;
    static result_type eval(T0 const& lhs, T1 const& rhs)
    { return lhs / rhs; }
};

//////////////////////////////////
template <typename T0, typename T1>
struct binary_operator<mod_op, T0, T1> {

    typedef typename higher_rank<T0, T1>::type const result_type;
    static result_type eval(T0 const& lhs, T1 const& rhs)
    { return lhs % rhs; }
};

//////////////////////////////////
template <typename T0, typename T1>
struct binary_operator<and_op, T0, T1> {

    typedef typename higher_rank<T0, T1>::type const result_type;
    static result_type eval(T0 const& lhs, T1 const& rhs)
    { return lhs & rhs; }
};

//////////////////////////////////
template <typename T0, typename T1>
struct binary_operator<or_op, T0, T1> {

    typedef typename higher_rank<T0, T1>::type const result_type;
    static result_type eval(T0 const& lhs, T1 const& rhs)
    { return lhs | rhs; }
};

//////////////////////////////////
template <typename T0, typename T1>
struct binary_operator<xor_op, T0, T1> {

    typedef typename higher_rank<T0, T1>::type const result_type;
    static result_type eval(T0 const& lhs, T1 const& rhs)
    { return lhs ^ rhs; }
};

//////////////////////////////////
template <typename T0, typename T1>
struct binary_operator<shift_l_op, T0, T1> {

    typedef T0 const result_type;
    static result_type eval(T0 const& lhs, T1 const& rhs)
    { return lhs << rhs; }
};

//////////////////////////////////
template <typename T0, typename T1>
struct binary_operator<shift_r_op, T0, T1> {

    typedef T0 const result_type;
    static result_type eval(T0 const& lhs, T1 const& rhs)
    { return lhs >> rhs; }
};

//////////////////////////////////
template <typename T0, typename T1>
struct binary_operator<eq_op, T0, T1> {

    typedef bool result_type;
    static result_type eval(T0 const& lhs, T1 const& rhs)
    { return lhs == rhs; }
};

//////////////////////////////////
template <typename T0, typename T1>
struct binary_operator<not_eq_op, T0, T1> {

    typedef bool result_type;
    static result_type eval(T0 const& lhs, T1 const& rhs)
    { return lhs != rhs; }
};

//////////////////////////////////
template <typename T0, typename T1>
struct binary_operator<lt_op, T0, T1> {

    typedef bool result_type;
    static result_type eval(T0 const& lhs, T1 const& rhs)
    { return lhs < rhs; }
};

//////////////////////////////////
template <typename T0, typename T1>
struct binary_operator<lt_eq_op, T0, T1> {

    typedef bool result_type;
    static result_type eval(T0 const& lhs, T1 const& rhs)
    { return lhs <= rhs; }
};

//////////////////////////////////
template <typename T0, typename T1>
struct binary_operator<gt_op, T0, T1> {

    typedef bool result_type;
    static result_type eval(T0 const& lhs, T1 const& rhs)
    { return lhs > rhs; }
};

//////////////////////////////////
template <typename T0, typename T1>
struct binary_operator<gt_eq_op, T0, T1> {

    typedef bool result_type;
    static result_type eval(T0 const& lhs, T1 const& rhs)
    { return lhs >= rhs; }
};

//////////////////////////////////
template <typename T0, typename T1>
struct binary_operator<logical_and_op, T0, T1> {

    typedef bool result_type;
    //  no eval function, see comment above.
};

//////////////////////////////////
template <typename T0, typename T1>
struct binary_operator<logical_or_op, T0, T1> {

    typedef bool result_type;
    //  no eval function, see comment above.
};

///////////////////////////////////////////////////////////////////////////////
//
//  negative lazy operator (prefix -)
//
///////////////////////////////////////////////////////////////////////////////
struct negative_op {

    template <typename T0>
    struct result {

        typedef typename unary_operator<negative_op, T0>::result_type type;
    };

    template <typename T0>
    typename unary_operator<negative_op, T0>::result_type
    operator()(T0& _0) const
    { return unary_operator<negative_op, T0>::eval(_0); }
};

//////////////////////////////////
template <typename BaseT>
inline typename impl::make_unary<negative_op, BaseT>::type
operator-(actor<BaseT> const& _0)
{
    return impl::make_unary<negative_op, BaseT>::construct(_0);
}

///////////////////////////////////////////////////////////////////////////////
//
//  positive lazy operator (prefix +)
//
///////////////////////////////////////////////////////////////////////////////
struct positive_op {

    template <typename T0>
    struct result {

        typedef typename unary_operator<positive_op, T0>::result_type type;
    };

    template <typename T0>
    typename unary_operator<positive_op, T0>::result_type
    operator()(T0& _0) const
    { return unary_operator<positive_op, T0>::eval(_0); }
};

//////////////////////////////////
template <typename BaseT>
inline typename impl::make_unary<positive_op, BaseT>::type
operator+(actor<BaseT> const& _0)
{
    return impl::make_unary<positive_op, BaseT>::construct(_0);
}

///////////////////////////////////////////////////////////////////////////////
//
//  logical not lazy operator (prefix !)
//
///////////////////////////////////////////////////////////////////////////////
struct logical_not_op {

    template <typename T0>
    struct result {

        typedef typename unary_operator<logical_not_op, T0>::result_type type;
    };

    template <typename T0>
    typename unary_operator<logical_not_op, T0>::result_type
    operator()(T0& _0) const
    { return unary_operator<logical_not_op, T0>::eval(_0); }
};

//////////////////////////////////
template <typename BaseT>
inline typename impl::make_unary<logical_not_op, BaseT>::type
operator!(actor<BaseT> const& _0)
{
    return impl::make_unary<logical_not_op, BaseT>::construct(_0);
}

///////////////////////////////////////////////////////////////////////////////
//
//  invert lazy operator (prefix ~)
//
///////////////////////////////////////////////////////////////////////////////
struct invert_op {

    template <typename T0>
    struct result {

        typedef typename unary_operator<invert_op, T0>::result_type type;
    };

    template <typename T0>
    typename unary_operator<invert_op, T0>::result_type
    operator()(T0& _0) const
    { return unary_operator<invert_op, T0>::eval(_0); }
};

//////////////////////////////////
template <typename BaseT>
inline typename impl::make_unary<invert_op, BaseT>::type
operator~(actor<BaseT> const& _0)
{
    return impl::make_unary<invert_op, BaseT>::construct(_0);
}

///////////////////////////////////////////////////////////////////////////////
//
//  reference lazy operator (prefix &)
//
///////////////////////////////////////////////////////////////////////////////
struct reference_op {

    template <typename T0>
    struct result {

        typedef typename unary_operator<reference_op, T0>::result_type type;
    };

    template <typename T0>
    typename unary_operator<reference_op, T0>::result_type
    operator()(T0& _0) const
    { return unary_operator<reference_op, T0>::eval(_0); }
};

//////////////////////////////////
template <typename BaseT>
inline typename impl::make_unary<reference_op, BaseT>::type
operator&(actor<BaseT> const& _0)
{
    return impl::make_unary<reference_op, BaseT>::construct(_0);
}

///////////////////////////////////////////////////////////////////////////////
//
//  dereference lazy operator (prefix *)
//
///////////////////////////////////////////////////////////////////////////////
struct dereference_op {

    template <typename T0>
    struct result {

        typedef typename unary_operator<dereference_op, T0>::result_type type;
    };

    template <typename T0>
    typename unary_operator<dereference_op, T0>::result_type
    operator()(T0& _0) const
    { return unary_operator<dereference_op, T0>::eval(_0); }
};

//////////////////////////////////
template <typename BaseT>
inline typename impl::make_unary<dereference_op, BaseT>::type
operator*(actor<BaseT> const& _0)
{
    return impl::make_unary<dereference_op, BaseT>::construct(_0);
}

///////////////////////////////////////////////////////////////////////////////
//
//  pre increment lazy operator (prefix ++)
//
///////////////////////////////////////////////////////////////////////////////
struct pre_incr_op {

    template <typename T0>
    struct result {

        typedef typename unary_operator<pre_incr_op, T0>::result_type type;
    };

    template <typename T0>
    typename unary_operator<pre_incr_op, T0>::result_type
    operator()(T0& _0) const
    { return unary_operator<pre_incr_op, T0>::eval(_0); }
};

//////////////////////////////////
template <typename BaseT>
inline typename impl::make_unary<pre_incr_op, BaseT>::type
operator++(actor<BaseT> const& _0)
{
    return impl::make_unary<pre_incr_op, BaseT>::construct(_0);
}

///////////////////////////////////////////////////////////////////////////////
//
//  pre decrement lazy operator (prefix --)
//
///////////////////////////////////////////////////////////////////////////////
struct pre_decr_op {

    template <typename T0>
    struct result {

        typedef typename unary_operator<pre_decr_op, T0>::result_type type;
    };

    template <typename T0>
    typename unary_operator<pre_decr_op, T0>::result_type
    operator()(T0& _0) const
    { return unary_operator<pre_decr_op, T0>::eval(_0); }
};

//////////////////////////////////
template <typename BaseT>
inline typename impl::make_unary<pre_decr_op, BaseT>::type
operator--(actor<BaseT> const& _0)
{
    return impl::make_unary<pre_decr_op, BaseT>::construct(_0);
}

///////////////////////////////////////////////////////////////////////////////
//
//  post increment lazy operator (postfix ++)
//
///////////////////////////////////////////////////////////////////////////////
struct post_incr_op {

    template <typename T0>
    struct result {

        typedef typename unary_operator<post_incr_op, T0>::result_type type;
    };

    template <typename T0>
    typename unary_operator<post_incr_op, T0>::result_type
    operator()(T0& _0) const
    { return unary_operator<post_incr_op, T0>::eval(_0); }
};

//////////////////////////////////
template <typename BaseT>
inline typename impl::make_unary<post_incr_op, BaseT>::type
operator++(actor<BaseT> const& _0, int)
{
    return impl::make_unary<post_incr_op, BaseT>::construct(_0);
}

///////////////////////////////////////////////////////////////////////////////
//
//  post decrement lazy operator (postfix --)
//
///////////////////////////////////////////////////////////////////////////////
struct post_decr_op {

    template <typename T0>
    struct result {

        typedef typename unary_operator<post_decr_op, T0>::result_type type;
    };

    template <typename T0>
    typename unary_operator<post_decr_op, T0>::result_type
    operator()(T0& _0) const
    { return unary_operator<post_decr_op, T0>::eval(_0); }
};

//////////////////////////////////
template <typename BaseT>
inline typename impl::make_unary<post_decr_op, BaseT>::type
operator--(actor<BaseT> const& _0, int)
{
    return impl::make_unary<post_decr_op, BaseT>::construct(_0);
}

///////////////////////////////////////////////////////////////////////////////
//
//  assignment lazy operator (infix =)
//  The acual lazy operator is a member of the actor class.
//
///////////////////////////////////////////////////////////////////////////////
struct assign_op {

    template <typename T0, typename T1>
    struct result {

        typedef typename binary_operator<assign_op, T0, T1>
            ::result_type type;
    };

    template <typename T0, typename T1>
    typename binary_operator<assign_op, T0, T1>::result_type
    operator()(T0& _0, T1& _1) const
    { return binary_operator<assign_op, T0, T1>::eval(_0, _1); }
};

//////////////////////////////////
template <typename BaseT>
template <typename B>
inline typename impl::make_binary1<assign_op, BaseT, B>::type
actor<BaseT>::operator=(B const& _1) const
{
    return impl::make_binary1<assign_op, BaseT, B>::construct(*this, _1);
}

///////////////////////////////////////////////////////////////////////////////
//
//  index lazy operator (array index [])
//  The acual lazy operator is a member of the actor class.
//
///////////////////////////////////////////////////////////////////////////////
struct index_op {

    template <typename T0, typename T1>
    struct result {

        typedef typename binary_operator<index_op, T0, T1>
            ::result_type type;
    };

    template <typename T0, typename T1>
    typename binary_operator<index_op, T0, T1>::result_type
    operator()(T0& _0, T1& _1) const
    { return binary_operator<index_op, T0, T1>::eval(_0, _1); }
};

//////////////////////////////////
template <typename BaseT>
template <typename B>
inline typename impl::make_binary1<index_op, BaseT, B>::type
actor<BaseT>::operator[](B const& _1) const
{
    return impl::make_binary1<index_op, BaseT, B>::construct(*this, _1);
}

///////////////////////////////////////////////////////////////////////////////
//
//  plus assign lazy operator (infix +=)
//
///////////////////////////////////////////////////////////////////////////////
struct plus_assign_op {

    template <typename T0, typename T1>
    struct result {

        typedef typename binary_operator<plus_assign_op, T0, T1>
            ::result_type type;
    };

    template <typename T0, typename T1>
    typename binary_operator<plus_assign_op, T0, T1>::result_type
    operator()(T0& _0, T1& _1) const
    { return binary_operator<plus_assign_op, T0, T1>::eval(_0, _1); }
};

//////////////////////////////////
template <typename BaseT, typename T1>
inline typename impl::make_binary1<plus_assign_op, BaseT, T1>::type
operator+=(actor<BaseT> const& _0, T1 CREF _1)
{
    return impl::make_binary1<plus_assign_op, BaseT, T1>::construct(_0, _1);
}

///////////////////////////////////////////////////////////////////////////////
//
//  minus assign lazy operator (infix -=)
//
///////////////////////////////////////////////////////////////////////////////
struct minus_assign_op {

    template <typename T0, typename T1>
    struct result {

        typedef typename binary_operator<minus_assign_op, T0, T1>
            ::result_type type;
    };

    template <typename T0, typename T1>
    typename binary_operator<minus_assign_op, T0, T1>::result_type
    operator()(T0& _0, T1& _1) const
    { return binary_operator<minus_assign_op, T0, T1>::eval(_0, _1); }
};

//////////////////////////////////
template <typename BaseT, typename T1>
inline typename impl::make_binary1<minus_assign_op, BaseT, T1>::type
operator-=(actor<BaseT> const& _0, T1 CREF _1)
{
    return impl::make_binary1<minus_assign_op, BaseT, T1>::construct(_0, _1);
}

///////////////////////////////////////////////////////////////////////////////
//
//  times assign lazy operator (infix *=)
//
///////////////////////////////////////////////////////////////////////////////
struct times_assign_op {

    template <typename T0, typename T1>
    struct result {

        typedef typename binary_operator<times_assign_op, T0, T1>
            ::result_type type;
    };

    template <typename T0, typename T1>
    typename binary_operator<times_assign_op, T0, T1>::result_type
    operator()(T0& _0, T1& _1) const
    { return binary_operator<times_assign_op, T0, T1>::eval(_0, _1); }
};

//////////////////////////////////
template <typename BaseT, typename T1>
inline typename impl::make_binary1<times_assign_op, BaseT, T1>::type
operator*=(actor<BaseT> const& _0, T1 CREF _1)
{
    return impl::make_binary1<times_assign_op, BaseT, T1>::construct(_0, _1);
}

///////////////////////////////////////////////////////////////////////////////
//
//  divide assign lazy operator (infix /=)
//
///////////////////////////////////////////////////////////////////////////////
struct divide_assign_op {

    template <typename T0, typename T1>
    struct result {

        typedef typename binary_operator<divide_assign_op, T0, T1>
            ::result_type type;
    };

    template <typename T0, typename T1>
    typename binary_operator<divide_assign_op, T0, T1>::result_type
    operator()(T0& _0, T1& _1) const
    { return binary_operator<divide_assign_op, T0, T1>::eval(_0, _1); }
};

//////////////////////////////////
template <typename BaseT, typename T1>
inline typename impl::make_binary1<divide_assign_op, BaseT, T1>::type
operator/=(actor<BaseT> const& _0, T1 CREF _1)
{
    return impl::make_binary1<divide_assign_op, BaseT, T1>::construct(_0, _1);
}

///////////////////////////////////////////////////////////////////////////////
//
//  mod assign lazy operator (infix %=)
//
///////////////////////////////////////////////////////////////////////////////
struct mod_assign_op {

    template <typename T0, typename T1>
    struct result {

        typedef typename binary_operator<mod_assign_op, T0, T1>
            ::result_type type;
    };

    template <typename T0, typename T1>
    typename binary_operator<mod_assign_op, T0, T1>::result_type
    operator()(T0& _0, T1& _1) const
    { return binary_operator<mod_assign_op, T0, T1>::eval(_0, _1); }
};

//////////////////////////////////
template <typename BaseT, typename T1>
inline typename impl::make_binary1<mod_assign_op, BaseT, T1>::type
operator%=(actor<BaseT> const& _0, T1 CREF _1)
{
    return impl::make_binary1<mod_assign_op, BaseT, T1>::construct(_0, _1);
}

///////////////////////////////////////////////////////////////////////////////
//
//  and assign lazy operator (infix &=)
//
///////////////////////////////////////////////////////////////////////////////
struct and_assign_op {

    template <typename T0, typename T1>
    struct result {

        typedef typename binary_operator<and_assign_op, T0, T1>
            ::result_type type;
    };

    template <typename T0, typename T1>
    typename binary_operator<and_assign_op, T0, T1>::result_type
    operator()(T0& _0, T1& _1) const
    { return binary_operator<and_assign_op, T0, T1>::eval(_0, _1); }
};

//////////////////////////////////
template <typename BaseT, typename T1>
inline typename impl::make_binary1<and_assign_op, BaseT, T1>::type
operator&=(actor<BaseT> const& _0, T1 CREF _1)
{
    return impl::make_binary1<and_assign_op, BaseT, T1>::construct(_0, _1);
}

///////////////////////////////////////////////////////////////////////////////
//
//  or assign lazy operator (infix |=)
//
///////////////////////////////////////////////////////////////////////////////
struct or_assign_op {

    template <typename T0, typename T1>
    struct result {

        typedef typename binary_operator<or_assign_op, T0, T1>
            ::result_type type;
    };

    template <typename T0, typename T1>
    typename binary_operator<or_assign_op, T0, T1>::result_type
    operator()(T0& _0, T1& _1) const
    { return binary_operator<or_assign_op, T0, T1>::eval(_0, _1); }
};

//////////////////////////////////
template <typename BaseT, typename T1>
inline typename impl::make_binary1<or_assign_op, BaseT, T1>::type
operator|=(actor<BaseT> const& _0, T1 CREF _1)
{
    return impl::make_binary1<or_assign_op, BaseT, T1>::construct(_0, _1);
}

///////////////////////////////////////////////////////////////////////////////
//
//  xor assign lazy operator (infix ^=)
//
///////////////////////////////////////////////////////////////////////////////
struct xor_assign_op {

    template <typename T0, typename T1>
    struct result {

        typedef typename binary_operator<xor_assign_op, T0, T1>
            ::result_type type;
    };

    template <typename T0, typename T1>
    typename binary_operator<xor_assign_op, T0, T1>::result_type
    operator()(T0& _0, T1& _1) const
    { return binary_operator<xor_assign_op, T0, T1>::eval(_0, _1); }
};

//////////////////////////////////
template <typename BaseT, typename T1>
inline typename impl::make_binary1<xor_assign_op, BaseT, T1>::type
operator^=(actor<BaseT> const& _0, T1 CREF _1)
{
    return impl::make_binary1<xor_assign_op, BaseT, T1>::construct(_0, _1);
}

///////////////////////////////////////////////////////////////////////////////
//
//  shift left assign lazy operator (infix <<=)
//
///////////////////////////////////////////////////////////////////////////////
struct shift_l_assign_op {

    template <typename T0, typename T1>
    struct result {

        typedef typename binary_operator<shift_l_assign_op, T0, T1>
            ::result_type type;
    };

    template <typename T0, typename T1>
    typename binary_operator<shift_l_assign_op, T0, T1>::result_type
    operator()(T0& _0, T1& _1) const
    { return binary_operator<shift_l_assign_op, T0, T1>::eval(_0, _1); }
};

//////////////////////////////////
template <typename BaseT, typename T1>
inline typename impl::make_binary1<shift_l_assign_op, BaseT, T1>::type
operator<<=(actor<BaseT> const& _0, T1 CREF _1)
{
    return impl::make_binary1<shift_l_assign_op, BaseT, T1>::construct(_0, _1);
}

///////////////////////////////////////////////////////////////////////////////
//
//  shift right assign lazy operator (infix >>=)
//
///////////////////////////////////////////////////////////////////////////////
struct shift_r_assign_op {

    template <typename T0, typename T1>
    struct result {

        typedef typename binary_operator<shift_r_assign_op, T0, T1>
            ::result_type type;
    };

    template <typename T0, typename T1>
    typename binary_operator<shift_r_assign_op, T0, T1>::result_type
    operator()(T0& _0, T1& _1) const
    { return binary_operator<shift_r_assign_op, T0, T1>::eval(_0, _1); }
};

//////////////////////////////////
template <typename BaseT, typename T1>
inline typename impl::make_binary1<shift_r_assign_op, BaseT, T1>::type
operator>>=(actor<BaseT> const& _0, T1 CREF _1)
{
    return impl::make_binary1<shift_r_assign_op, BaseT, T1>::construct(_0, _1);
}

///////////////////////////////////////////////////////////////////////////////
//
//  plus lazy operator (infix +)
//
///////////////////////////////////////////////////////////////////////////////
struct plus_op {

    template <typename T0, typename T1>
    struct result {

        typedef typename binary_operator<plus_op, T0, T1>
            ::result_type type;
    };

    template <typename T0, typename T1>
    typename binary_operator<plus_op, T0, T1>::result_type
    operator()(T0& _0, T1& _1) const
    { return binary_operator<plus_op, T0, T1>::eval(_0, _1); }
};

//////////////////////////////////
template <typename BaseT, typename T1>
inline typename impl::make_binary1<plus_op, BaseT, T1>::type
operator+(actor<BaseT> const& _0, T1 CREF _1)
{
    return impl::make_binary1<plus_op, BaseT, T1>::construct(_0, _1);
}

//////////////////////////////////
template <typename T0, typename BaseT>
inline typename impl::make_binary2<plus_op, T0, BaseT>::type
operator+(T0 CREF _0, actor<BaseT> const& _1)
{
    return impl::make_binary2<plus_op, T0, BaseT>::construct(_0, _1);
}

//////////////////////////////////
template <typename BaseT0, typename BaseT1>
inline typename impl::make_binary3<plus_op, BaseT0, BaseT1>::type
operator+(actor<BaseT0> const& _0, actor<BaseT1> const& _1)
{
    return impl::make_binary3<plus_op, BaseT0, BaseT1>::construct(_0, _1);
}

///////////////////////////////////////////////////////////////////////////////
//
//  minus lazy operator (infix -)
//
///////////////////////////////////////////////////////////////////////////////
struct minus_op {

    template <typename T0, typename T1>
    struct result {

        typedef typename binary_operator<minus_op, T0, T1>
            ::result_type type;
    };

    template <typename T0, typename T1>
    typename binary_operator<minus_op, T0, T1>::result_type
    operator()(T0& _0, T1& _1) const
    { return binary_operator<minus_op, T0, T1>::eval(_0, _1); }
};

//////////////////////////////////
template <typename BaseT, typename T1>
inline typename impl::make_binary1<minus_op, BaseT, T1>::type
operator-(actor<BaseT> const& _0, T1 CREF _1)
{
    return impl::make_binary1<minus_op, BaseT, T1>::construct(_0, _1);
}

//////////////////////////////////
template <typename T0, typename BaseT>
inline typename impl::make_binary2<minus_op, T0, BaseT>::type
operator-(T0 CREF _0, actor<BaseT> const& _1)
{
    return impl::make_binary2<minus_op, T0, BaseT>::construct(_0, _1);
}

//////////////////////////////////
template <typename BaseT0, typename BaseT1>
inline typename impl::make_binary3<minus_op, BaseT0, BaseT1>::type
operator-(actor<BaseT0> const& _0, actor<BaseT1> const& _1)
{
    return impl::make_binary3<minus_op, BaseT0, BaseT1>::construct(_0, _1);
}

///////////////////////////////////////////////////////////////////////////////
//
//  times lazy operator (infix *)
//
///////////////////////////////////////////////////////////////////////////////
struct times_op {

    template <typename T0, typename T1>
    struct result {

        typedef typename binary_operator<times_op, T0, T1>
            ::result_type type;
    };

    template <typename T0, typename T1>
    typename binary_operator<times_op, T0, T1>::result_type
    operator()(T0& _0, T1& _1) const
    { return binary_operator<times_op, T0, T1>::eval(_0, _1); }
};

//////////////////////////////////
template <typename BaseT, typename T1>
inline typename impl::make_binary1<times_op, BaseT, T1>::type
operator*(actor<BaseT> const& _0, T1 CREF _1)
{
    return impl::make_binary1<times_op, BaseT, T1>::construct(_0, _1);
}

//////////////////////////////////
template <typename T0, typename BaseT>
inline typename impl::make_binary2<times_op, T0, BaseT>::type
operator*(T0 CREF _0, actor<BaseT> const& _1)
{
    return impl::make_binary2<times_op, T0, BaseT>::construct(_0, _1);
}

//////////////////////////////////
template <typename BaseT0, typename BaseT1>
inline typename impl::make_binary3<times_op, BaseT0, BaseT1>::type
operator*(actor<BaseT0> const& _0, actor<BaseT1> const& _1)
{
    return impl::make_binary3<times_op, BaseT0, BaseT1>::construct(_0, _1);
}

///////////////////////////////////////////////////////////////////////////////
//
//  divide lazy operator (infix /)
//
///////////////////////////////////////////////////////////////////////////////
struct divide_op {

    template <typename T0, typename T1>
    struct result {

        typedef typename binary_operator<divide_op, T0, T1>
            ::result_type type;
    };

    template <typename T0, typename T1>
    typename binary_operator<divide_op, T0, T1>::result_type
    operator()(T0& _0, T1& _1) const
    { return binary_operator<divide_op, T0, T1>::eval(_0, _1); }
};

//////////////////////////////////
template <typename BaseT, typename T1>
inline typename impl::make_binary1<divide_op, BaseT, T1>::type
operator/(actor<BaseT> const& _0, T1 CREF _1)
{
    return impl::make_binary1<divide_op, BaseT, T1>::construct(_0, _1);
}

//////////////////////////////////
template <typename T0, typename BaseT>
inline typename impl::make_binary2<divide_op, T0, BaseT>::type
operator/(T0 CREF _0, actor<BaseT> const& _1)
{
    return impl::make_binary2<divide_op, T0, BaseT>::construct(_0, _1);
}

//////////////////////////////////
template <typename BaseT0, typename BaseT1>
inline typename impl::make_binary3<divide_op, BaseT0, BaseT1>::type
operator/(actor<BaseT0> const& _0, actor<BaseT1> const& _1)
{
    return impl::make_binary3<divide_op, BaseT0, BaseT1>::construct(_0, _1);
}

///////////////////////////////////////////////////////////////////////////////
//
//  mod lazy operator (infix %)
//
///////////////////////////////////////////////////////////////////////////////
struct mod_op {

    template <typename T0, typename T1>
    struct result {

        typedef typename binary_operator<mod_op, T0, T1>
            ::result_type type;
    };

    template <typename T0, typename T1>
    typename binary_operator<mod_op, T0, T1>::result_type
    operator()(T0& _0, T1& _1) const
    { return binary_operator<mod_op, T0, T1>::eval(_0, _1); }
};

//////////////////////////////////
template <typename BaseT, typename T1>
inline typename impl::make_binary1<mod_op, BaseT, T1>::type
operator%(actor<BaseT> const& _0, T1 CREF _1)
{
    return impl::make_binary1<mod_op, BaseT, T1>::construct(_0, _1);
}

//////////////////////////////////
template <typename T0, typename BaseT>
inline typename impl::make_binary2<mod_op, T0, BaseT>::type
operator%(T0 CREF _0, actor<BaseT> const& _1)
{
    return impl::make_binary2<mod_op, T0, BaseT>::construct(_0, _1);
}

//////////////////////////////////
template <typename BaseT0, typename BaseT1>
inline typename impl::make_binary3<mod_op, BaseT0, BaseT1>::type
operator%(actor<BaseT0> const& _0, actor<BaseT1> const& _1)
{
    return impl::make_binary3<mod_op, BaseT0, BaseT1>::construct(_0, _1);
}

///////////////////////////////////////////////////////////////////////////////
//
//  and lazy operator (infix &)
//
///////////////////////////////////////////////////////////////////////////////
struct and_op {

    template <typename T0, typename T1>
    struct result {

        typedef typename binary_operator<and_op, T0, T1>
            ::result_type type;
    };

    template <typename T0, typename T1>
    typename binary_operator<and_op, T0, T1>::result_type
    operator()(T0& _0, T1& _1) const
    { return binary_operator<and_op, T0, T1>::eval(_0, _1); }
};

//////////////////////////////////
template <typename BaseT, typename T1>
inline typename impl::make_binary1<and_op, BaseT, T1>::type
operator&(actor<BaseT> const& _0, T1 CREF _1)
{
    return impl::make_binary1<and_op, BaseT, T1>::construct(_0, _1);
}

//////////////////////////////////
template <typename T0, typename BaseT>
inline typename impl::make_binary2<and_op, T0, BaseT>::type
operator&(T0 CREF _0, actor<BaseT> const& _1)
{
    return impl::make_binary2<and_op, T0, BaseT>::construct(_0, _1);
}

//////////////////////////////////
template <typename BaseT0, typename BaseT1>
inline typename impl::make_binary3<and_op, BaseT0, BaseT1>::type
operator&(actor<BaseT0> const& _0, actor<BaseT1> const& _1)
{
    return impl::make_binary3<and_op, BaseT0, BaseT1>::construct(_0, _1);
}

///////////////////////////////////////////////////////////////////////////////
//
//  or lazy operator (infix |)
//
///////////////////////////////////////////////////////////////////////////////
struct or_op {

    template <typename T0, typename T1>
    struct result {

        typedef typename binary_operator<or_op, T0, T1>
            ::result_type type;
    };

    template <typename T0, typename T1>
    typename binary_operator<or_op, T0, T1>::result_type
    operator()(T0& _0, T1& _1) const
    { return binary_operator<or_op, T0, T1>::eval(_0, _1); }
};

//////////////////////////////////
template <typename BaseT, typename T1>
inline typename impl::make_binary1<or_op, BaseT, T1>::type
operator|(actor<BaseT> const& _0, T1 CREF _1)
{
    return impl::make_binary1<or_op, BaseT, T1>::construct(_0, _1);
}

//////////////////////////////////
template <typename T0, typename BaseT>
inline typename impl::make_binary2<or_op, T0, BaseT>::type
operator|(T0 CREF _0, actor<BaseT> const& _1)
{
    return impl::make_binary2<or_op, T0, BaseT>::construct(_0, _1);
}

//////////////////////////////////
template <typename BaseT0, typename BaseT1>
inline typename impl::make_binary3<or_op, BaseT0, BaseT1>::type
operator|(actor<BaseT0> const& _0, actor<BaseT1> const& _1)
{
    return impl::make_binary3<or_op, BaseT0, BaseT1>::construct(_0, _1);
}

///////////////////////////////////////////////////////////////////////////////
//
//  xor lazy operator (infix ^)
//
///////////////////////////////////////////////////////////////////////////////
struct xor_op {

    template <typename T0, typename T1>
    struct result {

        typedef typename binary_operator<xor_op, T0, T1>
            ::result_type type;
    };

    template <typename T0, typename T1>
    typename binary_operator<xor_op, T0, T1>::result_type
    operator()(T0& _0, T1& _1) const
    { return binary_operator<xor_op, T0, T1>::eval(_0, _1); }
};

//////////////////////////////////
template <typename BaseT, typename T1>
inline typename impl::make_binary1<xor_op, BaseT, T1>::type
operator^(actor<BaseT> const& _0, T1 CREF _1)
{
    return impl::make_binary1<xor_op, BaseT, T1>::construct(_0, _1);
}

//////////////////////////////////
template <typename T0, typename BaseT>
inline typename impl::make_binary2<xor_op, T0, BaseT>::type
operator^(T0 CREF _0, actor<BaseT> const& _1)
{
    return impl::make_binary2<xor_op, T0, BaseT>::construct(_0, _1);
}

//////////////////////////////////
template <typename BaseT0, typename BaseT1>
inline typename impl::make_binary3<xor_op, BaseT0, BaseT1>::type
operator^(actor<BaseT0> const& _0, actor<BaseT1> const& _1)
{
    return impl::make_binary3<xor_op, BaseT0, BaseT1>::construct(_0, _1);
}

///////////////////////////////////////////////////////////////////////////////
//
//  shift left lazy operator (infix <<)
//
///////////////////////////////////////////////////////////////////////////////
struct shift_l_op {

    template <typename T0, typename T1>
    struct result {

        typedef typename binary_operator<shift_l_op, T0, T1>
            ::result_type type;
    };

    template <typename T0, typename T1>
    typename binary_operator<shift_l_op, T0, T1>::result_type
    operator()(T0& _0, T1& _1) const
    { return binary_operator<shift_l_op, T0, T1>::eval(_0, _1); }
};

//////////////////////////////////
template <typename BaseT, typename T1>
inline typename impl::make_binary1<shift_l_op, BaseT, T1>::type
operator<<(actor<BaseT> const& _0, T1 CREF _1)
{
    return impl::make_binary1<shift_l_op, BaseT, T1>::construct(_0, _1);
}

//////////////////////////////////
template <typename T0, typename BaseT>
inline typename impl::make_binary2<shift_l_op, T0, BaseT>::type
operator<<(T0 CREF _0, actor<BaseT> const& _1)
{
    return impl::make_binary2<shift_l_op, T0, BaseT>::construct(_0, _1);
}

//////////////////////////////////
template <typename BaseT0, typename BaseT1>
inline typename impl::make_binary3<shift_l_op, BaseT0, BaseT1>::type
operator<<(actor<BaseT0> const& _0, actor<BaseT1> const& _1)
{
    return impl::make_binary3<shift_l_op, BaseT0, BaseT1>::construct(_0, _1);
}

///////////////////////////////////////////////////////////////////////////////
//
//  shift right lazy operator (infix >>)
//
///////////////////////////////////////////////////////////////////////////////
struct shift_r_op {

    template <typename T0, typename T1>
    struct result {

        typedef typename binary_operator<shift_r_op, T0, T1>
            ::result_type type;
    };

    template <typename T0, typename T1>
    typename binary_operator<shift_r_op, T0, T1>::result_type
    operator()(T0& _0, T1& _1) const
    { return binary_operator<shift_r_op, T0, T1>::eval(_0, _1); }
};

//////////////////////////////////
template <typename BaseT, typename T1>
inline typename impl::make_binary1<shift_r_op, BaseT, T1>::type
operator>>(actor<BaseT> const& _0, T1 CREF _1)
{
    return impl::make_binary1<shift_r_op, BaseT, T1>::construct(_0, _1);
}

//////////////////////////////////
template <typename T0, typename BaseT>
inline typename impl::make_binary2<shift_r_op, T0, BaseT>::type
operator>>(T0 CREF _0, actor<BaseT> const& _1)
{
    return impl::make_binary2<shift_r_op, T0, BaseT>::construct(_0, _1);
}

//////////////////////////////////
template <typename BaseT0, typename BaseT1>
inline typename impl::make_binary3<shift_r_op, BaseT0, BaseT1>::type
operator>>(actor<BaseT0> const& _0, actor<BaseT1> const& _1)
{
    return impl::make_binary3<shift_r_op, BaseT0, BaseT1>::construct(_0, _1);
}

///////////////////////////////////////////////////////////////////////////////
//
//  equal lazy operator (infix ==)
//
///////////////////////////////////////////////////////////////////////////////
struct eq_op {

    template <typename T0, typename T1>
    struct result {

        typedef typename binary_operator<eq_op, T0, T1>
            ::result_type type;
    };

    template <typename T0, typename T1>
    typename binary_operator<eq_op, T0, T1>::result_type
    operator()(T0& _0, T1& _1) const
    { return binary_operator<eq_op, T0, T1>::eval(_0, _1); }
};

//////////////////////////////////
template <typename BaseT, typename T1>
inline typename impl::make_binary1<eq_op, BaseT, T1>::type
operator==(actor<BaseT> const& _0, T1 CREF _1)
{
    return impl::make_binary1<eq_op, BaseT, T1>::construct(_0, _1);
}

//////////////////////////////////
template <typename T0, typename BaseT>
inline typename impl::make_binary2<eq_op, T0, BaseT>::type
operator==(T0 CREF _0, actor<BaseT> const& _1)
{
    return impl::make_binary2<eq_op, T0, BaseT>::construct(_0, _1);
}

//////////////////////////////////
template <typename BaseT0, typename BaseT1>
inline typename impl::make_binary3<eq_op, BaseT0, BaseT1>::type
operator==(actor<BaseT0> const& _0, actor<BaseT1> const& _1)
{
    return impl::make_binary3<eq_op, BaseT0, BaseT1>::construct(_0, _1);
}

///////////////////////////////////////////////////////////////////////////////
//
//  not equal lazy operator (infix !=)
//
///////////////////////////////////////////////////////////////////////////////
struct not_eq_op {

    template <typename T0, typename T1>
    struct result {

        typedef typename binary_operator<not_eq_op, T0, T1>
            ::result_type type;
    };

    template <typename T0, typename T1>
    typename binary_operator<not_eq_op, T0, T1>::result_type
    operator()(T0& _0, T1& _1) const
    { return binary_operator<not_eq_op, T0, T1>::eval(_0, _1); }
};

//////////////////////////////////
template <typename BaseT, typename T1>
inline typename impl::make_binary1<not_eq_op, BaseT, T1>::type
operator!=(actor<BaseT> const& _0, T1 CREF _1)
{
    return impl::make_binary1<not_eq_op, BaseT, T1>::construct(_0, _1);
}

//////////////////////////////////
template <typename T0, typename BaseT>
inline typename impl::make_binary2<not_eq_op, T0, BaseT>::type
operator!=(T0 CREF _0, actor<BaseT> const& _1)
{
    return impl::make_binary2<not_eq_op, T0, BaseT>::construct(_0, _1);
}

//////////////////////////////////
template <typename BaseT0, typename BaseT1>
inline typename impl::make_binary3<not_eq_op, BaseT0, BaseT1>::type
operator!=(actor<BaseT0> const& _0, actor<BaseT1> const& _1)
{
    return impl::make_binary3<not_eq_op, BaseT0, BaseT1>::construct(_0, _1);
}

///////////////////////////////////////////////////////////////////////////////
//
//  less than lazy operator (infix <)
//
///////////////////////////////////////////////////////////////////////////////
struct lt_op {

    template <typename T0, typename T1>
    struct result {

        typedef typename binary_operator<lt_op, T0, T1>
            ::result_type type;
    };

    template <typename T0, typename T1>
    typename binary_operator<lt_op, T0, T1>::result_type
    operator()(T0& _0, T1& _1) const
    { return binary_operator<lt_op, T0, T1>::eval(_0, _1); }
};

//////////////////////////////////
template <typename BaseT, typename T1>
inline typename impl::make_binary1<lt_op, BaseT, T1>::type
operator<(actor<BaseT> const& _0, T1 CREF _1)
{
    return impl::make_binary1<lt_op, BaseT, T1>::construct(_0, _1);
}

//////////////////////////////////
template <typename T0, typename BaseT>
inline typename impl::make_binary2<lt_op, T0, BaseT>::type
operator<(T0 CREF _0, actor<BaseT> const& _1)
{
    return impl::make_binary2<lt_op, T0, BaseT>::construct(_0, _1);
}

//////////////////////////////////
template <typename BaseT0, typename BaseT1>
inline typename impl::make_binary3<lt_op, BaseT0, BaseT1>::type
operator<(actor<BaseT0> const& _0, actor<BaseT1> const& _1)
{
    return impl::make_binary3<lt_op, BaseT0, BaseT1>::construct(_0, _1);
}

///////////////////////////////////////////////////////////////////////////////
//
//  less than equal lazy operator (infix <=)
//
///////////////////////////////////////////////////////////////////////////////
struct lt_eq_op {

    template <typename T0, typename T1>
    struct result {

        typedef typename binary_operator<lt_eq_op, T0, T1>
            ::result_type type;
    };

    template <typename T0, typename T1>
    typename binary_operator<lt_eq_op, T0, T1>::result_type
    operator()(T0& _0, T1& _1) const
    { return binary_operator<lt_eq_op, T0, T1>::eval(_0, _1); }
};

//////////////////////////////////
template <typename BaseT, typename T1>
inline typename impl::make_binary1<lt_eq_op, BaseT, T1>::type
operator<=(actor<BaseT> const& _0, T1 CREF _1)
{
    return impl::make_binary1<lt_eq_op, BaseT, T1>::construct(_0, _1);
}

//////////////////////////////////
template <typename T0, typename BaseT>
inline typename impl::make_binary2<lt_eq_op, T0, BaseT>::type
operator<=(T0 CREF _0, actor<BaseT> const& _1)
{
    return impl::make_binary2<lt_eq_op, T0, BaseT>::construct(_0, _1);
}

//////////////////////////////////
template <typename BaseT0, typename BaseT1>
inline typename impl::make_binary3<lt_eq_op, BaseT0, BaseT1>::type
operator<=(actor<BaseT0> const& _0, actor<BaseT1> const& _1)
{
    return impl::make_binary3<lt_eq_op, BaseT0, BaseT1>::construct(_0, _1);
}

///////////////////////////////////////////////////////////////////////////////
//
//  greater than lazy operator (infix >)
//
///////////////////////////////////////////////////////////////////////////////
struct gt_op {

    template <typename T0, typename T1>
    struct result {

        typedef typename binary_operator<gt_op, T0, T1>
            ::result_type type;
    };

    template <typename T0, typename T1>
    typename binary_operator<gt_op, T0, T1>::result_type
    operator()(T0& _0, T1& _1) const
    { return binary_operator<gt_op, T0, T1>::eval(_0, _1); }
};

//////////////////////////////////
template <typename BaseT, typename T1>
inline typename impl::make_binary1<gt_op, BaseT, T1>::type
operator>(actor<BaseT> const& _0, T1 CREF _1)
{
    return impl::make_binary1<gt_op, BaseT, T1>::construct(_0, _1);
}

//////////////////////////////////
template <typename T0, typename BaseT>
inline typename impl::make_binary2<gt_op, T0, BaseT>::type
operator>(T0 CREF _0, actor<BaseT> const& _1)
{
    return impl::make_binary2<gt_op, T0, BaseT>::construct(_0, _1);
}

//////////////////////////////////
template <typename BaseT0, typename BaseT1>
inline typename impl::make_binary3<gt_op, BaseT0, BaseT1>::type
operator>(actor<BaseT0> const& _0, actor<BaseT1> const& _1)
{
    return impl::make_binary3<gt_op, BaseT0, BaseT1>::construct(_0, _1);
}

///////////////////////////////////////////////////////////////////////////////
//
//  greater than equal lazy operator (infix >=)
//
///////////////////////////////////////////////////////////////////////////////
struct gt_eq_op {

    template <typename T0, typename T1>
    struct result {

        typedef typename binary_operator<gt_eq_op, T0, T1>
            ::result_type type;
    };

    template <typename T0, typename T1>
    typename binary_operator<gt_eq_op, T0, T1>::result_type
    operator()(T0& _0, T1& _1) const
    { return binary_operator<gt_eq_op, T0, T1>::eval(_0, _1); }
};

//////////////////////////////////
template <typename BaseT, typename T1>
inline typename impl::make_binary1<gt_eq_op, BaseT, T1>::type
operator>=(actor<BaseT> const& _0, T1 CREF _1)
{
    return impl::make_binary1<gt_eq_op, BaseT, T1>::construct(_0, _1);
}

//////////////////////////////////
template <typename T0, typename BaseT>
inline typename impl::make_binary2<gt_eq_op, T0, BaseT>::type
operator>=(T0 CREF _0, actor<BaseT> const& _1)
{
    return impl::make_binary2<gt_eq_op, T0, BaseT>::construct(_0, _1);
}

//////////////////////////////////
template <typename BaseT0, typename BaseT1>
inline typename impl::make_binary3<gt_eq_op, BaseT0, BaseT1>::type
operator>=(actor<BaseT0> const& _0, actor<BaseT1> const& _1)
{
    return impl::make_binary3<gt_eq_op, BaseT0, BaseT1>::construct(_0, _1);
}

///////////////////////////////////////////////////////////////////////////////
//
//  logical and lazy operator (infix &&)
//
//  The logical_and_composite class and its corresponding generators are
//  provided to allow short-circuit evaluation of the operator's
//  operands.
//
///////////////////////////////////////////////////////////////////////////////
template <typename A0, typename A1>
struct logical_and_composite {

    typedef logical_and_composite<A0, A1> self_t;

    template <typename TupleT>
    struct result {

        typedef typename binary_operator<logical_and_op,
            typename actor_result<A0, TupleT>::plain_type,
            typename actor_result<A1, TupleT>::plain_type
        >::result_type type;
    };

    logical_and_composite(A0 const& _0, A1 const& _1)
    :   a0(_0), a1(_1) {}

    template <typename TupleT>
    typename actor_result<self_t, TupleT>::type
    eval(TupleT const& args) const
    {
        return a0.eval(args) && a1.eval(args);
    }

    A0 a0; A1 a1; //  actors
};

#if !(defined(__ICL) && __ICL <= 500)
//////////////////////////////////
template <typename BaseT, typename T1>
inline actor<logical_and_composite
<actor<BaseT>, typename as_actor<T1>::type> >
operator&&(actor<BaseT> const& _0, T1 CREF _1)
{
    return logical_and_composite
        <actor<BaseT>, typename as_actor<T1>::type>
        (_0, as_actor<T1>::convert(_1));
}

//////////////////////////////////
template <typename T0, typename BaseT>
inline actor<logical_and_composite
<typename as_actor<T0>::type, actor<BaseT> > >
operator&&(T0 CREF _0, actor<BaseT> const& _1)
{
    return logical_and_composite
        <typename as_actor<T0>::type, actor<BaseT> >
        (as_actor<T0>::convert(_0), _1);
}

//////////////////////////////////
template <typename BaseT0, typename BaseT1>
inline actor<logical_and_composite
<actor<BaseT0>, actor<BaseT1> > >
operator&&(actor<BaseT0> const& _0, actor<BaseT1> const& _1)
{
    return logical_and_composite
        <actor<BaseT0>, actor<BaseT1> >
        (_0, _1);
}
#else
//////////////////////////////////
template <typename T0, typename T1>
inline actor<logical_and_composite
<typename as_actor<T0>::type, typename as_actor<T1>::type> >
operator&&(T0 CREF _0, T1 CREF _1)
{
    return logical_and_composite
        <typename as_actor<T0>::type, typename as_actor<T1>::type>
        (as_actor<T0>::convert(_0), as_actor<T1>::convert(_1));
}
#endif // !(__ICL && __ICL <= 500)

///////////////////////////////////////////////////////////////////////////////
//
//  logical or lazy operator (infix ||)
//
//  The logical_or_composite class and its corresponding generators are
//  provided to allow short-circuit evaluation of the operator's
//  operands.
//
///////////////////////////////////////////////////////////////////////////////
template <typename A0, typename A1>
struct logical_or_composite {

    typedef logical_or_composite<A0, A1> self_t;

    template <typename TupleT>
    struct result {

        typedef typename binary_operator<logical_or_op,
            typename actor_result<A0, TupleT>::plain_type,
            typename actor_result<A1, TupleT>::plain_type
        >::result_type type;
    };

    logical_or_composite(A0 const& _0, A1 const& _1)
    :   a0(_0), a1(_1) {}

    template <typename TupleT>
    typename actor_result<self_t, TupleT>::type
    eval(TupleT const& args) const
    {
        return a0.eval(args) || a1.eval(args);
    }

    A0 a0; A1 a1; //  actors
};

//////////////////////////////////
template <typename BaseT, typename T1>
inline actor<logical_or_composite
<actor<BaseT>, typename as_actor<T1>::type> >
operator||(actor<BaseT> const& _0, T1 CREF _1)
{
    return logical_or_composite
        <actor<BaseT>, typename as_actor<T1>::type>
        (_0, as_actor<T1>::convert(_1));
}

//////////////////////////////////
template <typename T0, typename BaseT>
inline actor<logical_or_composite
<typename as_actor<T0>::type, actor<BaseT> > >
operator||(T0 CREF _0, actor<BaseT> const& _1)
{
    return logical_or_composite
        <typename as_actor<T0>::type, actor<BaseT> >
        (as_actor<T0>::convert(_0), _1);
}

//////////////////////////////////
template <typename BaseT0, typename BaseT1>
inline actor<logical_or_composite
<actor<BaseT0>, actor<BaseT1> > >
operator||(actor<BaseT0> const& _0, actor<BaseT1> const& _1)
{
    return logical_or_composite
        <actor<BaseT0>, actor<BaseT1> >
        (_0, _1);
}

}   //  namespace phoenix

#undef CREF
#endif

/*=============================================================================
    Phoenix V1.2.1
    Copyright (c) 2001-2002 Joel de Guzman

  Distributed under the Boost Software License, Version 1.0. (See accompanying
  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef BOOST_SPIRIT_CLASSIC_PHOENIX_SPECIAL_OPS_HPP
#define BOOST_SPIRIT_CLASSIC_PHOENIX_SPECIAL_OPS_HPP

#include <boost/config.hpp>
#ifdef BOOST_NO_STRINGSTREAM
#include <strstream>
#define PHOENIX_SSTREAM strstream
#else
#include <sstream>
#define PHOENIX_SSTREAM stringstream
#endif

///////////////////////////////////////////////////////////////////////////////
#include <boost/spirit/home/classic/phoenix/operators.hpp>
#include <iosfwd>
#include <complex>

///////////////////////////////////////////////////////////////////////////////
#if defined(_STLPORT_VERSION) && defined(__STL_USE_OWN_NAMESPACE)
#define PHOENIX_STD _STLP_STD
#define PHOENIX_NO_STD_NAMESPACE
#else
#define PHOENIX_STD std
#endif

///////////////////////////////////////////////////////////////////////////////
namespace phoenix
{

///////////////////////////////////////////////////////////////////////////////
//
//  The following specializations take into account the C++ standard
//  library components. There are a couple of issues that have to be
//  dealt with to enable lazy operator overloads for the standard
//  library classes.
//
//      *iostream (e.g. cout, cin, strstream/ stringstream) uses non-
//      canonical shift operator overloads where the lhs is taken in
//      by reference.
//
//      *I/O manipulators overloads for the RHS of the << and >>
//      operators.
//
//      *STL iterators can be objects that conform to pointer semantics.
//      Some operators need to be specialized for these.
//
//      *std::complex is given a rank (see rank class in operators.hpp)
//
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//
//  specialization for rank<std::complex>
//
///////////////////////////////////////////////////////////////////////////////
template <typename T> struct rank<PHOENIX_STD::complex<T> >
{ static int const value = 170 + rank<T>::value; };

///////////////////////////////////////////////////////////////////////////////
//
//  specializations for std::istream
//
///////////////////////////////////////////////////////////////////////////////

//////////////////////////////////
template <typename T1>
struct binary_operator<shift_r_op, PHOENIX_STD::istream, T1>
{
    typedef PHOENIX_STD::istream& result_type;
    static result_type eval(PHOENIX_STD::istream& out, T1& rhs)
    { return out >> rhs; }
};

//////////////////////////////////
template <typename BaseT>
inline typename impl::make_binary3
    <shift_r_op, variable<PHOENIX_STD::istream>, BaseT>::type
operator>>(PHOENIX_STD::istream& _0, actor<BaseT> const& _1)
{
    return impl::make_binary3
    <shift_r_op, variable<PHOENIX_STD::istream>, BaseT>
    ::construct(var(_0), _1);
}

///////////////////////////////////////////////////////////////////////////////
//
//  specializations for std::ostream
//
///////////////////////////////////////////////////////////////////////////////

//////////////////////////////////
template <typename T1>
struct binary_operator<shift_l_op, PHOENIX_STD::ostream, T1>
{
    typedef PHOENIX_STD::ostream& result_type;
    static result_type eval(PHOENIX_STD::ostream& out, T1 const& rhs)
    { return out << rhs; }
};

//////////////////////////////////
template <typename BaseT>
inline typename impl::make_binary3
    <shift_l_op, variable<PHOENIX_STD::ostream>, BaseT>::type
operator<<(PHOENIX_STD::ostream& _0, actor<BaseT> const& _1)
{
    return impl::make_binary3
    <shift_l_op, variable<PHOENIX_STD::ostream>, BaseT>
    ::construct(var(_0), _1);
}

///////////////////////////////////////////////////////////////////////////////
//
//  specializations for std::strstream / stringstream
//
///////////////////////////////////////////////////////////////////////////////
template <typename T1>
struct binary_operator<shift_r_op, PHOENIX_STD::PHOENIX_SSTREAM, T1>
{
    typedef PHOENIX_STD::istream& result_type;
    static result_type eval(PHOENIX_STD::istream& out, T1& rhs)
    { return out >> rhs; }
};

//////////////////////////////////
template <typename BaseT>
inline typename impl::make_binary3
    <shift_r_op, variable<PHOENIX_STD::PHOENIX_SSTREAM>, BaseT>::type
operator>>(PHOENIX_STD::PHOENIX_SSTREAM& _0, actor<BaseT> const& _1)
{
    return impl::make_binary3
    <shift_r_op, variable<PHOENIX_STD::PHOENIX_SSTREAM>, BaseT>
    ::construct(var(_0), _1);
}

//////////////////////////////////
template <typename T1>
struct binary_operator<shift_l_op, PHOENIX_STD::PHOENIX_SSTREAM, T1>
{
    typedef PHOENIX_STD::ostream& result_type;
    static result_type eval(PHOENIX_STD::ostream& out, T1 const& rhs)
    { return out << rhs; }
};

//////////////////////////////////
template <typename BaseT>
inline typename impl::make_binary3
    <shift_l_op, variable<PHOENIX_STD::PHOENIX_SSTREAM>, BaseT>::type
operator<<(PHOENIX_STD::PHOENIX_SSTREAM& _0, actor<BaseT> const& _1)
{
    return impl::make_binary3
    <shift_l_op, variable<PHOENIX_STD::PHOENIX_SSTREAM>, BaseT>
    ::construct(var(_0), _1);
}

///////////////////////////////////////////////////////////////////////////////
//
//      I/O manipulator specializations
//
///////////////////////////////////////////////////////////////////////////////

typedef PHOENIX_STD::ios_base&  (*iomanip_t)(PHOENIX_STD::ios_base&);
typedef PHOENIX_STD::istream&   (*imanip_t)(PHOENIX_STD::istream&);
typedef PHOENIX_STD::ostream&   (*omanip_t)(PHOENIX_STD::ostream&);

#if defined(BOOST_BORLANDC)

///////////////////////////////////////////////////////////////////////////////
//
//      Borland does not like i/o manipulators functions such as endl to
//      be the rhs of a lazy << operator (Borland incorrectly reports
//      ambiguity). To get around the problem, we provide function
//      pointer versions of the same name with a single trailing
//      underscore.
//
//      You can use the same trick for other i/o manipulators.
//      Alternatively, you can prefix the manipulator with a '&'
//      operator. Example:
//
//          cout << arg1 << &endl
//
///////////////////////////////////////////////////////////////////////////////

imanip_t    ws_     = &PHOENIX_STD::ws;
iomanip_t   dec_    = &PHOENIX_STD::dec;
iomanip_t   hex_    = &PHOENIX_STD::hex;
iomanip_t   oct_    = &PHOENIX_STD::oct;
omanip_t    endl_   = &PHOENIX_STD::endl;
omanip_t    ends_   = &PHOENIX_STD::ends;
omanip_t    flush_  = &PHOENIX_STD::flush;

#else // BOOST_BORLANDC

///////////////////////////////////////////////////////////////////////////////
//
//      The following are overloads for I/O manipulators.
//
///////////////////////////////////////////////////////////////////////////////
template <typename BaseT>
inline typename impl::make_binary1<shift_l_op, BaseT, imanip_t>::type
operator>>(actor<BaseT> const& _0, imanip_t _1)
{
    return impl::make_binary1<shift_l_op, BaseT, imanip_t>::construct(_0, _1);
}

//////////////////////////////////
template <typename BaseT>
inline typename impl::make_binary1<shift_l_op, BaseT, iomanip_t>::type
operator>>(actor<BaseT> const& _0, iomanip_t _1)
{
    return impl::make_binary1<shift_l_op, BaseT, iomanip_t>::construct(_0, _1);
}

//////////////////////////////////
template <typename BaseT>
inline typename impl::make_binary1<shift_l_op, BaseT, omanip_t>::type
operator<<(actor<BaseT> const& _0, omanip_t _1)
{
    return impl::make_binary1<shift_l_op, BaseT, omanip_t>::construct(_0, _1);
}

//////////////////////////////////
template <typename BaseT>
inline typename impl::make_binary1<shift_l_op, BaseT, iomanip_t>::type
operator<<(actor<BaseT> const& _0, iomanip_t _1)
{
    return impl::make_binary1<shift_l_op, BaseT, iomanip_t>::construct(_0, _1);
}

#endif // BOOST_BORLANDC

///////////////////////////////////////////////////////////////////////////////
//
//  specializations for stl iterators and containers
//
///////////////////////////////////////////////////////////////////////////////
template <typename T>
struct unary_operator<dereference_op, T>
{
    typedef typename T::reference result_type;
    static result_type eval(T const& iter)
    { return *iter; }
};

//////////////////////////////////
template <typename T0, typename T1>
struct binary_operator<index_op, T0, T1>
{
    typedef typename T0::reference result_type;
    static result_type eval(T0& container, T1 const& index)
    { return container[index]; }
};

//////////////////////////////////
template <typename T0, typename T1>
struct binary_operator<index_op, T0 const, T1>
{
    typedef typename T0::const_reference result_type;
    static result_type eval(T0 const& container, T1 const& index)
    { return container[index]; }
};

///////////////////////////////////////////////////////////////////////////////
}   //  namespace phoenix

#undef PHOENIX_SSTREAM
#undef PHOENIX_STD
#endif

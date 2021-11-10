/*=============================================================================
    Copyright (c) 2001-2003 Joel de Guzman
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#ifndef BOOST_SPIRIT_CHSET_OPERATORS_IPP
#define BOOST_SPIRIT_CHSET_OPERATORS_IPP

///////////////////////////////////////////////////////////////////////////////
#include <boost/limits.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit {

BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN

///////////////////////////////////////////////////////////////////////////////
//
//  chset free operators implementation
//
///////////////////////////////////////////////////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator|(chset<CharT> const& a, chset<CharT> const& b)
{
    return chset<CharT>(a) |= b;
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator-(chset<CharT> const& a, chset<CharT> const& b)
{
    return chset<CharT>(a) -= b;
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator~(chset<CharT> const& a)
{
    return chset<CharT>(a).inverse();
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator&(chset<CharT> const& a, chset<CharT> const& b)
{
    return chset<CharT>(a) &= b;
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator^(chset<CharT> const& a, chset<CharT> const& b)
{
    return chset<CharT>(a) ^= b;
}

///////////////////////////////////////////////////////////////////////////////
//
//  range <--> chset free operators implementation
//
///////////////////////////////////////////////////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator|(chset<CharT> const& a, range<CharT> const& b)
{
    chset<CharT> a_(a);
    a_.set(b);
    return a_;
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator&(chset<CharT> const& a, range<CharT> const& b)
{
    chset<CharT> a_(a);
    if(b.first != (std::numeric_limits<CharT>::min)()) {
        a_.clear(range<CharT>((std::numeric_limits<CharT>::min)(), b.first - 1));
    }
    if(b.last != (std::numeric_limits<CharT>::max)()) {
        a_.clear(range<CharT>(b.last + 1, (std::numeric_limits<CharT>::max)()));
    }
    return a_;
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator-(chset<CharT> const& a, range<CharT> const& b)
{
    chset<CharT> a_(a);
    a_.clear(b);
    return a_;
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator^(chset<CharT> const& a, range<CharT> const& b)
{
    return a ^ chset<CharT>(b);
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator|(range<CharT> const& a, chset<CharT> const& b)
{
    chset<CharT> b_(b);
    b_.set(a);
    return b_;
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator&(range<CharT> const& a, chset<CharT> const& b)
{
    chset<CharT> b_(b);
    if(a.first != (std::numeric_limits<CharT>::min)()) {
        b_.clear(range<CharT>((std::numeric_limits<CharT>::min)(), a.first - 1));
    }
    if(a.last != (std::numeric_limits<CharT>::max)()) {
        b_.clear(range<CharT>(a.last + 1, (std::numeric_limits<CharT>::max)()));
    }
    return b_;
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator-(range<CharT> const& a, chset<CharT> const& b)
{
    return chset<CharT>(a) - b;
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator^(range<CharT> const& a, chset<CharT> const& b)
{
    return chset<CharT>(a) ^ b;
}

///////////////////////////////////////////////////////////////////////////////
//
//  literal primitives <--> chset free operators implementation
//
///////////////////////////////////////////////////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator|(chset<CharT> const& a, CharT b)
{
    return a | chset<CharT>(b);
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator&(chset<CharT> const& a, CharT b)
{
    return a & chset<CharT>(b);
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator-(chset<CharT> const& a, CharT b)
{
    return a - chset<CharT>(b);
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator^(chset<CharT> const& a, CharT b)
{
    return a ^ chset<CharT>(b);
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator|(CharT a, chset<CharT> const& b)
{
    return chset<CharT>(a) | b;
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator&(CharT a, chset<CharT> const& b)
{
    return chset<CharT>(a) & b;
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator-(CharT a, chset<CharT> const& b)
{
    return chset<CharT>(a) - b;
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator^(CharT a, chset<CharT> const& b)
{
    return chset<CharT>(a) ^ b;
}

///////////////////////////////////////////////////////////////////////////////
//
//  chlit <--> chset free operators implementation
//
///////////////////////////////////////////////////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator|(chset<CharT> const& a, chlit<CharT> const& b)
{
    return a | chset<CharT>(b.ch);
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator&(chset<CharT> const& a, chlit<CharT> const& b)
{
    return a & chset<CharT>(b.ch);
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator-(chset<CharT> const& a, chlit<CharT> const& b)
{
    return a - chset<CharT>(b.ch);
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator^(chset<CharT> const& a, chlit<CharT> const& b)
{
    return a ^ chset<CharT>(b.ch);
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator|(chlit<CharT> const& a, chset<CharT> const& b)
{
    return chset<CharT>(a.ch) | b;
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator&(chlit<CharT> const& a, chset<CharT> const& b)
{
    return chset<CharT>(a.ch) & b;
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator-(chlit<CharT> const& a, chset<CharT> const& b)
{
    return chset<CharT>(a.ch) - b;
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator^(chlit<CharT> const& a, chset<CharT> const& b)
{
    return chset<CharT>(a.ch) ^ b;
}

///////////////////////////////////////////////////////////////////////////////
//
//  negated_char_parser<range> <--> chset free operators implementation
//
///////////////////////////////////////////////////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator|(chset<CharT> const& a, negated_char_parser<range<CharT> > const& b)
{
    return a | chset<CharT>(b);
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator&(chset<CharT> const& a, negated_char_parser<range<CharT> > const& b)
{
    return a & chset<CharT>(b);
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator-(chset<CharT> const& a, negated_char_parser<range<CharT> > const& b)
{
    return a - chset<CharT>(b);
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator^(chset<CharT> const& a, negated_char_parser<range<CharT> > const& b)
{
    return a ^ chset<CharT>(b);
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator|(negated_char_parser<range<CharT> > const& a, chset<CharT> const& b)
{
    return chset<CharT>(a) | b;
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator&(negated_char_parser<range<CharT> > const& a, chset<CharT> const& b)
{
    return chset<CharT>(a) & b;
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator-(negated_char_parser<range<CharT> > const& a, chset<CharT> const& b)
{
    return chset<CharT>(a) - b;
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator^(negated_char_parser<range<CharT> > const& a, chset<CharT> const& b)
{
    return chset<CharT>(a) ^ b;
}

///////////////////////////////////////////////////////////////////////////////
//
//  negated_char_parser<chlit> <--> chset free operators implementation
//
///////////////////////////////////////////////////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator|(chset<CharT> const& a, negated_char_parser<chlit<CharT> > const& b)
{
    return a | chset<CharT>(b);
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator&(chset<CharT> const& a, negated_char_parser<chlit<CharT> > const& b)
{
    return a & chset<CharT>(b);
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator-(chset<CharT> const& a, negated_char_parser<chlit<CharT> > const& b)
{
    return a - chset<CharT>(b);
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator^(chset<CharT> const& a, negated_char_parser<chlit<CharT> > const& b)
{
    return a ^ chset<CharT>(b);
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator|(negated_char_parser<chlit<CharT> > const& a, chset<CharT> const& b)
{
    return chset<CharT>(a) | b;
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator&(negated_char_parser<chlit<CharT> > const& a, chset<CharT> const& b)
{
    return chset<CharT>(a) & b;
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator-(negated_char_parser<chlit<CharT> > const& a, chset<CharT> const& b)
{
    return chset<CharT>(a) - b;
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator^(negated_char_parser<chlit<CharT> > const& a, chset<CharT> const& b)
{
    return chset<CharT>(a) ^ b;
}

///////////////////////////////////////////////////////////////////////////////
//
//  anychar_parser <--> chset free operators
//
//      Where a is chset and b is a anychar_parser, and vice-versa, implements:
//
//          a | b, a & b, a - b, a ^ b
//
///////////////////////////////////////////////////////////////////////////////
namespace impl {

    template <typename CharT>
    inline BOOST_SPIRIT_CLASSIC_NS::range<CharT> const&
    full()
    {
        static BOOST_SPIRIT_CLASSIC_NS::range<CharT> full_(
            (std::numeric_limits<CharT>::min)(),
            (std::numeric_limits<CharT>::max)());
        return full_;
    }

    template <typename CharT>
    inline BOOST_SPIRIT_CLASSIC_NS::range<CharT> const&
    empty()
    {
        static BOOST_SPIRIT_CLASSIC_NS::range<CharT> empty_;
        return empty_;
    }
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator|(chset<CharT> const&, anychar_parser)
{
    return chset<CharT>(impl::full<CharT>());
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator&(chset<CharT> const& a, anychar_parser)
{
    return a;
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator-(chset<CharT> const&, anychar_parser)
{
    return chset<CharT>();
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator^(chset<CharT> const& a, anychar_parser)
{
    return ~a;
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator|(anychar_parser, chset<CharT> const& /*b*/)
{
    return chset<CharT>(impl::full<CharT>());
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator&(anychar_parser, chset<CharT> const& b)
{
    return b;
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator-(anychar_parser, chset<CharT> const& b)
{
    return ~b;
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator^(anychar_parser, chset<CharT> const& b)
{
    return ~b;
}

///////////////////////////////////////////////////////////////////////////////
//
//  nothing_parser <--> chset free operators implementation
//
///////////////////////////////////////////////////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator|(chset<CharT> const& a, nothing_parser)
{
    return a;
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator&(chset<CharT> const& /*a*/, nothing_parser)
{
    return impl::empty<CharT>();
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator-(chset<CharT> const& a, nothing_parser)
{
    return a;
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator^(chset<CharT> const& a, nothing_parser)
{
    return a;
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator|(nothing_parser, chset<CharT> const& b)
{
    return b;
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator&(nothing_parser, chset<CharT> const& /*b*/)
{
    return impl::empty<CharT>();
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator-(nothing_parser, chset<CharT> const& /*b*/)
{
    return impl::empty<CharT>();
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator^(nothing_parser, chset<CharT> const& b)
{
    return b;
}

///////////////////////////////////////////////////////////////////////////////
BOOST_SPIRIT_CLASSIC_NAMESPACE_END

}} // namespace boost::spirit

#endif


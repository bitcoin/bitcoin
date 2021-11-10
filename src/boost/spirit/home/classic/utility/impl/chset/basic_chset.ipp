/*=============================================================================
    Copyright (c) 2001-2003 Joel de Guzman
    Copyright (c) 2001-2003 Daniel Nuffer
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#ifndef BOOST_SPIRIT_BASIC_CHSET_IPP
#define BOOST_SPIRIT_BASIC_CHSET_IPP

///////////////////////////////////////////////////////////////////////////////
#include <bitset>
#include <boost/spirit/home/classic/utility/impl/chset/basic_chset.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit {

BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN

///////////////////////////////////////////////////////////////////////////////
//
//  basic_chset: character set implementation
//
///////////////////////////////////////////////////////////////////////////////
template <typename CharT>
inline basic_chset<CharT>::basic_chset() {}

//////////////////////////////////
template <typename CharT>
inline basic_chset<CharT>::basic_chset(basic_chset const& arg_)
: rr(arg_.rr) {}

//////////////////////////////////
template <typename CharT>
inline bool
basic_chset<CharT>::test(CharT v) const
{ return rr.test(v); }

//////////////////////////////////
template <typename CharT>
inline void
basic_chset<CharT>::set(CharT from, CharT to)
{ rr.set(utility::impl::range<CharT>(from, to)); }

//////////////////////////////////
template <typename CharT>
inline void
basic_chset<CharT>::set(CharT c)
{ rr.set(utility::impl::range<CharT>(c, c)); }

//////////////////////////////////
template <typename CharT>
inline void
basic_chset<CharT>::clear(CharT from, CharT to)
{ rr.clear(utility::impl::range<CharT>(from, to)); }

//////////////////////////////////
template <typename CharT>
inline void
basic_chset<CharT>::clear()
{ rr.clear(); }

/////////////////////////////////
template <typename CharT>
inline void
basic_chset<CharT>::inverse()
{
    basic_chset inv;
    inv.set(
        (std::numeric_limits<CharT>::min)(),
        (std::numeric_limits<CharT>::max)()
    );
    inv -= *this;
    swap(inv);
}

/////////////////////////////////
template <typename CharT>
inline void
basic_chset<CharT>::swap(basic_chset& x)
{ rr.swap(x.rr); }

/////////////////////////////////
template <typename CharT>
inline basic_chset<CharT>&
basic_chset<CharT>::operator|=(basic_chset<CharT> const& x)
{
    typedef typename utility::impl::range_run<CharT>::const_iterator const_iterator;
    for (const_iterator iter = x.rr.begin(); iter != x.rr.end(); ++iter)
        rr.set(*iter);
    return *this;
}

/////////////////////////////////
template <typename CharT>
inline basic_chset<CharT>&
basic_chset<CharT>::operator&=(basic_chset<CharT> const& x)
{
    basic_chset inv;
    inv.set(
        (std::numeric_limits<CharT>::min)(),
        (std::numeric_limits<CharT>::max)()
    );
    inv -= x;
    *this -= inv;
    return *this;
}

/////////////////////////////////
template <typename CharT>
inline basic_chset<CharT>&
basic_chset<CharT>::operator-=(basic_chset<CharT> const& x)
{
    typedef typename utility::impl::range_run<CharT>::const_iterator const_iterator;
    for (const_iterator iter = x.rr.begin(); iter != x.rr.end(); ++iter)
        rr.clear(*iter);
    return *this;
}

/////////////////////////////////
template <typename CharT>
inline basic_chset<CharT>&
basic_chset<CharT>::operator^=(basic_chset<CharT> const& x)
{
    basic_chset bma = x;
    bma -= *this;
    *this -= x;
    *this |= bma;
    return *this;
}

#if (CHAR_BIT == 8)

///////////////////////////////////////////////////////////////////////////////
//
//  basic_chset: specializations for 8 bit chars using std::bitset
//
///////////////////////////////////////////////////////////////////////////////
template <typename CharT>
inline basic_chset_8bit<CharT>::basic_chset_8bit() {}

/////////////////////////////////
template <typename CharT>
inline basic_chset_8bit<CharT>::basic_chset_8bit(basic_chset_8bit const& arg_)
: bset(arg_.bset) {}

/////////////////////////////////
template <typename CharT>
inline bool
basic_chset_8bit<CharT>::test(CharT v) const
{ return bset.test((unsigned char)v); }

/////////////////////////////////
template <typename CharT>
inline void
basic_chset_8bit<CharT>::set(CharT from, CharT to)
{
    for (int i = from; i <= to; ++i)
        bset.set((unsigned char)i);
}

/////////////////////////////////
template <typename CharT>
inline void
basic_chset_8bit<CharT>::set(CharT c)
{ bset.set((unsigned char)c); }

/////////////////////////////////
template <typename CharT>
inline void
basic_chset_8bit<CharT>::clear(CharT from, CharT to)
{
    for (int i = from; i <= to; ++i)
        bset.reset((unsigned char)i);
}

/////////////////////////////////
template <typename CharT>
inline void
basic_chset_8bit<CharT>::clear(CharT c)
{ bset.reset((unsigned char)c); }

/////////////////////////////////
template <typename CharT>
inline void
basic_chset_8bit<CharT>::clear()
{ bset.reset(); }

/////////////////////////////////
template <typename CharT>
inline void
basic_chset_8bit<CharT>::inverse()
{ bset.flip(); }

/////////////////////////////////
template <typename CharT>
inline void
basic_chset_8bit<CharT>::swap(basic_chset_8bit& x)
{ std::swap(bset, x.bset); }

/////////////////////////////////
template <typename CharT>
inline basic_chset_8bit<CharT>&
basic_chset_8bit<CharT>::operator|=(basic_chset_8bit const& x)
{
    bset |= x.bset;
    return *this;
}

/////////////////////////////////
template <typename CharT>
inline basic_chset_8bit<CharT>&
basic_chset_8bit<CharT>::operator&=(basic_chset_8bit const& x)
{
    bset &= x.bset;
    return *this;
}

/////////////////////////////////
template <typename CharT>
inline basic_chset_8bit<CharT>&
basic_chset_8bit<CharT>::operator-=(basic_chset_8bit const& x)
{
    bset &= ~x.bset;
    return *this;
}

/////////////////////////////////
template <typename CharT>
inline basic_chset_8bit<CharT>&
basic_chset_8bit<CharT>::operator^=(basic_chset_8bit const& x)
{
    bset ^= x.bset;
    return *this;
}

#endif

BOOST_SPIRIT_CLASSIC_NAMESPACE_END

}} // namespace boost::spirit

#endif


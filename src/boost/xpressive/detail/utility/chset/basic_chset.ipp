/*=============================================================================
    Copyright (c) 2001-2003 Joel de Guzman
    Copyright (c) 2001-2003 Daniel Nuffer
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#ifndef BOOST_XPRESSIVE_SPIRIT_BASIC_CHSET_IPP
#define BOOST_XPRESSIVE_SPIRIT_BASIC_CHSET_IPP

///////////////////////////////////////////////////////////////////////////////
#include <bitset>
#include <boost/xpressive/detail/utility/chset/basic_chset.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace xpressive { namespace detail
{

///////////////////////////////////////////////////////////////////////////////
//
//  basic_chset: character set implementation
//
///////////////////////////////////////////////////////////////////////////////
template<typename Char>
inline basic_chset<Char>::basic_chset()
{
}

//////////////////////////////////
template<typename Char>
inline basic_chset<Char>::basic_chset(basic_chset const &arg)
  : rr_(arg.rr_)
{
}

//////////////////////////////////
template<typename Char>
inline bool basic_chset<Char>::empty() const
{
    return this->rr_.empty();
}

//////////////////////////////////
template<typename Char>
template<typename Traits>
inline bool basic_chset<Char>::test(Char v, Traits const &, mpl::false_) const // case-sensitive
{
    return this->rr_.test(v);
}

//////////////////////////////////
template<typename Char>
template<typename Traits>
inline bool basic_chset<Char>::test(Char v, Traits const &tr, mpl::true_) const // case-insensitive
{
    return this->rr_.test(v, tr);
}

//////////////////////////////////
template<typename Char>
inline void basic_chset<Char>::set(Char from, Char to)
{
    this->rr_.set(range<Char>(from, to));
}

//////////////////////////////////
template<typename Char>
template<typename Traits>
inline void basic_chset<Char>::set(Char from, Char to, Traits const &)
{
    this->rr_.set(range<Char>(from, to));
}

//////////////////////////////////
template<typename Char>
inline void basic_chset<Char>::set(Char c)
{
    this->rr_.set(range<Char>(c, c));
}

//////////////////////////////////
template<typename Char>
template<typename Traits>
inline void basic_chset<Char>::set(Char c, Traits const &)
{
    this->rr_.set(range<Char>(c, c));
}

//////////////////////////////////
template<typename Char>
inline void basic_chset<Char>::clear(Char c)
{
    this->rr_.clear(range<Char>(c, c));
}

//////////////////////////////////
template<typename Char>
template<typename Traits>
inline void basic_chset<Char>::clear(Char c, Traits const &)
{
    this->rr_.clear(range<Char>(c, c));
}

//////////////////////////////////
template<typename Char>
inline void basic_chset<Char>::clear(Char from, Char to)
{
    this->rr_.clear(range<Char>(from, to));
}

//////////////////////////////////
template<typename Char>
template<typename Traits>
inline void basic_chset<Char>::clear(Char from, Char to, Traits const &)
{
    this->rr_.clear(range<Char>(from, to));
}

//////////////////////////////////
template<typename Char>
inline void basic_chset<Char>::clear()
{
    this->rr_.clear();
}

/////////////////////////////////
template<typename Char>
inline void basic_chset<Char>::inverse()
{
    // BUGBUG is this right? Does this handle icase correctly?
    basic_chset<Char> inv;
    inv.set((std::numeric_limits<Char>::min)(), (std::numeric_limits<Char>::max)());
    inv -= *this;
    this->swap(inv);
}

/////////////////////////////////
template<typename Char>
inline void basic_chset<Char>::swap(basic_chset<Char> &that)
{
    this->rr_.swap(that.rr_);
}

/////////////////////////////////
template<typename Char>
inline basic_chset<Char> &
basic_chset<Char>::operator |=(basic_chset<Char> const &that)
{
    typedef typename range_run<Char>::const_iterator const_iterator;
    for(const_iterator iter = that.rr_.begin(); iter != that.rr_.end(); ++iter)
    {
        this->rr_.set(*iter);
    }
    return *this;
}

/////////////////////////////////
template<typename Char>
inline basic_chset<Char> &
basic_chset<Char>::operator &=(basic_chset<Char> const &that)
{
    basic_chset<Char> inv;
    inv.set((std::numeric_limits<Char>::min)(), (std::numeric_limits<Char>::max)());
    inv -= that;
    *this -= inv;
    return *this;
}

/////////////////////////////////
template<typename Char>
inline basic_chset<Char> &
basic_chset<Char>::operator -=(basic_chset<Char> const &that)
{
    typedef typename range_run<Char>::const_iterator const_iterator;
    for(const_iterator iter = that.rr_.begin(); iter != that.rr_.end(); ++iter)
    {
        this->rr_.clear(*iter);
    }
    return *this;
}

/////////////////////////////////
template<typename Char>
inline basic_chset<Char> &
basic_chset<Char>::operator ^=(basic_chset<Char> const &that)
{
    basic_chset bma = that;
    bma -= *this;
    *this -= that;
    *this |= bma;
    return *this;
}

#if(CHAR_BIT == 8)

///////////////////////////////////////////////////////////////////////////////
//
//  basic_chset: specializations for 8 bit chars using std::bitset
//
///////////////////////////////////////////////////////////////////////////////
template<typename Char>
inline basic_chset_8bit<Char>::basic_chset_8bit()
{
}

/////////////////////////////////
template<typename Char>
inline basic_chset_8bit<Char>::basic_chset_8bit(basic_chset_8bit<Char> const &arg)
  : bset_(arg.bset_)
{
}

/////////////////////////////////
template<typename Char>
inline bool basic_chset_8bit<Char>::empty() const
{
    return !this->bset_.any();
}

/////////////////////////////////
template<typename Char>
template<typename Traits>
inline bool basic_chset_8bit<Char>::test(Char v, Traits const &, mpl::false_) const // case-sensitive
{
    return this->bset_.test((unsigned char)v);
}

/////////////////////////////////
template<typename Char>
template<typename Traits>
inline bool basic_chset_8bit<Char>::test(Char v, Traits const &tr, mpl::true_) const // case-insensitive
{
    return this->bset_.test((unsigned char)tr.translate_nocase(v));
}

/////////////////////////////////
template<typename Char>
inline void basic_chset_8bit<Char>::set(Char from, Char to)
{
    for(int i = from; i <= to; ++i)
    {
        this->bset_.set((unsigned char)i);
    }
}

/////////////////////////////////
template<typename Char>
template<typename Traits>
inline void basic_chset_8bit<Char>::set(Char from, Char to, Traits const &tr)
{
    for(int i = from; i <= to; ++i)
    {
        this->bset_.set((unsigned char)tr.translate_nocase((Char)i));
    }
}

/////////////////////////////////
template<typename Char>
inline void basic_chset_8bit<Char>::set(Char c)
{
    this->bset_.set((unsigned char)c);
}

/////////////////////////////////
template<typename Char>
template<typename Traits>
inline void basic_chset_8bit<Char>::set(Char c, Traits const &tr)
{
    this->bset_.set((unsigned char)tr.translate_nocase(c));
}

/////////////////////////////////
template<typename Char>
inline void basic_chset_8bit<Char>::clear(Char from, Char to)
{
    for(int i = from; i <= to; ++i)
    {
        this->bset_.reset((unsigned char)i);
    }
}

/////////////////////////////////
template<typename Char>
template<typename Traits>
inline void basic_chset_8bit<Char>::clear(Char from, Char to, Traits const &tr)
{
    for(int i = from; i <= to; ++i)
    {
        this->bset_.reset((unsigned char)tr.translate_nocase((Char)i));
    }
}

/////////////////////////////////
template<typename Char>
inline void basic_chset_8bit<Char>::clear(Char c)
{
    this->bset_.reset((unsigned char)c);
}

/////////////////////////////////
template<typename Char>
template<typename Traits>
inline void basic_chset_8bit<Char>::clear(Char c, Traits const &tr)
{
    this->bset_.reset((unsigned char)tr.tranlsate_nocase(c));
}

/////////////////////////////////
template<typename Char>
inline void basic_chset_8bit<Char>::clear()
{
    this->bset_.reset();
}

/////////////////////////////////
template<typename Char>
inline void basic_chset_8bit<Char>::inverse()
{
    this->bset_.flip();
}

/////////////////////////////////
template<typename Char>
inline void basic_chset_8bit<Char>::swap(basic_chset_8bit<Char> &that)
{
    std::swap(this->bset_, that.bset_);
}

/////////////////////////////////
template<typename Char>
inline basic_chset_8bit<Char> &
basic_chset_8bit<Char>::operator |=(basic_chset_8bit<Char> const &that)
{
    this->bset_ |= that.bset_;
    return *this;
}

/////////////////////////////////
template<typename Char>
inline basic_chset_8bit<Char> &
basic_chset_8bit<Char>::operator &=(basic_chset_8bit<Char> const &that)
{
    this->bset_ &= that.bset_;
    return *this;
}

/////////////////////////////////
template<typename Char>
inline basic_chset_8bit<Char> &
basic_chset_8bit<Char>::operator -=(basic_chset_8bit<Char> const &that)
{
    this->bset_ &= ~that.bset_;
    return *this;
}

/////////////////////////////////
template<typename Char>
inline basic_chset_8bit<Char> &
basic_chset_8bit<Char>::operator ^=(basic_chset_8bit<Char> const &that)
{
    this->bset_ ^= that.bset_;
    return *this;
}

template<typename Char>
inline std::bitset<256> const &
basic_chset_8bit<Char>::base() const
{
    return this->bset_;
}

#endif // if(CHAR_BIT == 8)


///////////////////////////////////////////////////////////////////////////////
// helpers
template<typename Char, typename Traits>
inline void set_char(basic_chset<Char> &chset, Char ch, Traits const &tr, bool icase)
{
    icase ? chset.set(ch, tr) : chset.set(ch);
}

template<typename Char, typename Traits>
inline void set_range(basic_chset<Char> &chset, Char from, Char to, Traits const &tr, bool icase)
{
    icase ? chset.set(from, to, tr) : chset.set(from, to);
}

template<typename Char, typename Traits>
inline void set_class(basic_chset<Char> &chset, typename Traits::char_class_type char_class, bool no, Traits const &tr)
{
    BOOST_MPL_ASSERT_RELATION(1, ==, sizeof(Char));
    for(std::size_t i = 0; i <= UCHAR_MAX; ++i)
    {
        typedef typename std::char_traits<Char>::int_type int_type;
        Char ch = std::char_traits<Char>::to_char_type(static_cast<int_type>(i));
        if(no != tr.isctype(ch, char_class))
        {
            chset.set(ch);
        }
    }
}

}}} // namespace boost::xpressive::detail

#endif


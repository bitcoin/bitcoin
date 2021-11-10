/*=============================================================================
    Copyright (c) 2001-2003 Joel de Guzman
    Copyright (c) 2001-2003 Daniel Nuffer
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#ifndef BOOST_XPRESSIVE_SPIRIT_BASIC_CHSET_HPP_EAN_10_04_2005
#define BOOST_XPRESSIVE_SPIRIT_BASIC_CHSET_HPP_EAN_10_04_2005

///////////////////////////////////////////////////////////////////////////////
#include <bitset>
#include <boost/mpl/bool.hpp>
#include <boost/xpressive/detail/utility/chset/range_run.ipp>

namespace boost { namespace xpressive { namespace detail
{

///////////////////////////////////////////////////////////////////////////
//
//  basic_chset: basic character set implementation using range_run
//
///////////////////////////////////////////////////////////////////////////
template<typename Char>
struct basic_chset
{
    basic_chset();
    basic_chset(basic_chset const &arg);

    bool empty() const;
    void set(Char from, Char to);
    template<typename Traits>
    void set(Char from, Char to, Traits const &tr);
    void set(Char c);
    template<typename Traits>
    void set(Char c, Traits const &tr);

    void clear(Char from, Char to);
    template<typename Traits>
    void clear(Char from, Char to, Traits const &tr);
    void clear(Char c);
    template<typename Traits>
    void clear(Char c, Traits const &tr);
    void clear();

    template<typename Traits>
    bool test(Char v, Traits const &tr, mpl::false_) const; // case-sensitive
    template<typename Traits>
    bool test(Char v, Traits const &tr, mpl::true_) const; // case-insensitive

    void inverse();
    void swap(basic_chset& x);

    basic_chset &operator |=(basic_chset const &x);
    basic_chset &operator &=(basic_chset const &x);
    basic_chset &operator -=(basic_chset const &x);
    basic_chset &operator ^=(basic_chset const &x);

private:
    range_run<Char> rr_;
};

#if(CHAR_BIT == 8)

///////////////////////////////////////////////////////////////////////////
//
//  basic_chset: specializations for 8 bit chars using std::bitset
//
///////////////////////////////////////////////////////////////////////////
template<typename Char>
struct basic_chset_8bit
{
    basic_chset_8bit();
    basic_chset_8bit(basic_chset_8bit const &arg);

    bool empty() const;

    void set(Char from, Char to);
    template<typename Traits>
    void set(Char from, Char to, Traits const &tr);
    void set(Char c);
    template<typename Traits>
    void set(Char c, Traits const &tr);

    void clear(Char from, Char to);
    template<typename Traits>
    void clear(Char from, Char to, Traits const &tr);
    void clear(Char c);
    template<typename Traits>
    void clear(Char c, Traits const &tr);
    void clear();

    template<typename Traits>
    bool test(Char v, Traits const &tr, mpl::false_) const; // case-sensitive
    template<typename Traits>
    bool test(Char v, Traits const &tr, mpl::true_) const; // case-insensitive

    void inverse();
    void swap(basic_chset_8bit& x);

    basic_chset_8bit &operator |=(basic_chset_8bit const &x);
    basic_chset_8bit &operator &=(basic_chset_8bit const &x);
    basic_chset_8bit &operator -=(basic_chset_8bit const &x);
    basic_chset_8bit &operator ^=(basic_chset_8bit const &x);

    std::bitset<256> const &base() const;

private:
    std::bitset<256> bset_; // BUGBUG range-checking slows this down
};

/////////////////////////////////
template<>
struct basic_chset<char>
  : basic_chset_8bit<char>
{
};

/////////////////////////////////
template<>
struct basic_chset<signed char>
  : basic_chset_8bit<signed char>
{
};

/////////////////////////////////
template<>
struct basic_chset<unsigned char>
  : basic_chset_8bit<unsigned char>
{
};

#endif

///////////////////////////////////////////////////////////////////////////////
// is_narrow_char
template<typename Char>
struct is_narrow_char
  : mpl::false_
{};

template<>
struct is_narrow_char<char>
  : mpl::true_
{};

template<>
struct is_narrow_char<signed char>
  : mpl::true_
{};

template<>
struct is_narrow_char<unsigned char>
  : mpl::true_
{};

///////////////////////////////////////////////////////////////////////////////
// helpers
template<typename Char, typename Traits>
void set_char(basic_chset<Char> &chset, Char ch, Traits const &tr, bool icase);

template<typename Char, typename Traits>
void set_range(basic_chset<Char> &chset, Char from, Char to, Traits const &tr, bool icase);

template<typename Char, typename Traits>
void set_class(basic_chset<Char> &chset, typename Traits::char_class_type char_class, bool no, Traits const &tr);

}}} // namespace boost::xpressive::detail

#endif

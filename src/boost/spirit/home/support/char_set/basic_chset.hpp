/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2001-2009 Daniel Nuffer
    http://spirit.sourceforge.net/

  Distributed under the Boost Software License, Version 1.0. (See accompanying
  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#ifndef BOOST_SPIRIT_BASIC_CHSET_APRIL_17_2008
#define BOOST_SPIRIT_BASIC_CHSET_APRIL_17_2008

#if defined(_MSC_VER)
#pragma once
#endif

///////////////////////////////////////////////////////////////////////////////
#include <bitset>
#include <climits> 
#include <boost/spirit/home/support/char_set/range_run.hpp>

namespace boost { namespace spirit { namespace support { namespace detail
{
    ///////////////////////////////////////////////////////////////////////////
    //
    //  basic_chset: basic character set implementation using range_run
    //
    ///////////////////////////////////////////////////////////////////////////
    template <typename Char>
    struct basic_chset
    {
        basic_chset() {}
        basic_chset(basic_chset const& arg_)
          : rr(arg_.rr) {}

        bool
        test(Char v) const
        {
            return rr.test(v);
        }

        void
        set(Char from, Char to)
        {
            rr.set(range<Char>(from, to));
        }

        void
        set(Char c)
        {
            rr.set(range<Char>(c, c));
        }

        void
        clear(Char from, Char to)
        {
            rr.clear(range<Char>(from, to));
        }

        void
        clear(Char c)
        {
            rr.clear(range<Char>(c, c));
        }

        void
        clear()
        {
            rr.clear();
        }

        void
        inverse()
        {
            basic_chset inv;
            inv.set(
                (std::numeric_limits<Char>::min)(),
                (std::numeric_limits<Char>::max)()
            );
            inv -= *this;
            swap(inv);
        }

        void
        swap(basic_chset& x)
        {
            rr.swap(x.rr);
        }


        basic_chset&
        operator|=(basic_chset const& x)
        {
            typedef typename range_run<Char>::const_iterator const_iterator;
            for (const_iterator iter = x.rr.begin(); iter != x.rr.end(); ++iter)
                rr.set(*iter);
            return *this;
        }

        basic_chset&
        operator&=(basic_chset const& x)
        {
            basic_chset inv;
            inv.set(
                (std::numeric_limits<Char>::min)(),
                (std::numeric_limits<Char>::max)()
            );
            inv -= x;
            *this -= inv;
            return *this;
        }

        basic_chset&
        operator-=(basic_chset const& x)
        {
            typedef typename range_run<Char>::const_iterator const_iterator;
            for (const_iterator iter = x.rr.begin(); iter != x.rr.end(); ++iter)
                rr.clear(*iter);
            return *this;
        }

        basic_chset&
        operator^=(basic_chset const& x)
        {
            basic_chset bma = x;
            bma -= *this;
            *this -= x;
            *this |= bma;
            return *this;
        }

        private: range_run<Char> rr;
    };

#if (CHAR_BIT == 8)

    ///////////////////////////////////////////////////////////////////////////
    //
    //  basic_chset: specializations for 8 bit chars using std::bitset
    //
    ///////////////////////////////////////////////////////////////////////////
    template <typename Char>
    struct basic_chset_8bit
    {
        basic_chset_8bit() {}
        basic_chset_8bit(basic_chset_8bit const& arg_)
          : bset(arg_.bset) {}

        bool
        test(Char v) const
        {
            return bset.test((unsigned char)v);
        }

        void
        set(Char from, Char to)
        {
            for (int i = from; i <= to; ++i)
                bset.set((unsigned char)i);
        }

        void
        set(Char c)
        {
            bset.set((unsigned char)c);
        }

        void
        clear(Char from, Char to)
        {
            for (int i = from; i <= to; ++i)
                bset.reset((unsigned char)i);
        }

        void
        clear(Char c)
        {
            bset.reset((unsigned char)c);
        }

        void
        clear()
        {
            bset.reset();
        }

        void
        inverse()
        {
            bset.flip();
        }

        void
        swap(basic_chset_8bit& x)
        {
            std::swap(bset, x.bset);
        }

        basic_chset_8bit&
        operator|=(basic_chset_8bit const& x)
        {
            bset |= x.bset;
            return *this;
        }

        basic_chset_8bit&
        operator&=(basic_chset_8bit const& x)
        {
            bset &= x.bset;
            return *this;
        }

        basic_chset_8bit&
        operator-=(basic_chset_8bit const& x)
        {
            bset &= ~x.bset;
            return *this;
        }

        basic_chset_8bit&
        operator^=(basic_chset_8bit const& x)
        {
            bset ^= x.bset;
            return *this;
        }

        private: std::bitset<256> bset;
    };

    /////////////////////////////////
    template <>
    struct basic_chset<char>
      : basic_chset_8bit<char> {};

    /////////////////////////////////
    template <>
    struct basic_chset<signed char>
      : basic_chset_8bit<signed char> {};

    /////////////////////////////////
    template <>
    struct basic_chset<unsigned char>
      : basic_chset_8bit<unsigned char> {};

#endif // #if (CHAR_BIT == 8)

}}}}

#endif


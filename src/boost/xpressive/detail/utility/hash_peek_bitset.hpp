///////////////////////////////////////////////////////////////////////////////
// hash_peek_bitset.hpp
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_XPRESSIVE_DETAIL_HASH_PEEK_BITSET_HPP_EAN_10_04_2005
#define BOOST_XPRESSIVE_DETAIL_HASH_PEEK_BITSET_HPP_EAN_10_04_2005

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
# pragma warning(push)
# pragma warning(disable : 4100) // unreferenced formal parameter
# pragma warning(disable : 4127) // conditional expression constant
#endif

#include <bitset>
#include <string> // for std::char_traits
#include <boost/xpressive/detail/utility/chset/basic_chset.ipp>

namespace boost { namespace xpressive { namespace detail
{

///////////////////////////////////////////////////////////////////////////////
// hash_peek_bitset
//
template<typename Char>
struct hash_peek_bitset
{
    typedef Char char_type;
    typedef typename std::char_traits<char_type>::int_type int_type;

    hash_peek_bitset()
      : icase_(false)
      , bset_()
    {
    }

    std::size_t count() const
    {
        return this->bset_.count();
    }

    void set_all()
    {
        this->icase_ = false;
        this->bset_.set();
    }

    template<typename Traits>
    void set_char(char_type ch, bool icase, Traits const &tr)
    {
        if(this->test_icase_(icase))
        {
            ch = icase ? tr.translate_nocase(ch) : tr.translate(ch);
            this->bset_.set(tr.hash(ch));
        }
    }

    template<typename Traits>
    void set_range(char_type from, char_type to, bool no, bool icase, Traits const &tr)
    {
        int_type ifrom = std::char_traits<char_type>::to_int_type(from);
        int_type ito = std::char_traits<char_type>::to_int_type(to);
        BOOST_ASSERT(ifrom <= ito);
        // bound the computational complexity. BUGBUG could set the inverse range
        if(no || 256 < (ito - ifrom))
        {
            this->set_all();
        }
        else if(this->test_icase_(icase))
        {
            for(int_type i = ifrom; i <= ito; ++i)
            {
                char_type ch = std::char_traits<char_type>::to_char_type(i);
                ch = icase ? tr.translate_nocase(ch) : tr.translate(ch);
                this->bset_.set(tr.hash(ch));
            }
        }
    }

    template<typename Traits>
    void set_class(typename Traits::char_class_type char_class, bool no, Traits const &tr)
    {
        if(1 != sizeof(char_type))
        {
            // wide character set, no efficient way of filling in the bitset, so set them all to 1
            this->set_all();
        }
        else
        {
            for(std::size_t i = 0; i <= UCHAR_MAX; ++i)
            {
                char_type ch = std::char_traits<char_type>::to_char_type(static_cast<int_type>(i));
                if(no != tr.isctype(ch, char_class))
                {
                    this->bset_.set(i);
                }
            }
        }
    }

    void set_bitset(hash_peek_bitset<Char> const &that)
    {
        if(this->test_icase_(that.icase()))
        {
            this->bset_ |= that.bset_;
        }
    }

    void set_charset(basic_chset_8bit<Char> const &that, bool icase)
    {
        if(this->test_icase_(icase))
        {
            this->bset_ |= that.base();
        }
    }

    bool icase() const
    {
        return this->icase_;
    }

    template<typename Traits>
    bool test(char_type ch, Traits const &tr) const
    {
        ch = this->icase_ ? tr.translate_nocase(ch) : tr.translate(ch);
        return this->bset_.test(tr.hash(ch));
    }

    template<typename Traits>
    bool test(char_type ch, Traits const &tr, mpl::false_) const
    {
        BOOST_ASSERT(!this->icase_);
        return this->bset_.test(tr.hash(tr.translate(ch)));
    }

    template<typename Traits>
    bool test(char_type ch, Traits const &tr, mpl::true_) const
    {
        BOOST_ASSERT(this->icase_);
        return this->bset_.test(tr.hash(tr.translate_nocase(ch)));
    }

private:

    // Make sure all sub-expressions being merged have the same case-sensitivity
    bool test_icase_(bool icase)
    {
        std::size_t count = this->bset_.count();

        if(256 == count)
        {
            return false; // all set already, nothing to do
        }
        else if(0 != count && this->icase_ != icase)
        {
            this->set_all(); // icase mismatch! set all and bail
            return false;
        }

        this->icase_ = icase;
        return true;
    }

    bool icase_;
    std::bitset<256> bset_;
};

}}} // namespace boost::xpressive::detail

#if defined(_MSC_VER)
# pragma warning(pop)
#endif

#endif

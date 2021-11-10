///////////////////////////////////////////////////////////////////////////////
// chset.hpp
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_XPRESSIVE_DETAIL_CHSET_CHSET_HPP_EAN_10_04_2005
#define BOOST_XPRESSIVE_DETAIL_CHSET_CHSET_HPP_EAN_10_04_2005

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

#include <vector>
#include <boost/call_traits.hpp>
#include <boost/xpressive/detail/detail_fwd.hpp>
#include <boost/xpressive/detail/utility/algorithm.hpp>
#include <boost/xpressive/detail/utility/chset/basic_chset.ipp>

namespace boost { namespace xpressive { namespace detail
{

///////////////////////////////////////////////////////////////////////////////
// compound_charset
//
template<typename Traits>
struct compound_charset
  : private basic_chset<typename Traits::char_type>
{
    typedef typename Traits::char_type char_type;
    typedef basic_chset<char_type> base_type;
    typedef Traits traits_type;
    typedef typename Traits::char_class_type char_class_type;

    compound_charset()
      : base_type()
      , complement_(false)
      , has_posix_(false)
      , posix_yes_()
      , posix_no_()
    {
    }

    ///////////////////////////////////////////////////////////////////////////////
    // accessors
    basic_chset<char_type> const &base() const
    {
        return *this;
    }

    bool is_inverted() const
    {
        return this->complement_;
    }

    char_class_type posix_yes() const
    {
        return this->posix_yes_;
    }

    std::vector<char_class_type> const &posix_no() const
    {
        return this->posix_no_;
    }

    ///////////////////////////////////////////////////////////////////////////////
    // complement
    void inverse()
    {
        this->complement_ = !this->complement_;
    }

    ///////////////////////////////////////////////////////////////////////////////
    // set
    void set_char(char_type ch, Traits const &tr, bool icase)
    {
        icase ? this->base_type::set(ch, tr) : this->base_type::set(ch);
    }

    void set_range(char_type from, char_type to, Traits const &tr, bool icase)
    {
        icase ? this->base_type::set(from, to, tr) : this->base_type::set(from, to);
    }

    void set_class(char_class_type const &m, bool no)
    {
        this->has_posix_ = true;

        if(no)
        {
            this->posix_no_.push_back(m);
        }
        else
        {
            this->posix_yes_ |= m;
        }
    }

    ///////////////////////////////////////////////////////////////////////////////
    // test
    template<typename ICase>
    bool test(char_type ch, Traits const &tr, ICase) const
    {
        return this->complement_ !=
            (this->base_type::test(ch, tr, ICase()) ||
            (this->has_posix_ && this->test_posix(ch, tr)));
    }

private:

    ///////////////////////////////////////////////////////////////////////////////
    // not_posix_pred
    struct not_posix_pred
    {
        char_type ch_;
        Traits const *traits_ptr_;

        bool operator ()(typename call_traits<char_class_type>::param_type m) const
        {
            return !this->traits_ptr_->isctype(this->ch_, m);
        }
    };

    ///////////////////////////////////////////////////////////////////////////////
    // test_posix
    bool test_posix(char_type ch, Traits const &tr) const
    {
        not_posix_pred const pred = {ch, &tr};
        return tr.isctype(ch, this->posix_yes_)
            || any(this->posix_no_.begin(), this->posix_no_.end(), pred);
    }

    bool complement_;
    bool has_posix_;
    char_class_type posix_yes_;
    std::vector<char_class_type> posix_no_;
};


///////////////////////////////////////////////////////////////////////////////
// helpers
template<typename Char, typename Traits>
inline void set_char(compound_charset<Traits> &chset, Char ch, Traits const &tr, bool icase)
{
    chset.set_char(ch, tr, icase);
}

template<typename Char, typename Traits>
inline void set_range(compound_charset<Traits> &chset, Char from, Char to, Traits const &tr, bool icase)
{
    chset.set_range(from, to, tr, icase);
}

template<typename Traits>
inline void set_class(compound_charset<Traits> &chset, typename Traits::char_class_type char_class, bool no, Traits const &)
{
    chset.set_class(char_class, no);
}

}}} // namespace boost::xpressive::detail

#endif


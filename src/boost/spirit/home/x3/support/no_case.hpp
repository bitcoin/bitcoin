/*=============================================================================
    Copyright (c) 2001-2014 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_SPIRIT_X3_SUPPORT_NO_CASE_SEPT_24_2014_1125PM)
#define BOOST_SPIRIT_X3_SUPPORT_NO_CASE_SEPT_24_2014_1125PM

#include <boost/spirit/home/x3/support/unused.hpp>
#include <boost/spirit/home/x3/support/context.hpp>
#include <boost/spirit/home/x3/char/char_class_tags.hpp>

namespace boost { namespace spirit { namespace x3
{
    struct no_case_tag {};

    template <typename Encoding>
    struct case_compare
    {
        template <typename Char, typename CharSet>
        bool in_set(Char ch, CharSet const& set)
        {
            return set.test(ch);
        }

        template <typename Char>
        int32_t operator()(Char lc, Char rc) const
        {
            return lc - rc;
        }

        template <typename CharClassTag>
        CharClassTag get_char_class_tag(CharClassTag tag) const
        {
            return tag;
        }
    };

    template <typename Encoding>
    struct no_case_compare
    {
        template <typename Char, typename CharSet>
        bool in_set(Char ch_, CharSet const& set)
        {
            using char_type = typename Encoding::classify_type;
            auto ch = char_type(ch_);
            return set.test(ch)
                || set.test(Encoding::islower(ch)
                    ? Encoding::toupper(ch) : Encoding::tolower(ch));
        }

        template <typename Char>
        int32_t operator()(Char lc_, Char const rc_) const
        {
            using char_type = typename Encoding::classify_type;
            auto lc = char_type(lc_);
            auto rc = char_type(rc_);
            return Encoding::islower(rc)
                ? Encoding::tolower(lc) - rc : Encoding::toupper(lc) - rc;
        }

        template <typename CharClassTag>
        CharClassTag get_char_class_tag(CharClassTag tag) const
        {
            return tag;
        }

        alpha_tag get_char_class_tag(lower_tag ) const
        {
            return {};
        }

        alpha_tag get_char_class_tag(upper_tag ) const
        {
            return {};
        }

    };

    template <typename Encoding>
    case_compare<Encoding> get_case_compare_impl(unused_type const&)
    {
        return {};
    }

    template <typename Encoding>
    no_case_compare<Encoding> get_case_compare_impl(no_case_tag const&)
    {
        return {};
    }

    template <typename Encoding, typename Context>
    inline decltype(auto) get_case_compare(Context const& context)
    {
        return get_case_compare_impl<Encoding>(x3::get<no_case_tag>(context));
    }

    auto const no_case_compare_ = no_case_tag{};

}}}

#endif

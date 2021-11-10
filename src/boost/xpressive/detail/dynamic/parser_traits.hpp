///////////////////////////////////////////////////////////////////////////////
// detail/dynamic/parser_traits.hpp
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_XPRESSIVE_DETAIL_DYNAMIC_PARSER_TRAITS_HPP_EAN_10_04_2005
#define BOOST_XPRESSIVE_DETAIL_DYNAMIC_PARSER_TRAITS_HPP_EAN_10_04_2005

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

#include <string>
#include <climits>
#include <boost/config.hpp>
#include <boost/assert.hpp>
#include <boost/throw_exception.hpp>
#include <boost/xpressive/regex_error.hpp>
#include <boost/xpressive/regex_traits.hpp>
#include <boost/xpressive/detail/detail_fwd.hpp>
#include <boost/xpressive/detail/dynamic/matchable.hpp>
#include <boost/xpressive/detail/dynamic/parser_enum.hpp>
#include <boost/xpressive/detail/utility/literals.hpp>
#include <boost/xpressive/detail/utility/algorithm.hpp>

namespace boost { namespace xpressive
{

///////////////////////////////////////////////////////////////////////////////
// compiler_traits
//  this works for char and wchar_t. it must be specialized for anything else.
//
template<typename RegexTraits>
struct compiler_traits
{
    typedef RegexTraits regex_traits;
    typedef typename regex_traits::char_type char_type;
    typedef typename regex_traits::string_type string_type;
    typedef typename regex_traits::locale_type locale_type;

    ///////////////////////////////////////////////////////////////////////////////
    // constructor
    explicit compiler_traits(RegexTraits const &traits = RegexTraits())
      : traits_(traits)
      , flags_(regex_constants::ECMAScript)
      , space_(lookup_classname(traits_, "space"))
      , alnum_(lookup_classname(traits_, "alnum"))
    {
    }

    ///////////////////////////////////////////////////////////////////////////////
    // flags
    regex_constants::syntax_option_type flags() const
    {
        return this->flags_;
    }

    ///////////////////////////////////////////////////////////////////////////////
    // flags
    void flags(regex_constants::syntax_option_type flags)
    {
        this->flags_ = flags;
    }

    ///////////////////////////////////////////////////////////////////////////////
    // traits
    regex_traits &traits()
    {
        return this->traits_;
    }

    regex_traits const &traits() const
    {
        return this->traits_;
    }

    ///////////////////////////////////////////////////////////////////////////////
    // imbue
    locale_type imbue(locale_type const &loc)
    {
        locale_type oldloc = this->traits().imbue(loc);
        this->space_ = lookup_classname(this->traits(), "space");
        this->alnum_ = lookup_classname(this->traits(), "alnum");
        return oldloc;
    }

    ///////////////////////////////////////////////////////////////////////////////
    // getloc
    locale_type getloc() const
    {
        return this->traits().getloc();
    }

    ///////////////////////////////////////////////////////////////////////////////
    // get_token
    //  get a token and advance the iterator
    template<typename FwdIter>
    regex_constants::compiler_token_type get_token(FwdIter &begin, FwdIter end)
    {
        using namespace regex_constants;
        if(this->eat_ws_(begin, end) == end)
        {
            return regex_constants::token_end_of_pattern;
        }

        switch(*begin)
        {
        case BOOST_XPR_CHAR_(char_type, '\\'): return this->get_escape_token(++begin, end);
        case BOOST_XPR_CHAR_(char_type, '.'): ++begin; return token_any;
        case BOOST_XPR_CHAR_(char_type, '^'): ++begin; return token_assert_begin_line;
        case BOOST_XPR_CHAR_(char_type, '$'): ++begin; return token_assert_end_line;
        case BOOST_XPR_CHAR_(char_type, '('): ++begin; return token_group_begin;
        case BOOST_XPR_CHAR_(char_type, ')'): ++begin; return token_group_end;
        case BOOST_XPR_CHAR_(char_type, '|'): ++begin; return token_alternate;
        case BOOST_XPR_CHAR_(char_type, '['): ++begin; return token_charset_begin;

        case BOOST_XPR_CHAR_(char_type, '*'):
        case BOOST_XPR_CHAR_(char_type, '+'):
        case BOOST_XPR_CHAR_(char_type, '?'):
            return token_invalid_quantifier;

        case BOOST_XPR_CHAR_(char_type, ']'):
        case BOOST_XPR_CHAR_(char_type, '{'):
        default:
            return token_literal;
        }
    }

    ///////////////////////////////////////////////////////////////////////////////
    // get_quant_spec
    template<typename FwdIter>
    bool get_quant_spec(FwdIter &begin, FwdIter end, detail::quant_spec &spec)
    {
        using namespace regex_constants;
        FwdIter old_begin;

        if(this->eat_ws_(begin, end) == end)
        {
            return false;
        }

        switch(*begin)
        {
        case BOOST_XPR_CHAR_(char_type, '*'):
            spec.min_ = 0;
            spec.max_ = (std::numeric_limits<unsigned int>::max)();
            break;

        case BOOST_XPR_CHAR_(char_type, '+'):
            spec.min_ = 1;
            spec.max_ = (std::numeric_limits<unsigned int>::max)();
            break;

        case BOOST_XPR_CHAR_(char_type, '?'):
            spec.min_ = 0;
            spec.max_ = 1;
            break;

        case BOOST_XPR_CHAR_(char_type, '{'):
            old_begin = this->eat_ws_(++begin, end);
            spec.min_ = spec.max_ = detail::toi(begin, end, this->traits());
            BOOST_XPR_ENSURE_
            (
                begin != old_begin && begin != end, error_brace, "invalid quantifier"
            );

            if(*begin == BOOST_XPR_CHAR_(char_type, ','))
            {
                old_begin = this->eat_ws_(++begin, end);
                spec.max_ = detail::toi(begin, end, this->traits());
                BOOST_XPR_ENSURE_
                (
                    begin != end && BOOST_XPR_CHAR_(char_type, '}') == *begin
                  , error_brace, "invalid quantifier"
                );

                if(begin == old_begin)
                {
                    spec.max_ = (std::numeric_limits<unsigned int>::max)();
                }
                else
                {
                    BOOST_XPR_ENSURE_
                    (
                        spec.min_ <= spec.max_, error_badbrace, "invalid quantification range"
                    );
                }
            }
            else
            {
                BOOST_XPR_ENSURE_
                (
                    BOOST_XPR_CHAR_(char_type, '}') == *begin, error_brace, "invalid quantifier"
                );
            }
            break;

        default:
            return false;
        }

        spec.greedy_ = true;
        if(this->eat_ws_(++begin, end) != end && BOOST_XPR_CHAR_(char_type, '?') == *begin)
        {
            ++begin;
            spec.greedy_ = false;
        }

        return true;
    }

    ///////////////////////////////////////////////////////////////////////////
    // get_group_type
    template<typename FwdIter>
    regex_constants::compiler_token_type get_group_type(FwdIter &begin, FwdIter end, string_type &name)
    {
        using namespace regex_constants;
        if(this->eat_ws_(begin, end) != end && BOOST_XPR_CHAR_(char_type, '?') == *begin)
        {
            this->eat_ws_(++begin, end);
            BOOST_XPR_ENSURE_(begin != end, error_paren, "incomplete extension");

            switch(*begin)
            {
            case BOOST_XPR_CHAR_(char_type, ':'): ++begin; return token_no_mark;
            case BOOST_XPR_CHAR_(char_type, '>'): ++begin; return token_independent_sub_expression;
            case BOOST_XPR_CHAR_(char_type, '#'): ++begin; return token_comment;
            case BOOST_XPR_CHAR_(char_type, '='): ++begin; return token_positive_lookahead;
            case BOOST_XPR_CHAR_(char_type, '!'): ++begin; return token_negative_lookahead;
            case BOOST_XPR_CHAR_(char_type, 'R'): ++begin; return token_recurse;
            case BOOST_XPR_CHAR_(char_type, '$'):
                this->get_name_(++begin, end, name);
                BOOST_XPR_ENSURE_(begin != end, error_paren, "incomplete extension");
                if(BOOST_XPR_CHAR_(char_type, '=') == *begin)
                {
                    ++begin;
                    return token_rule_assign;
                }
                return token_rule_ref;

            case BOOST_XPR_CHAR_(char_type, '<'):
                this->eat_ws_(++begin, end);
                BOOST_XPR_ENSURE_(begin != end, error_paren, "incomplete extension");
                switch(*begin)
                {
                case BOOST_XPR_CHAR_(char_type, '='): ++begin; return token_positive_lookbehind;
                case BOOST_XPR_CHAR_(char_type, '!'): ++begin; return token_negative_lookbehind;
                default:
                    BOOST_THROW_EXCEPTION(regex_error(error_badbrace, "unrecognized extension"));
                }

            case BOOST_XPR_CHAR_(char_type, 'P'):
                this->eat_ws_(++begin, end);
                BOOST_XPR_ENSURE_(begin != end, error_paren, "incomplete extension");
                switch(*begin)
                {
                case BOOST_XPR_CHAR_(char_type, '<'):
                    this->get_name_(++begin, end, name);
                    BOOST_XPR_ENSURE_(begin != end && BOOST_XPR_CHAR_(char_type, '>') == *begin++, error_paren, "incomplete extension");
                    return token_named_mark;
                case BOOST_XPR_CHAR_(char_type, '='):
                    this->get_name_(++begin, end, name);
                    BOOST_XPR_ENSURE_(begin != end, error_paren, "incomplete extension");
                    return token_named_mark_ref;
                default:
                    BOOST_THROW_EXCEPTION(regex_error(error_badbrace, "unrecognized extension"));
                }

            case BOOST_XPR_CHAR_(char_type, 'i'):
            case BOOST_XPR_CHAR_(char_type, 'm'):
            case BOOST_XPR_CHAR_(char_type, 's'):
            case BOOST_XPR_CHAR_(char_type, 'x'):
            case BOOST_XPR_CHAR_(char_type, '-'):
                return this->parse_mods_(begin, end);

            default:
                BOOST_THROW_EXCEPTION(regex_error(error_badbrace, "unrecognized extension"));
            }
        }

        return token_literal;
    }

    //////////////////////////////////////////////////////////////////////////
    // get_charset_token
    //  NOTE: white-space is *never* ignored in a charset.
    template<typename FwdIter>
    regex_constants::compiler_token_type get_charset_token(FwdIter &begin, FwdIter end)
    {
        using namespace regex_constants;
        BOOST_ASSERT(begin != end);
        switch(*begin)
        {
        case BOOST_XPR_CHAR_(char_type, '^'): ++begin; return token_charset_invert;
        case BOOST_XPR_CHAR_(char_type, '-'): ++begin; return token_charset_hyphen;
        case BOOST_XPR_CHAR_(char_type, ']'): ++begin; return token_charset_end;
        case BOOST_XPR_CHAR_(char_type, '['):
            {
                FwdIter next = begin; ++next;
                if(next != end)
                {
                    BOOST_XPR_ENSURE_(
                        *next != BOOST_XPR_CHAR_(char_type, '=')
                      , error_collate
                      , "equivalence classes are not yet supported"
                    );

                    BOOST_XPR_ENSURE_(
                        *next != BOOST_XPR_CHAR_(char_type, '.')
                      , error_collate
                      , "collation sequences are not yet supported"
                    );

                    if(*next == BOOST_XPR_CHAR_(char_type, ':'))
                    {
                        begin = ++next;
                        return token_posix_charset_begin;
                    }
                }
            }
            break;
        case BOOST_XPR_CHAR_(char_type, ':'):
            {
                FwdIter next = begin; ++next;
                if(next != end && *next == BOOST_XPR_CHAR_(char_type, ']'))
                {
                    begin = ++next;
                    return token_posix_charset_end;
                }
            }
            break;
        case BOOST_XPR_CHAR_(char_type, '\\'):
            if(++begin != end)
            {
                switch(*begin)
                {
                case BOOST_XPR_CHAR_(char_type, 'b'): ++begin; return token_charset_backspace;
                default:;
                }
            }
            return token_escape;
        default:;
        }
        return token_literal;
    }

    //////////////////////////////////////////////////////////////////////////
    // get_escape_token
    template<typename FwdIter>
    regex_constants::compiler_token_type get_escape_token(FwdIter &begin, FwdIter end)
    {
        using namespace regex_constants;
        if(begin != end)
        {
            switch(*begin)
            {
            //case BOOST_XPR_CHAR_(char_type, 'a'): ++begin; return token_escape_bell;
            //case BOOST_XPR_CHAR_(char_type, 'c'): ++begin; return token_escape_control;
            //case BOOST_XPR_CHAR_(char_type, 'e'): ++begin; return token_escape_escape;
            //case BOOST_XPR_CHAR_(char_type, 'f'): ++begin; return token_escape_formfeed;
            //case BOOST_XPR_CHAR_(char_type, 'n'): ++begin; return token_escape_newline;
            //case BOOST_XPR_CHAR_(char_type, 't'): ++begin; return token_escape_horizontal_tab;
            //case BOOST_XPR_CHAR_(char_type, 'v'): ++begin; return token_escape_vertical_tab;
            case BOOST_XPR_CHAR_(char_type, 'A'): ++begin; return token_assert_begin_sequence;
            case BOOST_XPR_CHAR_(char_type, 'b'): ++begin; return token_assert_word_boundary;
            case BOOST_XPR_CHAR_(char_type, 'B'): ++begin; return token_assert_not_word_boundary;
            case BOOST_XPR_CHAR_(char_type, 'E'): ++begin; return token_quote_meta_end;
            case BOOST_XPR_CHAR_(char_type, 'Q'): ++begin; return token_quote_meta_begin;
            case BOOST_XPR_CHAR_(char_type, 'Z'): ++begin; return token_assert_end_sequence;
            // Non-standard extension to ECMAScript syntax
            case BOOST_XPR_CHAR_(char_type, '<'): ++begin; return token_assert_word_begin;
            case BOOST_XPR_CHAR_(char_type, '>'): ++begin; return token_assert_word_end;
            default:; // fall-through
            }
        }

        return token_escape;
    }

private:

    //////////////////////////////////////////////////////////////////////////
    // parse_mods_
    template<typename FwdIter>
    regex_constants::compiler_token_type parse_mods_(FwdIter &begin, FwdIter end)
    {
        using namespace regex_constants;
        bool set = true;
        do switch(*begin)
        {
        case BOOST_XPR_CHAR_(char_type, 'i'): this->flag_(set, icase_); break;
        case BOOST_XPR_CHAR_(char_type, 'm'): this->flag_(!set, single_line); break;
        case BOOST_XPR_CHAR_(char_type, 's'): this->flag_(!set, not_dot_newline); break;
        case BOOST_XPR_CHAR_(char_type, 'x'): this->flag_(set, ignore_white_space); break;
        case BOOST_XPR_CHAR_(char_type, ':'): ++begin; BOOST_FALLTHROUGH;
        case BOOST_XPR_CHAR_(char_type, ')'): return token_no_mark;
        case BOOST_XPR_CHAR_(char_type, '-'): if(false == (set = !set)) break; BOOST_FALLTHROUGH;
        default: BOOST_THROW_EXCEPTION(regex_error(error_paren, "unknown pattern modifier"));
        }
        while(BOOST_XPR_ENSURE_(++begin != end, error_paren, "incomplete extension"));
        // this return is technically unreachable, but this must
        // be here to work around a bug in gcc 4.0
        return token_no_mark;
    }

    ///////////////////////////////////////////////////////////////////////////////
    // flag_
    void flag_(bool set, regex_constants::syntax_option_type flag)
    {
        this->flags_ = set ? (this->flags_ | flag) : (this->flags_ & ~flag);
    }

    ///////////////////////////////////////////////////////////////////////////
    // is_space_
    bool is_space_(char_type ch) const
    {
        return 0 != this->space_ && this->traits().isctype(ch, this->space_);
    }

    ///////////////////////////////////////////////////////////////////////////
    // is_alnum_
    bool is_alnum_(char_type ch) const
    {
        return 0 != this->alnum_ && this->traits().isctype(ch, this->alnum_);
    }

    ///////////////////////////////////////////////////////////////////////////
    // get_name_
    template<typename FwdIter>
    void get_name_(FwdIter &begin, FwdIter end, string_type &name)
    {
        this->eat_ws_(begin, end);
        for(name.clear(); begin != end && this->is_alnum_(*begin); ++begin)
        {
            name.push_back(*begin);
        }
        this->eat_ws_(begin, end);
        BOOST_XPR_ENSURE_(!name.empty(), regex_constants::error_paren, "incomplete extension");
    }

    ///////////////////////////////////////////////////////////////////////////////
    // eat_ws_
    template<typename FwdIter>
    FwdIter &eat_ws_(FwdIter &begin, FwdIter end)
    {
        if(0 != (regex_constants::ignore_white_space & this->flags()))
        {
            while(end != begin && (BOOST_XPR_CHAR_(char_type, '#') == *begin || this->is_space_(*begin)))
            {
                if(BOOST_XPR_CHAR_(char_type, '#') == *begin++)
                {
                    while(end != begin && BOOST_XPR_CHAR_(char_type, '\n') != *begin++) {}
                }
                else
                {
                    for(; end != begin && this->is_space_(*begin); ++begin) {}
                }
            }
        }

        return begin;
    }

    regex_traits traits_;
    regex_constants::syntax_option_type flags_;
    typename regex_traits::char_class_type space_;
    typename regex_traits::char_class_type alnum_;
};

}} // namespace boost::xpressive

#endif

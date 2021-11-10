///////////////////////////////////////////////////////////////////////////////
// detail_fwd.hpp
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_XPRESSIVE_DETAIL_DETAIL_FWD_HPP_EAN_10_04_2005
#define BOOST_XPRESSIVE_DETAIL_DETAIL_FWD_HPP_EAN_10_04_2005

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

#include <map>
#include <string>
#include <vector>
#include <climits> // for INT_MAX
#include <typeinfo>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/size_t.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/xpressive/xpressive_fwd.hpp>

namespace boost { namespace xpressive { namespace detail
{
    typedef unsigned int uint_t;

    template<uint_t Min, uint_t Max = Min>
    struct generic_quant_tag;

    struct modifier_tag;

    struct check_tag;

    typedef mpl::size_t<INT_MAX / 2 - 1> unknown_width;

    struct type_info_less;

    typedef std::map<std::type_info const *, void *, type_info_less> action_args_type;

    struct action_context;

    struct ReplaceAlgo;

    ///////////////////////////////////////////////////////////////////////////////
    // placeholders
    //
    struct mark_placeholder;

    struct posix_charset_placeholder;

    template<typename Cond>
    struct assert_word_placeholder;

    template<typename Char>
    struct range_placeholder;

    struct assert_bol_placeholder;

    struct assert_eol_placeholder;

    struct logical_newline_placeholder;

    struct self_placeholder;

    template<typename Nbr>
    struct attribute_placeholder;

    ///////////////////////////////////////////////////////////////////////////////
    // matchers
    //
    struct end_matcher;

    struct independent_end_matcher;

    struct assert_bos_matcher;

    struct assert_eos_matcher;

    template<typename Traits>
    struct assert_bol_matcher;

    template<typename Traits>
    struct assert_eol_matcher;

    template<typename Cond, typename Traits>
    struct assert_word_matcher;

    struct true_matcher;

    template<typename Alternates, typename Traits>
    struct alternate_matcher;

    struct alternate_end_matcher;

    template<typename Traits>
    struct posix_charset_matcher;

    template<typename BidiIter>
    struct sequence;

    template<typename Traits, typename ICase>
    struct mark_matcher;

    struct mark_begin_matcher;

    struct mark_end_matcher;

    template<typename BidiIter>
    struct regex_matcher;

    template<typename BidiIter>
    struct regex_byref_matcher;

    template<typename Traits>
    struct compound_charset;

    template<typename Traits, typename ICase, typename CharSet = compound_charset<Traits> >
    struct charset_matcher;

    template<typename Traits, typename ICase>
    struct range_matcher;

    template<typename Traits, typename Size>
    struct set_matcher;

    template<typename Xpr, typename Greedy>
    struct simple_repeat_matcher;

    struct repeat_begin_matcher;

    template<typename Greedy>
    struct repeat_end_matcher;

    template<typename Traits, typename ICase, typename Not>
    struct literal_matcher;

    template<typename Traits, typename ICase>
    struct string_matcher;

    template<typename Actor>
    struct action_matcher;

    template<typename Predicate>
    struct predicate_matcher;

    template<typename Xpr, typename Greedy>
    struct optional_matcher;

    template<typename Xpr, typename Greedy>
    struct optional_mark_matcher;

    template<typename Matcher, typename Traits, typename ICase>
    struct attr_matcher;

    template<typename Nbr>
    struct attr_begin_matcher;

    struct attr_end_matcher;

    template<typename Xpr>
    struct is_modifiable;

    template<typename Head, typename Tail>
    struct alternates_list;

    template<typename Modifier>
    struct modifier_op;

    struct icase_modifier;

    template<typename BidiIter, typename ICase, typename Traits>
    struct xpression_visitor;

    template<typename BidiIter>
    struct regex_impl;

    struct epsilon_matcher;

    template<typename BidiIter>
    struct nested_results;

    template<typename BidiIter>
    struct regex_id_filter_predicate;

    template<typename Xpr>
    struct keeper_matcher;

    template<typename Xpr>
    struct lookahead_matcher;

    template<typename Xpr>
    struct lookbehind_matcher;

    template<typename IsBoundary>
    struct word_boundary;

    template<typename BidiIter, typename Matcher>
    sequence<BidiIter> make_dynamic(Matcher const &matcher);

    template<typename Char>
    struct xpression_linker;

    template<typename Char>
    struct xpression_peeker;

    struct any_matcher;

    template<typename Traits>
    struct logical_newline_matcher;

    typedef proto::expr<proto::tag::terminal, proto::term<logical_newline_placeholder>, 0> logical_newline_xpression;

    struct set_initializer;

    typedef proto::expr<proto::tag::terminal, proto::term<set_initializer>, 0> set_initializer_type;

    struct lookahead_tag;

    struct lookbehind_tag;

    struct keeper_tag;

    template<typename Locale>
    struct locale_modifier;

    template<typename Matcher>
    struct matcher_wrapper;

    template<typename Locale, typename BidiIter>
    struct regex_traits_type;

    template<typename Expr>
    struct let_;

    template<typename Args, typename BidiIter>
    void bind_args(let_<Args> const &, match_results<BidiIter> &);

    ///////////////////////////////////////////////////////////////////////////////
    // Misc.
    struct no_next;

    template<typename BidiIter>
    struct core_access;

    template<typename BidiIter>
    struct match_state;

    template<typename BidiIter>
    struct matchable;

    template<typename BidiIter>
    struct matchable_ex;

    template<typename Matcher, typename BidiIter>
    struct dynamic_xpression;

    template<typename BidiIter>
    struct shared_matchable;

    template<typename BidiIter>
    struct alternates_vector;

    template<typename Matcher, typename Next>
    struct static_xpression;

    typedef static_xpression<end_matcher, no_next> end_xpression;

    typedef static_xpression<alternate_end_matcher, no_next> alternate_end_xpression;

    typedef static_xpression<independent_end_matcher, no_next> independent_end_xpression;

    typedef static_xpression<true_matcher, no_next> true_xpression;

    template<typename Matcher, typename Next = end_xpression>
    struct static_xpression;

    template<typename Top, typename Next>
    struct stacked_xpression;

    template<typename Xpr>
    struct is_static_xpression;

    template<typename BidiIter>
    struct sub_match_impl;

    template<typename T>
    struct list;

    template<typename BidiIter>
    struct results_cache;

    template<typename T>
    struct sequence_stack;

    template<typename BidiIter>
    struct results_extras;

    template<typename BidiIter>
    struct match_context;

    template<typename BidiIter>
    struct sub_match_vector;

    template<typename T, typename U>
    struct action_arg;

    struct actionable;

    template<typename Char>
    struct traits;

    template<typename Traits, typename BidiIter>
    Traits const &traits_cast(match_state<BidiIter> const &state);

    template<typename Char>
    struct basic_chset;

    template<typename Char>
    struct named_mark;

    template<typename BidiIter>
    struct memento;

    template<typename Char, typename Traits>
    void set_char(compound_charset<Traits> &chset, Char ch, Traits const &tr, bool icase);

    template<typename Char, typename Traits>
    void set_range(compound_charset<Traits> &chset, Char from, Char to, Traits const &tr, bool icase);

    template<typename Traits>
    void set_class(compound_charset<Traits> &chset, typename Traits::char_class_type char_class, bool no, Traits const &tr);

    template<typename Char, typename Traits>
    void set_char(basic_chset<Char> &chset, Char ch, Traits const &tr, bool icase);

    template<typename Char, typename Traits>
    void set_range(basic_chset<Char> &chset, Char from, Char to, Traits const &tr, bool icase);

    template<typename Char, typename Traits>
    void set_class(basic_chset<Char> &chset, typename Traits::char_class_type char_class, bool no, Traits const &tr);

    template<typename Matcher>
    static_xpression<Matcher> const
    make_static(Matcher const &matcher);

    template<typename Matcher, typename Next>
    static_xpression<Matcher, Next> const
    make_static(Matcher const &matcher, Next const &next);

    int get_mark_number(basic_mark_tag const &);

    template<typename Xpr, typename BidiIter>
    void static_compile(Xpr const &xpr, shared_ptr<regex_impl<BidiIter> > const &impl);

    struct quant_spec;

    template<typename BidiIter, typename Xpr>
    void make_simple_repeat(quant_spec const &spec, sequence<BidiIter> &seq, Xpr const &xpr);

    template<typename BidiIter>
    void make_simple_repeat(quant_spec const &spec, sequence<BidiIter> &seq);

    template<typename BidiIter>
    void make_repeat(quant_spec const &spec, sequence<BidiIter> &seq, int mark_nbr);

    template<typename BidiIter>
    void make_repeat(quant_spec const &spec, sequence<BidiIter> &seq);

    template<typename BidiIter>
    void make_optional(quant_spec const &spec, sequence<BidiIter> &seq);

    template<typename BidiIter>
    void make_optional(quant_spec const &spec, sequence<BidiIter> &seq, int mark_nbr);

    template<typename Char>
    struct string_type
    {
        typedef std::vector<Char> type;
    };

    template<>
    struct string_type<char>
    {
        typedef std::string type;
    };

    #ifndef BOOST_XPRESSIVE_NO_WREGEX
    template<>
    struct string_type<wchar_t>
    {
        typedef std::wstring type;
    };
    #endif

}}} // namespace boost::xpressive::detail

namespace boost { namespace xpressive { namespace grammar_detail
{
    using proto::_;
    using proto::or_;
    using proto::if_;
    using proto::call;
    using proto::when;
    using proto::otherwise;
    using proto::switch_;
    using proto::make;
    using proto::_child;
    using proto::_value;
    using proto::_left;
    using proto::_right;
    using proto::not_;
    using proto::_state;
    using proto::_data;
    using proto::callable;
    using proto::transform;
    using proto::fold;
    using proto::reverse_fold;
    using proto::fold_tree;
    using proto::reverse_fold_tree;
    using proto::terminal;
    using proto::shift_right;
    using proto::bitwise_or;
    using proto::logical_not;
    using proto::dereference;
    using proto::unary_plus;
    using proto::negate;
    using proto::complement;
    using proto::comma;
    using proto::assign;
    using proto::subscript;
    using proto::nary_expr;
    using proto::unary_expr;
    using proto::binary_expr;
    using proto::_deep_copy;
    using proto::vararg;
    namespace tag = proto::tag;
}}}

namespace boost { namespace xpressive { namespace op
{
    struct push;
    struct push_back;
    struct pop;
    struct push_front;
    struct pop_back;
    struct pop_front;
    struct back;
    struct front;
    struct top;
    struct first;
    struct second;
    struct matched;
    struct length;
    struct str;
    struct insert;
    struct make_pair;

    template<typename T>
    struct as;
    template<typename T>
    struct static_cast_;
    template<typename T>
    struct dynamic_cast_;
    template<typename T>
    struct const_cast_;
    template<typename T>
    struct construct;
    template<typename T>
    struct throw_;
}}} // namespace boost::xpressive::op

/// INTERNAL ONLY
namespace boost { namespace xpressive
{

    /// INTERNAL ONLY
    template<typename Traits, std::size_t N>
    typename Traits::char_class_type
    lookup_classname(Traits const &traits, char const (&cname)[N], bool icase = false);

}} // namespace boost::xpressive

#endif

///////////////////////////////////////////////////////////////////////////////
/// \file regex_primitives.hpp
/// Contains the syntax elements for writing static regular expressions.
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_XPRESSIVE_REGEX_PRIMITIVES_HPP_EAN_10_04_2005
#define BOOST_XPRESSIVE_REGEX_PRIMITIVES_HPP_EAN_10_04_2005

#include <vector>
#include <climits>
#include <boost/config.hpp>
#include <boost/assert.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/and.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/detail/workaround.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/xpressive/detail/detail_fwd.hpp>
#include <boost/xpressive/detail/core/matchers.hpp>
#include <boost/xpressive/detail/core/regex_domain.hpp>
#include <boost/xpressive/detail/utility/ignore_unused.hpp>

// Doxygen can't handle proto :-(
#ifndef BOOST_XPRESSIVE_DOXYGEN_INVOKED
# include <boost/proto/core.hpp>
# include <boost/proto/transform/arg.hpp>
# include <boost/proto/transform/when.hpp>
# include <boost/xpressive/detail/core/icase.hpp>
# include <boost/xpressive/detail/static/compile.hpp>
# include <boost/xpressive/detail/static/modifier.hpp>
#endif

namespace boost { namespace xpressive { namespace detail
{

    typedef assert_word_placeholder<word_boundary<mpl::true_> > assert_word_boundary;
    typedef assert_word_placeholder<word_begin> assert_word_begin;
    typedef assert_word_placeholder<word_end> assert_word_end;

    // workaround msvc-7.1 bug with function pointer types
    // within function types:
    #if BOOST_WORKAROUND(BOOST_MSVC, == 1310)
    #define mark_number(x) proto::call<mark_number(x)>
    #define minus_one() proto::make<minus_one()>
    #endif

    struct push_back : proto::callable
    {
        typedef int result_type;

        template<typename Subs>
        int operator ()(Subs &subs, int i) const
        {
            subs.push_back(i);
            return i;
        }
    };

    struct mark_number : proto::callable
    {
        typedef int result_type;

        template<typename Expr>
        int operator ()(Expr const &expr) const
        {
            return expr.mark_number_;
        }
    };

    typedef mpl::int_<-1> minus_one;

    // s1 or -s1
    struct SubMatch
      : proto::or_<
            proto::when<basic_mark_tag,                push_back(proto::_data, mark_number(proto::_value))   >
          , proto::when<proto::negate<basic_mark_tag>, push_back(proto::_data, minus_one())                  >
        >
    {};

    struct SubMatchList
      : proto::or_<SubMatch, proto::comma<SubMatchList, SubMatch> >
    {};

    template<typename Subs>
    typename enable_if<
        mpl::and_<proto::is_expr<Subs>, proto::matches<Subs, SubMatchList> >
      , std::vector<int>
    >::type
    to_vector(Subs const &subs)
    {
        std::vector<int> subs_;
        SubMatchList()(subs, 0, subs_);
        return subs_;
    }

    #if BOOST_WORKAROUND(BOOST_MSVC, == 1310)
    #undef mark_number
    #undef minus_one
    #endif

    // replace "Expr" with "keep(*State) >> Expr"
    struct skip_primitives : proto::transform<skip_primitives>
    {
        template<typename Expr, typename State, typename Data>
        struct impl : proto::transform_impl<Expr, State, Data>
        {
            typedef
                typename proto::shift_right<
                    typename proto::unary_expr<
                        keeper_tag
                      , typename proto::dereference<State>::type
                    >::type
                  , Expr
                >::type
            result_type;

            result_type operator ()(
                typename impl::expr_param expr
              , typename impl::state_param state
              , typename impl::data_param
            ) const
            {
                result_type that = {{{state}}, expr};
                return that;
            }
        };
    };

    struct Primitives
      : proto::or_<
            proto::terminal<proto::_>
          , proto::comma<proto::_, proto::_>
          , proto::subscript<proto::terminal<set_initializer>, proto::_>
          , proto::assign<proto::terminal<set_initializer>, proto::_>
          , proto::assign<proto::terminal<attribute_placeholder<proto::_> >, proto::_>
          , proto::complement<Primitives>
        >
    {};

    struct SkipGrammar
      : proto::or_<
            proto::when<Primitives, skip_primitives>
          , proto::assign<proto::terminal<mark_placeholder>, SkipGrammar>   // don't "skip" mark tags
          , proto::subscript<SkipGrammar, proto::_>                         // don't put skips in actions
          , proto::binary_expr<modifier_tag, proto::_, SkipGrammar>         // don't skip modifiers
          , proto::unary_expr<lookbehind_tag, proto::_>                     // don't skip lookbehinds
          , proto::nary_expr<proto::_, proto::vararg<SkipGrammar> >         // everything else is fair game!
        >
    {};

    template<typename Skip>
    struct skip_directive
    {
        typedef typename proto::result_of::as_expr<Skip>::type skip_type;

        skip_directive(Skip const &skip)
          : skip_(proto::as_expr(skip))
        {}

        template<typename Sig>
        struct result {};

        template<typename This, typename Expr>
        struct result<This(Expr)>
        {
            typedef
                SkipGrammar::impl<
                    typename proto::result_of::as_expr<Expr>::type
                  , skip_type const &
                  , mpl::void_ &
                >
            skip_transform;

            typedef
                typename proto::shift_right<
                    typename skip_transform::result_type
                  , typename proto::dereference<skip_type>::type
                >::type
            type;
        };

        template<typename Expr>
        typename result<skip_directive(Expr)>::type
        operator ()(Expr const &expr) const
        {
            mpl::void_ ignore;
            typedef result<skip_directive(Expr)> result_fun;
            typename result_fun::type that = {
                typename result_fun::skip_transform()(proto::as_expr(expr), this->skip_, ignore)
              , {skip_}
            };
            return that;
        }

    private:
        skip_type skip_;
    };

/*
///////////////////////////////////////////////////////////////////////////////
/// INTERNAL ONLY
// BOOST_XPRESSIVE_GLOBAL
//  for defining globals that neither violate the One Definition Rule nor
//  lead to undefined behavior due to global object initialization order.
//#define BOOST_XPRESSIVE_GLOBAL(type, name, init)                                        \
//    namespace detail                                                                    \
//    {                                                                                   \
//        template<int Dummy>                                                             \
//        struct BOOST_PP_CAT(global_pod_, name)                                          \
//        {                                                                               \
//            static type const value;                                                    \
//        private:                                                                        \
//            union type_must_be_pod                                                      \
//            {                                                                           \
//                type t;                                                                 \
//                char ch;                                                                \
//            } u;                                                                        \
//        };                                                                              \
//        template<int Dummy>                                                             \
//        type const BOOST_PP_CAT(global_pod_, name)<Dummy>::value = init;                \
//    }                                                                                   \
//    type const &name = detail::BOOST_PP_CAT(global_pod_, name)<0>::value
*/


} // namespace detail

/// INTERNAL ONLY (for backwards compatibility)
unsigned int const repeat_max = UINT_MAX-1;

///////////////////////////////////////////////////////////////////////////////
/// \brief For infinite repetition of a sub-expression.
///
/// Magic value used with the repeat\<\>() function template
/// to specify an unbounded repeat. Use as: repeat<17, inf>('a').
/// The equivalent in perl is /a{17,}/.
unsigned int const inf = UINT_MAX-1;

/// INTERNAL ONLY (for backwards compatibility)
proto::terminal<detail::epsilon_matcher>::type const epsilon = {{}};

///////////////////////////////////////////////////////////////////////////////
/// \brief Successfully matches nothing.
///
/// Successfully matches a zero-width sequence. nil always succeeds and
/// never consumes any characters.
proto::terminal<detail::epsilon_matcher>::type const nil = {{}};

///////////////////////////////////////////////////////////////////////////////
/// \brief Matches an alpha-numeric character.
///
/// The regex traits are used to determine which characters are alpha-numeric.
/// To match any character that is not alpha-numeric, use ~alnum.
///
/// \attention alnum is equivalent to /[[:alnum:]]/ in perl. ~alnum is equivalent
/// to /[[:^alnum:]]/ in perl.
proto::terminal<detail::posix_charset_placeholder>::type const alnum = {{"alnum", false}};

///////////////////////////////////////////////////////////////////////////////
/// \brief Matches an alphabetic character.
///
/// The regex traits are used to determine which characters are alphabetic.
/// To match any character that is not alphabetic, use ~alpha.
///
/// \attention alpha is equivalent to /[[:alpha:]]/ in perl. ~alpha is equivalent
/// to /[[:^alpha:]]/ in perl.
proto::terminal<detail::posix_charset_placeholder>::type const alpha = {{"alpha", false}};

///////////////////////////////////////////////////////////////////////////////
/// \brief Matches a blank (horizonal white-space) character.
///
/// The regex traits are used to determine which characters are blank characters.
/// To match any character that is not blank, use ~blank.
///
/// \attention blank is equivalent to /[[:blank:]]/ in perl. ~blank is equivalent
/// to /[[:^blank:]]/ in perl.
proto::terminal<detail::posix_charset_placeholder>::type const blank = {{"blank", false}};

///////////////////////////////////////////////////////////////////////////////
/// \brief Matches a control character.
///
/// The regex traits are used to determine which characters are control characters.
/// To match any character that is not a control character, use ~cntrl.
///
/// \attention cntrl is equivalent to /[[:cntrl:]]/ in perl. ~cntrl is equivalent
/// to /[[:^cntrl:]]/ in perl.
proto::terminal<detail::posix_charset_placeholder>::type const cntrl = {{"cntrl", false}};

///////////////////////////////////////////////////////////////////////////////
/// \brief Matches a digit character.
///
/// The regex traits are used to determine which characters are digits.
/// To match any character that is not a digit, use ~digit.
///
/// \attention digit is equivalent to /[[:digit:]]/ in perl. ~digit is equivalent
/// to /[[:^digit:]]/ in perl.
proto::terminal<detail::posix_charset_placeholder>::type const digit = {{"digit", false}};

///////////////////////////////////////////////////////////////////////////////
/// \brief Matches a graph character.
///
/// The regex traits are used to determine which characters are graphable.
/// To match any character that is not graphable, use ~graph.
///
/// \attention graph is equivalent to /[[:graph:]]/ in perl. ~graph is equivalent
/// to /[[:^graph:]]/ in perl.
proto::terminal<detail::posix_charset_placeholder>::type const graph = {{"graph", false}};

///////////////////////////////////////////////////////////////////////////////
/// \brief Matches a lower-case character.
///
/// The regex traits are used to determine which characters are lower-case.
/// To match any character that is not a lower-case character, use ~lower.
///
/// \attention lower is equivalent to /[[:lower:]]/ in perl. ~lower is equivalent
/// to /[[:^lower:]]/ in perl.
proto::terminal<detail::posix_charset_placeholder>::type const lower = {{"lower", false}};

///////////////////////////////////////////////////////////////////////////////
/// \brief Matches a printable character.
///
/// The regex traits are used to determine which characters are printable.
/// To match any character that is not printable, use ~print.
///
/// \attention print is equivalent to /[[:print:]]/ in perl. ~print is equivalent
/// to /[[:^print:]]/ in perl.
proto::terminal<detail::posix_charset_placeholder>::type const print = {{"print", false}};

///////////////////////////////////////////////////////////////////////////////
/// \brief Matches a punctuation character.
///
/// The regex traits are used to determine which characters are punctuation.
/// To match any character that is not punctuation, use ~punct.
///
/// \attention punct is equivalent to /[[:punct:]]/ in perl. ~punct is equivalent
/// to /[[:^punct:]]/ in perl.
proto::terminal<detail::posix_charset_placeholder>::type const punct = {{"punct", false}};

///////////////////////////////////////////////////////////////////////////////
/// \brief Matches a space character.
///
/// The regex traits are used to determine which characters are space characters.
/// To match any character that is not white-space, use ~space.
///
/// \attention space is equivalent to /[[:space:]]/ in perl. ~space is equivalent
/// to /[[:^space:]]/ in perl.
proto::terminal<detail::posix_charset_placeholder>::type const space = {{"space", false}};

///////////////////////////////////////////////////////////////////////////////
/// \brief Matches an upper-case character.
///
/// The regex traits are used to determine which characters are upper-case.
/// To match any character that is not upper-case, use ~upper.
///
/// \attention upper is equivalent to /[[:upper:]]/ in perl. ~upper is equivalent
/// to /[[:^upper:]]/ in perl.
proto::terminal<detail::posix_charset_placeholder>::type const upper = {{"upper", false}};

///////////////////////////////////////////////////////////////////////////////
/// \brief Matches a hexadecimal digit character.
///
/// The regex traits are used to determine which characters are hex digits.
/// To match any character that is not a hex digit, use ~xdigit.
///
/// \attention xdigit is equivalent to /[[:xdigit:]]/ in perl. ~xdigit is equivalent
/// to /[[:^xdigit:]]/ in perl.
proto::terminal<detail::posix_charset_placeholder>::type const xdigit = {{"xdigit", false}};

///////////////////////////////////////////////////////////////////////////////
/// \brief Beginning of sequence assertion.
///
/// For the character sequence [begin, end), 'bos' matches the
/// zero-width sub-sequence [begin, begin).
proto::terminal<detail::assert_bos_matcher>::type const bos = {{}};

///////////////////////////////////////////////////////////////////////////////
/// \brief End of sequence assertion.
///
/// For the character sequence [begin, end),
/// 'eos' matches the zero-width sub-sequence [end, end).
///
/// \attention Unlike the perl end of sequence assertion \$, 'eos' will
/// not match at the position [end-1, end-1) if *(end-1) is '\\n'. To
/// get that behavior, use (!_n >> eos).
proto::terminal<detail::assert_eos_matcher>::type const eos = {{}};

///////////////////////////////////////////////////////////////////////////////
/// \brief Beginning of line assertion.
///
/// 'bol' matches the zero-width sub-sequence
/// immediately following a logical newline sequence. The regex traits
/// is used to determine what constitutes a logical newline sequence.
proto::terminal<detail::assert_bol_placeholder>::type const bol = {{}};

///////////////////////////////////////////////////////////////////////////////
/// \brief End of line assertion.
///
/// 'eol' matches the zero-width sub-sequence
/// immediately preceeding a logical newline sequence. The regex traits
/// is used to determine what constitutes a logical newline sequence.
proto::terminal<detail::assert_eol_placeholder>::type const eol = {{}};

///////////////////////////////////////////////////////////////////////////////
/// \brief Beginning of word assertion.
///
/// 'bow' matches the zero-width sub-sequence
/// immediately following a non-word character and preceeding a word character.
/// The regex traits are used to determine what constitutes a word character.
proto::terminal<detail::assert_word_begin>::type const bow = {{}};

///////////////////////////////////////////////////////////////////////////////
/// \brief End of word assertion.
///
/// 'eow' matches the zero-width sub-sequence
/// immediately following a word character and preceeding a non-word character.
/// The regex traits are used to determine what constitutes a word character.
proto::terminal<detail::assert_word_end>::type const eow = {{}};

///////////////////////////////////////////////////////////////////////////////
/// \brief Word boundary assertion.
///
/// '_b' matches the zero-width sub-sequence at the beginning or the end of a word.
/// It is equivalent to (bow | eow). The regex traits are used to determine what
/// constitutes a word character. To match a non-word boundary, use ~_b.
///
/// \attention _b is like \\b in perl. ~_b is like \\B in perl.
proto::terminal<detail::assert_word_boundary>::type const _b = {{}};

///////////////////////////////////////////////////////////////////////////////
/// \brief Matches a word character.
///
/// '_w' matches a single word character. The regex traits are used to determine which
/// characters are word characters. Use ~_w to match a character that is not a word
/// character.
///
/// \attention _w is like \\w in perl. ~_w is like \\W in perl.
proto::terminal<detail::posix_charset_placeholder>::type const _w = {{"w", false}};

///////////////////////////////////////////////////////////////////////////////
/// \brief Matches a digit character.
///
/// '_d' matches a single digit character. The regex traits are used to determine which
/// characters are digits. Use ~_d to match a character that is not a digit
/// character.
///
/// \attention _d is like \\d in perl. ~_d is like \\D in perl.
proto::terminal<detail::posix_charset_placeholder>::type const _d = {{"d", false}};

///////////////////////////////////////////////////////////////////////////////
/// \brief Matches a space character.
///
/// '_s' matches a single space character. The regex traits are used to determine which
/// characters are space characters. Use ~_s to match a character that is not a space
/// character.
///
/// \attention _s is like \\s in perl. ~_s is like \\S in perl.
proto::terminal<detail::posix_charset_placeholder>::type const _s = {{"s", false}};

///////////////////////////////////////////////////////////////////////////////
/// \brief Matches a literal newline character, '\\n'.
///
/// '_n' matches a single newline character, '\\n'. Use ~_n to match a character
/// that is not a newline.
///
/// \attention ~_n is like '.' in perl without the /s modifier.
proto::terminal<char>::type const _n = {'\n'};

///////////////////////////////////////////////////////////////////////////////
/// \brief Matches a logical newline sequence.
///
/// '_ln' matches a logical newline sequence. This can be any character in the
/// line separator class, as determined by the regex traits, or the '\\r\\n' sequence.
/// For the purpose of back-tracking, '\\r\\n' is treated as a unit.
/// To match any one character that is not a logical newline, use ~_ln.
detail::logical_newline_xpression const _ln = {{}};

///////////////////////////////////////////////////////////////////////////////
/// \brief Matches any one character.
///
/// Match any character, similar to '.' in perl syntax with the /s modifier.
/// '_' matches any one character, including the newline.
///
/// \attention To match any character except the newline, use ~_n
proto::terminal<detail::any_matcher>::type const _ = {{}};

///////////////////////////////////////////////////////////////////////////////
/// \brief Reference to the current regex object
///
/// Useful when constructing recursive regular expression objects. The 'self'
/// identifier is a short-hand for the current regex object. For instance,
/// sregex rx = '(' >> (self | nil) >> ')'; will create a regex object that
/// matches balanced parens such as "((()))".
proto::terminal<detail::self_placeholder>::type const self = {{}};

///////////////////////////////////////////////////////////////////////////////
/// \brief Used to create character sets.
///
/// There are two ways to create character sets with the 'set' identifier. The
/// easiest is to create a comma-separated list of the characters in the set,
/// as in (set= 'a','b','c'). This set will match 'a', 'b', or 'c'. The other
/// way is to define the set as an argument to the set subscript operator.
/// For instance, set[ 'a' | range('b','c') | digit ] will match an 'a', 'b',
/// 'c' or a digit character.
///
/// To complement a set, apply the '~' operator. For instance, ~(set= 'a','b','c')
/// will match any character that is not an 'a', 'b', or 'c'.
///
/// Sets can be composed of other, possibly complemented, sets. For instance,
/// set[ ~digit | ~(set= 'a','b','c') ].
detail::set_initializer_type const set = {{}};

///////////////////////////////////////////////////////////////////////////////
/// \brief Sub-match placeholder type, used to create named captures in
/// static regexes.
///
/// \c mark_tag is the type of the global sub-match placeholders \c s0, \c s1, etc.. You
/// can use the \c mark_tag type to create your own sub-match placeholders with
/// more meaningful names. This is roughly equivalent to the "named capture"
/// feature of dynamic regular expressions.
///
/// To create a named sub-match placeholder, initialize it with a unique integer.
/// The integer must only be unique within the regex in which the placeholder
/// is used. Then you can use it within static regexes to created sub-matches
/// by assigning a sub-expression to it, or to refer back to already created
/// sub-matches.
/// 
/// \code
/// mark_tag number(1); // "number" is now equivalent to "s1"
/// // Match a number, followed by a space and the same number again
/// sregex rx = (number = +_d) >> ' ' >> number;
/// \endcode
///
/// After a successful \c regex_match() or \c regex_search(), the sub-match placeholder
/// can be used to index into the <tt>match_results\<\></tt> object to retrieve the
/// corresponding sub-match.
struct mark_tag
  : proto::extends<detail::basic_mark_tag, mark_tag, detail::regex_domain>
{
private:
    typedef proto::extends<detail::basic_mark_tag, mark_tag, detail::regex_domain> base_type;

    static detail::basic_mark_tag make_tag(int mark_nbr)
    {
        detail::basic_mark_tag mark = {{mark_nbr}};
        return mark;
    }

public:
    /// \brief Initialize a mark_tag placeholder
    /// \param mark_nbr An integer that uniquely identifies this \c mark_tag
    /// within the static regexes in which this \c mark_tag will be used.
    /// \pre <tt>mark_nbr \> 0</tt>
    mark_tag(int mark_nbr)
      : base_type(mark_tag::make_tag(mark_nbr))
    {
        // Marks numbers must be integers greater than 0.
        BOOST_ASSERT(mark_nbr > 0);
    }

    /// INTERNAL ONLY
    operator detail::basic_mark_tag const &() const
    {
        return this->proto_base();
    }

    BOOST_PROTO_EXTENDS_USING_ASSIGN_NON_DEPENDENT(mark_tag)
};

// This macro is used when declaring mark_tags that are global because
// it guarantees that they are statically initialized. That avoids
// order-of-initialization bugs. In user code, the simpler: mark_tag s0(0);
// would be preferable.
/// INTERNAL ONLY
#define BOOST_XPRESSIVE_GLOBAL_MARK_TAG(NAME, VALUE)                            \
    boost::xpressive::mark_tag::proto_base_expr const NAME = {{VALUE}}          \
    /**/

///////////////////////////////////////////////////////////////////////////////
/// \brief Sub-match placeholder, like $& in Perl
BOOST_XPRESSIVE_GLOBAL_MARK_TAG(s0, 0);

///////////////////////////////////////////////////////////////////////////////
/// \brief Sub-match placeholder, like $1 in perl.
///
/// To create a sub-match, assign a sub-expression to the sub-match placeholder.
/// For instance, (s1= _) will match any one character and remember which
/// character was matched in the 1st sub-match. Later in the pattern, you can
/// refer back to the sub-match. For instance,  (s1= _) >> s1  will match any
/// character, and then match the same character again.
///
/// After a successful regex_match() or regex_search(), the sub-match placeholders
/// can be used to index into the match_results\<\> object to retrieve the Nth
/// sub-match.
BOOST_XPRESSIVE_GLOBAL_MARK_TAG(s1, 1);
BOOST_XPRESSIVE_GLOBAL_MARK_TAG(s2, 2);
BOOST_XPRESSIVE_GLOBAL_MARK_TAG(s3, 3);
BOOST_XPRESSIVE_GLOBAL_MARK_TAG(s4, 4);
BOOST_XPRESSIVE_GLOBAL_MARK_TAG(s5, 5);
BOOST_XPRESSIVE_GLOBAL_MARK_TAG(s6, 6);
BOOST_XPRESSIVE_GLOBAL_MARK_TAG(s7, 7);
BOOST_XPRESSIVE_GLOBAL_MARK_TAG(s8, 8);
BOOST_XPRESSIVE_GLOBAL_MARK_TAG(s9, 9);

// NOTE: For the purpose of xpressive's documentation, make icase() look like an
// ordinary function. In reality, it is a function object defined in detail/icase.hpp
// so that it can serve double-duty as regex_constants::icase, the syntax_option_type.
#ifdef BOOST_XPRESSIVE_DOXYGEN_INVOKED
///////////////////////////////////////////////////////////////////////////////
/// \brief Makes a sub-expression case-insensitive.
///
/// Use icase() to make a sub-expression case-insensitive. For instance,
/// "foo" >> icase(set['b'] >> "ar") will match "foo" exactly followed by
/// "bar" irrespective of case.
template<typename Expr> detail::unspecified icase(Expr const &expr) { return 0; }
#endif

///////////////////////////////////////////////////////////////////////////////
/// \brief Makes a literal into a regular expression.
///
/// Use as_xpr() to turn a literal into a regular expression. For instance,
/// "foo" >> "bar" will not compile because both operands to the right-shift
/// operator are const char*, and no such operator exists. Use as_xpr("foo") >> "bar"
/// instead.
///
/// You can use as_xpr() with character literals in addition to string literals.
/// For instance, as_xpr('a') will match an 'a'. You can also complement a
/// character literal, as with ~as_xpr('a'). This will match any one character
/// that is not an 'a'.
#ifdef BOOST_XPRESSIVE_DOXYGEN_INVOKED
template<typename Literal> detail::unspecified as_xpr(Literal const &literal) { return 0; }
#else
proto::functional::as_expr<> const as_xpr = {};
#endif

///////////////////////////////////////////////////////////////////////////////
/// \brief Embed a regex object by reference.
///
/// \param rex The basic_regex object to embed by reference.
template<typename BidiIter>
inline typename proto::terminal<reference_wrapper<basic_regex<BidiIter> const> >::type const
by_ref(basic_regex<BidiIter> const &rex)
{
    reference_wrapper<basic_regex<BidiIter> const> ref(rex);
    return proto::terminal<reference_wrapper<basic_regex<BidiIter> const> >::type::make(ref);
}

///////////////////////////////////////////////////////////////////////////////
/// \brief Match a range of characters.
///
/// Match any character in the range [ch_min, ch_max].
///
/// \param ch_min The lower end of the range to match.
/// \param ch_max The upper end of the range to match.
template<typename Char>
inline typename proto::terminal<detail::range_placeholder<Char> >::type const
range(Char ch_min, Char ch_max)
{
    detail::range_placeholder<Char> that = {ch_min, ch_max, false};
    return proto::terminal<detail::range_placeholder<Char> >::type::make(that);
}

///////////////////////////////////////////////////////////////////////////////
/// \brief Make a sub-expression optional. Equivalent to !as_xpr(expr).
///
/// \param expr The sub-expression to make optional.
template<typename Expr>
typename proto::result_of::make_expr<
    proto::tag::logical_not
  , proto::default_domain
  , Expr const &
>::type const
optional(Expr const &expr)
{
    return proto::make_expr<
        proto::tag::logical_not
      , proto::default_domain
    >(boost::ref(expr));
}

///////////////////////////////////////////////////////////////////////////////
/// \brief Repeat a sub-expression multiple times.
///
/// There are two forms of the repeat\<\>() function template. To match a
/// sub-expression N times, use repeat\<N\>(expr). To match a sub-expression
/// from M to N times, use repeat\<M,N\>(expr).
///
/// The repeat\<\>() function creates a greedy quantifier. To make the quantifier
/// non-greedy, apply the unary minus operator, as in -repeat\<M,N\>(expr).
///
/// \param expr The sub-expression to repeat.
template<unsigned int Min, unsigned int Max, typename Expr>
typename proto::result_of::make_expr<
    detail::generic_quant_tag<Min, Max>
  , proto::default_domain
  , Expr const &
>::type const
repeat(Expr const &expr)
{
    return proto::make_expr<
        detail::generic_quant_tag<Min, Max>
      , proto::default_domain
    >(boost::ref(expr));
}

/// \overload
///
template<unsigned int Count, typename Expr2>
typename proto::result_of::make_expr<
    detail::generic_quant_tag<Count, Count>
  , proto::default_domain
  , Expr2 const &
>::type const
repeat(Expr2 const &expr2)
{
    return proto::make_expr<
        detail::generic_quant_tag<Count, Count>
      , proto::default_domain
    >(boost::ref(expr2));
}

///////////////////////////////////////////////////////////////////////////////
/// \brief Create an independent sub-expression.
///
/// Turn off back-tracking for a sub-expression. Any branches or repeats within
/// the sub-expression will match only one way, and no other alternatives are
/// tried.
///
/// \attention keep(expr) is equivalent to the perl (?>...) extension.
///
/// \param expr The sub-expression to modify.
template<typename Expr>
typename proto::result_of::make_expr<
    detail::keeper_tag
  , proto::default_domain
  , Expr const &
>::type const
keep(Expr const &expr)
{
    return proto::make_expr<
        detail::keeper_tag
      , proto::default_domain
    >(boost::ref(expr));
}

///////////////////////////////////////////////////////////////////////////////
/// \brief Look-ahead assertion.
///
/// before(expr) succeeds if the expr sub-expression would match at the current
/// position in the sequence, but expr is not included in the match. For instance,
/// before("foo") succeeds if we are before a "foo". Look-ahead assertions can be
/// negated with the bit-compliment operator.
///
/// \attention before(expr) is equivalent to the perl (?=...) extension.
/// ~before(expr) is a negative look-ahead assertion, equivalent to the
/// perl (?!...) extension.
///
/// \param expr The sub-expression to put in the look-ahead assertion.
template<typename Expr>
typename proto::result_of::make_expr<
    detail::lookahead_tag
  , proto::default_domain
  , Expr const &
>::type const
before(Expr const &expr)
{
    return proto::make_expr<
        detail::lookahead_tag
      , proto::default_domain
    >(boost::ref(expr));
}

///////////////////////////////////////////////////////////////////////////////
/// \brief Look-behind assertion.
///
/// after(expr) succeeds if the expr sub-expression would match at the current
/// position minus N in the sequence, where N is the width of expr. expr is not included in
/// the match. For instance,  after("foo") succeeds if we are after a "foo". Look-behind
/// assertions can be negated with the bit-complement operator.
///
/// \attention after(expr) is equivalent to the perl (?<=...) extension.
/// ~after(expr) is a negative look-behind assertion, equivalent to the
/// perl (?<!...) extension.
///
/// \param expr The sub-expression to put in the look-ahead assertion.
///
/// \pre expr cannot match a variable number of characters.
template<typename Expr>
typename proto::result_of::make_expr<
    detail::lookbehind_tag
  , proto::default_domain
  , Expr const &
>::type const
after(Expr const &expr)
{
    return proto::make_expr<
        detail::lookbehind_tag
      , proto::default_domain
    >(boost::ref(expr));
}

///////////////////////////////////////////////////////////////////////////////
/// \brief Specify a regex traits or a std::locale.
///
/// imbue() instructs the regex engine to use the specified traits or locale
/// when matching the regex. The entire expression must use the same traits/locale.
/// For instance, the following specifies a locale for use with a regex:
///   std::locale loc;
///   sregex rx = imbue(loc)(+digit);
///
/// \param loc The std::locale or regex traits object.
template<typename Locale>
inline detail::modifier_op<detail::locale_modifier<Locale> > const
imbue(Locale const &loc)
{
    detail::modifier_op<detail::locale_modifier<Locale> > mod =
    {
        detail::locale_modifier<Locale>(loc)
      , regex_constants::ECMAScript
    };
    return mod;
}

proto::terminal<detail::attribute_placeholder<mpl::int_<1> > >::type const a1 = {{}};
proto::terminal<detail::attribute_placeholder<mpl::int_<2> > >::type const a2 = {{}};
proto::terminal<detail::attribute_placeholder<mpl::int_<3> > >::type const a3 = {{}};
proto::terminal<detail::attribute_placeholder<mpl::int_<4> > >::type const a4 = {{}};
proto::terminal<detail::attribute_placeholder<mpl::int_<5> > >::type const a5 = {{}};
proto::terminal<detail::attribute_placeholder<mpl::int_<6> > >::type const a6 = {{}};
proto::terminal<detail::attribute_placeholder<mpl::int_<7> > >::type const a7 = {{}};
proto::terminal<detail::attribute_placeholder<mpl::int_<8> > >::type const a8 = {{}};
proto::terminal<detail::attribute_placeholder<mpl::int_<9> > >::type const a9 = {{}};

///////////////////////////////////////////////////////////////////////////////
/// \brief Specify which characters to skip when matching a regex.
///
/// <tt>skip()</tt> instructs the regex engine to skip certain characters when matching
/// a regex. It is most useful for writing regexes that ignore whitespace.
/// For instance, the following specifies a regex that skips whitespace and
/// punctuation:
///
/// \code
/// // A sentence is one or more words separated by whitespace
/// // and punctuation.
/// sregex word = +alpha;
/// sregex sentence = skip(set[_s | punct])( +word );
/// \endcode
///
/// The way it works in the above example is to insert
/// <tt>keep(*set[_s | punct])</tt> before each primitive within the regex.
/// A "primitive" includes terminals like strings, character sets and nested
/// regexes. A final <tt>*set[_s | punct]</tt> is added to the end of the
/// regex. The regex <tt>sentence</tt> specified above is equivalent to
/// the following:
///
/// \code
/// sregex sentence = +( keep(*set[_s | punct]) >> word )
///                        >> *set[_s | punct];
/// \endcode
///
/// \attention Skipping does not affect how nested regexes are handled because
/// they are treated atomically. String literals are also treated
/// atomically; that is, no skipping is done within a string literal. So
/// <tt>skip(_s)("this that")</tt> is not the same as
/// <tt>skip(_s)("this" >> as_xpr("that"))</tt>. The first will only match
/// when there is only one space between "this" and "that". The second will
/// skip any and all whitespace between "this" and "that".
///
/// \param skip A regex that specifies which characters to skip.
template<typename Skip>
detail::skip_directive<Skip> skip(Skip const &skip)
{
    return detail::skip_directive<Skip>(skip);
}

namespace detail
{
    inline void ignore_unused_regex_primitives()
    {
        detail::ignore_unused(repeat_max);
        detail::ignore_unused(inf);
        detail::ignore_unused(epsilon);
        detail::ignore_unused(nil);
        detail::ignore_unused(alnum);
        detail::ignore_unused(bos);
        detail::ignore_unused(eos);
        detail::ignore_unused(bol);
        detail::ignore_unused(eol);
        detail::ignore_unused(bow);
        detail::ignore_unused(eow);
        detail::ignore_unused(_b);
        detail::ignore_unused(_w);
        detail::ignore_unused(_d);
        detail::ignore_unused(_s);
        detail::ignore_unused(_n);
        detail::ignore_unused(_ln);
        detail::ignore_unused(_);
        detail::ignore_unused(self);
        detail::ignore_unused(set);
        detail::ignore_unused(s0);
        detail::ignore_unused(s1);
        detail::ignore_unused(s2);
        detail::ignore_unused(s3);
        detail::ignore_unused(s4);
        detail::ignore_unused(s5);
        detail::ignore_unused(s6);
        detail::ignore_unused(s7);
        detail::ignore_unused(s8);
        detail::ignore_unused(s9);
        detail::ignore_unused(a1);
        detail::ignore_unused(a2);
        detail::ignore_unused(a3);
        detail::ignore_unused(a4);
        detail::ignore_unused(a5);
        detail::ignore_unused(a6);
        detail::ignore_unused(a7);
        detail::ignore_unused(a8);
        detail::ignore_unused(a9);
        detail::ignore_unused(as_xpr);
    }
}

}} // namespace boost::xpressive

#endif

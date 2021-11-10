/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library

    http://www.boost.org/

    Copyright (c) 2020      Jeff Trull
    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#if !defined(BOOST_CPP_HAS_INCLUDE_GRAMMAR_HPP_F48287B2_DC67_40A8_B4A1_800EFBD67869_INCLUDED)
#define BOOST_CPP_HAS_INCLUDE_GRAMMAR_HPP_F48287B2_DC67_40A8_B4A1_800EFBD67869_INCLUDED

#include <boost/wave/wave_config.hpp>

#include <boost/assert.hpp>
#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_closure.hpp>
#include <boost/spirit/include/classic_assign_actor.hpp>
#include <boost/spirit/include/classic_push_back_actor.hpp>

#include <boost/wave/token_ids.hpp>
#include <boost/wave/util/pattern_parser.hpp>
#include <boost/wave/grammars/cpp_has_include_grammar_gen.hpp>

#if !defined(spirit_append_actor)
#define spirit_append_actor(actor) boost::spirit::classic::push_back_a(actor)
#endif // !has_include(spirit_append_actor)

// this must occur after all of the includes and before any code appears
#ifdef BOOST_HAS_ABI_HEADERS
#include BOOST_ABI_PREFIX
#endif

///////////////////////////////////////////////////////////////////////////////
namespace boost {
namespace wave {
namespace grammars {

///////////////////////////////////////////////////////////////////////////////
//  define, whether the rule's should generate some debug output
#define TRACE_CPP_HAS_INCLUDE_GRAMMAR \
    bool(BOOST_SPIRIT_DEBUG_FLAGS_CPP & BOOST_SPIRIT_DEBUG_FLAGS_HAS_INCLUDE_GRAMMAR) \
    /**/

template <typename ContainerT>
struct has_include_grammar :
    public boost::spirit::classic::grammar<has_include_grammar<ContainerT> >
{
    has_include_grammar(ContainerT &tokens_seq_,
                        bool &is_quoted_filename_,
                        bool &is_system_)
    :   tokens_seq(tokens_seq_), is_quoted_filename(is_quoted_filename_),
        is_system(is_system_), true_(true)
    {
        BOOST_SPIRIT_DEBUG_TRACE_GRAMMAR_NAME(*this, "has_include_grammar",
            TRACE_CPP_HAS_INCLUDE_GRAMMAR);
        is_quoted_filename = false;
        is_system = false;
    }

    template <typename ScannerT>
    struct definition
    {
        typedef boost::spirit::classic::rule<ScannerT> rule_t;

        rule_t has_include_op;
        rule_t system_include;
        rule_t nonsystem_include;
        rule_t computed_include;

        definition(has_include_grammar const & self)
        {
            using namespace boost::spirit::classic;
            using namespace boost::wave;
            using namespace boost::wave::util;

            has_include_op
                =   ch_p(T_IDENTIFIER)      // token contains '__has_include'
                    >>  (
                        ch_p(T_LEFTPAREN) >>
                        (system_include | nonsystem_include | computed_include)
                        // final right paren removed by caller
                        )
                    >> end_p
                ;

            system_include
                = ch_p(T_LESS)
                [
                    spirit_append_actor(self.tokens_seq)
                ]
                >> * (~ch_p(T_GREATER))
                [
                    spirit_append_actor(self.tokens_seq)
                ]
                >> ch_p(T_GREATER)
                [
                    spirit_append_actor(self.tokens_seq)
                ][
                    assign_a(self.is_quoted_filename, self.true_)
                ][
                    assign_a(self.is_system, self.true_)
                ]
                ;

            nonsystem_include = ch_p(T_STRINGLIT)
                [
                    spirit_append_actor(self.tokens_seq)
                ][
                    assign_a(self.is_quoted_filename, self.true_)
                ]
                ;

            // if neither of the above match we take everything between
            // the parentheses and evaluate it ("computed include")
            computed_include = * anychar_p
                [
                    spirit_append_actor(self.tokens_seq)
                ]
                ;


            BOOST_SPIRIT_DEBUG_TRACE_RULE(has_include_op, TRACE_CPP_HAS_INCLUDE_GRAMMAR);
            BOOST_SPIRIT_DEBUG_TRACE_RULE(system_include, TRACE_CPP_HAS_INCLUDE_GRAMMAR);
            BOOST_SPIRIT_DEBUG_TRACE_RULE(nonsystem_include, TRACE_CPP_HAS_INCLUDE_GRAMMAR);
            BOOST_SPIRIT_DEBUG_TRACE_RULE(computed_include, TRACE_CPP_HAS_INCLUDE_GRAMMAR);
        }

        // start rule of this grammar
        rule_t const& start() const
        { return has_include_op; }
    };

    ContainerT &tokens_seq;
    bool &is_quoted_filename;
    bool &is_system;
    const bool true_;  // Spirit Classic actors operate on references, not values
};

///////////////////////////////////////////////////////////////////////////////
#undef TRACE_CPP_HAS_INCLUDE_GRAMMAR

///////////////////////////////////////////////////////////////////////////////
//
//  The following parse function is has_include here, to allow the separation of
//  the compilation of the has_include_grammar from the function
//  using it.
//
///////////////////////////////////////////////////////////////////////////////

#if BOOST_WAVE_SEPARATE_GRAMMAR_INSTANTIATION != 0
#define BOOST_WAVE_HAS_INCLUDE_GRAMMAR_GEN_INLINE
#else
#define BOOST_WAVE_HAS_INCLUDE_GRAMMAR_GEN_INLINE inline
#endif

//  The parse_operator_define function is instantiated manually twice to
//  simplify the explicit specialization of this template. This way the user
//  has only to specify one template parameter (the lexer type) to correctly
//  formulate the required explicit specialization.
//  This results in no code overhead, because otherwise the function would be
//  generated by the compiler twice anyway.

template <typename LexIteratorT>
BOOST_WAVE_HAS_INCLUDE_GRAMMAR_GEN_INLINE
boost::spirit::classic::parse_info<
    typename has_include_grammar_gen<LexIteratorT>::iterator1_type
>
has_include_grammar_gen<LexIteratorT>::parse_operator_has_include (
    iterator1_type const &first, iterator1_type const &last,
    token_sequence_type &tokens,
    bool &is_quoted_filename, bool &is_system)
{
    using namespace boost::spirit::classic;
    using namespace boost::wave;

    has_include_grammar<token_sequence_type>
        g(tokens, is_quoted_filename, is_system);
    return boost::spirit::classic::parse (
        first, last, g, ch_p(T_SPACE) | ch_p(T_CCOMMENT));
}

template <typename LexIteratorT>
BOOST_WAVE_HAS_INCLUDE_GRAMMAR_GEN_INLINE
boost::spirit::classic::parse_info<
    typename has_include_grammar_gen<LexIteratorT>::iterator2_type
>
has_include_grammar_gen<LexIteratorT>::parse_operator_has_include (
    iterator2_type const &first, iterator2_type const &last,
    token_sequence_type &found_qualified_name,
    bool &is_quoted_filename, bool &is_system)
{
    using namespace boost::spirit::classic;
    using namespace boost::wave;

    has_include_grammar<token_sequence_type>
        g(found_qualified_name, is_quoted_filename, is_system);
    return boost::spirit::classic::parse (
        first, last, g, ch_p(T_SPACE) | ch_p(T_CCOMMENT));
}

#undef BOOST_WAVE_HAS_INCLUDE_GRAMMAR_GEN_INLINE

///////////////////////////////////////////////////////////////////////////////
}   // namespace grammars
}   // namespace wave
}   // namespace boost

// the suffix header occurs after all of the code
#ifdef BOOST_HAS_ABI_HEADERS
#include BOOST_ABI_SUFFIX
#endif

#endif // !defined(BOOST_CPP_HAS_INCLUDE_GRAMMAR_HPP_F48287B2_DC67_40A8_B4A1_800EFBD67869_INCLUDED)

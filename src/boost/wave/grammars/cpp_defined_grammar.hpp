/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library

    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#if !defined(BOOST_CPP_DEFINED_GRAMMAR_HPP_F48287B2_DC67_40A8_B4A1_800EFBD67869_INCLUDED)
#define BOOST_CPP_DEFINED_GRAMMAR_HPP_F48287B2_DC67_40A8_B4A1_800EFBD67869_INCLUDED

#include <boost/wave/wave_config.hpp>

#include <boost/assert.hpp>
#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_closure.hpp>
#include <boost/spirit/include/classic_assign_actor.hpp>
#include <boost/spirit/include/classic_push_back_actor.hpp>

#include <boost/wave/token_ids.hpp>
#include <boost/wave/util/pattern_parser.hpp>
#include <boost/wave/grammars/cpp_defined_grammar_gen.hpp>

#if !defined(spirit_append_actor)
#define spirit_append_actor(actor) boost::spirit::classic::push_back_a(actor)
#define spirit_assign_actor(actor) boost::spirit::classic::assign_a(actor)
#endif // !defined(spirit_append_actor)

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
#define TRACE_CPP_DEFINED_GRAMMAR \
    bool(BOOST_SPIRIT_DEBUG_FLAGS_CPP & BOOST_SPIRIT_DEBUG_FLAGS_DEFINED_GRAMMAR) \
    /**/

template <typename ContainerT>
struct defined_grammar :
    public boost::spirit::classic::grammar<defined_grammar<ContainerT> >
{
    defined_grammar(ContainerT &result_seq_)
    :   result_seq(result_seq_)
    {
        BOOST_SPIRIT_DEBUG_TRACE_GRAMMAR_NAME(*this, "defined_grammar",
            TRACE_CPP_DEFINED_GRAMMAR);
    }

    template <typename ScannerT>
    struct definition
    {
        typedef boost::spirit::classic::rule<ScannerT> rule_t;

        rule_t defined_op;
        rule_t identifier;

        definition(defined_grammar const &self)
        {
            using namespace boost::spirit::classic;
            using namespace boost::wave;
            using namespace boost::wave::util;

            defined_op      // parens not required, see C++ standard 16.1.1
                =   ch_p(T_IDENTIFIER)      // token contains 'defined'
                    >>  (
                            (   ch_p(T_LEFTPAREN)
                                >>  identifier
                                >>  ch_p(T_RIGHTPAREN)
                            )
                            |   identifier
                        )
                ;

            identifier
                =   ch_p(T_IDENTIFIER)
                    [
                        spirit_append_actor(self.result_seq)
                    ]
                |   pattern_p(KeywordTokenType, TokenTypeMask|PPTokenFlag)
                    [
                        spirit_append_actor(self.result_seq)
                    ]
                |   pattern_p(OperatorTokenType|AltExtTokenType,
                        ExtTokenTypeMask|PPTokenFlag)
                    [
                        spirit_append_actor(self.result_seq)
                    ]
                |   pattern_p(BoolLiteralTokenType, TokenTypeMask|PPTokenFlag)
                    [
                        spirit_append_actor(self.result_seq)
                    ]
                ;

            BOOST_SPIRIT_DEBUG_TRACE_RULE(defined_op, TRACE_CPP_DEFINED_GRAMMAR);
            BOOST_SPIRIT_DEBUG_TRACE_RULE(identifier, TRACE_CPP_DEFINED_GRAMMAR);
        }

        // start rule of this grammar
        rule_t const& start() const
        { return defined_op; }
    };

    ContainerT &result_seq;
};

///////////////////////////////////////////////////////////////////////////////
#undef TRACE_CPP_DEFINED_GRAMMAR

///////////////////////////////////////////////////////////////////////////////
//
//  The following parse function is defined here, to allow the separation of
//  the compilation of the defined_grammar from the function
//  using it.
//
///////////////////////////////////////////////////////////////////////////////

#if BOOST_WAVE_SEPARATE_GRAMMAR_INSTANTIATION != 0
#define BOOST_WAVE_DEFINED_GRAMMAR_GEN_INLINE
#else
#define BOOST_WAVE_DEFINED_GRAMMAR_GEN_INLINE inline
#endif

//  The parse_operator_define function is instantiated manually twice to
//  simplify the explicit specialization of this template. This way the user
//  has only to specify one template parameter (the lexer type) to correctly
//  formulate the required explicit specialization.
//  This results in no code overhead, because otherwise the function would be
//  generated by the compiler twice anyway.

template <typename LexIteratorT>
BOOST_WAVE_DEFINED_GRAMMAR_GEN_INLINE
boost::spirit::classic::parse_info<
    typename defined_grammar_gen<LexIteratorT>::iterator1_type
>
defined_grammar_gen<LexIteratorT>::parse_operator_defined (
    iterator1_type const &first, iterator1_type const &last,
    token_sequence_type &found_qualified_name)
{
    using namespace boost::spirit::classic;
    using namespace boost::wave;

    defined_grammar<token_sequence_type> g(found_qualified_name);
    return boost::spirit::classic::parse (
        first, last, g, ch_p(T_SPACE) | ch_p(T_CCOMMENT));
}

template <typename LexIteratorT>
BOOST_WAVE_DEFINED_GRAMMAR_GEN_INLINE
boost::spirit::classic::parse_info<
    typename defined_grammar_gen<LexIteratorT>::iterator2_type
>
defined_grammar_gen<LexIteratorT>::parse_operator_defined (
    iterator2_type const &first, iterator2_type const &last,
    token_sequence_type &found_qualified_name)
{
    using namespace boost::spirit::classic;
    using namespace boost::wave;

    defined_grammar<token_sequence_type> g(found_qualified_name);
    return boost::spirit::classic::parse (
        first, last, g, ch_p(T_SPACE) | ch_p(T_CCOMMENT));
}

#undef BOOST_WAVE_DEFINED_GRAMMAR_GEN_INLINE

///////////////////////////////////////////////////////////////////////////////
}   // namespace grammars
}   // namespace wave
}   // namespace boost

// the suffix header occurs after all of the code
#ifdef BOOST_HAS_ABI_HEADERS
#include BOOST_ABI_SUFFIX
#endif

#endif // !defined(BOOST_CPP_DEFINED_GRAMMAR_HPP_F48287B2_DC67_40A8_B4A1_800EFBD67869_INCLUDED)

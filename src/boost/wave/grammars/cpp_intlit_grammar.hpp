/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library

    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#if !defined(BOOST_CPP_INTLIT_GRAMMAR_HPP_2E1E70B1_F15C_4132_8554_10A231B0D91C_INCLUDED)
#define BOOST_CPP_INTLIT_GRAMMAR_HPP_2E1E70B1_F15C_4132_8554_10A231B0D91C_INCLUDED

#include <boost/wave/wave_config.hpp>

#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_closure.hpp>
#include <boost/spirit/include/classic_assign_actor.hpp>
#include <boost/spirit/include/classic_push_back_actor.hpp>

#include <boost/spirit/include/phoenix1_operators.hpp>
#include <boost/spirit/include/phoenix1_primitives.hpp>
#include <boost/spirit/include/phoenix1_statements.hpp>

#include <boost/wave/cpp_exceptions.hpp>
#include <boost/wave/grammars/cpp_literal_grammar_gen.hpp>

#if !defined(spirit_append_actor)
#define spirit_append_actor(actor) boost::spirit::classic::push_back_a(actor)
#define spirit_assign_actor(actor) boost::spirit::classic::assign_a(actor)
#endif // !defined(spirit_append_actor)

// this must occur after all of the includes and before any code appears
#ifdef BOOST_HAS_ABI_HEADERS
#include BOOST_ABI_PREFIX
#endif

///////////////////////////////////////////////////////////////////////////////
//
//  Reusable grammar for parsing of C++ style integer literals
//
///////////////////////////////////////////////////////////////////////////////
namespace boost {
namespace wave {
namespace grammars {

///////////////////////////////////////////////////////////////////////////////
namespace closures {

    struct intlit_closure
    :   boost::spirit::classic::closure<intlit_closure, uint_literal_type>
    {
        member1 val;
    };
}

///////////////////////////////////////////////////////////////////////////////
//  define, whether the rule's should generate some debug output
#define TRACE_INTLIT_GRAMMAR \
    bool(BOOST_SPIRIT_DEBUG_FLAGS_CPP & BOOST_SPIRIT_DEBUG_FLAGS_INTLIT_GRAMMAR) \
    /**/

struct intlit_grammar :
    boost::spirit::classic::grammar<intlit_grammar, closures::intlit_closure::context_t>
{
    intlit_grammar(bool &is_unsigned_) : is_unsigned(is_unsigned_)
    {
        BOOST_SPIRIT_DEBUG_TRACE_GRAMMAR_NAME(*this, "intlit_grammar",
            TRACE_INTLIT_GRAMMAR);
    }

    template <typename ScannerT>
    struct definition
    {
        typedef boost::spirit::classic::rule<ScannerT> rule_t;

        rule_t int_lit;
        boost::spirit::classic::subrule<0> sub_int_lit;
        boost::spirit::classic::subrule<1> oct_lit;
        boost::spirit::classic::subrule<2> hex_lit;
        boost::spirit::classic::subrule<3> dec_lit;

        definition(intlit_grammar const &self)
        {
            using namespace boost::spirit::classic;
            namespace phx = phoenix;


            int_lit = (
                    sub_int_lit =
                        (   ch_p('0')[self.val = 0] >> (hex_lit | oct_lit)
                        |   dec_lit
                        )
                        >> !as_lower_d[
                                (ch_p('u')[phx::var(self.is_unsigned) = true] || ch_p('l'))
                            |   (ch_p('l') || ch_p('u')[phx::var(self.is_unsigned) = true])
                            ]
                    ,

                    hex_lit =
                            (ch_p('X') | ch_p('x'))
                        >>  uint_parser<uint_literal_type, 16>()
                            [
                                self.val = phx::arg1,
                                phx::var(self.is_unsigned) = true
                            ]
                    ,

                    oct_lit =
                       !uint_parser<uint_literal_type, 8>()
                        [
                            self.val = phx::arg1,
                            phx::var(self.is_unsigned) = true
                        ]
                    ,

                    dec_lit =
                        uint_parser<uint_literal_type, 10>()
                        [
                            self.val = phx::arg1
                        ]
                    )
                ;

            BOOST_SPIRIT_DEBUG_TRACE_RULE(int_lit, TRACE_INTLIT_GRAMMAR);
            BOOST_SPIRIT_DEBUG_TRACE_RULE(sub_int_lit, TRACE_INTLIT_GRAMMAR);
            BOOST_SPIRIT_DEBUG_TRACE_RULE(hex_lit, TRACE_INTLIT_GRAMMAR);
            BOOST_SPIRIT_DEBUG_TRACE_RULE(oct_lit, TRACE_INTLIT_GRAMMAR);
            BOOST_SPIRIT_DEBUG_TRACE_RULE(dec_lit, TRACE_INTLIT_GRAMMAR);
        }

        // start rule of this grammar
        rule_t const& start() const
        { return int_lit; }
    };

    bool &is_unsigned;
};

#undef TRACE_INTLIT_GRAMMAR

///////////////////////////////////////////////////////////////////////////////
//
//  The following function is defined here, to allow the separation of
//  the compilation of the intlit_grammar from the function using it.
//
///////////////////////////////////////////////////////////////////////////////

#if BOOST_WAVE_SEPARATE_GRAMMAR_INSTANTIATION != 0
#define BOOST_WAVE_INTLITGRAMMAR_GEN_INLINE
#else
#define BOOST_WAVE_INTLITGRAMMAR_GEN_INLINE inline
#endif

template <typename TokenT>
BOOST_WAVE_INTLITGRAMMAR_GEN_INLINE
uint_literal_type
intlit_grammar_gen<TokenT>::evaluate(TokenT const &token,
    bool &is_unsigned)
{
    intlit_grammar g(is_unsigned);
    uint_literal_type result = 0;
    typename TokenT::string_type const &token_val = token.get_value();
    using boost::spirit::classic::parse_info;
    parse_info<typename TokenT::string_type::const_iterator> hit =
        parse(token_val.begin(), token_val.end(), g[spirit_assign_actor(result)]);

    if (!hit.hit) {
        BOOST_WAVE_THROW(preprocess_exception, ill_formed_integer_literal,
            token_val.c_str(), token.get_position());
    }
    return result;
}

#undef BOOST_WAVE_INTLITGRAMMAR_GEN_INLINE

///////////////////////////////////////////////////////////////////////////////
}   // namespace grammars
}   // namespace wave
}   // namespace boost

// the suffix header occurs after all of the code
#ifdef BOOST_HAS_ABI_HEADERS
#include BOOST_ABI_SUFFIX
#endif

#endif // !defined(BOOST_CPP_INTLIT_GRAMMAR_HPP_2E1E70B1_F15C_4132_8554_10A231B0D91C_INCLUDED)

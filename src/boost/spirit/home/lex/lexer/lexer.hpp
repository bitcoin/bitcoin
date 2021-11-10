//  Copyright (c) 2001-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_LEX_LEXER_MAR_13_2007_0145PM)
#define BOOST_SPIRIT_LEX_LEXER_MAR_13_2007_0145PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/support/info.hpp>
#include <boost/spirit/home/qi/skip_over.hpp>
#include <boost/spirit/home/qi/parser.hpp>
#include <boost/spirit/home/qi/detail/assign_to.hpp>
#include <boost/spirit/home/lex/reference.hpp>
#include <boost/spirit/home/lex/meta_compiler.hpp>
#include <boost/spirit/home/lex/lexer_type.hpp>
#include <boost/spirit/home/lex/lexer/token_def.hpp>
#include <boost/assert.hpp>
#include <boost/noncopyable.hpp>
#include <boost/fusion/include/vector.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/proto/extends.hpp>
#include <boost/proto/traits.hpp>
#include <boost/range/iterator_range_core.hpp>
#include <iterator> // for std::iterator_traits
#include <string>

namespace boost { namespace spirit { namespace lex
{
    ///////////////////////////////////////////////////////////////////////////
    namespace detail
    {
        ///////////////////////////////////////////////////////////////////////
        template <typename LexerDef>
        struct lexer_def_
          : proto::extends<
                typename proto::terminal<
                   lex::reference<lexer_def_<LexerDef> const> 
                >::type
              , lexer_def_<LexerDef> >
          , qi::parser<lexer_def_<LexerDef> >
          , lex::lexer_type<lexer_def_<LexerDef> >
        {
        private:
            // avoid warnings about using 'this' in constructor
            lexer_def_& this_() { return *this; }

            typedef typename LexerDef::char_type char_type;
            typedef typename LexerDef::string_type string_type;
            typedef typename LexerDef::id_type id_type;

            typedef lex::reference<lexer_def_ const> reference_;
            typedef typename proto::terminal<reference_>::type terminal_type;
            typedef proto::extends<terminal_type, lexer_def_> proto_base_type;

            reference_ alias() const
            {
                return reference_(*this);
            }

        public:
            // Qi interface: metafunction calculating parser attribute type
            template <typename Context, typename Iterator>
            struct attribute
            {
                //  the return value of a token set contains the matched token 
                //  id, and the corresponding pair of iterators
                typedef typename Iterator::base_iterator_type iterator_type;
                typedef 
                    fusion::vector2<id_type, iterator_range<iterator_type> > 
                type;
            };

            // Qi interface: parse functionality
            template <typename Iterator, typename Context
              , typename Skipper, typename Attribute>
            bool parse(Iterator& first, Iterator const& last
              , Context& /*context*/, Skipper const& skipper
              , Attribute& attr) const
            {
                qi::skip_over(first, last, skipper);   // always do a pre-skip

                if (first != last) {
                    typedef typename 
                        std::iterator_traits<Iterator>::value_type 
                    token_type;

                    token_type const& t = *first;
                    if (token_is_valid(t) && t.state() == first.get_state()) {
                    // any of the token definitions matched
                        spirit::traits::assign_to(t, attr);
                        ++first;
                        return true;
                    }
                }
                return false;
            }

            // Qi interface: 'what' functionality
            template <typename Context>
            info what(Context& /*context*/) const
            {
                return info("lexer");
            }

        private:
            // allow to use the lexer.self.add("regex1", id1)("regex2", id2);
            // syntax
            struct adder
            {
                adder(lexer_def_& def_) 
                  : def(def_) {}

                // Add a token definition based on a single character as given
                // by the first parameter, the second parameter allows to 
                // specify the token id to use for the new token. If no token
                // id is given the character code is used.
                adder const& operator()(char_type c
                  , id_type token_id = id_type()) const
                {
                    if (id_type() == token_id)
                        token_id = static_cast<id_type>(c);
                    def.def.add_token (def.state.c_str(), c, token_id
                        , def.targetstate.empty() ? 0 : def.targetstate.c_str());
                    return *this;
                }

                // Add a token definition based on a character sequence as 
                // given by the first parameter, the second parameter allows to 
                // specify the token id to use for the new token. If no token
                // id is given this function will generate a unique id to be 
                // used as the token's id.
                adder const& operator()(string_type const& s
                  , id_type token_id = id_type()) const
                {
                    if (id_type() == token_id)
                        token_id = def.def.get_next_id();
                    def.def.add_token (def.state.c_str(), s, token_id
                        , def.targetstate.empty() ? 0 : def.targetstate.c_str());
                    return *this;
                }

                template <typename Attribute>
                adder const& operator()(
                    token_def<Attribute, char_type, id_type>& tokdef
                  , id_type token_id = id_type()) const
                {
                    // make sure we have a token id
                    if (id_type() == token_id) {
                        if (id_type() == tokdef.id()) {
                            token_id = def.def.get_next_id();
                            tokdef.id(token_id);
                        }
                        else {
                            token_id = tokdef.id();
                        }
                    }
                    else { 
                    // the following assertion makes sure that the token_def
                    // instance has not been assigned a different id earlier
                        BOOST_ASSERT(id_type() == tokdef.id() 
                                  || token_id == tokdef.id());
                        tokdef.id(token_id);
                    }

                    def.define(tokdef);
                    return *this;
                }

//                 template <typename F>
//                 adder const& operator()(char_type c, id_type token_id, F act) const
//                 {
//                     if (id_type() == token_id)
//                         token_id = def.def.get_next_id();
//                     std::size_t unique_id = 
//                         def.def.add_token (def.state.c_str(), s, token_id);
//                     def.def.add_action(unique_id, def.state.c_str(), act);
//                     return *this;
//                 }

                lexer_def_& def;

                // silence MSVC warning C4512: assignment operator could not be generated
                BOOST_DELETED_FUNCTION(adder& operator= (adder const&))
            };
            friend struct adder;

            // allow to use lexer.self.add_pattern("pattern1", "regex1")(...);
            // syntax
            struct pattern_adder
            {
                pattern_adder(lexer_def_& def_) 
                  : def(def_) {}

                pattern_adder const& operator()(string_type const& p
                  , string_type const& s) const
                {
                    def.def.add_pattern (def.state.c_str(), p, s);
                    return *this;
                }

                lexer_def_& def;

                // silence MSVC warning C4512: assignment operator could not be generated
                BOOST_DELETED_FUNCTION(pattern_adder& operator= (pattern_adder const&))
            };
            friend struct pattern_adder;

        private:
            // Helper function to invoke the necessary 2 step compilation
            // process on token definition expressions
            template <typename TokenExpr>
            void compile2pass(TokenExpr const& expr) 
            {
                expr.collect(def, state, targetstate);
                expr.add_actions(def);
            }

        public:
            ///////////////////////////////////////////////////////////////////
            template <typename Expr>
            void define(Expr const& expr)
            {
                compile2pass(compile<lex::domain>(expr));
            }

            lexer_def_(LexerDef& def_, string_type const& state_
                  , string_type const& targetstate_ = string_type())
              : proto_base_type(terminal_type::make(alias()))
              , add(this_()), add_pattern(this_()), def(def_)
              , state(state_), targetstate(targetstate_)
            {}

            // allow to switch states
            lexer_def_ operator()(char_type const* state) const
            {
                return lexer_def_(def, state);
            }
            lexer_def_ operator()(char_type const* state
              , char_type const* targetstate) const
            {
                return lexer_def_(def, state, targetstate);
            }
            lexer_def_ operator()(string_type const& state
              , string_type const& targetstate = string_type()) const
            {
                return lexer_def_(def, state, targetstate);
            }

            // allow to assign a token definition expression
            template <typename Expr>
            lexer_def_& operator= (Expr const& xpr)
            {
                // Report invalid expression error as early as possible.
                // If you got an error_invalid_expression error message here,
                // then the expression (expr) is not a valid spirit lex 
                // expression.
                BOOST_SPIRIT_ASSERT_MATCH(lex::domain, Expr);

                def.clear(state.c_str());
                define(xpr);
                return *this;
            }

            // explicitly tell the lexer that the given state will be defined
            // (useful in conjunction with "*")
            std::size_t add_state(char_type const* state = 0)
            {
                return def.add_state(state ? state : def.initial_state().c_str());
            }

            adder add;
            pattern_adder add_pattern;

        private:
            LexerDef& def;
            string_type state;
            string_type targetstate;

            // silence MSVC warning C4512: assignment operator could not be generated
            BOOST_DELETED_FUNCTION(lexer_def_& operator= (lexer_def_ const&))
        };

#if defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
        // allow to assign a token definition expression
        template <typename LexerDef, typename Expr>
        inline lexer_def_<LexerDef>&
        operator+= (lexer_def_<LexerDef>& lexdef, Expr& xpr)
        {
            // Report invalid expression error as early as possible.
            // If you got an error_invalid_expression error message here,
            // then the expression (expr) is not a valid spirit lex 
            // expression.
            BOOST_SPIRIT_ASSERT_MATCH(lex::domain, Expr);

            lexdef.define(xpr);
            return lexdef;
        }
#else
        // allow to assign a token definition expression
        template <typename LexerDef, typename Expr>
        inline lexer_def_<LexerDef>&
        operator+= (lexer_def_<LexerDef>& lexdef, Expr&& xpr)
        {
            // Report invalid expression error as early as possible.
            // If you got an error_invalid_expression error message here,
            // then the expression (expr) is not a valid spirit lex 
            // expression.
            BOOST_SPIRIT_ASSERT_MATCH(lex::domain, Expr);

            lexdef.define(xpr);
            return lexdef;
        }
#endif

        template <typename LexerDef, typename Expr>
        inline lexer_def_<LexerDef>& 
        operator+= (lexer_def_<LexerDef>& lexdef, Expr const& xpr)
        {
            // Report invalid expression error as early as possible.
            // If you got an error_invalid_expression error message here,
            // then the expression (expr) is not a valid spirit lex 
            // expression.
            BOOST_SPIRIT_ASSERT_MATCH(lex::domain, Expr);

            lexdef.define(xpr);
            return lexdef;
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    //  The match_flags flags are used to influence different matching 
    //  modes of the lexer
    struct match_flags
    {
        enum enum_type 
        {
            match_default = 0,          // no flags
            match_not_dot_newline = 1,  // the regex '.' doesn't match newlines
            match_icase = 2             // all matching operations are case insensitive
        };
    };

    ///////////////////////////////////////////////////////////////////////////
    //  This represents a lexer object
    ///////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////
    // This is the first token id automatically assigned by the library 
    // if needed
    enum tokenids 
    {
        min_token_id = 0x10000
    };

    template <typename Lexer>
    class lexer : public Lexer
    {
    private:
        // avoid warnings about using 'this' in constructor
        lexer& this_() { return *this; }

        std::size_t next_token_id;   // has to be an integral type

    public:
        typedef Lexer lexer_type;
        typedef typename Lexer::id_type id_type;
        typedef typename Lexer::char_type char_type;
        typedef typename Lexer::iterator_type iterator_type;
        typedef lexer base_type;

        typedef detail::lexer_def_<lexer> lexer_def;
        typedef std::basic_string<char_type> string_type;

        // if `id_type` was specified but `first_id` is not provided
        // the `min_token_id` value may be out of range for `id_type`,
        // but it will be a problem only if unique ids feature is in use.
        lexer(unsigned int flags = match_flags::match_default)
          : lexer_type(flags)
          , next_token_id(min_token_id)
          , self(this_(), lexer_type::initial_state())
        {}

        lexer(unsigned int flags, id_type first_id)
          : lexer_type(flags)
          , next_token_id(first_id)
          , self(this_(), lexer_type::initial_state()) 
        {}

        // access iterator interface
        template <typename Iterator>
        iterator_type begin(Iterator& first, Iterator const& last
                , char_type const* initial_state = 0) const
            { return this->lexer_type::begin(first, last, initial_state); }
        iterator_type end() const 
            { return this->lexer_type::end(); }

        std::size_t map_state(char_type const* state)
            { return this->lexer_type::add_state(state); }

        //  create a unique token id
        id_type get_next_id() { return id_type(next_token_id++); }

        lexer_def self;  // allow for easy token definition
    };

}}}

#endif

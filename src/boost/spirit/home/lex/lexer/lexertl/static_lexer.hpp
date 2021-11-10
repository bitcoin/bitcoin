//  Copyright (c) 2001-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_LEX_STATIC_LEXER_FEB_10_2008_0753PM)
#define BOOST_SPIRIT_LEX_STATIC_LEXER_FEB_10_2008_0753PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/lex/lexer/lexertl/token.hpp>
#include <boost/spirit/home/lex/lexer/lexertl/functor.hpp>
#include <boost/spirit/home/lex/lexer/lexertl/static_functor_data.hpp>
#include <boost/spirit/home/lex/lexer/lexertl/iterator.hpp>
#include <boost/spirit/home/lex/lexer/lexertl/static_version.hpp>
#if defined(BOOST_SPIRIT_DEBUG)
#include <boost/spirit/home/support/detail/lexer/debug.hpp>
#endif
#include <iterator> // for std::iterator_traits

namespace boost { namespace spirit { namespace lex { namespace lexertl
{ 
    ///////////////////////////////////////////////////////////////////////////
    //  forward declaration
    ///////////////////////////////////////////////////////////////////////////
    namespace static_
    {
        struct lexer;
    }

    ///////////////////////////////////////////////////////////////////////////
    //
    //  Every lexer type to be used as a lexer for Spirit has to conform to 
    //  the following public interface:
    //
    //    typedefs: 
    //        iterator_type   The type of the iterator exposed by this lexer.
    //        token_type      The type of the tokens returned from the exposed 
    //                        iterators.
    //
    //    functions:
    //        default constructor
    //                        Since lexers are instantiated as base classes 
    //                        only it might be a good idea to make this 
    //                        constructor protected.
    //        begin, end      Return a pair of iterators, when dereferenced
    //                        returning the sequence of tokens recognized in 
    //                        the input stream given as the parameters to the 
    //                        begin() function.
    //        add_token       Should add the definition of a token to be 
    //                        recognized by this lexer.
    //        clear           Should delete all current token definitions
    //                        associated with the given state of this lexer 
    //                        object.
    //
    //    template parameters:
    //        Token           The type of the tokens to be returned from the
    //                        exposed token iterator.
    //        LexerTables     See explanations below.
    //        Iterator        The type of the iterator used to access the
    //                        underlying character stream.
    //        Functor         The type of the InputPolicy to use to instantiate
    //                        the multi_pass iterator type to be used as the 
    //                        token iterator (returned from begin()/end()).
    //
    //    Additionally, this implementation of a static lexer has a template
    //    parameter LexerTables allowing to customize the static lexer tables
    //    to be used. The LexerTables is expected to be a type exposing 
    //    the following functions:
    //
    //        static std::size_t const state_count()
    //
    //                This function needs toreturn the number of lexer states
    //                contained in the table returned from the state_names()
    //                function.
    //
    //        static char const* const* state_names()
    //
    //                This function needs to return a pointer to a table of
    //                names of all lexer states. The table needs to have as 
    //                much entries as the state_count() function returns
    //
    //        template<typename Iterator>
    //        std::size_t next(std::size_t &start_state_, Iterator const& start_
    //          , Iterator &start_token_, Iterator const& end_
    //          , std::size_t& unique_id_);
    //
    //                This function is expected to return the next matched
    //                token from the underlying input stream.
    //
    ///////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////
    //
    //  The static_lexer class is a implementation of a Spirit.Lex 
    //  lexer on top of Ben Hanson's lexertl library (For more information 
    //  about lexertl go here: http://www.benhanson.net/lexertl.html). 
    //
    //  This class is designed to be used in conjunction with a generated, 
    //  static lexer. For more information see the documentation (The Static 
    //  Lexer Model).
    //
    //  This class is supposed to be used as the first and only template 
    //  parameter while instantiating instances of a lex::lexer class.
    //
    ///////////////////////////////////////////////////////////////////////////
    template <typename Token = token<>
      , typename LexerTables = static_::lexer
      , typename Iterator = typename Token::iterator_type
      , typename Functor = functor<Token, detail::static_data, Iterator> >
    class static_lexer 
    {
    private:
        struct dummy { void true_() {} };
        typedef void (dummy::*safe_bool)();

    public:
        // object is always valid
        operator safe_bool() const { return &dummy::true_; }

        typedef typename std::iterator_traits<Iterator>::value_type char_type;
        typedef std::basic_string<char_type> string_type;

        //  Every lexer type to be used as a lexer for Spirit has to conform to 
        //  a public interface 
        typedef Token token_type;
        typedef typename Token::id_type id_type;
        typedef iterator<Functor> iterator_type;

    private:
        // this type is purely used for the iterator_type construction below
        struct iterator_data_type 
        {
            typedef typename Functor::next_token_functor next_token_functor;
            typedef typename Functor::semantic_actions_type semantic_actions_type;
            typedef typename Functor::get_state_name_type get_state_name_type;

            iterator_data_type(next_token_functor next
                  , semantic_actions_type const& actions
                  , get_state_name_type get_state_name, std::size_t num_states
                  , bool bol)
              : next_(next), actions_(actions), get_state_name_(get_state_name)
              , num_states_(num_states), bol_(bol)
            {}

            next_token_functor next_;
            semantic_actions_type const& actions_;
            get_state_name_type get_state_name_;
            std::size_t num_states_;
            bool bol_;

            // silence MSVC warning C4512: assignment operator could not be generated
            BOOST_DELETED_FUNCTION(iterator_data_type& operator= (iterator_data_type const&))
        };

        typedef LexerTables tables_type;

        // The following static assertion fires if the referenced static lexer 
        // tables are generated by a different static lexer version as used for
        // the current compilation unit. Please regenerate your static lexer
        // tables before trying to create a static_lexer<> instance.
        BOOST_SPIRIT_ASSERT_MSG(
            tables_type::static_version == SPIRIT_STATIC_LEXER_VERSION
          , incompatible_static_lexer_version, (LexerTables));

    public:
        //  Return the start iterator usable for iterating over the generated
        //  tokens, the generated function next_token(...) is called to match 
        //  the next token from the input.
        template <typename Iterator_>
        iterator_type begin(Iterator_& first, Iterator_ const& last
          , char_type const* initial_state = 0) const
        { 
            iterator_data_type iterator_data( 
                    &tables_type::template next<Iterator_>, actions_
                  , &tables_type::state_name, tables_type::state_count()
                  , tables_type::supports_bol
                );
            return iterator_type(iterator_data, first, last, initial_state);
        }

        //  Return the end iterator usable to stop iterating over the generated 
        //  tokens.
        iterator_type end() const
        { 
            return iterator_type(); 
        }

    protected:
        //  Lexer instances can be created by means of a derived class only.
        static_lexer(unsigned int) : unique_id_(0) {}

    public:
        // interface for token definition management
        std::size_t add_token (char_type const*, char_type, std::size_t
          , char_type const*) 
        {
            return unique_id_++;
        }
        std::size_t add_token (char_type const*, string_type const&
          , std::size_t, char_type const*) 
        {
            return unique_id_++;
        }

        // interface for pattern definition management
        void add_pattern (char_type const*, string_type const&
          , string_type const&) {}

        void clear(char_type const*) {}

        std::size_t add_state(char_type const* state)
        {
            return detail::get_state_id(state, &tables_type::state_name
              , tables_type::state_count());
        }
        string_type initial_state() const 
        { 
            return tables_type::state_name(0);
        }

        // register a semantic action with the given id
        template <typename F>
        void add_action(id_type unique_id, std::size_t state, F act) 
        {
            typedef typename Functor::wrap_action_type wrapper_type;
            actions_.add_action(unique_id, state, wrapper_type::call(act));
        }

        bool init_dfa(bool /*minimize*/ = false) const { return true; }

    private:
        typename Functor::semantic_actions_type actions_;
        std::size_t unique_id_;
    };

    ///////////////////////////////////////////////////////////////////////////
    //
    //  The static_actor_lexer class is another implementation of a 
    //  Spirit.Lex lexer on top of Ben Hanson's lexertl library as outlined 
    //  above (For more information about lexertl go here: 
    //  http://www.benhanson.net/lexertl.html).
    //
    //  Just as the static_lexer class it is meant to be used with 
    //  a statically generated lexer as outlined above.
    //
    //  The only difference to the static_lexer class above is that 
    //  token_def definitions may have semantic (lexer) actions attached while 
    //  being defined:
    //
    //      int w;
    //      token_def<> word = "[^ \t\n]+";
    //      self = word[++ref(w)];        // see example: word_count_lexer
    //
    //  This class is supposed to be used as the first and only template 
    //  parameter while instantiating instances of a lex::lexer class.
    //
    ///////////////////////////////////////////////////////////////////////////
    template <typename Token = token<>
      , typename LexerTables = static_::lexer
      , typename Iterator = typename Token::iterator_type
      , typename Functor 
          = functor<Token, detail::static_data, Iterator, mpl::true_> >
    class static_actor_lexer 
      : public static_lexer<Token, LexerTables, Iterator, Functor>
    {
    protected:
        // Lexer instances can be created by means of a derived class only.
        static_actor_lexer(unsigned int flags) 
          : static_lexer<Token, LexerTables, Iterator, Functor>(flags) 
        {}
    };

}}}}

#endif

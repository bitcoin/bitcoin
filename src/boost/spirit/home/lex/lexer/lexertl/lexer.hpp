//  Copyright (c) 2001-2011 Hartmut Kaiser
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_LEX_LEXER_MAR_17_2007_0139PM)
#define BOOST_SPIRIT_LEX_LEXER_MAR_17_2007_0139PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <iosfwd>

#include <boost/spirit/home/support/detail/lexer/generator.hpp>
#include <boost/spirit/home/support/detail/lexer/rules.hpp>
#include <boost/spirit/home/support/detail/lexer/consts.hpp>
#include <boost/spirit/home/support/unused.hpp>

#include <boost/spirit/home/lex/lexer/lexertl/token.hpp>
#include <boost/spirit/home/lex/lexer/lexertl/functor.hpp>
#include <boost/spirit/home/lex/lexer/lexertl/functor_data.hpp>
#include <boost/spirit/home/lex/lexer/lexertl/iterator.hpp>
#if defined(BOOST_SPIRIT_LEXERTL_DEBUG)
#include <boost/spirit/home/support/detail/lexer/debug.hpp>
#endif

#include <iterator> // for std::iterator_traits

namespace boost { namespace spirit { namespace lex { namespace lexertl
{
    ///////////////////////////////////////////////////////////////////////////
    namespace detail
    {
        ///////////////////////////////////////////////////////////////////////
        //  The must_escape function checks if the given character value needs
        //  to be preceded by a backslash character to disable its special
        //  meaning in the context of a regular expression
        ///////////////////////////////////////////////////////////////////////
        template <typename Char>
        inline bool must_escape(Char c)
        {
            // FIXME: more needed?
            switch (c) {
            case '+': case '/': case '*': case '?':
            case '|':
            case '(': case ')':
            case '[': case ']':
            case '{': case '}':
            case '.':
            case '^': case '$':
            case '\\':
            case '"':
                return true;

            default:
                break;
            }
            return false;
        }

        ///////////////////////////////////////////////////////////////////////
        //  The escape function returns the string representation of the given
        //  character value, possibly escaped with a backslash character, to
        //  allow it being safely used in a regular expression definition.
        ///////////////////////////////////////////////////////////////////////
        template <typename Char>
        inline std::basic_string<Char> escape(Char ch)
        {
            std::basic_string<Char> result(1, ch);
            if (detail::must_escape(ch))
            {
                typedef typename std::basic_string<Char>::size_type size_type;
                result.insert((size_type)0, 1, '\\');
            }
            return result;
        }

        ///////////////////////////////////////////////////////////////////////
        //
        ///////////////////////////////////////////////////////////////////////
        inline boost::lexer::regex_flags map_flags(unsigned int flags)
        {
            unsigned int retval = boost::lexer::none;
            if (flags & match_flags::match_not_dot_newline)
                retval |= boost::lexer::dot_not_newline;
            if (flags & match_flags::match_icase)
                retval |= boost::lexer::icase;

            return boost::lexer::regex_flags(retval);
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename Lexer, typename F>
    bool generate_static(Lexer const&
      , std::basic_ostream<typename Lexer::char_type>&
      , typename Lexer::char_type const*, F);

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
    //        Iterator        The type of the iterator used to access the
    //                        underlying character stream.
    //        Token           The type of the tokens to be returned from the
    //                        exposed token iterator.
    //        Functor         The type of the InputPolicy to use to instantiate
    //                        the multi_pass iterator type to be used as the
    //                        token iterator (returned from begin()/end()).
    //
    ///////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////
    //
    //  The lexer class is a implementation of a Spirit.Lex lexer on
    //  top of Ben Hanson's lexertl library as outlined above (For more
    //  information about lexertl go here: http://www.benhanson.net/lexertl.html).
    //
    //  This class is supposed to be used as the first and only template
    //  parameter while instantiating instances of a lex::lexer class.
    //
    ///////////////////////////////////////////////////////////////////////////
    template <typename Token = token<>
      , typename Iterator = typename Token::iterator_type
      , typename Functor = functor<Token, lexertl::detail::data, Iterator> >
    class lexer
    {
    private:
        struct dummy { void true_() {} };
        typedef void (dummy::*safe_bool)();

        static std::size_t const all_states_id = static_cast<std::size_t>(-2);

    public:
        operator safe_bool() const
            { return initialized_dfa_ ? &dummy::true_ : 0; }

        typedef typename std::iterator_traits<Iterator>::value_type char_type;
        typedef std::basic_string<char_type> string_type;

        typedef boost::lexer::basic_rules<char_type> basic_rules_type;

        //  Every lexer type to be used as a lexer for Spirit has to conform to
        //  a public interface .
        typedef Token token_type;
        typedef typename Token::id_type id_type;
        typedef iterator<Functor> iterator_type;

    private:
        // this type is purely used for the iterator_type construction below
        struct iterator_data_type
        {
            typedef typename Functor::semantic_actions_type semantic_actions_type;

            iterator_data_type(
                    boost::lexer::basic_state_machine<char_type> const& sm
                  , boost::lexer::basic_rules<char_type> const& rules
                  , semantic_actions_type const& actions)
              : state_machine_(sm), rules_(rules), actions_(actions)
            {}

            boost::lexer::basic_state_machine<char_type> const& state_machine_;
            boost::lexer::basic_rules<char_type> const& rules_;
            semantic_actions_type const& actions_;

            // silence MSVC warning C4512: assignment operator could not be generated
            BOOST_DELETED_FUNCTION(iterator_data_type& operator= (iterator_data_type const&))
        };

    public:
        //  Return the start iterator usable for iterating over the generated
        //  tokens.
        iterator_type begin(Iterator& first, Iterator const& last
          , char_type const* initial_state = 0) const
        {
            if (!init_dfa())    // never minimize DFA for dynamic lexers
                return iterator_type();

            iterator_data_type iterator_data(state_machine_, rules_, actions_);
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
        lexer(unsigned int flags)
          : flags_(detail::map_flags(flags))
          , rules_(flags_)
          , initialized_dfa_(false)
        {}

    public:
        // interface for token definition management
        std::size_t add_token(char_type const* state, char_type tokendef,
            std::size_t token_id, char_type const* targetstate)
        {
            add_state(state);
            initialized_dfa_ = false;
            if (state == all_states())
                return rules_.add(state, detail::escape(tokendef), token_id, rules_.dot());

            if (0 == targetstate)
                targetstate = state;
            else
                add_state(targetstate);
            return rules_.add(state, detail::escape(tokendef), token_id, targetstate);
        }
        std::size_t add_token(char_type const* state, string_type const& tokendef,
            std::size_t token_id, char_type const* targetstate)
        {
            add_state(state);
            initialized_dfa_ = false;
            if (state == all_states())
                return rules_.add(state, tokendef, token_id, rules_.dot());

            if (0 == targetstate)
                targetstate = state;
            else
                add_state(targetstate);
            return rules_.add(state, tokendef, token_id, targetstate);
        }

        // interface for pattern definition management
        void add_pattern (char_type const* state, string_type const& name,
            string_type const& patterndef)
        {
            add_state(state);
            rules_.add_macro(name.c_str(), patterndef);
            initialized_dfa_ = false;
        }

        boost::lexer::rules const& get_rules() const { return rules_; }

        void clear(char_type const* state)
        {
            std::size_t s = rules_.state(state);
            if (boost::lexer::npos != s)
                rules_.clear(state);
            initialized_dfa_ = false;
        }
        std::size_t add_state(char_type const* state)
        {
            if (state == all_states())
                return all_states_id;

            std::size_t stateid = rules_.state(state);
            if (boost::lexer::npos == stateid) {
                stateid = rules_.add_state(state);
                initialized_dfa_ = false;
            }
            return stateid;
        }
        string_type initial_state() const
        {
            return string_type(rules_.initial());
        }
        string_type all_states() const
        {
            return string_type(rules_.all_states());
        }

        //  Register a semantic action with the given id
        template <typename F>
        void add_action(std::size_t unique_id, std::size_t state, F act)
        {
            // If you see an error here stating add_action is not a member of
            // fusion::unused_type then you are probably having semantic actions
            // attached to at least one token in the lexer definition without
            // using the lex::lexertl::actor_lexer<> as its base class.
            typedef typename Functor::wrap_action_type wrapper_type;
            if (state == all_states_id) {
                // add the action to all known states
                typedef typename
                    basic_rules_type::string_size_t_map::const_iterator
                state_iterator;

                std::size_t states = rules_.statemap().size();
                for (state_iterator it = rules_.statemap().begin(),
                                    end = rules_.statemap().end(); it != end; ++it) {
                    for (std::size_t j = 0; j < states; ++j)
                        actions_.add_action(unique_id + j, it->second, wrapper_type::call(act));
                }
            }
            else {
                actions_.add_action(unique_id, state, wrapper_type::call(act));
            }
        }
//         template <typename F>
//         void add_action(std::size_t unique_id, char_type const* state, F act)
//         {
//             typedef typename Functor::wrap_action_type wrapper_type;
//             actions_.add_action(unique_id, add_state(state), wrapper_type::call(act));
//         }

        // We do not minimize the state machine by default anymore because
        // Ben said: "If you can afford to generate a lexer at runtime, there
        //            is little point in calling minimise."
        // Go figure.
        bool init_dfa(bool minimize = false) const
        {
            if (!initialized_dfa_) {
                state_machine_.clear();
                typedef boost::lexer::basic_generator<char_type> generator;
                generator::build (rules_, state_machine_);
                if (minimize)
                    generator::minimise (state_machine_);

#if defined(BOOST_SPIRIT_LEXERTL_DEBUG)
                boost::lexer::debug::dump(state_machine_, std::cerr);
#endif
                initialized_dfa_ = true;

//                 // release memory held by rules description
//                 basic_rules_type rules;
//                 rules.init_state_info(rules_);        // preserve states
//                 std::swap(rules, rules_);
            }
            return true;
        }

    private:
        // lexertl specific data
        mutable boost::lexer::basic_state_machine<char_type> state_machine_;
        boost::lexer::regex_flags flags_;
        /*mutable*/ basic_rules_type rules_;

        typename Functor::semantic_actions_type actions_;
        mutable bool initialized_dfa_;

        // generator functions must be able to access members directly
        template <typename Lexer, typename F>
        friend bool generate_static(Lexer const&
          , std::basic_ostream<typename Lexer::char_type>&
          , typename Lexer::char_type const*, F);
    };

    ///////////////////////////////////////////////////////////////////////////
    //
    //  The actor_lexer class is another implementation of a Spirit.Lex
    //  lexer on top of Ben Hanson's lexertl library as outlined above (For
    //  more information about lexertl go here:
    //  http://www.benhanson.net/lexertl.html).
    //
    //  The only difference to the lexer class above is that token_def
    //  definitions may have semantic (lexer) actions attached while being
    //  defined:
    //
    //      int w;
    //      token_def word = "[^ \t\n]+";
    //      self = word[++ref(w)];        // see example: word_count_lexer
    //
    //  This class is supposed to be used as the first and only template
    //  parameter while instantiating instances of a lex::lexer class.
    //
    ///////////////////////////////////////////////////////////////////////////
    template <typename Token = token<>
      , typename Iterator = typename Token::iterator_type
      , typename Functor = functor<Token, lexertl::detail::data, Iterator, mpl::true_> >
    class actor_lexer : public lexer<Token, Iterator, Functor>
    {
    protected:
        //  Lexer instances can be created by means of a derived class only.
        actor_lexer(unsigned int flags)
          : lexer<Token, Iterator, Functor>(flags) {}
    };

}}}}

#endif

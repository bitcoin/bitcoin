//  Copyright (c) 2001-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_LEX_LEXER_FUNCTOR_NOV_18_2007_1112PM)
#define BOOST_SPIRIT_LEX_LEXER_FUNCTOR_NOV_18_2007_1112PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/mpl/bool.hpp>
#include <boost/detail/workaround.hpp>
#include <boost/spirit/home/lex/lexer/pass_flags.hpp>
#include <boost/assert.hpp>
#include <iterator> // for std::iterator_traits

#if 0 != __COMO_VERSION__ || !BOOST_WORKAROUND(BOOST_MSVC, <= 1310)
#define BOOST_SPIRIT_STATIC_EOF 1
#define BOOST_SPIRIT_EOF_PREFIX static
#else
#define BOOST_SPIRIT_EOF_PREFIX 
#endif

namespace boost { namespace spirit { namespace lex { namespace lexertl
{ 
    ///////////////////////////////////////////////////////////////////////////
    //
    //  functor is a template usable as the functor object for the 
    //  multi_pass iterator allowing to wrap a lexertl based dfa into a 
    //  iterator based interface.
    //  
    //    Token:      the type of the tokens produced by this functor
    //                this needs to expose a constructor with the following
    //                prototype:
    //
    //                Token(std::size_t id, std::size_t state, 
    //                      Iterator start, Iterator end)
    //
    //                where 'id' is the token id, state is the lexer state,
    //                this token has been matched in, and 'first' and 'end'  
    //                mark the start and the end of the token with respect 
    //                to the underlying character stream.
    //    FunctorData:
    //                this is expected to encapsulate the shared part of the 
    //                functor (see lex/lexer/lexertl/functor_data.hpp for an
    //                example and documentation).
    //    Iterator:   the type of the underlying iterator
    //    SupportsActors:
    //                this is expected to be a mpl::bool_, if mpl::true_ the
    //                functor invokes functors which (optionally) have 
    //                been attached to the token definitions.
    //    SupportState:
    //                this is expected to be a mpl::bool_, if mpl::true_ the
    //                functor supports different lexer states, 
    //                otherwise no lexer state is supported.
    //
    ///////////////////////////////////////////////////////////////////////////
    template <typename Token
      , template <typename, typename, typename, typename> class FunctorData
      , typename Iterator = typename Token::iterator_type
      , typename SupportsActors = mpl::false_
      , typename SupportsState = typename Token::has_state>
    class functor
    {
    public:
        typedef typename 
            std::iterator_traits<Iterator>::value_type 
        char_type;

    private:
        // Needed by compilers not implementing the resolution to DR45. For
        // reference, see
        // http://www.open-std.org/JTC1/SC22/WG21/docs/cwg_defects.html#45.
        typedef typename Token::token_value_type token_value_type;
        friend class FunctorData<Iterator, SupportsActors, SupportsState
          , token_value_type>;

        // Helper template allowing to assign a value on exit
        template <typename T>
        struct assign_on_exit
        {
            assign_on_exit(T& dst, T const& src)
              : dst_(dst), src_(src) {}

            ~assign_on_exit()
            {
                dst_ = src_;
            }

            T& dst_;
            T const& src_;

            // silence MSVC warning C4512: assignment operator could not be generated
            BOOST_DELETED_FUNCTION(assign_on_exit& operator= (assign_on_exit const&))
        };

    public:
        functor() {}

#if BOOST_WORKAROUND(BOOST_MSVC, <= 1310)
        // somehow VC7.1 needs this (meaningless) assignment operator
        functor& operator=(functor const& rhs)
        {
            return *this;
        }
#endif

        ///////////////////////////////////////////////////////////////////////
        // interface to the iterator_policies::split_functor_input policy
        typedef Token result_type;
        typedef functor unique;
        typedef FunctorData<Iterator, SupportsActors, SupportsState
          , token_value_type> shared;

        BOOST_SPIRIT_EOF_PREFIX result_type const eof;

        ///////////////////////////////////////////////////////////////////////
        typedef Iterator iterator_type;
        typedef typename shared::semantic_actions_type semantic_actions_type;
        typedef typename shared::next_token_functor next_token_functor;
        typedef typename shared::get_state_name_type get_state_name_type;

        // this is needed to wrap the semantic actions in a proper way
        typedef typename shared::wrap_action_type wrap_action_type;

        ///////////////////////////////////////////////////////////////////////
        template <typename MultiPass>
        static result_type& get_next(MultiPass& mp, result_type& result)
        {
            typedef typename result_type::id_type id_type;

            shared& data = mp.shared()->ftor;
            for(;;) 
            {
                if (data.get_first() == data.get_last()) 
#if defined(BOOST_SPIRIT_STATIC_EOF)
                    return result = eof;
#else
                    return result = mp.ftor.eof;
#endif

                data.reset_value();
                Iterator end = data.get_first();
                std::size_t unique_id = boost::lexer::npos;
                bool prev_bol = false;

                // lexer matching might change state
                std::size_t state = data.get_state();
                std::size_t id = data.next(end, unique_id, prev_bol);

                if (boost::lexer::npos == id) {   // no match
#if defined(BOOST_SPIRIT_LEXERTL_DEBUG)
                    std::string next;
                    Iterator it = data.get_first();
                    for (std::size_t i = 0; i < 10 && it != data.get_last(); ++it, ++i)
                        next += *it;

                    std::cerr << "Not matched, in state: " << state 
                              << ", lookahead: >" << next << "<" << std::endl;
#endif
                    return result = result_type(0);
                }
                else if (0 == id) {         // EOF reached
#if defined(BOOST_SPIRIT_STATIC_EOF)
                    return result = eof;
#else
                    return result = mp.ftor.eof;
#endif
                }

#if defined(BOOST_SPIRIT_LEXERTL_DEBUG)
                {
                    std::string next;
                    Iterator it = end;
                    for (std::size_t i = 0; i < 10 && it != data.get_last(); ++it, ++i)
                        next += *it;

                    std::cerr << "Matched: " << id << ", in state: " 
                              << state << ", string: >" 
                              << std::basic_string<char_type>(data.get_first(), end) << "<"
                              << ", lookahead: >" << next << "<" << std::endl;
                    if (data.get_state() != state) {
                        std::cerr << "Switched to state: " 
                                  << data.get_state() << std::endl;
                    }
                }
#endif
                // account for a possibly pending lex::more(), i.e. moving 
                // data.first_ back to the start of the previously matched token.
                bool adjusted = data.adjust_start();

                // set the end of the matched input sequence in the token data
                data.set_end(end);

                // invoke attached semantic actions, if defined, might change
                // state, id, data.first_, and/or end
                BOOST_SCOPED_ENUM(pass_flags) pass = 
                    data.invoke_actions(state, id, unique_id, end);

                if (data.has_value()) {
                    // return matched token using the token value as set before
                    // using data.set_value(), advancing 'data.first_' past the 
                    // matched sequence
                    assign_on_exit<Iterator> on_exit(data.get_first(), end);
                    return result = result_type(id_type(id), state, data.get_value());
                }
                else if (pass_flags::pass_normal == pass) {
                    // return matched token, advancing 'data.first_' past the 
                    // matched sequence
                    assign_on_exit<Iterator> on_exit(data.get_first(), end);
                    return result = result_type(id_type(id), state, data.get_first(), end);
                }
                else if (pass_flags::pass_fail == pass) {
#if defined(BOOST_SPIRIT_LEXERTL_DEBUG)
                    std::cerr << "Matching forced to fail" << std::endl; 
#endif
                    // if the data.first_ got adjusted above, revert this adjustment
                    if (adjusted)
                        data.revert_adjust_start();

                    // one of the semantic actions signaled no-match
                    data.reset_bol(prev_bol);
                    if (state != data.get_state())
                        continue;       // retry matching if state has changed

                    // if the state is unchanged repeating the match wouldn't
                    // move the input forward, causing an infinite loop
                    return result = result_type(0);
                }

#if defined(BOOST_SPIRIT_LEXERTL_DEBUG)
                std::cerr << "Token ignored, continuing matching" << std::endl; 
#endif
            // if this token needs to be ignored, just repeat the matching,
            // while starting right after the current match
                data.get_first() = end;
            }
        }

        // set_state are propagated up to the iterator interface, allowing to 
        // manipulate the current lexer state through any of the exposed 
        // iterators.
        template <typename MultiPass>
        static std::size_t set_state(MultiPass& mp, std::size_t state) 
        { 
            std::size_t oldstate = mp.shared()->ftor.get_state();
            mp.shared()->ftor.set_state(state);

#if defined(BOOST_SPIRIT_LEXERTL_DEBUG)
            std::cerr << "Switching state from: " << oldstate 
                      << " to: " << state
                      << std::endl;
#endif
            return oldstate; 
        }

        template <typename MultiPass>
        static std::size_t get_state(MultiPass& mp) 
        { 
            return mp.shared()->ftor.get_state();
        }

        template <typename MultiPass>
        static std::size_t 
        map_state(MultiPass const& mp, char_type const* statename)  
        { 
            return mp.shared()->ftor.get_state_id(statename);
        }

        // we don't need this, but it must be there
        template <typename MultiPass>
        static void destroy(MultiPass const&) {}
    };

#if defined(BOOST_SPIRIT_STATIC_EOF)
    ///////////////////////////////////////////////////////////////////////////
    //  eof token
    ///////////////////////////////////////////////////////////////////////////
    template <typename Token
      , template <typename, typename, typename, typename> class FunctorData
      , typename Iterator, typename SupportsActors, typename SupportsState>
    typename functor<Token, FunctorData, Iterator, SupportsActors, SupportsState>::result_type const
        functor<Token, FunctorData, Iterator, SupportsActors, SupportsState>::eof = 
            typename functor<Token, FunctorData, Iterator, SupportsActors
              , SupportsState>::result_type();
#endif

}}}}

#undef BOOST_SPIRIT_EOF_PREFIX
#undef BOOST_SPIRIT_STATIC_EOF

#endif

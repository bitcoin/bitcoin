//  Copyright (c) 2001-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_LEX_LEXER_STATIC_FUNCTOR_DATA_FEB_10_2008_0755PM)
#define BOOST_SPIRIT_LEX_LEXER_STATIC_FUNCTOR_DATA_FEB_10_2008_0755PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/support/detail/lexer/generator.hpp>
#include <boost/spirit/home/support/detail/lexer/rules.hpp>
#include <boost/spirit/home/support/detail/lexer/state_machine.hpp>
#include <boost/spirit/home/lex/lexer/lexertl/iterator_tokenizer.hpp>
#include <boost/spirit/home/lex/lexer/lexertl/semantic_action_data.hpp>
#include <boost/spirit/home/lex/lexer/lexertl/wrap_action.hpp>
#include <boost/spirit/home/support/assert_msg.hpp>
#include <boost/mpl/bool.hpp>
#include <iterator> // for std::iterator_traits

namespace boost { namespace spirit { namespace lex { namespace lexertl
{ 
    namespace detail
    {
        ///////////////////////////////////////////////////////////////////////
        template <typename Char>
        inline bool zstr_compare(Char const* s1, Char const* s2)
        {
            for (; *s1 || *s2; ++s1, ++s2)
                if (*s1 != *s2)
                    return false;
            return true;
        }

        template <typename Char, typename F>
        inline std::size_t get_state_id(Char const* state, F f
          , std::size_t numstates)
        {
            for (std::size_t i = 0; i < numstates; ++i)
            {
                if (zstr_compare(f(i), state))
                    return i;
            }
            return boost::lexer::npos;
        }

        ///////////////////////////////////////////////////////////////////////
        template <typename Iterator, typename HasActors, typename HasState
          , typename TokenValue>
        class static_data;    // no default specialization

        ///////////////////////////////////////////////////////////////////////
        //  doesn't support no state and no actors
        template <typename Iterator, typename TokenValue>
        class static_data<Iterator, mpl::false_, mpl::false_, TokenValue>
        {
        protected:
            typedef typename 
                std::iterator_traits<Iterator>::value_type 
            char_type;

        public:
            typedef Iterator base_iterator_type;
            typedef iterator_range<Iterator> token_value_type;
            typedef token_value_type get_value_type;
            typedef std::size_t state_type;
            typedef char_type const* state_name_type;
            typedef unused_type semantic_actions_type;
            typedef detail::wrap_action<unused_type, Iterator, static_data
              , std::size_t> wrap_action_type;
 
            typedef std::size_t (*next_token_functor)(std::size_t&, 
                bool&, Iterator&, Iterator const&, std::size_t&);
            typedef char_type const* (*get_state_name_type)(std::size_t);

            // initialize the shared data 
            template <typename IterData>
            static_data (IterData const& data, Iterator& first
                  , Iterator const& last)
              : first_(first), last_(last) 
              , next_token_(data.next_)
              , get_state_name_(data.get_state_name_)
              , bol_(data.bol_) {}

            // The following functions are used by the implementation of the 
            // placeholder '_state'.
            template <typename Char>
            void set_state_name (Char const*) 
            {
                // If you see a compile time assertion below you're probably 
                // using a token type not supporting lexer states (the 3rd 
                // template parameter of the token is mpl::false_), but your 
                // code uses state changes anyways.
                BOOST_SPIRIT_ASSERT_FAIL(Char,
                    tried_to_set_state_of_stateless_token, ());
            }
            char_type const* get_state_name() const 
            { 
                return get_state_name_(0); 
            }
            std::size_t get_state_id(char_type const*) const 
            { 
                return 0; 
            }

            // The function get_eoi() is used by the implementation of the 
            // placeholder '_eoi'.
            Iterator const& get_eoi() const { return last_; }

            // The function less() is used by the implementation of the support
            // function lex::less(). Its functionality is equivalent to flex'
            // function yyless(): it returns an iterator positioned to the 
            // nth input character beyond the current start iterator (i.e. by
            // assigning the return value to the placeholder '_end' it is 
            // possible to return all but the first n characters of the current 
            // token back to the input stream. 
            //
            // This function does nothing as long as no semantic actions are 
            // used.
            Iterator const& less(Iterator const& it, int) 
            { 
                // The following assertion fires most likely because you are 
                // using lexer semantic actions without using the actor_lexer
                // as the base class for your token definition class.
                BOOST_ASSERT(false && 
                    "Are you using lexer semantic actions without using the "
                    "actor_lexer base?");
                return it; 
            }

            // The function more() is used by the implementation of the support 
            // function lex::more(). Its functionality is equivalent to flex'
            // function yymore(): it tells the lexer that the next time it 
            // matches a rule, the corresponding token should be appended onto 
            // the current token value rather than replacing it.
            // 
            // These functions do nothing as long as no semantic actions are 
            // used.
            void more() 
            { 
                // The following assertion fires most likely because you are 
                // using lexer semantic actions without using the actor_lexer
                // as the base class for your token definition class.
                BOOST_ASSERT(false && 
                    "Are you using lexer semantic actions without using the "
                    "actor_lexer base?"); 
            }
            bool adjust_start() { return false; }
            void revert_adjust_start() {}

            // The function lookahead() is used by the implementation of the 
            // support function lex::lookahead. It can be used to implement 
            // lookahead for lexer engines not supporting constructs like flex'
            // a/b  (match a, but only when followed by b):
            //
            // This function does nothing as long as no semantic actions are 
            // used.
            bool lookahead(std::size_t, std::size_t /*state*/ = std::size_t(~0)) 
            { 
                // The following assertion fires most likely because you are 
                // using lexer semantic actions without using the actor_lexer
                // as the base class for your token definition class.
                BOOST_ASSERT(false && 
                    "Are you using lexer semantic actions without using the "
                    "actor_lexer base?");
                return false; 
            }

            // the functions next, invoke_actions, and get_state are used by 
            // the functor implementation below

            // The function next() tries to match the next token from the 
            // underlying input sequence. 
            std::size_t next(Iterator& end, std::size_t& unique_id, bool& prev_bol)
            {
                prev_bol = bol_;

                std::size_t state = 0;
                return next_token_(state, bol_, end, last_, unique_id);
            }

            // nothing to invoke, so this is empty
            BOOST_SCOPED_ENUM(pass_flags) invoke_actions(std::size_t
              , std::size_t, std::size_t, Iterator const&) 
            {
                return pass_flags::pass_normal;    // always accept
            }

            std::size_t get_state() const { return 0; }
            void set_state(std::size_t) {}

            void set_end(Iterator const&) {}

            Iterator& get_first() { return first_; }
            Iterator const& get_first() const { return first_; }
            Iterator const& get_last() const { return last_; }

            iterator_range<Iterator> get_value() const 
            { 
                return iterator_range<Iterator>(first_, last_); 
            }
            bool has_value() const { return false; }
            void reset_value() {}

            void reset_bol(bool bol) { bol_ = bol; }

        protected:
            Iterator& first_;
            Iterator last_;

            next_token_functor next_token_;
            get_state_name_type get_state_name_;

            bool bol_;      // helper storing whether last character was \n

            // silence MSVC warning C4512: assignment operator could not be generated
            BOOST_DELETED_FUNCTION(static_data& operator= (static_data const&))
        };

        ///////////////////////////////////////////////////////////////////////
        //  doesn't support lexer semantic actions, but supports state
        template <typename Iterator, typename TokenValue>
        class static_data<Iterator, mpl::false_, mpl::true_, TokenValue>
          : public static_data<Iterator, mpl::false_, mpl::false_, TokenValue>
        {
        protected:
            typedef static_data<Iterator, mpl::false_, mpl::false_, TokenValue> base_type;
            typedef typename base_type::char_type char_type;

        public:
            typedef Iterator base_iterator_type;
            typedef iterator_range<Iterator> token_value_type;
            typedef token_value_type get_value_type;
            typedef typename base_type::state_type state_type;
            typedef typename base_type::state_name_type state_name_type;
            typedef typename base_type::semantic_actions_type 
                semantic_actions_type;

            // initialize the shared data 
            template <typename IterData>
            static_data (IterData const& data, Iterator& first
                  , Iterator const& last)
              : base_type(data, first, last), state_(0)
              , num_states_(data.num_states_) {}

            // The following functions are used by the implementation of the 
            // placeholder '_state'.
            void set_state_name (char_type const* new_state) 
            { 
                std::size_t state_id = lexertl::detail::get_state_id(new_state
                  , this->get_state_name_, num_states_);

                // if the following assertion fires you've probably been using 
                // a lexer state name which was not defined in your token 
                // definition
                BOOST_ASSERT(state_id != boost::lexer::npos);

                if (state_id != boost::lexer::npos)
                    state_ = state_id;
            }
            char_type const* get_state_name() const
            {
                return this->get_state_name_(state_);
            }
            std::size_t get_state_id(char_type const* state) const 
            { 
                return lexertl::detail::get_state_id(state
                  , this->get_state_name_, num_states_); 
            }

            // the functions next() and get_state() are used by the functor 
            // implementation below

            // The function next() tries to match the next token from the 
            // underlying input sequence. 
            std::size_t next(Iterator& end, std::size_t& unique_id, bool& prev_bol)
            {
                prev_bol = this->bol_;
                return this->next_token_(state_, this->bol_, end, this->last_
                  , unique_id);
            }

            std::size_t& get_state() { return state_; }
            void set_state(std::size_t state) { state_ = state; }

        protected:
            std::size_t state_;
            std::size_t num_states_;

            // silence MSVC warning C4512: assignment operator could not be generated
            BOOST_DELETED_FUNCTION(static_data& operator= (static_data const&))
        };

        ///////////////////////////////////////////////////////////////////////
        //  does support actors, but may have no state
        template <typename Iterator, typename HasState, typename TokenValue>
        class static_data<Iterator, mpl::true_, HasState, TokenValue> 
          : public static_data<Iterator, mpl::false_, HasState, TokenValue>
        {
        public:
            typedef semantic_actions<Iterator, HasState, static_data>
                semantic_actions_type;

        protected:
            typedef static_data<Iterator, mpl::false_, HasState, TokenValue> 
                base_type;
            typedef typename base_type::char_type char_type;
            typedef typename semantic_actions_type::functor_wrapper_type
                functor_wrapper_type;

        public:
            typedef Iterator base_iterator_type;
            typedef TokenValue token_value_type;
            typedef TokenValue const& get_value_type;
            typedef typename base_type::state_type state_type;
            typedef typename base_type::state_name_type state_name_type;

            typedef detail::wrap_action<functor_wrapper_type
              , Iterator, static_data, std::size_t> wrap_action_type;

            template <typename IterData>
            static_data (IterData const& data, Iterator& first
                  , Iterator const& last)
              : base_type(data, first, last)
              , actions_(data.actions_), hold_()
              , value_(iterator_range<Iterator>(first, last))
              , has_value_(false)
              , has_hold_(false)
            {}

            // invoke attached semantic actions, if defined
            BOOST_SCOPED_ENUM(pass_flags) invoke_actions(std::size_t state
              , std::size_t& id, std::size_t unique_id, Iterator& end)
            {
                return actions_.invoke_actions(state, id, unique_id, end, *this); 
            }

            // The function less() is used by the implementation of the support
            // function lex::less(). Its functionality is equivalent to flex'
            // function yyless(): it returns an iterator positioned to the 
            // nth input character beyond the current start iterator (i.e. by
            // assigning the return value to the placeholder '_end' it is 
            // possible to return all but the first n characters of the current 
            // token back to the input stream). 
            Iterator const& less(Iterator& it, int n) 
            {
                it = this->get_first();
                std::advance(it, n);
                return it;
            }

            // The function more() is used by the implementation of the support 
            // function lex::more(). Its functionality is equivalent to flex'
            // function yymore(): it tells the lexer that the next time it 
            // matches a rule, the corresponding token should be appended onto 
            // the current token value rather than replacing it.
            void more()
            {
                hold_ = this->get_first();
                has_hold_ = true;
            }

            // The function lookahead() is used by the implementation of the 
            // support function lex::lookahead. It can be used to implement 
            // lookahead for lexer engines not supporting constructs like flex'
            // a/b  (match a, but only when followed by b)
            bool lookahead(std::size_t id, std::size_t state = std::size_t(~0))
            {
                Iterator end = end_;
                std::size_t unique_id = boost::lexer::npos;
                bool bol = this->bol_;

                if (std::size_t(~0) == state)
                    state = this->state_;

                return id == this->next_token_(
                    state, bol, end, this->get_eoi(), unique_id);
            }

            // The adjust_start() and revert_adjust_start() are helper 
            // functions needed to implement the functionality required for 
            // lex::more(). It is called from the functor body below.
            bool adjust_start()
            {
                if (!has_hold_)
                    return false;

                std::swap(this->get_first(), hold_);
                has_hold_ = false;
                return true;
            }
            void revert_adjust_start()
            {
                // this will be called only if adjust_start above returned true
                std::swap(this->get_first(), hold_);
                has_hold_ = true;
            }

            TokenValue const& get_value() const 
            {
                if (!has_value_) {
                    value_ = iterator_range<Iterator>(this->get_first(), end_);
                    has_value_ = true;
                }
                return value_;
            }
            template <typename Value>
            void set_value(Value const& val)
            {
                value_ = val;
                has_value_ = true;
            }
            void set_end(Iterator const& it)
            {
                end_ = it;
            }
            bool has_value() const { return has_value_; }
            void reset_value() { has_value_ = false; }

        protected:
            semantic_actions_type const& actions_;
            Iterator hold_;     // iterator needed to support lex::more()
            Iterator end_;      // iterator pointing to end of matched token
            mutable TokenValue value_;  // token value to use
            mutable bool has_value_;    // 'true' if value_ is valid
            bool has_hold_;     // 'true' if hold_ is valid

            // silence MSVC warning C4512: assignment operator could not be generated
            BOOST_DELETED_FUNCTION(static_data& operator= (static_data const&))
        };

        ///////////////////////////////////////////////////////////////////////
        //  does support lexer semantic actions, may support state, is used for
        //  position_token exposing exactly one type
        template <typename Iterator, typename HasState, typename TokenValue>
        class static_data<Iterator, mpl::true_, HasState, boost::optional<TokenValue> > 
          : public static_data<Iterator, mpl::false_, HasState, TokenValue>
        {
        public:
            typedef semantic_actions<Iterator, HasState, static_data> 
                semantic_actions_type;

        protected:
            typedef static_data<Iterator, mpl::false_, HasState, TokenValue> 
                base_type;
            typedef typename base_type::char_type char_type;
            typedef typename semantic_actions_type::functor_wrapper_type
                functor_wrapper_type;

        public:
            typedef Iterator base_iterator_type;
            typedef boost::optional<TokenValue> token_value_type;
            typedef boost::optional<TokenValue> const& get_value_type;
            typedef typename base_type::state_type state_type;
            typedef typename base_type::state_name_type state_name_type;

            typedef detail::wrap_action<functor_wrapper_type
              , Iterator, static_data, std::size_t> wrap_action_type;

            template <typename IterData>
            static_data (IterData const& data_, Iterator& first, Iterator const& last)
              : base_type(data_, first, last)
              , actions_(data_.actions_), hold_()
              , has_value_(false), has_hold_(false) 
            {
                spirit::traits::assign_to(first, last, value_);
                has_value_ = true;
            }

            // invoke attached semantic actions, if defined
            BOOST_SCOPED_ENUM(pass_flags) invoke_actions(std::size_t state
              , std::size_t& id, std::size_t unique_id, Iterator& end)
            {
                return actions_.invoke_actions(state, id, unique_id, end, *this); 
            }

            // The function less() is used by the implementation of the support
            // function lex::less(). Its functionality is equivalent to flex'
            // function yyless(): it returns an iterator positioned to the 
            // nth input character beyond the current start iterator (i.e. by
            // assigning the return value to the placeholder '_end' it is 
            // possible to return all but the first n characters of the current 
            // token back to the input stream). 
            Iterator const& less(Iterator& it, int n) 
            {
                it = this->get_first();
                std::advance(it, n);
                return it;
            }

            // The function more() is used by the implementation of the support 
            // function lex::more(). Its functionality is equivalent to flex'
            // function yymore(): it tells the lexer that the next time it 
            // matches a rule, the corresponding token should be appended onto 
            // the current token value rather than replacing it.
            void more()
            {
                hold_ = this->get_first();
                has_hold_ = true;
            }

            // The function lookahead() is used by the implementation of the 
            // support function lex::lookahead. It can be used to implement 
            // lookahead for lexer engines not supporting constructs like flex'
            // a/b  (match a, but only when followed by b)
            bool lookahead(std::size_t id, std::size_t state = std::size_t(~0))
            {
                Iterator end = end_;
                std::size_t unique_id = boost::lexer::npos;
                bool bol = this->bol_;

                if (std::size_t(~0) == state)
                    state = this->state_;

                return id == this->next_token_(
                    state, bol, end, this->get_eoi(), unique_id);
            }

            // The adjust_start() and revert_adjust_start() are helper 
            // functions needed to implement the functionality required for 
            // lex::more(). It is called from the functor body below.
            bool adjust_start()
            {
                if (!has_hold_)
                    return false;

                std::swap(this->get_first(), hold_);
                has_hold_ = false;
                return true;
            }
            void revert_adjust_start()
            {
                // this will be called only if adjust_start above returned true
                std::swap(this->get_first(), hold_);
                has_hold_ = true;
            }

            TokenValue const& get_value() const 
            {
                if (!has_value_) {
                    spirit::traits::assign_to(this->get_first(), end_, value_);
                    has_value_ = true;
                }
                return value_;
            }
            template <typename Value>
            void set_value(Value const& val)
            {
                value_ = val;
                has_value_ = true;
            }
            void set_end(Iterator const& it)
            {
                end_ = it;
            }
            bool has_value() const { return has_value_; }
            void reset_value() { has_value_ = false; }

        protected:
            semantic_actions_type const& actions_;
            Iterator hold_;     // iterator needed to support lex::more()
            Iterator end_;      // iterator pointing to end of matched token
            mutable token_value_type value_;  // token value to use
            mutable bool has_value_;    // 'true' if value_ is valid
            bool has_hold_;     // 'true' if hold_ is valid

            // silence MSVC warning C4512: assignment operator could not be generated
            BOOST_DELETED_FUNCTION(static_data& operator= (static_data const&))
        };
    }
}}}}

#endif

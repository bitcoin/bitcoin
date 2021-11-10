//  Copyright (c) 2001-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_LEXERTL_ITERATOR_TOKENISER_MARCH_22_2007_0859AM)
#define BOOST_SPIRIT_LEXERTL_ITERATOR_TOKENISER_MARCH_22_2007_0859AM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/support/detail/lexer/state_machine.hpp>
#include <boost/spirit/home/support/detail/lexer/consts.hpp>
#include <boost/spirit/home/support/detail/lexer/size_t.hpp>
#include <boost/spirit/home/support/detail/lexer/char_traits.hpp>
#include <iterator> // for std::iterator_traits
#include <vector>

namespace boost { namespace spirit { namespace lex { namespace lexertl
{ 
    ///////////////////////////////////////////////////////////////////////////
    template<typename Iterator>
    class basic_iterator_tokeniser
    {
    public:
        typedef std::vector<std::size_t> size_t_vector;
        typedef typename std::iterator_traits<Iterator>::value_type char_type;

        static std::size_t next (
            boost::lexer::basic_state_machine<char_type> const& state_machine_
          , std::size_t &dfa_state_, bool& bol_, Iterator &start_token_
          , Iterator const& end_, std::size_t& unique_id_)
        {
            if (start_token_ == end_) 
            {
                unique_id_ = boost::lexer::npos;
                return 0;
            }

            bool bol = bol_;
            boost::lexer::detail::internals const& internals_ =
                state_machine_.data();

        again:
            std::size_t const* lookup_ = &internals_._lookup[dfa_state_]->
                front ();
            std::size_t dfa_alphabet_ = internals_._dfa_alphabet[dfa_state_];
            std::size_t const* dfa_ = &internals_._dfa[dfa_state_]->front ();

            std::size_t const* ptr_ = dfa_ + dfa_alphabet_;
            Iterator curr_ = start_token_;
            bool end_state_ = *ptr_ != 0;
            std::size_t id_ = *(ptr_ + boost::lexer::id_index);
            std::size_t uid_ = *(ptr_ + boost::lexer::unique_id_index);
            std::size_t end_start_state_ = dfa_state_;
            bool end_bol_ = bol_;
            Iterator end_token_ = start_token_;

            while (curr_ != end_)
            {
                std::size_t const BOL_state_ = ptr_[boost::lexer::bol_index];
                std::size_t const EOL_state_ = ptr_[boost::lexer::eol_index];

                if (BOL_state_ && bol)
                {
                    ptr_ = &dfa_[BOL_state_ * dfa_alphabet_];
                }
                else if (EOL_state_ && *curr_ == '\n')
                {
                    ptr_ = &dfa_[EOL_state_ * dfa_alphabet_];
                }
                else
                {
                    typedef typename 
                        std::iterator_traits<Iterator>::value_type 
                    value_type;
                    typedef typename 
                        boost::lexer::char_traits<value_type>::index_type 
                    index_type;

                    index_type index = 
                        boost::lexer::char_traits<value_type>::call(*curr_++);
                    bol = (index == '\n') ? true : false;
                    std::size_t const state_ = ptr_[
                        lookup_[static_cast<std::size_t>(index)]];

                    if (state_ == 0)
                    {
                        break;
                    }

                    ptr_ = &dfa_[state_ * dfa_alphabet_];
                }

                if (*ptr_)
                {
                    end_state_ = true;
                    id_ = *(ptr_ + boost::lexer::id_index);
                    uid_ = *(ptr_ + boost::lexer::unique_id_index);
                    end_start_state_ = *(ptr_ + boost::lexer::state_index);
                    end_bol_ = bol;
                    end_token_ = curr_;
                }
            }

            std::size_t const EOL_state_ = ptr_[boost::lexer::eol_index];

            if (EOL_state_ && curr_ == end_)
            {
                ptr_ = &dfa_[EOL_state_ * dfa_alphabet_];

                if (*ptr_)
                {
                    end_state_ = true;
                    id_ = *(ptr_ + boost::lexer::id_index);
                    uid_ = *(ptr_ + boost::lexer::unique_id_index);
                    end_start_state_ = *(ptr_ + boost::lexer::state_index);
                    end_bol_ = bol;
                    end_token_ = curr_;
                }
            }

            if (end_state_) {
                // return longest match
                dfa_state_ = end_start_state_;
                start_token_ = end_token_;

                if (id_ == 0)
                {
                    bol = end_bol_;
                    goto again;
                }
                else
                {
                    bol_ = end_bol_;
                }
            }
            else {
                bol_ = (*start_token_ == '\n') ? true : false;
                id_ = boost::lexer::npos;
                uid_ = boost::lexer::npos;
            }

            unique_id_ = uid_;
            return id_;
        }

        ///////////////////////////////////////////////////////////////////////
        static std::size_t next (
            boost::lexer::basic_state_machine<char_type> const& state_machine_
          , bool& bol_, Iterator &start_token_, Iterator const& end_
          , std::size_t& unique_id_)
        {
            if (start_token_ == end_)
            {
                unique_id_ = boost::lexer::npos;
                return 0;
            }

            bool bol = bol_;
            std::size_t const* lookup_ = &state_machine_.data()._lookup[0]->front();
            std::size_t dfa_alphabet_ = state_machine_.data()._dfa_alphabet[0];
            std::size_t const* dfa_ = &state_machine_.data()._dfa[0]->front ();
            std::size_t const* ptr_ = dfa_ + dfa_alphabet_;

            Iterator curr_ = start_token_;
            bool end_state_ = *ptr_ != 0;
            std::size_t id_ = *(ptr_ + boost::lexer::id_index);
            std::size_t uid_ = *(ptr_ + boost::lexer::unique_id_index);
            bool end_bol_ = bol_;
            Iterator end_token_ = start_token_;

            while (curr_ != end_)
            {
                std::size_t const BOL_state_ = ptr_[boost::lexer::bol_index];
                std::size_t const EOL_state_ = ptr_[boost::lexer::eol_index];

                if (BOL_state_ && bol)
                {
                    ptr_ = &dfa_[BOL_state_ * dfa_alphabet_];
                }
                else if (EOL_state_ && *curr_ == '\n')
                {
                    ptr_ = &dfa_[EOL_state_ * dfa_alphabet_];
                }
                else
                {
                    typedef typename 
                        std::iterator_traits<Iterator>::value_type 
                    value_type;
                    typedef typename 
                        boost::lexer::char_traits<value_type>::index_type 
                    index_type;

                    index_type index = 
                        boost::lexer::char_traits<value_type>::call(*curr_++);
                    bol = (index == '\n') ? true : false;
                    std::size_t const state_ = ptr_[
                        lookup_[static_cast<std::size_t>(index)]];

                    if (state_ == 0)
                    {
                        break;
                    }

                    ptr_ = &dfa_[state_ * dfa_alphabet_];
                }

                if (*ptr_)
                {
                    end_state_ = true;
                    id_ = *(ptr_ + boost::lexer::id_index);
                    uid_ = *(ptr_ + boost::lexer::unique_id_index);
                    end_bol_ = bol;
                    end_token_ = curr_;
                }
            }

            std::size_t const EOL_state_ = ptr_[boost::lexer::eol_index];

            if (EOL_state_ && curr_ == end_)
            {
                ptr_ = &dfa_[EOL_state_ * dfa_alphabet_];

                if (*ptr_)
                {
                    end_state_ = true;
                    id_ = *(ptr_ + boost::lexer::id_index);
                    uid_ = *(ptr_ + boost::lexer::unique_id_index);
                    end_bol_ = bol;
                    end_token_ = curr_;
                }
            }

            if (end_state_) {
                // return longest match
                bol_ = end_bol_;
                start_token_ = end_token_;
            }
            else {
                bol_ = *start_token_ == '\n';
                id_ = boost::lexer::npos;
                uid_ = boost::lexer::npos;
            }

            unique_id_ = uid_;
            return id_;
        }
    };

}}}}

#endif

// debug.hpp
// Copyright (c) 2007-2009 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_SPIRIT_SUPPORT_DETAIL_LEXER_DEBUG_HPP
#define BOOST_SPIRIT_SUPPORT_DETAIL_LEXER_DEBUG_HPP

#include <map>
#include <ostream>
#include "rules.hpp"
#include "size_t.hpp"
#include "state_machine.hpp"
#include "string_token.hpp"
#include <vector>

namespace boost
{
namespace lexer
{
template<typename CharT>
class basic_debug
{
public:
    typedef std::basic_ostream<CharT> ostream;
    typedef std::basic_string<CharT> string;
    typedef std::vector<std::size_t> size_t_vector;

    static void escape_control_chars (const string &in_, string &out_)
    {
        const CharT *ptr_ = in_.c_str ();
        std::size_t size_ = in_.size ();

        out_.clear ();

        while (size_)
        {
            basic_string_token<CharT>::escape_char (*ptr_, out_);
            ++ptr_;
            --size_;
        }
    }

    static void dump (const basic_state_machine<CharT> &state_machine_,
        basic_rules<CharT> &rules_, ostream &stream_)
    {
        typename basic_state_machine<CharT>::iterator iter_ =
            state_machine_.begin ();
        typename basic_state_machine<CharT>::iterator end_ =
            state_machine_.end ();

        for (std::size_t dfa_ = 0, dfas_ = state_machine_.size ();
            dfa_ < dfas_; ++dfa_)
        {
            lexer_state (stream_);
            stream_ << rules_.state (dfa_) << std::endl << std::endl;

            dump_ex (iter_, stream_);
        }
    }

    static void dump (const basic_state_machine<CharT> &state_machine_,
        ostream &stream_)
    {
        typename basic_state_machine<CharT>::iterator iter_ =
            state_machine_.begin ();
        typename basic_state_machine<CharT>::iterator end_ =
            state_machine_.end ();

        for (std::size_t dfa_ = 0, dfas_ = state_machine_.size ();
            dfa_ < dfas_; ++dfa_)
        {
            lexer_state (stream_);
            stream_ << dfa_ << std::endl << std::endl;

            dump_ex (iter_, stream_);
        }
    }

protected:
    typedef std::basic_stringstream<CharT> stringstream;

    static void dump_ex (typename basic_state_machine<CharT>::iterator &iter_,
        ostream &stream_)
    {
        const std::size_t states_ = iter_->states;

        for (std::size_t i_ = 0; i_ < states_; ++i_)
        {
            state (stream_);
            stream_ << i_ << std::endl;

            if (iter_->end_state)
            {
                end_state (stream_);
                stream_ << iter_->id;
                unique_id (stream_);
                stream_ << iter_->unique_id;
                dfa (stream_);
                stream_ << iter_->goto_dfa;
                stream_ << std::endl;
            }

            if (iter_->bol_index != npos)
            {
                bol (stream_);
                stream_ << iter_->bol_index << std::endl;
            }

            if (iter_->eol_index != npos)
            {
                eol (stream_);
                stream_ << iter_->eol_index << std::endl;
            }

            const std::size_t transitions_ = iter_->transitions;

            if (transitions_ == 0)
            {
                ++iter_;
            }

            for (std::size_t t_ = 0; t_ < transitions_; ++t_)
            {
                std::size_t goto_state_ = iter_->goto_state;

                if (iter_->token.any ())
                {
                    any (stream_);
                }
                else
                {
                    open_bracket (stream_);

                    if (iter_->token._negated)
                    {
                        negated (stream_);
                    }

                    string charset_;
                    CharT c_ = 0;

                    escape_control_chars (iter_->token._charset,
                        charset_);
                    c_ = *charset_.c_str ();

                    if (!iter_->token._negated &&
                        (c_ == '^' || c_ == ']'))
                    {
                        stream_ << '\\';
                    }

                    stream_ << charset_;
                    close_bracket (stream_);
                }

                stream_ << goto_state_ << std::endl;
                ++iter_;
            }

            stream_ << std::endl;
        }
    }

    static void lexer_state (std::ostream &stream_)
    {
        stream_ << "Lexer state: ";
    }

    static void lexer_state (std::wostream &stream_)
    {
        stream_ << L"Lexer state: ";
    }

    static void state (std::ostream &stream_)
    {
        stream_ << "State: ";
    }

    static void state (std::wostream &stream_)
    {
        stream_ << L"State: ";
    }

    static void bol (std::ostream &stream_)
    {
        stream_ << "  BOL -> ";
    }

    static void bol (std::wostream &stream_)
    {
        stream_ << L"  BOL -> ";
    }

    static void eol (std::ostream &stream_)
    {
        stream_ << "  EOL -> ";
    }

    static void eol (std::wostream &stream_)
    {
        stream_ << L"  EOL -> ";
    }

    static void end_state (std::ostream &stream_)
    {
        stream_ << "  END STATE, Id = ";
    }

    static void end_state (std::wostream &stream_)
    {
        stream_ << L"  END STATE, Id = ";
    }

    static void unique_id (std::ostream &stream_)
    {
        stream_ << ", Unique Id = ";
    }

    static void unique_id (std::wostream &stream_)
    {
        stream_ << L", Unique Id = ";
    }

    static void any (std::ostream &stream_)
    {
        stream_ << "  . -> ";
    }

    static void any (std::wostream &stream_)
    {
        stream_ << L"  . -> ";
    }

    static void open_bracket (std::ostream &stream_)
    {
        stream_ << "  [";
    }

    static void open_bracket (std::wostream &stream_)
    {
        stream_ << L"  [";
    }

    static void negated (std::ostream &stream_)
    {
        stream_ << "^";
    }

    static void negated (std::wostream &stream_)
    {
        stream_ << L"^";
    }

    static void close_bracket (std::ostream &stream_)
    {
        stream_ << "] -> ";
    }

    static void close_bracket (std::wostream &stream_)
    {
        stream_ << L"] -> ";
    }

    static void dfa (std::ostream &stream_)
    {
        stream_ << ", dfa = ";
    }

    static void dfa (std::wostream &stream_)
    {
        stream_ << L", dfa = ";
    }
};

typedef basic_debug<char> debug;
typedef basic_debug<wchar_t> wdebug;
}
}

#endif

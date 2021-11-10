// file_input.hpp
// Copyright (c) 2008-2009 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_SPIRIT_SUPPORT_DETAIL_LEXER_FILE_INPUT_HPP
#define BOOST_SPIRIT_SUPPORT_DETAIL_LEXER_FILE_INPUT_HPP

#include "char_traits.hpp"
// memcpy
#include <cstring>
#include <fstream>
#include "size_t.hpp"
#include "state_machine.hpp"

namespace boost
{
namespace lexer
{
template<typename CharT, typename Traits = char_traits<CharT> >
class basic_file_input
{
public:
    class iterator
    {
    public:
        friend class basic_file_input;

        struct data
        {
            std::size_t id;
            std::size_t unique_id;
            const CharT *start;
            const CharT *end;
            std::size_t state;

            // Construct in end() state.
            data () :
                id (0),
                unique_id (npos),
                state (npos)
            {
            }

            bool operator == (const data &rhs_) const
            {
                return id == rhs_.id && unique_id == rhs_.unique_id &&
                    start == rhs_.start && end == rhs_.end &&
                    state == rhs_.state;
            }
        };

        iterator () :
            _input (0)
        {
        }

        bool operator == (const iterator &rhs_) const
        {
            return _data == rhs_._data;
        }

        bool operator != (const iterator &rhs_) const
        {
            return !(*this == rhs_);
        }

        data &operator * ()
        {
            return _data;
        }

        data *operator -> ()
        {
            return &_data;
        }

        // Let compiler generate operator = ().

        // prefix version
        iterator &operator ++ ()
        {
            next_token ();
            return *this;
        }

        // postfix version
        iterator operator ++ (int)
        {
            iterator iter_ = *this;

            next_token ();
            return iter_;
        }

        void next_token ()
        {
            const detail::internals &internals_ =
                _input->_state_machine->data ();

            _data.start = _data.end;

            if (internals_._dfa->size () == 1)
            {
                _data.id = _input->next (&internals_._lookup->front ()->
                    front (), internals_._dfa_alphabet.front (),
                    &internals_._dfa->front ()->front (), _data.start,
                    _data.end, _data.unique_id);
            }
            else
            {
                _data.id = _input->next (internals_, _data.state, _data.start,
                    _data.end, _data.unique_id);
            }

            if (_data.id == 0)
            {
                _data.start = 0;
                _data.end = 0;
                // Ensure current state matches that returned by end().
                _data.state = npos;
            }
        }

    private:
        // Not owner (obviously!)
        basic_file_input *_input;
        data _data;
    };

    friend class iterator;

    // Make it explicit that we are NOT taking a copy of state_machine_!
    basic_file_input (const basic_state_machine<CharT> *state_machine_,
        std::basic_ifstream<CharT> *is_,
        const std::streamsize buffer_size_ = 4096,
        const std::streamsize buffer_increment_ = 1024) :
        _state_machine (state_machine_),
        _stream (is_),
        _buffer_size (buffer_size_),
        _buffer_increment (buffer_increment_),
        _buffer (_buffer_size, '!')
    {
        _start_buffer = &_buffer.front ();
        _end_buffer = _start_buffer + _buffer.size ();
        _start_token = _end_buffer;
        _end_token = _end_buffer;
    }

    iterator begin ()
    {
        iterator iter_;

        iter_._input = this;
        // Over-ride default of 0 (EOF)
        iter_._data.id = npos;
        iter_._data.start = 0;
        iter_._data.end = 0;
        iter_._data.state = 0;
        ++iter_;
        return iter_;
    }

    iterator end ()
    {
        iterator iter_;

        iter_._input = this;
        iter_._data.start = 0;
        iter_._data.end = 0;
        return iter_;
    }

    void flush ()
    {
        // This temporary is mandatory, otherwise the
        // pointer calculations won't work!
        const CharT *temp_ = _end_buffer;

        _start_token = _end_token = _end_buffer;
        reload_buffer (temp_, true, _end_token);
    }

private:
    typedef std::basic_istream<CharT> istream;
    typedef std::vector<CharT> buffer;

    const basic_state_machine<CharT> *_state_machine;
    const std::streamsize _buffer_size;
    const std::streamsize _buffer_increment;

    buffer _buffer;
    CharT *_start_buffer;
    istream *_stream;
    const CharT *_start_token;
    const CharT *_end_token;
    CharT *_end_buffer;

    std::size_t next (const detail::internals &internals_,
        std::size_t &start_state_, const CharT * &start_, const CharT * &end_,
        std::size_t &unique_id_)
    {
        _start_token = _end_token;

again:
        const std::size_t * lookup_ = &internals_._lookup[start_state_]->
            front ();
        std::size_t dfa_alphabet_ = internals_._dfa_alphabet[start_state_];
        const std::size_t *dfa_ = &internals_._dfa[start_state_]->front ();
        const std::size_t *ptr_ = dfa_ + dfa_alphabet_;
        const CharT *curr_ = _start_token;
        bool end_state_ = *ptr_ != 0;
        std::size_t id_ = *(ptr_ + id_index);
        std::size_t uid_ = *(ptr_ + unique_id_index);
        const CharT *end_token_ = curr_;

        for (;;)
        {
            if (curr_ >= _end_buffer)
            {
                if (!reload_buffer (curr_, end_state_, end_token_))
                {
                    // EOF
                    break;
                }
            }

            const std::size_t BOL_state_ = ptr_[bol_index];
            const std::size_t EOL_state_ = ptr_[eol_index];

            if (BOL_state_ && (_start_token == _start_buffer ||
                *(_start_token - 1) == '\n'))
            {
                ptr_ = &dfa_[BOL_state_ * dfa_alphabet_];
            }
            else if (EOL_state_ && *curr_ == '\n')
            {
                ptr_ = &dfa_[EOL_state_ * dfa_alphabet_];
            }
            else
            {
                const std::size_t state_ =
                    ptr_[lookup_[static_cast<typename Traits::index_type>
                        (*curr_++)]];

                if (state_ == 0)
                {
                    break;
                }

                ptr_ = &dfa_[state_ * dfa_alphabet_];
            }

            if (*ptr_)
            {
                end_state_ = true;
                id_ = *(ptr_ + id_index);
                uid_ = *(ptr_ + unique_id_index);
                start_state_ = *(ptr_ + state_index);
                end_token_ = curr_;
            }
        }

        if (_start_token >= _end_buffer)
        {
            // No more tokens...
            unique_id_ = npos;
            return 0;
        }

        const std::size_t EOL_state_ = ptr_[eol_index];

        if (EOL_state_ && curr_ == end_)
        {
            ptr_ = &dfa_[EOL_state_ * dfa_alphabet_];

            if (*ptr_)
            {
                end_state_ = true;
                id_ = *(ptr_ + id_index);
                uid_ = *(ptr_ + unique_id_index);
                start_state_ = *(ptr_ + state_index);
                end_token_ = curr_;
            }
        }

        if (end_state_)
        {
            // return longest match
            _end_token = end_token_;

            if (id_ == 0) goto again;
        }
        else
        {
            // No match causes char to be skipped
            _end_token = _start_token + 1;
            id_ = npos;
            uid_ = npos;
        }

        start_ = _start_token;
        end_ = _end_token;
        unique_id_ = uid_;
        return id_;
    }

    std::size_t next (const std::size_t * const lookup_,
        const std::size_t dfa_alphabet_, const std::size_t * const dfa_,
        const CharT * &start_, const CharT * &end_, std::size_t &unique_id_)
    {
        _start_token = _end_token;

        const std::size_t *ptr_ = dfa_ + dfa_alphabet_;
        const CharT *curr_ = _start_token;
        bool end_state_ = *ptr_ != 0;
        std::size_t id_ = *(ptr_ + id_index);
        std::size_t uid_ = *(ptr_ + unique_id_index);
        const CharT *end_token_ = curr_;

        for (;;)
        {
            if (curr_ >= _end_buffer)
            {
                if (!reload_buffer (curr_, end_state_, end_token_))
                {
                    // EOF
                    break;
                }
            }

            const std::size_t BOL_state_ = ptr_[bol_index];
            const std::size_t EOL_state_ = ptr_[eol_index];

            if (BOL_state_ && (_start_token == _start_buffer ||
                *(_start_token - 1) == '\n'))
            {
                ptr_ = &dfa_[BOL_state_ * dfa_alphabet_];
            }
            else if (EOL_state_ && *curr_ == '\n')
            {
                ptr_ = &dfa_[EOL_state_ * dfa_alphabet_];
            }
            else
            {
                const std::size_t state_ =
                    ptr_[lookup_[static_cast<typename Traits::index_type>
                        (*curr_++)]];

                if (state_ == 0)
                {
                    break;
                }

                ptr_ = &dfa_[state_ * dfa_alphabet_];
            }

            if (*ptr_)
            {
                end_state_ = true;
                id_ = *(ptr_ + id_index);
                uid_ = *(ptr_ + unique_id_index);
                end_token_ = curr_;
            }
        }

        if (_start_token >= _end_buffer)
        {
            // No more tokens...
            unique_id_ = npos;
            return 0;
        }

        const std::size_t EOL_state_ = ptr_[eol_index];

        if (EOL_state_ && curr_ == end_)
        {
            ptr_ = &dfa_[EOL_state_ * dfa_alphabet_];

            if (*ptr_)
            {
                end_state_ = true;
                id_ = *(ptr_ + id_index);
                uid_ = *(ptr_ + unique_id_index);
                end_token_ = curr_;
            }
        }

        if (end_state_)
        {
            // return longest match
            _end_token = end_token_;
        }
        else
        {
            // No match causes char to be skipped
            _end_token = _start_token + 1;
            id_ = npos;
            uid_ = npos;
        }

        start_ = _start_token;
        end_ = _end_token;
        unique_id_ = uid_;
        return id_;
    }

    bool reload_buffer (const CharT * &curr_, const bool end_state_,
        const CharT * &end_token_)
    {
        bool success_ = !_stream->eof ();

        if (success_)
        {
            const CharT *old_start_token_ = _start_token;
            std::size_t old_size_ = _buffer.size ();
            std::size_t count_ = 0;

            if (_start_token - 1 == _start_buffer)
            {
                // Run out of buffer space, so increase.
                _buffer.resize (old_size_ + _buffer_increment, '!');
                _start_buffer = &_buffer.front ();
                _start_token = _start_buffer + 1;
                _stream->read (_start_buffer + old_size_,
                    _buffer_increment);
                count_ = _stream->gcount ();
                _end_buffer = _start_buffer + old_size_ + count_;
            }
            else if (_start_token < _end_buffer)
            {
                const std::size_t len_ = _end_buffer - _start_token;
                // Some systems have memcpy in namespace std.
                using namespace std;

                memcpy (_start_buffer, _start_token - 1, (len_ + 1) *
                    sizeof (CharT));
                _stream->read (_start_buffer + len_ + 1,
                    static_cast<std::streamsize> (_buffer.size () - len_ - 1));
                count_ = _stream->gcount ();
                _start_token = _start_buffer + 1;
                _end_buffer = _start_buffer + len_ + 1 + count_;
            }
            else
            {
                _stream->read (_start_buffer, static_cast<std::streamsize>
                    (_buffer.size ()));
                count_ = _stream->gcount ();
                _start_token = _start_buffer;
                _end_buffer = _start_buffer + count_;
            }

            if (end_state_)
            {
                end_token_ = _start_token +
                    (end_token_ - old_start_token_);
            }

            curr_ = _start_token + (curr_ - old_start_token_);
        }

        return success_;
    }

    // Disallow copying of buffer
    basic_file_input (const basic_file_input &);
    const basic_file_input &operator = (const basic_file_input &);
};

typedef basic_file_input<char> file_input;
typedef basic_file_input<wchar_t> wfile_input;
}
}

#endif

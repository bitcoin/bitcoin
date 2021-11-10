// input.hpp
// Copyright (c) 2008-2009 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_SPIRIT_SUPPORT_DETAIL_LEXER_INPUT_HPP
#define BOOST_SPIRIT_SUPPORT_DETAIL_LEXER_INPUT_HPP

#include "char_traits.hpp"
#include "size_t.hpp"
#include "state_machine.hpp"
#include <iterator> // for std::iterator_traits

namespace boost
{
namespace lexer
{
template<typename FwdIter, typename Traits =
    char_traits<typename std::iterator_traits<FwdIter>::value_type> >
class basic_input
{
public:
    class iterator
    {
    public:
        friend class basic_input;

        struct data
        {
            std::size_t id;
            std::size_t unique_id;
            FwdIter start;
            FwdIter end;
            bool bol;
            std::size_t state;

            // Construct in end() state.
            data () :
                id (0),
                unique_id (npos),
                bol (false),
                state (npos)
            {
            }

            bool operator == (const data &rhs_) const
            {
                return id == rhs_.id && unique_id == rhs_.unique_id &&
                    start == rhs_.start && end == rhs_.end &&
                    bol == rhs_.bol && state == rhs_.state;
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

    private:
        // Not owner (obviously!)
        const basic_input *_input;
        data _data;

        void next_token ()
        {
            const detail::internals &internals_ =
                _input->_state_machine->data ();

            _data.start = _data.end;

            if (internals_._dfa->size () == 1)
            {
                if (internals_._seen_BOL_assertion ||
                    internals_._seen_EOL_assertion)
                {
                    _data.id = next
                        (&internals_._lookup->front ()->front (),
                        internals_._dfa_alphabet.front (),
                        &internals_._dfa->front ()->front (),
                        _data.bol, _data.end, _input->_end, _data.unique_id);
                }
                else
                {
                    _data.id = next (&internals_._lookup->front ()->front (),
                        internals_._dfa_alphabet.front (), &internals_.
                        _dfa->front ()->front (), _data.end, _input->_end,
                        _data.unique_id);
                }
            }
            else
            {
                if (internals_._seen_BOL_assertion ||
                    internals_._seen_EOL_assertion)
                {
                    _data.id = next (internals_, _data.state,
                        _data.bol, _data.end, _input->_end, _data.unique_id);
                }
                else
                {
                    _data.id = next (internals_, _data.state,
                        _data.end, _input->_end, _data.unique_id);
                }
            }

            if (_data.end == _input->_end && _data.start == _data.end)
            {
                // Ensure current state matches that returned by end().
                _data.state = npos;
            }
        }

        std::size_t next (const detail::internals &internals_,
            std::size_t &start_state_, bool bol_,
            FwdIter &start_token_, const FwdIter &end_,
            std::size_t &unique_id_)
        {
            if (start_token_ == end_)
            {
                unique_id_ = npos;
                return 0;
            }

        again:
            const std::size_t * lookup_ = &internals_._lookup[start_state_]->
                front ();
            std::size_t dfa_alphabet_ = internals_._dfa_alphabet[start_state_];
            const std::size_t *dfa_ = &internals_._dfa[start_state_]->front ();
            const std::size_t *ptr_ = dfa_ + dfa_alphabet_;
            FwdIter curr_ = start_token_;
            bool end_state_ = *ptr_ != 0;
            std::size_t id_ = *(ptr_ + id_index);
            std::size_t uid_ = *(ptr_ + unique_id_index);
            std::size_t end_start_state_ = start_state_;
            bool end_bol_ = bol_;
            FwdIter end_token_ = start_token_;

            while (curr_ != end_)
            {
                const std::size_t BOL_state_ = ptr_[bol_index];
                const std::size_t EOL_state_ = ptr_[eol_index];

                if (BOL_state_ && bol_)
                {
                    ptr_ = &dfa_[BOL_state_ * dfa_alphabet_];
                }
                else if (EOL_state_ && *curr_ == '\n')
                {
                    ptr_ = &dfa_[EOL_state_ * dfa_alphabet_];
                }
                else
                {
                    typename Traits::char_type prev_char_ = *curr_++;

                    bol_ = prev_char_ == '\n';

                    const std::size_t state_ =
                        ptr_[lookup_[static_cast<typename Traits::index_type>
                        (prev_char_)]];

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
                    end_start_state_ = *(ptr_ + state_index);
                    end_bol_ = bol_;
                    end_token_ = curr_;
                }
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
                    end_start_state_ = *(ptr_ + state_index);
                    end_bol_ = bol_;
                    end_token_ = curr_;
                }
            }

            if (end_state_)
            {
                // return longest match
                start_state_ = end_start_state_;
                start_token_ = end_token_;

                if (id_ == 0)
                {
                    bol_ = end_bol_;
                    goto again;
                }
                else
                {
                    _data.bol = end_bol_;
                }
            }
            else
            {
                // No match causes char to be skipped
                _data.bol = *start_token_ == '\n';
                ++start_token_;
                id_ = npos;
                uid_ = npos;
            }

            unique_id_ = uid_;
            return id_;
        }

        std::size_t next (const detail::internals &internals_,
            std::size_t &start_state_, FwdIter &start_token_,
            FwdIter const &end_, std::size_t &unique_id_)
        {
            if (start_token_ == end_)
            {
                unique_id_ = npos;
                return 0;
            }

        again:
            const std::size_t * lookup_ = &internals_._lookup[start_state_]->
                front ();
            std::size_t dfa_alphabet_ = internals_._dfa_alphabet[start_state_];
            const std::size_t *dfa_ = &internals_._dfa[start_state_]->front ();
            const std::size_t *ptr_ = dfa_ + dfa_alphabet_;
            FwdIter curr_ = start_token_;
            bool end_state_ = *ptr_ != 0;
            std::size_t id_ = *(ptr_ + id_index);
            std::size_t uid_ = *(ptr_ + unique_id_index);
            std::size_t end_start_state_ = start_state_;
            FwdIter end_token_ = start_token_;

            while (curr_ != end_)
            {
                const std::size_t state_ = ptr_[lookup_[static_cast
                    <typename Traits::index_type>(*curr_++)]];

                if (state_ == 0)
                {
                    break;
                }

                ptr_ = &dfa_[state_ * dfa_alphabet_];

                if (*ptr_)
                {
                    end_state_ = true;
                    id_ = *(ptr_ + id_index);
                    uid_ = *(ptr_ + unique_id_index);
                    end_start_state_ = *(ptr_ + state_index);
                    end_token_ = curr_;
                }
            }

            if (end_state_)
            {
                // return longest match
                start_state_ = end_start_state_;
                start_token_ = end_token_;

                if (id_ == 0) goto again;
            }
            else
            {
                // No match causes char to be skipped
                ++start_token_;
                id_ = npos;
                uid_ = npos;
            }

            unique_id_ = uid_;
            return id_;
        }

        std::size_t next (const std::size_t * const lookup_,
            const std::size_t dfa_alphabet_, const std::size_t * const dfa_,
            bool bol_, FwdIter &start_token_, FwdIter const &end_,
            std::size_t &unique_id_)
        {
            if (start_token_ == end_)
            {
                unique_id_ = npos;
                return 0;
            }

            const std::size_t *ptr_ = dfa_ + dfa_alphabet_;
            FwdIter curr_ = start_token_;
            bool end_state_ = *ptr_ != 0;
            std::size_t id_ = *(ptr_ + id_index);
            std::size_t uid_ = *(ptr_ + unique_id_index);
            bool end_bol_ = bol_;
            FwdIter end_token_ = start_token_;

            while (curr_ != end_)
            {
                const std::size_t BOL_state_ = ptr_[bol_index];
                const std::size_t EOL_state_ = ptr_[eol_index];

                if (BOL_state_ && bol_)
                {
                    ptr_ = &dfa_[BOL_state_ * dfa_alphabet_];
                }
                else if (EOL_state_ && *curr_ == '\n')
                {
                    ptr_ = &dfa_[EOL_state_ * dfa_alphabet_];
                }
                else
                {
                    typename Traits::char_type prev_char_ = *curr_++;

                    bol_ = prev_char_ == '\n';

                    const std::size_t state_ =
                        ptr_[lookup_[static_cast<typename Traits::index_type>
                        (prev_char_)]];

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
                    end_bol_ = bol_;
                    end_token_ = curr_;
                }
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
                    end_bol_ = bol_;
                    end_token_ = curr_;
                }
            }

            if (end_state_)
            {
                // return longest match
                _data.bol = end_bol_;
                start_token_ = end_token_;
            }
            else
            {
                // No match causes char to be skipped
                _data.bol = *start_token_ == '\n';
                ++start_token_;
                id_ = npos;
                uid_ = npos;
            }

            unique_id_ = uid_;
            return id_;
        }

        std::size_t next (const std::size_t * const lookup_,
            const std::size_t dfa_alphabet_, const std::size_t * const dfa_,
            FwdIter &start_token_, FwdIter const &end_,
            std::size_t &unique_id_)
        {
            if (start_token_ == end_)
            {
                unique_id_ = npos;
                return 0;
            }

            const std::size_t *ptr_ = dfa_ + dfa_alphabet_;
            FwdIter curr_ = start_token_;
            bool end_state_ = *ptr_ != 0;
            std::size_t id_ = *(ptr_ + id_index);
            std::size_t uid_ = *(ptr_ + unique_id_index);
            FwdIter end_token_ = start_token_;

            while (curr_ != end_)
            {
                const std::size_t state_ = ptr_[lookup_[static_cast
                    <typename Traits::index_type>(*curr_++)]];

                if (state_ == 0)
                {
                    break;
                }

                ptr_ = &dfa_[state_ * dfa_alphabet_];

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
                start_token_ = end_token_;
            }
            else
            {
                // No match causes char to be skipped
                ++start_token_;
                id_ = npos;
                uid_ = npos;
            }

            unique_id_ = uid_;
            return id_;
        }
    };

    friend class iterator;

    // Make it explicit that we are NOT taking a copy of state_machine_!
    basic_input (const basic_state_machine<typename Traits::char_type>
        *state_machine_, const FwdIter &begin_, const FwdIter &end_) :
        _state_machine (state_machine_),
        _begin (begin_),
        _end (end_)
    {
    }

    iterator begin () const
    {
        iterator iter_;

        iter_._input = this;
        // Over-ride default of 0 (EOI)
        iter_._data.id = npos;
        iter_._data.start = _begin;
        iter_._data.end = _begin;
        iter_._data.bol = _state_machine->data ()._seen_BOL_assertion;
        iter_._data.state = 0;
        ++iter_;
        return iter_;
    }

    iterator end () const
    {
        iterator iter_;

        iter_._input = this;
        iter_._data.start = _end;
        iter_._data.end = _end;
        return iter_;
    }

private:
    const basic_state_machine<typename Traits::char_type> *_state_machine;
    FwdIter _begin;
    FwdIter _end;
};

typedef basic_input<std::string::iterator> iter_input;
typedef basic_input<std::basic_string<wchar_t>::iterator> iter_winput;
typedef basic_input<const char *> ptr_input;
typedef basic_input<const wchar_t *> ptr_winput;
}
}

#endif

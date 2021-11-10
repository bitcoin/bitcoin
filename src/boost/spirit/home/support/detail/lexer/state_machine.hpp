// state_machine.hpp
// Copyright (c) 2007-2009 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_SPIRIT_SUPPORT_DETAIL_LEXER_STATE_MACHINE_HPP
#define BOOST_SPIRIT_SUPPORT_DETAIL_LEXER_STATE_MACHINE_HPP

#include <algorithm>
#include "conversion/char_state_machine.hpp"
#include "consts.hpp"
#include <deque>
#include "internals.hpp"
#include <map>
#include "containers/ptr_vector.hpp"
#include "size_t.hpp"
#include <string>

namespace boost
{
namespace lexer
{
template<typename CharT>
class basic_state_machine
{
public:
    typedef CharT char_type;

    class iterator
    {
    public:
        friend class basic_state_machine;

        struct data
        {
            // Current iterator info
            std::size_t dfa;
            std::size_t states;
            std::size_t state;
            std::size_t transitions;
            std::size_t transition;

            // Current state info
            bool end_state;
            std::size_t id;
            std::size_t unique_id;
            std::size_t goto_dfa;
            std::size_t bol_index;
            std::size_t eol_index;

            // Current transition info
            basic_string_token<CharT> token;
            std::size_t goto_state;

            data () :
                dfa (npos),
                states (0),
                state (npos),
                transitions (0),
                transition (npos),
                end_state (false),
                id (npos),
                unique_id (npos),
                goto_dfa (npos),
                bol_index (npos),
                eol_index (npos),
                goto_state (npos)
            {
            }

            bool operator == (const data &rhs_) const
            {
                return dfa == rhs_.dfa &&
                    states == rhs_.states &&
                    state == rhs_.state &&
                    transitions == rhs_.transitions &&
                    transition == rhs_.transition &&
                    end_state == rhs_.end_state &&
                    id == rhs_.id &&
                    unique_id == rhs_.unique_id &&
                    goto_dfa == rhs_.goto_dfa &&
                    bol_index == rhs_.bol_index &&
                    eol_index == rhs_.eol_index &&
                    token == rhs_.token &&
                    goto_state == rhs_.goto_state;
            }
        };

        iterator () :
            _sm (0),
            _dfas (0),
            _dfa (npos),
            _states (0),
            _state (npos),
            _transitions (0),
            _transition (npos)
        {
        }

        bool operator == (const iterator &rhs_) const
        {
            return _dfas == rhs_._dfas && _dfa == rhs_._dfa &&
                _states == rhs_._states && _state == rhs_._state &&
                _transitions == rhs_._transitions &&
                _transition == rhs_._transition;
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
            next ();
            return *this;
        }

        // postfix version
        iterator operator ++ (int)
        {
            iterator iter_ = *this;

            next ();
            return iter_;
        }

        void clear ()
        {
            _dfas = _states = _transitions = 0;
            _dfa = _state = _transition = npos;
        }

    private:
        basic_state_machine *_sm;
        data _data;
        std::size_t _dfas;
        std::size_t _dfa;
        std::size_t _states;
        std::size_t _state;
        std::size_t _transitions;
        std::size_t _transition;
        typename detail::basic_char_state_machine<CharT>::state::
            size_t_string_token_map::const_iterator _token_iter;
        typename detail::basic_char_state_machine<CharT>::state::
            size_t_string_token_map::const_iterator _token_end;

        void next ()
        {
            bool reset_state_ = false;

            if (_transition >= _transitions)
            {
                _transition = _data.transition = 0;
                _data.state = ++_state;
                reset_state_ = true;

                if (_state >= _states)
                {
                    ++_dfa;

                    if (_dfa >= _dfas)
                    {
                        clear ();
                        reset_state_ = false;
                    }
                    else
                    {
                        _states = _data.states =
                            _sm->_csm._sm_vector[_dfa].size ();
                        _state = _data.state = 0;
                    }
                }
            }
            else
            {
                _data.transition = _transition;
            }

            if (reset_state_)
            {
                const typename detail::basic_char_state_machine<CharT>::
                    state *ptr_ = &_sm->_csm._sm_vector[_dfa][_state];

                _transitions = _data.transitions = ptr_->_transitions.size ();
                _data.end_state = ptr_->_end_state;
                _data.id = ptr_->_id;
                _data.unique_id = ptr_->_unique_id;
                _data.goto_dfa = ptr_->_state;
                _data.bol_index = ptr_->_bol_index;
                _data.eol_index = ptr_->_eol_index;
                _token_iter = ptr_->_transitions.begin ();
                _token_end = ptr_->_transitions.end ();
            }

            if (_token_iter != _token_end)
            {
                _data.token = _token_iter->second;
                _data.goto_state = _token_iter->first;
                ++_token_iter;
                ++_transition;
            }
            else
            {
                _data.token.clear ();
                _data.goto_state = npos;
            }
        }
    };

    friend class iterator;

    basic_state_machine ()
    {
    }

    void clear ()
    {
        _internals.clear ();
        _csm.clear ();
    }

    bool empty () const
    {
        // Don't include _csm in this test, as irrelevant to state.
        return _internals._lookup->empty () &&
            _internals._dfa_alphabet.empty () &&
            _internals._dfa->empty ();
    }

    std::size_t size () const
    {
        return _internals._dfa->size ();
    }

    bool operator == (const basic_state_machine &rhs_) const
    {
        // Don't include _csm in this test, as irrelevant to state.
        return _internals._lookup == rhs_._internals._lookup &&
            _internals._dfa_alphabet == rhs_._internals._dfa_alphabet &&
            _internals._dfa == rhs_._internals._dfa &&
            _internals._seen_BOL_assertion ==
                rhs_._internals._seen_BOL_assertion &&
            _internals._seen_EOL_assertion ==
                rhs_._internals._seen_EOL_assertion;
    }

    iterator begin () const
    {
        iterator iter_;

        iter_._sm = const_cast<basic_state_machine *>(this);
        check_for_csm ();

        if (!_csm.empty ())
        {
            const typename detail::basic_char_state_machine<CharT>::
                state_vector *ptr_ = &_csm._sm_vector.front ();

            iter_._dfas = _csm._sm_vector.size ();
            iter_._states = iter_._data.states = ptr_->size ();
            iter_._transitions = iter_._data.transitions =
                ptr_->front ()._transitions.size ();
            iter_._dfa = iter_._data.dfa = 0;
            iter_._state = iter_._data.state = 0;
            iter_._transition = 0;
            iter_._data.end_state = ptr_->front ()._end_state;
            iter_._data.id = ptr_->front ()._id;
            iter_._data.unique_id = ptr_->front ()._unique_id;
            iter_._data.goto_dfa = ptr_->front ()._state;
            iter_._data.bol_index = ptr_->front ()._bol_index;
            iter_._data.eol_index = ptr_->front ()._eol_index;
            iter_._token_iter = ptr_->front ()._transitions.begin ();
            iter_._token_end = ptr_->front ()._transitions.end ();

            // Deal with case where there is only a bol or eol
            // but no other transitions.
            if (iter_._transitions)
            {
                ++iter_;
            }
        }

        return iter_;
    }

    iterator end () const
    {
        iterator iter_;

        iter_._sm = const_cast<basic_state_machine *>(this);
        return iter_;
    }

    void swap (basic_state_machine &sm_)
    {
        _internals.swap (sm_._internals);
        _csm.swap (sm_._csm);
    }

    const detail::internals &data () const
    {
        return _internals;
    }

private:
    detail::internals _internals;
    mutable detail::basic_char_state_machine<CharT> _csm;

    void check_for_csm () const
    {
        if (_csm.empty ())
        {
            human_readable (_csm);
        }
    }

    void human_readable (detail::basic_char_state_machine<CharT> &sm_) const
    {
        const std::size_t max_ = sizeof (CharT) == 1 ?
            num_chars : num_wchar_ts;
        const std::size_t start_states_ = _internals._dfa->size ();

        sm_.clear ();
        sm_._sm_vector.resize (start_states_);

        for (std::size_t start_state_index_ = 0;
            start_state_index_ < start_states_; ++start_state_index_)
        {
            const detail::internals::size_t_vector *lu_ =
                _internals._lookup[start_state_index_];
            const std::size_t alphabet_ =
                _internals._dfa_alphabet[start_state_index_] - dfa_offset;
            std::vector<std::basic_string<CharT> > chars_ (alphabet_);
            const std::size_t states_ = _internals._dfa[start_state_index_]->
                size () / (alphabet_ + dfa_offset);
            const std::size_t *read_ptr_ = &_internals.
                _dfa[start_state_index_]->front () + alphabet_ + dfa_offset;

            sm_._sm_vector[start_state_index_].resize (states_ - 1);

            for (std::size_t alpha_index_ = 0; alpha_index_ < max_;
                ++alpha_index_)
            {
                const std::size_t col_ = lu_->at (alpha_index_);

                if (col_ != static_cast<std::size_t>(dead_state_index))
                {
                    chars_[col_ - dfa_offset] += static_cast<CharT>
                        (alpha_index_);
                }
            }

            for (std::size_t state_index_ = 1; state_index_ < states_;
                ++state_index_)
            {
                typename detail::basic_char_state_machine<CharT>::state
                    *state_ = &sm_._sm_vector[start_state_index_]
                    [state_index_ - 1];

                state_->_end_state = *read_ptr_ != 0;
                state_->_id = *(read_ptr_ + id_index);
                state_->_unique_id = *(read_ptr_ + unique_id_index);
                state_->_state = *(read_ptr_ + state_index);
                state_->_bol_index = *(read_ptr_ + bol_index) - 1;
                state_->_eol_index = *(read_ptr_ + eol_index) - 1;
                read_ptr_ += dfa_offset;

                for (std::size_t col_index_ = 0; col_index_ < alphabet_;
                    ++col_index_, ++read_ptr_)
                {
                    const std::size_t transition_ = *read_ptr_;

                    if (transition_ != 0)
                    {
                        const std::size_t i_ = transition_ - 1;
                        typename detail::basic_char_state_machine<CharT>::
                            state::size_t_string_token_map::iterator iter_ =
                            state_->_transitions.find (i_);

                        if (iter_ == state_->_transitions.end ())
                        {
                            basic_string_token<CharT> token_
                                (false, chars_[col_index_]);
                            typename detail::basic_char_state_machine<CharT>::
                                state::size_t_string_token_pair pair_
                                (i_, token_);

                            state_->_transitions.insert (pair_);
                        }
                        else
                        {
                            iter_->second._charset += chars_[col_index_];
                        }
                    }
                }

                for (typename detail::basic_char_state_machine<CharT>::state::
                    size_t_string_token_map::iterator iter_ =
                    state_->_transitions.begin (),
                    end_ = state_->_transitions.end ();
                    iter_ != end_; ++iter_)
                {
                    std::sort (iter_->second._charset.begin (),
                        iter_->second._charset.end ());
                    iter_->second.normalise ();
                }
            }
        }
    }
};

typedef basic_state_machine<char> state_machine;
typedef basic_state_machine<wchar_t> wstate_machine;
}
}

#endif

// rules.hpp
// Copyright (c) 2007-2009 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_SPIRIT_SUPPORT_DETAIL_LEXER_RULES_HPP
#define BOOST_SPIRIT_SUPPORT_DETAIL_LEXER_RULES_HPP

#include "consts.hpp"
#include <deque>
#include <locale>
#include <map>
#include "runtime_error.hpp"
#include <set>
#include "size_t.hpp"
#include <sstream>
#include <string>
#include <vector>

namespace boost
{
namespace lexer
{
namespace detail
{
    // return name of initial state
    template <typename CharT>
    struct strings;

    template <>
    struct strings<char>
    {
        static const char *initial ()
        {
            return "INITIAL";
        }

        static const char *dot ()
        {
            return ".";
        }

        static const char *all_states ()
        {
            return "*";
        }

        static const char *char_name ()
        {
            return "char";
        }

        static const char *char_prefix ()
        {
            return "";
        }
    };

    template <>
    struct strings<wchar_t>
    {
        static const wchar_t *initial ()
        {
            return L"INITIAL";
        }

        static const wchar_t *dot ()
        {
            return L".";
        }

        static const wchar_t *all_states ()
        {
            return L"*";
        }

        static const char *char_name ()
        {
            return "wchar_t";
        }

        static const char *char_prefix ()
        {
            return "L";
        }
    };
}

template<typename CharT>
class basic_rules
{
public:
    typedef std::vector<std::size_t> id_vector;
    typedef std::deque<id_vector> id_vector_deque;
    typedef std::basic_string<CharT> string;
    typedef std::deque<string> string_deque;
    typedef std::deque<string_deque> string_deque_deque;
    typedef std::set<string> string_set;
    typedef std::pair<string, string> string_pair;
    typedef std::deque<string_pair> string_pair_deque;
    typedef std::map<string, std::size_t> string_size_t_map;
    typedef std::pair<string, std::size_t> string_size_t_pair;

    basic_rules (const regex_flags flags_ = dot_not_newline,
        std::size_t (*counter_ptr_) () = 0) :
        _flags (flags_),
        _counter (0),
        _counter_ptr (counter_ptr_)
    {
        add_state (initial ());
    }

    void clear ()
    {
        _statemap.clear ();
        _macrodeque.clear ();
        _macroset.clear ();
        _regexes.clear ();
        _ids.clear ();
        _unique_ids.clear ();
        _states.clear ();
        _flags = dot_not_newline;
        _locale = std::locale ();
        add_state (initial ());
    }

    void clear (const CharT *state_name_)
    {
        std::size_t state_ = state (state_name_);

        if (state_ != npos)
        {
            _regexes[state_].clear ();
            _ids[state_].clear ();
            _unique_ids[state_].clear ();
            _states[state_].clear ();
        }
    }

    void flags (const regex_flags flags_)
    {
        _flags = flags_;
    }

    regex_flags flags () const
    {
        return _flags;
    }

    std::size_t next_unique_id ()
    {
        return _counter_ptr ? _counter_ptr () : _counter++;
    }

    std::locale imbue (std::locale &locale_)
    {
        std::locale loc_ = _locale;

        _locale = locale_;
        return loc_;
    }

    const std::locale &locale () const
    {
        return _locale;
    }

    std::size_t state (const CharT *name_) const
    {
        std::size_t state_ = npos;
        typename string_size_t_map::const_iterator iter_ =
            _statemap.find (name_);

        if (iter_ != _statemap.end ())
        {
            state_ = iter_->second;
        }

        return state_;
    }

    const CharT *state (const std::size_t index_) const
    {
        if (index_ == 0)
        {
            return initial ();
        }
        else
        {
            const std::size_t vec_index_ = index_ - 1;

            if (vec_index_ > _lexer_state_names.size () - 1)
            {
                return 0;
            }
            else
            {
                return _lexer_state_names[vec_index_].c_str ();
            }
        }
    }

    std::size_t add_state (const CharT *name_)
    {
        validate (name_);

        if (_statemap.insert (string_size_t_pair (name_,
            _statemap.size ())).second)
        {
            _regexes.push_back (string_deque ());
            _ids.push_back (id_vector ());
            _unique_ids.push_back (id_vector ());
            _states.push_back (id_vector ());

            if (string (name_) != initial ())
            {
                _lexer_state_names.push_back (name_);
            }
        }

        // Initial is not stored, so no need to - 1.
        return _lexer_state_names.size ();
    }

    void add_macro (const CharT *name_, const CharT *regex_)
    {
        add_macro (name_, string (regex_));
    }

    void add_macro (const CharT *name_, const CharT *regex_start_,
        const CharT *regex_end_)
    {
        add_macro (name_, string (regex_start_, regex_end_));
    }

    void add_macro (const CharT *name_, const string &regex_)
    {
        validate (name_);

        typename string_set::const_iterator iter_ = _macroset.find (name_);

        if (iter_ == _macroset.end ())
        {
            _macrodeque.push_back (string_pair (name_, regex_));
            _macroset.insert (name_);
        }
        else
        {
            std::basic_stringstream<CharT> ss_;
            std::ostringstream os_;

            os_ << "Attempt to redefine MACRO '";

            while (*name_)
            {
                os_ << ss_.narrow (*name_++, static_cast<CharT> (' '));
            }

            os_ << "'.";
            throw runtime_error (os_.str ());
        }
    }

    void add_macros (const basic_rules<CharT> &rules_)
    {
        const string_pair_deque &macros_ = rules_.macrodeque ();
        typename string_pair_deque::const_iterator macro_iter_ =
            macros_.begin ();
        typename string_pair_deque::const_iterator macro_end_ =
            macros_.end ();

        for (; macro_iter_ != macro_end_; ++macro_iter_)
        {
            add_macro (macro_iter_->first.c_str (),
                macro_iter_->second.c_str ());
        }
    }

    void merge_macros (const basic_rules<CharT> &rules_)
    {
        const string_pair_deque &macros_ = rules_.macrodeque ();
        typename string_pair_deque::const_iterator macro_iter_ =
            macros_.begin ();
        typename string_pair_deque::const_iterator macro_end_ =
            macros_.end ();
        typename string_set::const_iterator macro_dest_iter_;
        typename string_set::const_iterator macro_dest_end_ = _macroset.end ();

        for (; macro_iter_ != macro_end_; ++macro_iter_)
        {
            macro_dest_iter_ = _macroset.find (macro_iter_->first);

            if (macro_dest_iter_ == macro_dest_end_)
            {
                add_macro (macro_iter_->first.c_str (),
                    macro_iter_->second.c_str ());
            }
        }
    }

    std::size_t add (const CharT *regex_, const std::size_t id_)
    {
        return add (string (regex_), id_);
    }

    std::size_t add (const CharT *regex_start_, const CharT *regex_end_,
        const std::size_t id_)
    {
        return add (string (regex_start_, regex_end_), id_);
    }

    std::size_t add (const string &regex_, const std::size_t id_)
    {
        const std::size_t counter_ = next_unique_id ();

        check_for_invalid_id (id_);
        _regexes.front ().push_back (regex_);
        _ids.front ().push_back (id_);
        _unique_ids.front ().push_back (counter_);
        _states.front ().push_back (0);
        return counter_;
    }

    std::size_t add (const CharT *curr_state_, const CharT *regex_,
        const CharT *new_state_)
    {
        return add (curr_state_, string (regex_), new_state_);
    }

    std::size_t add (const CharT *curr_state_, const CharT *regex_start_,
        const CharT *regex_end_, const CharT *new_state_)
    {
        return add (curr_state_, string (regex_start_, regex_end_),
            new_state_);
    }

    std::size_t add (const CharT *curr_state_, const string &regex_,
        const CharT *new_state_)
    {
        return add (curr_state_, regex_, 0, new_state_, false);
    }

    std::size_t add (const CharT *curr_state_, const CharT *regex_,
        const std::size_t id_, const CharT *new_state_)
    {
        return add (curr_state_, string (regex_), id_, new_state_);
    }

    std::size_t add (const CharT *curr_state_, const CharT *regex_start_,
        const CharT *regex_end_, const std::size_t id_,
        const CharT *new_state_)
    {
        return add (curr_state_, string (regex_start_, regex_end_), id_,
            new_state_);
    }

    std::size_t add (const CharT *curr_state_, const string &regex_,
        const std::size_t id_, const CharT *new_state_)
    {
        return add (curr_state_, regex_, id_, new_state_, true);
    }

    void add (const CharT *source_, const basic_rules<CharT> &rules_,
        const CharT *dest_, const CharT *to_ = detail::strings<CharT>::dot ())
    {
        const bool star_ = *source_ == '*' && *(source_ + 1) == 0;
        const bool dest_dot_ = *dest_ == '.' && *(dest_ + 1) == 0;
        const bool to_dot_ = *to_ == '.' && *(to_ + 1) == 0;
        std::size_t state_ = 0;
        const string_deque_deque &all_regexes_ = rules_.regexes ();
        const id_vector_deque &all_ids_ = rules_.ids ();
        const id_vector_deque &all_unique_ids_ = rules_.unique_ids ();
        const id_vector_deque &all_states_ = rules_.states ();
        typename string_deque::const_iterator regex_iter_;
        typename string_deque::const_iterator regex_end_;
        typename id_vector::const_iterator id_iter_;
        typename id_vector::const_iterator uid_iter_;
        typename id_vector::const_iterator state_iter_;

        if (star_)
        {
            typename string_deque_deque::const_iterator all_regexes_iter_ =
                all_regexes_.begin ();
            typename string_deque_deque::const_iterator all_regexes_end_ =
                all_regexes_.end ();
            typename id_vector_deque::const_iterator all_ids_iter_ =
                all_ids_.begin ();
            typename id_vector_deque::const_iterator all_uids_iter_ =
                all_unique_ids_.begin ();
            typename id_vector_deque::const_iterator all_states_iter_ =
                all_states_.begin ();

            for (; all_regexes_iter_ != all_regexes_end_;
                ++state_, ++all_regexes_iter_, ++all_ids_iter_,
                ++all_uids_iter_, ++all_states_iter_)
            {
                regex_iter_ = all_regexes_iter_->begin ();
                regex_end_ = all_regexes_iter_->end ();
                id_iter_ = all_ids_iter_->begin ();
                uid_iter_ = all_uids_iter_->begin ();
                state_iter_ = all_states_iter_->begin ();

                for (; regex_iter_ != regex_end_; ++regex_iter_, ++id_iter_,
                    ++uid_iter_, ++state_iter_)
                {
                    // If ..._dot_ then lookup state name from rules_; otherwise
                    // pass name through.
                    add (dest_dot_ ? rules_.state (state_) : dest_, *regex_iter_,
                        *id_iter_, to_dot_ ? rules_.state (*state_iter_) : to_, true,
                        *uid_iter_);
                }
            }
        }
        else
        {
            const CharT *start_ = source_;
            string state_name_;

            while (*source_)
            {
                while (*source_ && *source_ != ',')
                {
                    ++source_;
                }

                state_name_.assign (start_, source_);

                if (*source_)
                {
                    ++source_;
                    start_ = source_;
                }

                state_ = rules_.state (state_name_.c_str ());

                if (state_ == npos)
                {
                    std::basic_stringstream<CharT> ss_;
                    std::ostringstream os_;

                    os_ << "Unknown state name '";
                    source_ = state_name_.c_str ();

                    while (*source_)
                    {
                        os_ << ss_.narrow (*source_++, ' ');
                    }

                    os_ << "'.";
                    throw runtime_error (os_.str ());
                }

                regex_iter_ = all_regexes_[state_].begin ();
                regex_end_ = all_regexes_[state_].end ();
                id_iter_ = all_ids_[state_].begin ();
                uid_iter_ = all_unique_ids_[state_].begin ();
                state_iter_ = all_states_[state_].begin ();

                for (; regex_iter_ != regex_end_; ++regex_iter_, ++id_iter_,
                    ++uid_iter_, ++state_iter_)
                {
                    // If ..._dot_ then lookup state name from rules_; otherwise
                    // pass name through.
                    add (dest_dot_ ? state_name_.c_str () : dest_, *regex_iter_,
                        *id_iter_, to_dot_ ? rules_.state (*state_iter_) : to_, true,
                        *uid_iter_);
                }
            }
        }
    }
/*
    void add (const CharT *curr_state_, const basic_rules<CharT> &rules_)
    {
        const string_deque_deque &regexes_ = rules_.regexes ();
        const id_vector_deque &ids_ = rules_.ids ();
        const id_vector_deque &unique_ids_ = rules_.unique_ids ();
        typename string_deque_deque::const_iterator state_regex_iter_ =
            regexes_.begin ();
        typename string_deque_deque::const_iterator state_regex_end_ =
            regexes_.end ();
        typename id_vector_deque::const_iterator state_id_iter_ =
            ids_.begin ();
        typename id_vector_deque::const_iterator state_uid_iter_ =
            unique_ids_.begin ();
        typename string_deque::const_iterator regex_iter_;
        typename string_deque::const_iterator regex_end_;
        typename id_vector::const_iterator id_iter_;
        typename id_vector::const_iterator uid_iter_;

        for (; state_regex_iter_ != state_regex_end_; ++state_regex_iter_)
        {
            regex_iter_ = state_regex_iter_->begin ();
            regex_end_ = state_regex_iter_->end ();
            id_iter_ = state_id_iter_->begin ();
            uid_iter_ = state_uid_iter_->begin ();

            for (; regex_iter_ != regex_end_; ++regex_iter_, ++id_iter_,
                ++uid_iter_)
            {
                add (curr_state_, *regex_iter_, *id_iter_, curr_state_, true,
                    *uid_iter_);
            }
        }
    }
*/
    const string_size_t_map &statemap () const
    {
        return _statemap;
    }

    const string_pair_deque &macrodeque () const
    {
        return _macrodeque;
    }

    const string_deque_deque &regexes () const
    {
        return _regexes;
    }

    const id_vector_deque &ids () const
    {
        return _ids;
    }

    const id_vector_deque &unique_ids () const
    {
        return _unique_ids;
    }

    const id_vector_deque &states () const
    {
        return _states;
    }

    bool empty () const
    {
        typename string_deque_deque::const_iterator iter_ = _regexes.begin ();
        typename string_deque_deque::const_iterator end_ = _regexes.end ();
        bool empty_ = true;

        for (; iter_ != end_; ++iter_)
        {
            if (!iter_->empty ())
            {
                empty_ = false;
                break;
            }
        }

        return empty_;
    }

    static const CharT *initial ()
    {
        return detail::strings<CharT>::initial ();
    }

    static const CharT *all_states ()
    {
        return detail::strings<CharT>::all_states ();
    }

    static const CharT *dot ()
    {
        return detail::strings<CharT>::dot ();
    }

private:
    string_size_t_map _statemap;
    string_pair_deque _macrodeque;
    string_set _macroset;
    string_deque_deque _regexes;
    id_vector_deque _ids;
    id_vector_deque _unique_ids;
    id_vector_deque _states;
    regex_flags _flags;
    std::size_t _counter;
    std::size_t (*_counter_ptr) ();
    std::locale _locale;
    string_deque _lexer_state_names;

    std::size_t add (const CharT *curr_state_, const string &regex_,
        const std::size_t id_, const CharT *new_state_, const bool check_,
        const std::size_t uid_ = npos)
    {
        const bool star_ = *curr_state_ == '*' && *(curr_state_ + 1) == 0;
        const bool dot_ = *new_state_ == '.' && *(new_state_ + 1) == 0;

        if (check_)
        {
            check_for_invalid_id (id_);
        }

        if (!dot_)
        {
            validate (new_state_);
        }

        std::size_t new_ = string::npos;
        typename string_size_t_map::const_iterator iter_;
        typename string_size_t_map::const_iterator end_ = _statemap.end ();
        id_vector states_;

        if (!dot_)
        {
            iter_ = _statemap.find (new_state_);

            if (iter_ == end_)
            {
                std::basic_stringstream<CharT> ss_;
                std::ostringstream os_;

                os_ << "Unknown state name '";

                while (*new_state_)
                {
                    os_ << ss_.narrow (*new_state_++, ' ');
                }

                os_ << "'.";
                throw runtime_error (os_.str ());
            }

            new_ = iter_->second;
        }

        if (star_)
        {
            const std::size_t size_ = _statemap.size ();

            for (std::size_t i_ = 0; i_ < size_; ++i_)
            {
                states_.push_back (i_);
            }
        }
        else
        {
            const CharT *start_ = curr_state_;
            string state_;

            while (*curr_state_)
            {
                while (*curr_state_ && *curr_state_ != ',')
                {
                    ++curr_state_;
                }

                state_.assign (start_, curr_state_);

                if (*curr_state_)
                {
                    ++curr_state_;
                    start_ = curr_state_;
                }

                validate (state_.c_str ());
                iter_ = _statemap.find (state_.c_str ());

                if (iter_ == end_)
                {
                    std::basic_stringstream<CharT> ss_;
                    std::ostringstream os_;

                    os_ << "Unknown state name '";
                    curr_state_ = state_.c_str ();

                    while (*curr_state_)
                    {
                        os_ << ss_.narrow (*curr_state_++, ' ');
                    }

                    os_ << "'.";
                    throw runtime_error (os_.str ());
                }

                states_.push_back (iter_->second);
            }
        }

        std::size_t first_counter_ = npos;

        for (std::size_t i_ = 0, size_ = states_.size (); i_ < size_; ++i_)
        {
            const std::size_t curr_ = states_[i_];

            _regexes[curr_].push_back (regex_);
            _ids[curr_].push_back (id_);

            if (uid_ == npos)
            {
                const std::size_t counter_ = next_unique_id ();

                if (first_counter_ == npos)
                {
                    first_counter_ = counter_;
                }

                _unique_ids[curr_].push_back (counter_);
            }
            else
            {
                if (first_counter_ == npos)
                {
                    first_counter_ = uid_;
                }

                _unique_ids[curr_].push_back (uid_);
            }

            _states[curr_].push_back (dot_ ? curr_ : new_);
        }

        return first_counter_;
    }

    void validate (const CharT *name_) const
    {
        const CharT *start_ = name_;

        if (*name_ != '_' && !(*name_ >= 'A' && *name_ <= 'Z') &&
            !(*name_ >= 'a' && *name_ <= 'z'))
        {
            std::basic_stringstream<CharT> ss_;
            std::ostringstream os_;

            os_ << "Invalid name '";

            while (*name_)
            {
                os_ << ss_.narrow (*name_++, ' ');
            }

            os_ << "'.";
            throw runtime_error (os_.str ());
        }
        else if (*name_)
        {
            ++name_;
        }

        while (*name_)
        {
            if (*name_ != '_' && *name_ != '-' &&
                !(*name_ >= 'A' && *name_ <= 'Z') &&
                !(*name_ >= 'a' && *name_ <= 'z') &&
                !(*name_ >= '0' && *name_ <= '9'))
            {
                std::basic_stringstream<CharT> ss_;
                std::ostringstream os_;

                os_ << "Invalid name '";
                name_ = start_;

                while (*name_)
                {
                    os_ << ss_.narrow (*name_++, ' ');
                }

                os_ << "'.";
                throw runtime_error (os_.str ());
            }

            ++name_;
        }

        if (name_ - start_ > static_cast<std::ptrdiff_t>(max_macro_len))
        {
            std::basic_stringstream<CharT> ss_;
            std::ostringstream os_;

            os_ << "Name '";
            name_ = start_;

            while (*name_)
            {
                os_ << ss_.narrow (*name_++, ' ');
            }

            os_ << "' too long.";
            throw runtime_error (os_.str ());
        }
    }

    void check_for_invalid_id (const std::size_t id_) const
    {
        switch (id_)
        {
        case 0:
            throw runtime_error ("id 0 is reserved for EOF.");
        case npos:
            throw runtime_error ("id npos is reserved for the "
                "UNKNOWN token.");
        default:
            // OK
            break;
        }
    }
};

typedef basic_rules<char> rules;
typedef basic_rules<wchar_t> wrules;
}
}

#endif

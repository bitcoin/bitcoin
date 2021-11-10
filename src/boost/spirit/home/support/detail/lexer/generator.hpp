// generator.hpp
// Copyright (c) 2007-2009 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_SPIRIT_SUPPORT_DETAIL_LEXER_GENERATOR_HPP
#define BOOST_SPIRIT_SUPPORT_DETAIL_LEXER_GENERATOR_HPP

#include "char_traits.hpp"
// memcmp()
#include <cstring>
#include "partition/charset.hpp"
#include "partition/equivset.hpp"
#include <memory>
#include <limits>
#include "parser/tree/node.hpp"
#include "parser/parser.hpp"
#include "containers/ptr_list.hpp"
#include <boost/move/unique_ptr.hpp>
#include "rules.hpp"
#include "state_machine.hpp"

namespace boost
{
namespace lexer
{
template<typename CharT, typename Traits = char_traits<CharT> >
class basic_generator
{
public:
    typedef typename detail::internals::size_t_vector size_t_vector;
    typedef basic_rules<CharT> rules;

    static void build (const rules &rules_,
        basic_state_machine<CharT> &state_machine_)
    {
        std::size_t index_ = 0;
        std::size_t size_ = rules_.statemap ().size ();
        node_ptr_vector node_ptr_vector_;
        detail::internals &internals_ = const_cast<detail::internals &>
            (state_machine_.data ());
        bool seen_BOL_assertion_ = false;
        bool seen_EOL_assertion_ = false;

        state_machine_.clear ();

        for (; index_ < size_; ++index_)
        {
            internals_._lookup->push_back (static_cast<size_t_vector *>(0));
            internals_._lookup->back () = new size_t_vector;
            internals_._dfa_alphabet.push_back (0);
            internals_._dfa->push_back (static_cast<size_t_vector *>(0));
            internals_._dfa->back () = new size_t_vector;
        }

        for (index_ = 0, size_ = internals_._lookup->size ();
            index_ < size_; ++index_)
        {
            internals_._lookup[index_]->resize (sizeof (CharT) == 1 ?
                num_chars : num_wchar_ts, dead_state_index);

            if (!rules_.regexes ()[index_].empty ())
            {
                // vector mapping token indexes to partitioned token index sets
                index_set_vector set_mapping_;
                // syntax tree
                detail::node *root_ = build_tree (rules_, index_,
                    node_ptr_vector_, internals_, set_mapping_);

                build_dfa (root_, set_mapping_,
                    internals_._dfa_alphabet[index_],
                    *internals_._dfa[index_]);

                if (internals_._seen_BOL_assertion)
                {
                    seen_BOL_assertion_ = true;
                }

                if (internals_._seen_EOL_assertion)
                {
                    seen_EOL_assertion_ = true;
                }

                internals_._seen_BOL_assertion = false;
                internals_._seen_EOL_assertion = false;
            }
        }

        internals_._seen_BOL_assertion = seen_BOL_assertion_;
        internals_._seen_EOL_assertion = seen_EOL_assertion_;
    }

    static void minimise (basic_state_machine<CharT> &state_machine_)
    {
        detail::internals &internals_ = const_cast<detail::internals &>
            (state_machine_.data ());
        const std::size_t machines_ = internals_._dfa->size ();

        for (std::size_t i_ = 0; i_ < machines_; ++i_)
        {
            const std::size_t dfa_alphabet_ = internals_._dfa_alphabet[i_];
            size_t_vector *dfa_ = internals_._dfa[i_];

            if (dfa_alphabet_ != 0)
            {
                std::size_t size_ = 0;

                do
                {
                    size_ = dfa_->size ();
                    minimise_dfa (dfa_alphabet_, *dfa_, size_);
                } while (dfa_->size () != size_);
            }
        }
    }

protected:
    typedef detail::basic_charset<CharT> charset;
    typedef detail::ptr_list<charset> charset_list;
    typedef boost::movelib::unique_ptr<charset> charset_ptr;
    typedef detail::equivset equivset;
    typedef detail::ptr_list<equivset> equivset_list;
    typedef boost::movelib::unique_ptr<equivset> equivset_ptr;
    typedef typename charset::index_set index_set;
    typedef std::vector<index_set> index_set_vector;
    typedef detail::basic_parser<CharT> parser;
    typedef typename parser::node_ptr_vector node_ptr_vector;
    typedef std::set<const detail::node *> node_set;
    typedef detail::ptr_vector<node_set> node_set_vector;
    typedef std::vector<const detail::node *> node_vector;
    typedef detail::ptr_vector<node_vector> node_vector_vector;
    typedef typename parser::string string;
    typedef std::pair<string, string> string_pair;
    typedef typename parser::tokeniser::string_token string_token;
    typedef std::deque<string_pair> macro_deque;
    typedef std::pair<string, const detail::node *> macro_pair;
    typedef typename parser::macro_map::iterator macro_iter;
    typedef std::pair<macro_iter, bool> macro_iter_pair;
    typedef typename parser::tokeniser::token_map token_map;

    static detail::node *build_tree (const rules &rules_,
        const std::size_t state_, node_ptr_vector &node_ptr_vector_,
        detail::internals &internals_, index_set_vector &set_mapping_)
    {
        size_t_vector *lookup_ = internals_._lookup[state_];
        const typename rules::string_deque_deque &regexes_ =
            rules_.regexes ();
        const typename rules::id_vector_deque &ids_ = rules_.ids ();
        const typename rules::id_vector_deque &unique_ids_ =
            rules_.unique_ids ();
        const typename rules::id_vector_deque &states_ = rules_.states ();
        typename rules::string_deque::const_iterator regex_iter_ =
            regexes_[state_].begin ();
        typename rules::string_deque::const_iterator regex_iter_end_ =
            regexes_[state_].end ();
        typename rules::id_vector::const_iterator ids_iter_ =
            ids_[state_].begin ();
        typename rules::id_vector::const_iterator unique_ids_iter_ =
            unique_ids_[state_].begin ();
        typename rules::id_vector::const_iterator states_iter_ =
            states_[state_].begin ();
        const typename rules::string &regex_ = *regex_iter_;
        // map of regex charset tokens (strings) to index
        token_map token_map_;
        const typename rules::string_pair_deque &macrodeque_ =
            rules_.macrodeque ();
        typename parser::macro_map macromap_;
        typename detail::node::node_vector tree_vector_;

        build_macros (token_map_, macrodeque_, macromap_,
            rules_.flags (), rules_.locale (), node_ptr_vector_,
            internals_._seen_BOL_assertion, internals_._seen_EOL_assertion);

        detail::node *root_ = parser::parse (regex_.c_str (),
            regex_.c_str () + regex_.size (), *ids_iter_, *unique_ids_iter_,
            *states_iter_, rules_.flags (), rules_.locale (), node_ptr_vector_,
            macromap_, token_map_, internals_._seen_BOL_assertion,
            internals_._seen_EOL_assertion);

        ++regex_iter_;
        ++ids_iter_;
        ++unique_ids_iter_;
        ++states_iter_;
        tree_vector_.push_back (root_);

        // build syntax trees
        while (regex_iter_ != regex_iter_end_)
        {
            // re-declare var, otherwise we perform an assignment..!
            const typename rules::string &regex2_ = *regex_iter_;

            root_ = parser::parse (regex2_.c_str (),
                regex2_.c_str () + regex2_.size (), *ids_iter_,
                *unique_ids_iter_, *states_iter_, rules_.flags (),
                rules_.locale (), node_ptr_vector_, macromap_, token_map_,
                internals_._seen_BOL_assertion,
                internals_._seen_EOL_assertion);
            tree_vector_.push_back (root_);
            ++regex_iter_;
            ++ids_iter_;
            ++unique_ids_iter_;
            ++states_iter_;
        }

        if (internals_._seen_BOL_assertion)
        {
            // Fixup BOLs
            typename detail::node::node_vector::iterator iter_ =
                tree_vector_.begin ();
            typename detail::node::node_vector::iterator end_ =
                tree_vector_.end ();

            for (; iter_ != end_; ++iter_)
            {
                fixup_bol (*iter_, node_ptr_vector_);
            }
        }

        // join trees
        {
            typename detail::node::node_vector::iterator iter_ =
                tree_vector_.begin ();
            typename detail::node::node_vector::iterator end_ =
                tree_vector_.end ();

            if (iter_ != end_)
            {
                root_ = *iter_;
                ++iter_;
            }

            for (; iter_ != end_; ++iter_)
            {
                node_ptr_vector_->push_back (static_cast<detail::selection_node *>(0));
                node_ptr_vector_->back () = new detail::selection_node
                    (root_, *iter_);
                root_ = node_ptr_vector_->back ();
            }
        }

        // partitioned token list
        charset_list token_list_;

        set_mapping_.resize (token_map_.size ());
        partition_tokens (token_map_, token_list_);

        typename charset_list::list::const_iterator iter_ =
            token_list_->begin ();
        typename charset_list::list::const_iterator end_ =
            token_list_->end ();
        std::size_t index_ = 0;

        for (; iter_ != end_; ++iter_, ++index_)
        {
            const charset *cs_ = *iter_;
            typename charset::index_set::const_iterator set_iter_ =
                cs_->_index_set.begin ();
            typename charset::index_set::const_iterator set_end_ =
                cs_->_index_set.end ();

            fill_lookup (cs_->_token, lookup_, index_);

            for (; set_iter_ != set_end_; ++set_iter_)
            {
                set_mapping_[*set_iter_].insert (index_);
            }
        }

        internals_._dfa_alphabet[state_] = token_list_->size () + dfa_offset;
        return root_;
    }

    static void build_macros (token_map &token_map_,
        const macro_deque &macrodeque_,
        typename parser::macro_map &macromap_, const regex_flags flags_,
        const std::locale &locale_, node_ptr_vector &node_ptr_vector_,
        bool &seen_BOL_assertion_, bool &seen_EOL_assertion_)
    {
        for (typename macro_deque::const_iterator iter_ =
            macrodeque_.begin (), end_ = macrodeque_.end ();
            iter_ != end_; ++iter_)
        {
            const typename rules::string &name_ = iter_->first;
            const typename rules::string &regex_ = iter_->second;
            detail::node *node_ = parser::parse (regex_.c_str (),
                regex_.c_str () + regex_.size (), 0, 0, 0, flags_,
                locale_, node_ptr_vector_, macromap_, token_map_,
                seen_BOL_assertion_, seen_EOL_assertion_);
            macro_iter_pair map_iter_ = macromap_.
                insert (macro_pair (name_, static_cast<const detail::node *>
                (0)));

            map_iter_.first->second = node_;
        }
    }

    static void build_dfa (detail::node *root_,
        const index_set_vector &set_mapping_, const std::size_t dfa_alphabet_,
        size_t_vector &dfa_)
    {
        typename detail::node::node_vector *followpos_ =
            &root_->firstpos ();
        node_set_vector seen_sets_;
        node_vector_vector seen_vectors_;
        size_t_vector hash_vector_;

        // 'jam' state
        dfa_.resize (dfa_alphabet_, 0);
        closure (followpos_, seen_sets_, seen_vectors_,
            hash_vector_, dfa_alphabet_, dfa_);

        std::size_t *ptr_ = 0;

        for (std::size_t index_ = 0; index_ < seen_vectors_->size (); ++index_)
        {
            equivset_list equiv_list_;

            build_equiv_list (seen_vectors_[index_], set_mapping_, equiv_list_);

            for (typename equivset_list::list::const_iterator iter_ =
                equiv_list_->begin (), end_ = equiv_list_->end ();
                iter_ != end_; ++iter_)
            {
                equivset *equivset_ = *iter_;
                const std::size_t transition_ = closure
                    (&equivset_->_followpos, seen_sets_, seen_vectors_,
                    hash_vector_, dfa_alphabet_, dfa_);

                if (transition_ != npos)
                {
                    ptr_ = &dfa_.front () + ((index_ + 1) * dfa_alphabet_);

                    // Prune abstemious transitions from end states.
                    if (*ptr_ && !equivset_->_greedy) continue;

                    for (typename detail::equivset::index_vector::const_iterator
                        equiv_iter_ = equivset_->_index_vector.begin (),
                        equiv_end_ = equivset_->_index_vector.end ();
                        equiv_iter_ != equiv_end_; ++equiv_iter_)
                    {
                        const std::size_t equiv_index_ = *equiv_iter_;

                        if (equiv_index_ == bol_token)
                        {
                            if (ptr_[eol_index] == 0)
                            {
                                ptr_[bol_index] = transition_;
                            }
                        }
                        else if (equiv_index_ == eol_token)
                        {
                            if (ptr_[bol_index] == 0)
                            {
                                ptr_[eol_index] = transition_;
                            }
                        }
                        else
                        {
                            ptr_[equiv_index_ + dfa_offset] = transition_;
                        }
                    }
                }
            }
        }
    }

    static std::size_t closure (typename detail::node::node_vector *followpos_,
        node_set_vector &seen_sets_, node_vector_vector &seen_vectors_,
        size_t_vector &hash_vector_, const std::size_t size_,
        size_t_vector &dfa_)
    {
        bool end_state_ = false;
        std::size_t id_ = 0;
        std::size_t unique_id_ = npos;
        std::size_t state_ = 0;
        std::size_t hash_ = 0;

        if (followpos_->empty ()) return npos;

        std::size_t index_ = 0;
        boost::movelib::unique_ptr<node_set> set_ptr_ (new node_set);
        boost::movelib::unique_ptr<node_vector> vector_ptr_ (new node_vector);

        for (typename detail::node::node_vector::const_iterator iter_ =
            followpos_->begin (), end_ = followpos_->end ();
            iter_ != end_; ++iter_)
        {
            closure_ex (*iter_, end_state_, id_, unique_id_, state_,
                set_ptr_.get (), vector_ptr_.get (), hash_);
        }

        bool found_ = false;
        typename size_t_vector::const_iterator hash_iter_ =
            hash_vector_.begin ();
        typename size_t_vector::const_iterator hash_end_ =
            hash_vector_.end ();
        typename node_set_vector::vector::const_iterator set_iter_ =
            seen_sets_->begin ();

        for (; hash_iter_ != hash_end_; ++hash_iter_, ++set_iter_)
        {
            found_ = *hash_iter_ == hash_ && *(*set_iter_) == *set_ptr_;
            ++index_;

            if (found_) break;
        }

        if (!found_)
        {
            seen_sets_->push_back (static_cast<node_set *>(0));
            seen_sets_->back () = set_ptr_.release ();
            seen_vectors_->push_back (static_cast<node_vector *>(0));
            seen_vectors_->back () = vector_ptr_.release ();
            hash_vector_.push_back (hash_);
            // State 0 is the jam state...
            index_ = seen_sets_->size ();

            const std::size_t old_size_ = dfa_.size ();

            dfa_.resize (old_size_ + size_, 0);

            if (end_state_)
            {
                dfa_[old_size_] |= end_state;
                dfa_[old_size_ + id_index] = id_;
                dfa_[old_size_ + unique_id_index] = unique_id_;
                dfa_[old_size_ + state_index] = state_;
            }
        }

        return index_;
    }

    static void closure_ex (detail::node *node_, bool &end_state_,
        std::size_t &id_, std::size_t &unique_id_, std::size_t &state_,
        node_set *set_ptr_, node_vector *vector_ptr_, std::size_t &hash_)
    {
        const bool temp_end_state_ = node_->end_state ();

        if (temp_end_state_)
        {
            if (!end_state_)
            {
                end_state_ = true;
                id_ = node_->id ();
                unique_id_ = node_->unique_id ();
                state_ = node_->lexer_state ();
            }
        }

        if (set_ptr_->insert (node_).second)
        {
            vector_ptr_->push_back (node_);
            hash_ += reinterpret_cast<std::size_t> (node_);
        }
    }

    static void partition_tokens (const token_map &map_,
        charset_list &lhs_)
    {
        charset_list rhs_;

        fill_rhs_list (map_, rhs_);

        if (!rhs_->empty ())
        {
            typename charset_list::list::iterator iter_;
            typename charset_list::list::iterator end_;
            charset_ptr overlap_ (new charset);

            lhs_->push_back (static_cast<charset *>(0));
            lhs_->back () = rhs_->front ();
            rhs_->pop_front ();

            while (!rhs_->empty ())
            {
                charset_ptr r_ (rhs_->front ());

                rhs_->pop_front ();
                iter_ = lhs_->begin ();
                end_ = lhs_->end ();

                while (!r_->empty () && iter_ != end_)
                {
                    typename charset_list::list::iterator l_iter_ = iter_;

                    (*l_iter_)->intersect (*r_.get (), *overlap_.get ());

                    if (overlap_->empty ())
                    {
                        ++iter_;
                    }
                    else if ((*l_iter_)->empty ())
                    {
                        delete *l_iter_;
                        *l_iter_ = overlap_.release ();

                        overlap_.reset (new charset);
                        ++iter_;
                    }
                    else if (r_->empty ())
                    {
                        overlap_.swap (r_);
                        overlap_.reset (new charset);
                        break;
                    }
                    else
                    {
                        iter_ = lhs_->insert (++iter_,
                            static_cast<charset *>(0));
                        *iter_ = overlap_.release ();

                        overlap_.reset(new charset);
                        ++iter_;
                        end_ = lhs_->end ();
                    }
                }

                if (!r_->empty ())
                {
                    lhs_->push_back (static_cast<charset *>(0));
                    lhs_->back () = r_.release ();
                }
            }
        }
    }

    static void fill_rhs_list (const token_map &map_,
        charset_list &list_)
    {
        typename parser::tokeniser::token_map::const_iterator iter_ =
            map_.begin ();
        typename parser::tokeniser::token_map::const_iterator end_ =
            map_.end ();

        for (; iter_ != end_; ++iter_)
        {
            list_->push_back (static_cast<charset *>(0));
            list_->back () = new charset (iter_->first, iter_->second);
        }
    }

    static void fill_lookup (const string_token &token_,
        size_t_vector *lookup_, const std::size_t index_)
    {
        const CharT *curr_ = token_._charset.c_str ();
        const CharT *chars_end_ = curr_ + token_._charset.size ();
        std::size_t *ptr_ = &lookup_->front ();
        const std::size_t max_ = sizeof (CharT) == 1 ?
            num_chars : num_wchar_ts;

        if (token_._negated)
        {
            // $$$ FIXME JDG July 2014 $$$
            // this code is problematic on platforms where wchar_t is signed
            // with min generating negative numbers. This crashes with BAD_ACCESS
            // because of the vector index below:
            //  ptr_[static_cast<typename Traits::index_type>(curr_char_)]
            CharT curr_char_ = 0; // (std::numeric_limits<CharT>::min)();
            std::size_t i_ = 0;

            while (curr_ < chars_end_)
            {
                while (*curr_ > curr_char_)
                {
                    ptr_[static_cast<typename Traits::index_type>
                        (curr_char_)] = index_ + dfa_offset;
                    ++curr_char_;
                    ++i_;
                }

                ++curr_char_;
                ++curr_;
                ++i_;
            }

            for (; i_ < max_; ++i_)
            {
                ptr_[static_cast<typename Traits::index_type>(curr_char_)] =
                    index_ + dfa_offset;
                ++curr_char_;
            }
        }
        else
        {
            while (curr_ < chars_end_)
            {
                ptr_[static_cast<typename Traits::index_type>(*curr_)] =
                    index_ + dfa_offset;
                ++curr_;
            }
        }
    }

    static void build_equiv_list (const node_vector *vector_,
        const index_set_vector &set_mapping_, equivset_list &lhs_)
    {
        equivset_list rhs_;

        fill_rhs_list (vector_, set_mapping_, rhs_);

        if (!rhs_->empty ())
        {
            typename equivset_list::list::iterator iter_;
            typename equivset_list::list::iterator end_;
            equivset_ptr overlap_ (new equivset);

            lhs_->push_back (static_cast<equivset *>(0));
            lhs_->back () = rhs_->front ();
            rhs_->pop_front ();

            while (!rhs_->empty ())
            {
                equivset_ptr r_ (rhs_->front ());

                rhs_->pop_front ();
                iter_ = lhs_->begin ();
                end_ = lhs_->end ();

                while (!r_->empty () && iter_ != end_)
                {
                    typename equivset_list::list::iterator l_iter_ = iter_;

                    (*l_iter_)->intersect (*r_.get (), *overlap_.get ());

                    if (overlap_->empty ())
                    {
                        ++iter_;
                    }
                    else if ((*l_iter_)->empty ())
                    {
                        delete *l_iter_;
                        *l_iter_ = overlap_.release ();

                        overlap_.reset (new equivset);
                        ++iter_;
                    }
                    else if (r_->empty ())
                    {
                        overlap_.swap (r_);
                        overlap_.reset (new equivset);
                        break;
                    }
                    else
                    {
                        iter_ = lhs_->insert (++iter_,
                            static_cast<equivset *>(0));
                        *iter_ = overlap_.release ();

                        overlap_.reset (new equivset);
                        ++iter_;
                        end_ = lhs_->end ();
                    }
                }

                if (!r_->empty ())
                {
                    lhs_->push_back (static_cast<equivset *>(0));
                    lhs_->back () = r_.release ();
                }
            }
        }
    }

    static void fill_rhs_list (const node_vector *vector_,
        const index_set_vector &set_mapping_, equivset_list &list_)
    {
        typename node_vector::const_iterator iter_ =
            vector_->begin ();
        typename node_vector::const_iterator end_ =
            vector_->end ();

        for (; iter_ != end_; ++iter_)
        {
            const detail::node *node_ = *iter_;

            if (!node_->end_state ())
            {
                const std::size_t token_ = node_->token ();

                if (token_ != null_token)
                {
                    list_->push_back (static_cast<equivset *>(0));

                    if (token_ == bol_token || token_ == eol_token)
                    {
                        std::set<std::size_t> index_set_;

                        index_set_.insert (token_);
                        list_->back () = new equivset (index_set_,
                            node_->greedy (), token_, node_->followpos ());
                    }
                    else
                    {
                        list_->back () = new equivset (set_mapping_[token_],
                            node_->greedy (), token_, node_->followpos ());
                    }
                }
            }
        }
    }

    static void fixup_bol (detail::node * &root_,
        node_ptr_vector &node_ptr_vector_)
    {
        typename detail::node::node_vector *first_ = &root_->firstpos ();
        bool found_ = false;
        typename detail::node::node_vector::const_iterator iter_ =
            first_->begin ();
        typename detail::node::node_vector::const_iterator end_ =
            first_->end ();

        for (; iter_ != end_; ++iter_)
        {
            const detail::node *node_ = *iter_;

            found_ = !node_->end_state () && node_->token () == bol_token;

            if (found_) break;
        }

        if (!found_)
        {
            node_ptr_vector_->push_back (static_cast<detail::leaf_node *>(0));
            node_ptr_vector_->back () = new detail::leaf_node
                (bol_token, true);

            detail::node *lhs_ = node_ptr_vector_->back ();

            node_ptr_vector_->push_back (static_cast<detail::leaf_node *>(0));
            node_ptr_vector_->back () = new detail::leaf_node
                (null_token, true);

            detail::node *rhs_ = node_ptr_vector_->back ();

            node_ptr_vector_->push_back
                (static_cast<detail::selection_node *>(0));
            node_ptr_vector_->back () =
                new detail::selection_node (lhs_, rhs_);
            lhs_ = node_ptr_vector_->back ();

            node_ptr_vector_->push_back
                (static_cast<detail::sequence_node *>(0));
            node_ptr_vector_->back () =
                new detail::sequence_node (lhs_, root_);
            root_ = node_ptr_vector_->back ();
        }
    }

    static void minimise_dfa (const std::size_t dfa_alphabet_,
        size_t_vector &dfa_, std::size_t size_)
    {
        const std::size_t *first_ = &dfa_.front ();
        const std::size_t *second_ = 0;
        const std::size_t *end_ = first_ + size_;
        std::size_t index_ = 1;
        std::size_t new_index_ = 1;
        std::size_t curr_index_ = 0;
        index_set index_set_;
        size_t_vector lookup_;
        std::size_t *lookup_ptr_ = 0;

        lookup_.resize (size_ / dfa_alphabet_, null_token);
        lookup_ptr_ = &lookup_.front ();
        *lookup_ptr_ = 0;
        // Only one 'jam' state, so skip it.
        first_ += dfa_alphabet_;

        for (; first_ < end_; first_ += dfa_alphabet_, ++index_)
        {
            for (second_ = first_ + dfa_alphabet_, curr_index_ = index_ + 1;
                second_ < end_; second_ += dfa_alphabet_, ++curr_index_)
            {
                if (index_set_.find (curr_index_) != index_set_.end ())
                {
                    continue;
                }

                // Some systems have memcmp in namespace std.
                using namespace std;

                if (memcmp (first_, second_, sizeof (std::size_t) *
                    dfa_alphabet_) == 0)
                {
                    index_set_.insert (curr_index_);
                    lookup_ptr_[curr_index_] = new_index_;
                }
            }

            if (lookup_ptr_[index_] == null_token)
            {
                lookup_ptr_[index_] = new_index_;
                ++new_index_;
            }
        }

        if (!index_set_.empty ())
        {
            const std::size_t *front_ = &dfa_.front ();
            size_t_vector new_dfa_ (front_, front_ + dfa_alphabet_);
            typename index_set::iterator set_end_ =
                index_set_.end ();
            const std::size_t *ptr_ = front_ + dfa_alphabet_;
            std::size_t *new_ptr_ = 0;

            new_dfa_.resize (size_ - index_set_.size () * dfa_alphabet_, 0);
            new_ptr_ = &new_dfa_.front () + dfa_alphabet_;
            size_ /= dfa_alphabet_;

            for (index_ = 1; index_ < size_; ++index_)
            {
                if (index_set_.find (index_) != set_end_)
                {
                    ptr_ += dfa_alphabet_;
                    continue;
                }

                new_ptr_[end_state_index] = ptr_[end_state_index];
                new_ptr_[id_index] = ptr_[id_index];
                new_ptr_[unique_id_index] = ptr_[unique_id_index];
                new_ptr_[state_index] = ptr_[state_index];
                new_ptr_[bol_index] = lookup_ptr_[ptr_[bol_index]];
                new_ptr_[eol_index] = lookup_ptr_[ptr_[eol_index]];
                new_ptr_ += dfa_offset;
                ptr_ += dfa_offset;

                for (std::size_t i_ = dfa_offset; i_ < dfa_alphabet_; ++i_)
                {
                    *new_ptr_++ = lookup_ptr_[*ptr_++];
                }
            }

            dfa_.swap (new_dfa_);
        }
    }
};

typedef basic_generator<char> generator;
typedef basic_generator<wchar_t> wgenerator;
}
}

#endif

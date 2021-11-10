// equivset.hpp
// Copyright (c) 2007-2009 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_SPIRIT_SUPPORT_DETAIL_LEXER_PARTITION_EQUIVSET_HPP
#define BOOST_SPIRIT_SUPPORT_DETAIL_LEXER_PARTITION_EQUIVSET_HPP

#include <algorithm>
#include "../parser/tree/node.hpp"
#include <set>
#include "../size_t.hpp"

namespace boost
{
namespace lexer
{
namespace detail
{
struct equivset
{
    typedef std::set<std::size_t> index_set;
    typedef std::vector<std::size_t> index_vector;
    // Not owner of nodes:
    typedef std::vector<node *> node_vector;

    index_vector _index_vector;
    bool _greedy;
    std::size_t _id;
    node_vector _followpos;

    equivset () :
        _greedy (true),
        _id (0)
    {
    }

    equivset (const index_set &index_set_, const bool greedy_,
        const std::size_t id_, const node_vector &followpos_) :
        _greedy (greedy_),
        _id (id_),
        _followpos (followpos_)
    {
        index_set::const_iterator iter_ = index_set_.begin ();
        index_set::const_iterator end_ = index_set_.end ();

        for (; iter_ != end_; ++iter_)
        {
            _index_vector.push_back (*iter_);
        }
    }

    bool empty () const
    {
        return _index_vector.empty () && _followpos.empty ();
    }

    void intersect (equivset &rhs_, equivset &overlap_)
    {
        intersect_indexes (rhs_._index_vector, overlap_._index_vector);

        if (!overlap_._index_vector.empty ())
        {
            // Note that the LHS takes priority in order to
            // respect rule ordering priority in the lex spec.
            overlap_._id = _id;
            overlap_._greedy = _greedy;
            overlap_._followpos = _followpos;

            node_vector::const_iterator overlap_begin_ =
                overlap_._followpos.begin ();
            node_vector::const_iterator overlap_end_ =
                overlap_._followpos.end ();
            node_vector::const_iterator rhs_iter_ =
                rhs_._followpos.begin ();
            node_vector::const_iterator rhs_end_ =
                rhs_._followpos.end ();

            for (; rhs_iter_ != rhs_end_; ++rhs_iter_)
            {
                node *node_ = *rhs_iter_;

                if (std::find (overlap_begin_, overlap_end_, node_) ==
                    overlap_end_)
                {
                    overlap_._followpos.push_back (node_);
                    overlap_begin_ = overlap_._followpos.begin ();
                    overlap_end_ = overlap_._followpos.end ();
                }
            }

            if (_index_vector.empty ())
            {
                _followpos.clear ();
            }

            if (rhs_._index_vector.empty ())
            {
                rhs_._followpos.clear ();
            }
        }
    }

private:
    void intersect_indexes (index_vector &rhs_, index_vector &overlap_)
    {
        index_vector::iterator iter_ = _index_vector.begin ();
        index_vector::iterator end_ = _index_vector.end ();
        index_vector::iterator rhs_iter_ = rhs_.begin ();
        index_vector::iterator rhs_end_ = rhs_.end ();

        while (iter_ != end_ && rhs_iter_ != rhs_end_)
        {
            const std::size_t index_ = *iter_;
            const std::size_t rhs_index_ = *rhs_iter_;

            if (index_ < rhs_index_)
            {
                ++iter_;
            }
            else if (index_ > rhs_index_)
            {
                ++rhs_iter_;
            }
            else
            {
                overlap_.push_back (index_);
                iter_ = _index_vector.erase (iter_);
                end_ = _index_vector.end ();
                rhs_iter_ = rhs_.erase (rhs_iter_);
                rhs_end_ = rhs_.end ();
            }
        }
    }
};
}
}
}

#endif

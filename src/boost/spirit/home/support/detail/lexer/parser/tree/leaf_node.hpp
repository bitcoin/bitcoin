// leaf_node.hpp
// Copyright (c) 2007-2009 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_SPIRIT_SUPPORT_DETAIL_LEXER_PARSER_TREE_LEAF_NODE_HPP
#define BOOST_SPIRIT_SUPPORT_DETAIL_LEXER_PARSER_TREE_LEAF_NODE_HPP

#include "../../consts.hpp" // null_token
#include "node.hpp"
#include "../../size_t.hpp"

namespace boost
{
namespace lexer
{
namespace detail
{
class leaf_node : public node
{
public:
    leaf_node (const std::size_t token_, const bool greedy_) :
        node (token_ == null_token),
        _token (token_),
        _set_greedy (!greedy_),
        _greedy (greedy_)
    {
        if (!_nullable)
        {
            _firstpos.push_back (this);
            _lastpos.push_back (this);
        }
    }

    virtual ~leaf_node ()
    {
    }

    virtual void append_followpos (const node_vector &followpos_)
    {
        for (node_vector::const_iterator iter_ = followpos_.begin (),
            end_ = followpos_.end (); iter_ != end_; ++iter_)
        {
            _followpos.push_back (*iter_);
        }
    }

    virtual type what_type () const
    {
        return LEAF;
    }

    virtual bool traverse (const_node_stack &/*node_stack_*/,
        bool_stack &/*perform_op_stack_*/) const
    {
        return false;
    }

    virtual std::size_t token () const
    {
        return _token;
    }

    virtual void greedy (const bool greedy_)
    {
        if (!_set_greedy)
        {
            _greedy = greedy_;
            _set_greedy = true;
        }
    }

    virtual bool greedy () const
    {
        return _greedy;
    }

    virtual const node_vector &followpos () const
    {
        return _followpos;
    }

    virtual node_vector &followpos ()
    {
        return _followpos;
    }

private:
    std::size_t _token;
    bool _set_greedy;
    bool _greedy;
    node_vector _followpos;

    virtual void copy_node (node_ptr_vector &node_ptr_vector_,
        node_stack &new_node_stack_, bool_stack &/*perform_op_stack_*/,
        bool &/*down_*/) const
    {
        node_ptr_vector_->push_back (static_cast<leaf_node *>(0));
        node_ptr_vector_->back () = new leaf_node (_token, _greedy);
        new_node_stack_.push (node_ptr_vector_->back ());
    }
};
}
}
}

#endif

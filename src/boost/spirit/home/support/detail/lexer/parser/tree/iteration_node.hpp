// iteration_node.hpp
// Copyright (c) 2007-2009 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_SPIRIT_SUPPORT_DETAIL_LEXER_PARSER_TREE_ITERATION_NODE_HPP
#define BOOST_SPIRIT_SUPPORT_DETAIL_LEXER_PARSER_TREE_ITERATION_NODE_HPP

#include "node.hpp"

namespace boost
{
namespace lexer
{
namespace detail
{
class iteration_node : public node
{
public:
    iteration_node (node *next_, const bool greedy_) :
        node (true),
        _next (next_),
        _greedy (greedy_)
    {
        node_vector::iterator iter_;
        node_vector::iterator end_;

        _next->append_firstpos (_firstpos);
        _next->append_lastpos (_lastpos);

        for (iter_ = _lastpos.begin (), end_ = _lastpos.end ();
            iter_ != end_; ++iter_)
        {
            (*iter_)->append_followpos (_firstpos);
        }

        for (iter_ = _firstpos.begin (), end_ = _firstpos.end ();
            iter_ != end_; ++iter_)
        {
            (*iter_)->greedy (greedy_);
        }
    }

    virtual ~iteration_node ()
    {
    }

    virtual type what_type () const
    {
        return ITERATION;
    }

    virtual bool traverse (const_node_stack &node_stack_,
        bool_stack &perform_op_stack_) const
    {
        perform_op_stack_.push (true);
        node_stack_.push (_next);
        return true;
    }

private:
    // Not owner of this pointer...
    node *_next;
    bool _greedy;

    virtual void copy_node (node_ptr_vector &node_ptr_vector_,
        node_stack &new_node_stack_, bool_stack &perform_op_stack_,
        bool &down_) const
    {
        if (perform_op_stack_.top ())
        {
            node *ptr_ = new_node_stack_.top ();

            node_ptr_vector_->push_back (static_cast<iteration_node *>(0));
            node_ptr_vector_->back () = new iteration_node (ptr_, _greedy);
            new_node_stack_.top () = node_ptr_vector_->back ();
        }
        else
        {
            down_ = true;
        }

        perform_op_stack_.pop ();
    }
};
}
}
}

#endif

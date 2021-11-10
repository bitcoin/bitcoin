// sequence_node.hpp
// Copyright (c) 2007-2009 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_SPIRIT_SUPPORT_DETAIL_LEXER_PARSER_TREE_SEQUENCE_NODE_HPP
#define BOOST_SPIRIT_SUPPORT_DETAIL_LEXER_PARSER_TREE_SEQUENCE_NODE_HPP

#include "node.hpp"

namespace boost
{
namespace lexer
{
namespace detail
{
class sequence_node : public node
{
public:
    sequence_node (node *left_, node *right_) :
        node (left_->nullable () && right_->nullable ()),
        _left (left_),
        _right (right_)
    {
        _left->append_firstpos (_firstpos);

        if (_left->nullable ())
        {
            _right->append_firstpos (_firstpos);
        }

        if (_right->nullable ())
        {
            _left->append_lastpos (_lastpos);
        }

        _right->append_lastpos (_lastpos);

        node_vector &lastpos_ = _left->lastpos ();
        const node_vector &firstpos_ = _right->firstpos ();

        for (node_vector::iterator iter_ = lastpos_.begin (),
            end_ = lastpos_.end (); iter_ != end_; ++iter_)
        {
            (*iter_)->append_followpos (firstpos_);
        }
    }

    virtual ~sequence_node ()
    {
    }

    virtual type what_type () const
    {
        return SEQUENCE;
    }

    virtual bool traverse (const_node_stack &node_stack_,
        bool_stack &perform_op_stack_) const
    {
        perform_op_stack_.push (true);

        switch (_right->what_type ())
        {
        case SEQUENCE:
        case SELECTION:
        case ITERATION:
            perform_op_stack_.push (false);
            break;
        default:
            break;
        }

        node_stack_.push (_right);
        node_stack_.push (_left);
        return true;
    }

private:
    // Not owner of these pointers...
    node *_left;
    node *_right;

    virtual void copy_node (node_ptr_vector &node_ptr_vector_,
        node_stack &new_node_stack_, bool_stack &perform_op_stack_,
        bool &down_) const
    {
        if (perform_op_stack_.top ())
        {
            node *rhs_ = new_node_stack_.top ();

            new_node_stack_.pop ();

            node *lhs_ = new_node_stack_.top ();

            node_ptr_vector_->push_back (static_cast<sequence_node *>(0));
            node_ptr_vector_->back () = new sequence_node (lhs_, rhs_);
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

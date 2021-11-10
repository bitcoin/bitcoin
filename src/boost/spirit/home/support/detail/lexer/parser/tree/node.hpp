// node.hpp
// Copyright (c) 2007-2009 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_SPIRIT_SUPPORT_DETAIL_LEXER_PARSER_TREE_NODE_HPP
#define BOOST_SPIRIT_SUPPORT_DETAIL_LEXER_PARSER_TREE_NODE_HPP

#include <boost/assert.hpp>
#include "../../containers/ptr_vector.hpp"
#include "../../runtime_error.hpp"
#include "../../size_t.hpp"
#include <stack>
#include <vector>

namespace boost
{
namespace lexer
{
namespace detail
{
class node
{
public:
    enum type {LEAF, SEQUENCE, SELECTION, ITERATION, END};

    typedef std::stack<bool> bool_stack;
    typedef std::stack<node *> node_stack;
    // stack and vector not owner of node pointers
    typedef std::stack<const node *> const_node_stack;
    typedef std::vector<node *> node_vector;
    typedef ptr_vector<node> node_ptr_vector;

    node () :
        _nullable (false)
    {
    }

    node (const bool nullable_) :
        _nullable (nullable_)
    {
    }

    virtual ~node ()
    {
    }

    bool nullable () const
    {
        return _nullable;
    }

    void append_firstpos (node_vector &firstpos_) const
    {
        firstpos_.insert (firstpos_.end (),
            _firstpos.begin (), _firstpos.end ());
    }

    void append_lastpos (node_vector &lastpos_) const
    {
        lastpos_.insert (lastpos_.end (),
            _lastpos.begin (), _lastpos.end ());
    }

    virtual void append_followpos (const node_vector &/*followpos_*/)
    {
        throw runtime_error ("Internal error node::append_followpos()");
    }

    node *copy (node_ptr_vector &node_ptr_vector_) const
    {
        node *new_root_ = 0;
        const_node_stack node_stack_;
        bool_stack perform_op_stack_;
        bool down_ = true;
        node_stack new_node_stack_;

        node_stack_.push (this);

        while (!node_stack_.empty ())
        {
            while (down_)
            {
                down_ = node_stack_.top ()->traverse (node_stack_,
                    perform_op_stack_);
            }

            while (!down_ && !node_stack_.empty ())
            {
                const node *top_ = node_stack_.top ();

                top_->copy_node (node_ptr_vector_, new_node_stack_,
                    perform_op_stack_, down_);

                if (!down_) node_stack_.pop ();
            }
        }

        BOOST_ASSERT(new_node_stack_.size () == 1);
        new_root_ = new_node_stack_.top ();
        new_node_stack_.pop ();
        return new_root_;
    }

    virtual type what_type () const = 0;

    virtual bool traverse (const_node_stack &node_stack_,
        bool_stack &perform_op_stack_) const = 0;

    node_vector &firstpos ()
    {
        return _firstpos;
    }

    const node_vector &firstpos () const
    {
        return _firstpos;
    }

    // _lastpos modified externally, so not const &
    node_vector &lastpos ()
    {
        return _lastpos;
    }

    virtual bool end_state () const
    {
        return false;
    }

    virtual std::size_t id () const
    {
        throw runtime_error ("Internal error node::id()");
    }

    virtual std::size_t unique_id () const
    {
        throw runtime_error ("Internal error node::unique_id()");
    }

    virtual std::size_t lexer_state () const
    {
        throw runtime_error ("Internal error node::state()");
    }

    virtual std::size_t token () const
    {
        throw runtime_error ("Internal error node::token()");
    }

    virtual void greedy (const bool /*greedy_*/)
    {
        throw runtime_error ("Internal error node::token(bool)");
    }

    virtual bool greedy () const
    {
        throw runtime_error ("Internal error node::token()");
    }

    virtual const node_vector &followpos () const
    {
        throw runtime_error ("Internal error node::followpos()");
    }

    virtual node_vector &followpos ()
    {
        throw runtime_error ("Internal error node::followpos()");
    }

protected:
    const bool _nullable;
    node_vector _firstpos;
    node_vector _lastpos;

    virtual void copy_node (node_ptr_vector &node_ptr_vector_,
        node_stack &new_node_stack_, bool_stack &perform_op_stack_,
        bool &down_) const = 0;

private:
    node (node const &); // No copy construction.
    node &operator = (node const &); // No assignment.
};
}
}
}

#endif

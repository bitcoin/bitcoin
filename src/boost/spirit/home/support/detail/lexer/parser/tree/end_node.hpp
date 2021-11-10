// end_node.hpp
// Copyright (c) 2007-2009 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_SPIRIT_SUPPORT_DETAIL_LEXER_PARSER_TREE_END_NODE_HPP
#define BOOST_SPIRIT_SUPPORT_DETAIL_LEXER_PARSER_TREE_END_NODE_HPP

#include "node.hpp"
#include "../../size_t.hpp"

namespace boost
{
namespace lexer
{
namespace detail
{
class end_node : public node
{
public:
    end_node (const std::size_t id_, const std::size_t unique_id_,
        const std::size_t lexer_state_) :
        node (false),
        _id (id_),
        _unique_id (unique_id_),
        _lexer_state (lexer_state_)
    {
        node::_firstpos.push_back (this);
        node::_lastpos.push_back (this);
    }

    virtual ~end_node ()
    {
    }

    virtual type what_type () const
    {
        return END;
    }

    virtual bool traverse (const_node_stack &/*node_stack_*/,
        bool_stack &/*perform_op_stack_*/) const
    {
        return false;
    }

    virtual const node_vector &followpos () const
    {
        // _followpos is always empty..!
        return _followpos;
    }

    virtual bool end_state () const
    {
        return true;
    }

    virtual std::size_t id () const
    {
        return _id;
    }

    virtual std::size_t unique_id () const
    {
        return _unique_id;
    }

    virtual std::size_t lexer_state () const
    {
        return _lexer_state;
    }

private:
    std::size_t _id;
    std::size_t _unique_id;
    std::size_t _lexer_state;
    node_vector _followpos;

    virtual void copy_node (node_ptr_vector &/*node_ptr_vector_*/,
        node_stack &/*new_node_stack_*/, bool_stack &/*perform_op_stack_*/,
        bool &/*down_*/) const
    {
        // Nothing to do, as end_nodes are not copied.
    }
};
}
}
}

#endif

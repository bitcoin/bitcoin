// parser.hpp
// Copyright (c) 2007-2009 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_SPIRIT_SUPPORT_DETAIL_LEXER_PARSER_PARSER_HPP
#define BOOST_SPIRIT_SUPPORT_DETAIL_LEXER_PARSER_PARSER_HPP

#include <boost/assert.hpp>
#include "tree/end_node.hpp"
#include "tree/iteration_node.hpp"
#include "tree/leaf_node.hpp"
#include "../runtime_error.hpp"
#include "tree/selection_node.hpp"
#include "tree/sequence_node.hpp"
#include "../size_t.hpp"
#include "tokeniser/re_tokeniser.hpp"

namespace boost
{
namespace lexer
{
namespace detail
{
template<typename CharT>
class basic_parser
{
public:
    typedef basic_re_tokeniser<CharT> tokeniser;
    typedef typename tokeniser::string string;
    typedef std::map<string, const node *> macro_map;
    typedef node::node_ptr_vector node_ptr_vector;
    typedef typename tokeniser::num_token token;

/*
    General principles of regex parsing:
    - Every regex is a sequence of sub-regexes.
    - Regexes consist of operands and operators
    - All operators decompose to sequence, selection ('|') and iteration ('*')
    - Regex tokens are stored on the stack.
    - When a complete sequence of regex tokens is on the stack it is processed.

Grammar:

<REGEX>      -> <OREXP>
<OREXP>      -> <SEQUENCE> | <OREXP>'|'<SEQUENCE>
<SEQUENCE>   -> <SUB>
<SUB>        -> <EXPRESSION> | <SUB><EXPRESSION>
<EXPRESSION> -> <REPEAT>
<REPEAT>     -> charset | macro | '('<REGEX>')' | <REPEAT><DUPLICATE>
<DUPLICATE>  -> '?' | '*' | '+' | '{n[,[m]]}'
*/
    static node *parse (const CharT *start_, const CharT * const end_,
        const std::size_t id_, const std::size_t unique_id_,
        const std::size_t dfa_state_, const regex_flags flags_,
        const std::locale &locale_, node_ptr_vector &node_ptr_vector_,
        const macro_map &macromap_, typename tokeniser::token_map &map_,
        bool &seen_BOL_assertion_, bool &seen_EOL_assertion_)
    {
        node *root_ = 0;
        state state_ (start_, end_, flags_, locale_);
        token lhs_token_;
        token rhs_token_;
        token_stack token_stack_;
        tree_node_stack tree_node_stack_;
        char action_ = 0;

        token_stack_.push (rhs_token_);
        tokeniser::next (state_, map_, rhs_token_);

        do
        {
            lhs_token_ = token_stack_.top ();
            action_ = lhs_token_.precedence (rhs_token_._type);

            switch (action_)
            {
            case '<':
            case '=':
                token_stack_.push (rhs_token_);
                tokeniser::next (state_, map_, rhs_token_);
                break;
            case '>':
                reduce (token_stack_, macromap_, node_ptr_vector_,
                    tree_node_stack_);
                break;
            default:
                std::ostringstream ss_;

                ss_ << "A syntax error occurred: '" <<
                    lhs_token_.precedence_string () <<
                    "' against '" << rhs_token_.precedence_string () <<
                    "' at index " << state_.index () << ".";
                throw runtime_error (ss_.str ().c_str ());
                break;
            }
        } while (!token_stack_.empty ());

        if (tree_node_stack_.empty ())
        {
            throw runtime_error ("Empty rules are not allowed.");
        }

        BOOST_ASSERT(tree_node_stack_.size () == 1);

        node *lhs_node_ = tree_node_stack_.top ();

        tree_node_stack_.pop ();

        if (id_ == 0)
        {
            // Macros have no end state...
            root_ = lhs_node_;
        }
        else
        {
            node_ptr_vector_->push_back (static_cast<end_node *>(0));

            node *rhs_node_ = new end_node (id_, unique_id_, dfa_state_);

            node_ptr_vector_->back () = rhs_node_;
            node_ptr_vector_->push_back (static_cast<sequence_node *>(0));
            node_ptr_vector_->back () = new sequence_node
                (lhs_node_, rhs_node_);
            root_ = node_ptr_vector_->back ();
        }

        // Done this way as bug in VC++ 6 prevents |= operator working
        // properly!
        if (state_._seen_BOL_assertion) seen_BOL_assertion_ = true;

        if (state_._seen_EOL_assertion) seen_EOL_assertion_ = true;

        return root_;
    }

private:
    typedef typename tokeniser::state state;
    typedef std::stack<token> token_stack;
    typedef node::node_stack tree_node_stack;

    static void reduce (token_stack &token_stack_,
        const macro_map &macromap_, node_ptr_vector &node_vector_ptr_,
        tree_node_stack &tree_node_stack_)
    {
        typename tokeniser::num_token lhs_;
        typename tokeniser::num_token rhs_;
        token_stack handle_;
        char action_ = 0;

        do
        {
            rhs_ = token_stack_.top ();
            token_stack_.pop ();
            handle_.push (rhs_);

            if (!token_stack_.empty ())
            {
                lhs_ = token_stack_.top ();
                action_ = lhs_.precedence (rhs_._type);
            }
        } while (!token_stack_.empty () && action_ == '=');

        BOOST_ASSERT(token_stack_.empty () || action_ == '<');

        switch (rhs_._type)
        {
        case token::BEGIN:
            // finished processing so exit
            break;
        case token::REGEX:
            // finished parsing, nothing to do
            break;
        case token::OREXP:
            orexp (handle_, token_stack_, node_vector_ptr_, tree_node_stack_);
            break;
        case token::SEQUENCE:
            token_stack_.push (token::OREXP);
            break;
        case token::SUB:
            sub (handle_, token_stack_, node_vector_ptr_, tree_node_stack_);
            break;
        case token::EXPRESSION:
            token_stack_.push (token::SUB);
            break;
        case token::REPEAT:
            repeat (handle_, token_stack_);
            break;
        case token::CHARSET:
            charset (handle_, token_stack_, node_vector_ptr_,
                tree_node_stack_);
            break;
        case token::MACRO:
            macro (handle_, token_stack_, macromap_, node_vector_ptr_,
                tree_node_stack_);
            break;
        case token::OPENPAREN:
            openparen (handle_, token_stack_);
            break;
        case token::OPT:
        case token::AOPT:
            optional (rhs_._type == token::OPT, node_vector_ptr_,
                tree_node_stack_);
            token_stack_.push (token::DUP);
            break;
        case token::ZEROORMORE:
        case token::AZEROORMORE:
            zero_or_more (rhs_._type == token::ZEROORMORE, node_vector_ptr_,
                tree_node_stack_);
            token_stack_.push (token::DUP);
            break;
        case token::ONEORMORE:
        case token::AONEORMORE:
            one_or_more (rhs_._type == token::ONEORMORE, node_vector_ptr_,
                tree_node_stack_);
            token_stack_.push (token::DUP);
            break;
        case token::REPEATN:
        case token::AREPEATN:
            repeatn (rhs_._type == token::REPEATN, handle_.top (),
                node_vector_ptr_, tree_node_stack_);
            token_stack_.push (token::DUP);
            break;
        default:
            throw runtime_error
                ("Internal error regex_parser::reduce");
            break;
        }
    }

    static void orexp (token_stack &handle_, token_stack &token_stack_,
        node_ptr_vector &node_ptr_vector_, tree_node_stack &tree_node_stack_)
    {
        BOOST_ASSERT(handle_.top ()._type == token::OREXP &&
            (handle_.size () == 1 || handle_.size () == 3));

        if (handle_.size () == 1)
        {
            token_stack_.push (token::REGEX);
        }
        else
        {
            handle_.pop ();
            BOOST_ASSERT(handle_.top ()._type == token::OR);
            handle_.pop ();
            BOOST_ASSERT(handle_.top ()._type == token::SEQUENCE);
            perform_or (node_ptr_vector_, tree_node_stack_);
            token_stack_.push (token::OREXP);
        }
    }

    static void sub (token_stack &handle_, token_stack &token_stack_,
        node_ptr_vector &node_ptr_vector_, tree_node_stack &tree_node_stack_)
    {
        BOOST_ASSERT(handle_.top ()._type == token::SUB &&
            (handle_.size () == 1 || handle_.size () == 2));

        if (handle_.size () == 1)
        {
            token_stack_.push (token::SEQUENCE);
        }
        else
        {
            handle_.pop ();
            BOOST_ASSERT(handle_.top ()._type == token::EXPRESSION);
            // perform join
            sequence (node_ptr_vector_, tree_node_stack_);
            token_stack_.push (token::SUB);
        }
    }

    static void repeat (token_stack &handle_, token_stack &token_stack_)
    {
        BOOST_ASSERT(handle_.top ()._type == token::REPEAT &&
            handle_.size () >= 1 && handle_.size () <= 3);

        if (handle_.size () == 1)
        {
            token_stack_.push (token::EXPRESSION);
        }
        else
        {
            handle_.pop ();
            BOOST_ASSERT(handle_.top ()._type == token::DUP);
            token_stack_.push (token::REPEAT);
        }
    }

    static void charset (token_stack &handle_, token_stack &token_stack_,
        node_ptr_vector &node_ptr_vector_, tree_node_stack &tree_node_stack_)
    {
        BOOST_ASSERT(handle_.top ()._type == token::CHARSET &&
            handle_.size () == 1);
        // store charset
        node_ptr_vector_->push_back (static_cast<leaf_node *>(0));

        const size_t id_ = handle_.top ()._id;

        node_ptr_vector_->back () = new leaf_node (id_, true);
        tree_node_stack_.push (node_ptr_vector_->back ());
        token_stack_.push (token::REPEAT);
    }

    static void macro (token_stack &handle_, token_stack &token_stack_,
        const macro_map &macromap_, node_ptr_vector &node_ptr_vector_,
        tree_node_stack &tree_node_stack_)
    {
        token &top_ = handle_.top ();

        BOOST_ASSERT(top_._type == token::MACRO && handle_.size () == 1);

        typename macro_map::const_iterator iter_ =
            macromap_.find (top_._macro);

        if (iter_ == macromap_.end ())
        {
            const CharT *name_ = top_._macro;
            std::basic_stringstream<CharT> ss_;
            std::ostringstream os_;

            os_ << "Unknown MACRO name '";

            while (*name_)
            {
                os_ << ss_.narrow (*name_++, ' ');
            }

            os_ << "'.";
            throw runtime_error (os_.str ());
        }

        tree_node_stack_.push (iter_->second->copy (node_ptr_vector_));
        token_stack_.push (token::REPEAT);
    }

    static void openparen (token_stack &handle_, token_stack &token_stack_)
    {
        BOOST_ASSERT(handle_.top ()._type == token::OPENPAREN &&
            handle_.size () == 3);
        handle_.pop ();
        BOOST_ASSERT(handle_.top ()._type == token::REGEX);
        handle_.pop ();
        BOOST_ASSERT(handle_.top ()._type == token::CLOSEPAREN);
        token_stack_.push (token::REPEAT);
    }

    static void perform_or (node_ptr_vector &node_ptr_vector_,
        tree_node_stack &tree_node_stack_)
    {
        // perform or
        node *rhs_ = tree_node_stack_.top ();

        tree_node_stack_.pop ();

        node *lhs_ = tree_node_stack_.top ();

        node_ptr_vector_->push_back (static_cast<selection_node *>(0));
        node_ptr_vector_->back () = new selection_node (lhs_, rhs_);
        tree_node_stack_.top () = node_ptr_vector_->back ();
    }

    static void sequence (node_ptr_vector &node_ptr_vector_,
        tree_node_stack &tree_node_stack_)
    {
        node *rhs_ = tree_node_stack_.top ();

        tree_node_stack_.pop ();

        node *lhs_ = tree_node_stack_.top ();

        node_ptr_vector_->push_back (static_cast<sequence_node *>(0));
        node_ptr_vector_->back () = new sequence_node (lhs_, rhs_);
        tree_node_stack_.top () = node_ptr_vector_->back ();
    }

    static void optional (const bool greedy_,
        node_ptr_vector &node_ptr_vector_, tree_node_stack &tree_node_stack_)
    {
        // perform ?
        node *lhs_ = tree_node_stack_.top ();
        // You don't know if lhs_ is a leaf_node, so get firstpos.
        node::node_vector &firstpos_ = lhs_->firstpos ();

        for (node::node_vector::iterator iter_ = firstpos_.begin (),
            end_ = firstpos_.end (); iter_ != end_; ++iter_)
        {
            // These are leaf_nodes!
            (*iter_)->greedy (greedy_);
        }

        node_ptr_vector_->push_back (static_cast<leaf_node *>(0));

        node *rhs_ = new leaf_node (null_token, greedy_);

        node_ptr_vector_->back () = rhs_;
        node_ptr_vector_->push_back (static_cast<selection_node *>(0));
        node_ptr_vector_->back () = new selection_node (lhs_, rhs_);
        tree_node_stack_.top () = node_ptr_vector_->back ();
    }

    static void zero_or_more (const bool greedy_,
        node_ptr_vector &node_ptr_vector_, tree_node_stack &tree_node_stack_)
    {
        // perform *
        node *ptr_ = tree_node_stack_.top ();

        node_ptr_vector_->push_back (static_cast<iteration_node *>(0));
        node_ptr_vector_->back () = new iteration_node (ptr_, greedy_);
        tree_node_stack_.top () = node_ptr_vector_->back ();
    }

    static void one_or_more (const bool greedy_,
        node_ptr_vector &node_ptr_vector_, tree_node_stack &tree_node_stack_)
    {
        // perform +
        node *lhs_ = tree_node_stack_.top ();
        node *copy_ = lhs_->copy (node_ptr_vector_);

        node_ptr_vector_->push_back (static_cast<iteration_node *>(0));

        node *rhs_ = new iteration_node (copy_, greedy_);

        node_ptr_vector_->back () = rhs_;
        node_ptr_vector_->push_back (static_cast<sequence_node *>(0));
        node_ptr_vector_->back () = new sequence_node (lhs_, rhs_);
        tree_node_stack_.top () = node_ptr_vector_->back ();
    }

    // This is one of the most mind bending routines in this code...
    static void repeatn (const bool greedy_, const token &token_,
        node_ptr_vector &node_ptr_vector_, tree_node_stack &tree_node_stack_)
    {
        // perform {n[,[m]]}
        // Semantic checks have already been performed.
        // {0,}  = *
        // {0,1} = ?
        // {1,}  = +
        // therefore we do not check for these cases.
        if (!(token_._min == 1 && !token_._comma))
        {
            const std::size_t top_ = token_._min > 0 ?
                token_._min : token_._max;

            if (token_._min == 0)
            {
                optional (greedy_, node_ptr_vector_, tree_node_stack_);
            }

            node *prev_ = tree_node_stack_.top ()->copy (node_ptr_vector_);
            node *curr_ = 0;

            for (std::size_t i_ = 2; i_ < top_; ++i_)
            {
                curr_ = prev_->copy (node_ptr_vector_);
                tree_node_stack_.push (static_cast<node *>(0));
                tree_node_stack_.top () = prev_;
                sequence (node_ptr_vector_, tree_node_stack_);
                prev_ = curr_;
            }

            if (token_._comma && token_._min > 0)
            {
                if (token_._min > 1)
                {
                    curr_ = prev_->copy (node_ptr_vector_);
                    tree_node_stack_.push (static_cast<node *>(0));
                    tree_node_stack_.top () = prev_;
                    sequence (node_ptr_vector_, tree_node_stack_);
                    prev_ = curr_;
                }

                if (token_._comma && token_._max)
                {
                    tree_node_stack_.push (static_cast<node *>(0));
                    tree_node_stack_.top () = prev_;
                    optional (greedy_, node_ptr_vector_, tree_node_stack_);
                    prev_ = tree_node_stack_.top ();
                    tree_node_stack_.pop ();

                    const std::size_t count_ = token_._max - token_._min;

                    for (std::size_t i_ = 1; i_ < count_; ++i_)
                    {
                        curr_ = prev_->copy (node_ptr_vector_);
                        tree_node_stack_.push (static_cast<node *>(0));
                        tree_node_stack_.top () = prev_;
                        sequence (node_ptr_vector_, tree_node_stack_);
                        prev_ = curr_;
                    }
                }
                else
                {
                    tree_node_stack_.push (static_cast<node *>(0));
                    tree_node_stack_.top () = prev_;
                    zero_or_more (greedy_, node_ptr_vector_, tree_node_stack_);
                    prev_ = tree_node_stack_.top ();
                    tree_node_stack_.pop ();
                }
            }

            tree_node_stack_.push (static_cast<node *>(0));
            tree_node_stack_.top () = prev_;
            sequence (node_ptr_vector_, tree_node_stack_);
        }
    }
};
}
}
}

#endif

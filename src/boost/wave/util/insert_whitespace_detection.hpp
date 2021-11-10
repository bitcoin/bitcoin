/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library

    Detect the need to insert a whitespace token into the output stream

    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_INSERT_WHITESPACE_DETECTION_HPP_765EF77B_0513_4967_BDD6_6A38148C4C96_INCLUDED)
#define BOOST_INSERT_WHITESPACE_DETECTION_HPP_765EF77B_0513_4967_BDD6_6A38148C4C96_INCLUDED

#include <boost/wave/wave_config.hpp>
#include <boost/wave/token_ids.hpp>

// this must occur after all of the includes and before any code appears
#ifdef BOOST_HAS_ABI_HEADERS
#include BOOST_ABI_PREFIX
#endif

///////////////////////////////////////////////////////////////////////////////
namespace boost {
namespace wave {
namespace util {

namespace impl {

// T_IDENTIFIER
    template <typename StringT>
    inline bool
    would_form_universal_char (StringT const &value)
    {
        if ('u' != value[0] && 'U' != value[0])
            return false;
        if ('u' == value[0] && value.size() < 5)
            return false;
        if ('U' == value[0] && value.size() < 9)
            return false;

    typename StringT::size_type pos =
        value.find_first_not_of("0123456789abcdefABCDEF", 1);

        if (StringT::npos == pos ||
            ('u' == value[0] && pos > 5) ||
            ('U' == value[0] && pos > 9))
        {
            return true;        // would form an universal char
        }
        return false;
    }
    template <typename StringT>
    inline bool
    handle_identifier(boost::wave::token_id prev,
        boost::wave::token_id before, StringT const &value)
    {
        using namespace boost::wave;
        switch (prev) {
        case T_IDENTIFIER:
        case T_NONREPLACABLE_IDENTIFIER:
        case T_COMPL_ALT:
        case T_OR_ALT:
        case T_AND_ALT:
        case T_NOT_ALT:
        case T_XOR_ALT:
        case T_ANDASSIGN_ALT:
        case T_ORASSIGN_ALT:
        case T_XORASSIGN_ALT:
        case T_NOTEQUAL_ALT:
        case T_FIXEDPOINTLIT:
            return true;

        case T_FLOATLIT:
        case T_INTLIT:
        case T_PP_NUMBER:
            return (value.size() > 1 || (value[0] != 'e' && value[0] != 'E'));

         // avoid constructing universal characters (\u1234)
        case T_UNKNOWN_UNIVERSALCHAR:
            return would_form_universal_char(value);

        default:
            break;
        }
        return false;
    }
// T_INTLIT
    inline bool
    handle_intlit(boost::wave::token_id prev, boost::wave::token_id /*before*/)
    {
        using namespace boost::wave;
        switch (prev) {
        case T_IDENTIFIER:
        case T_NONREPLACABLE_IDENTIFIER:
        case T_INTLIT:
        case T_FLOATLIT:
        case T_FIXEDPOINTLIT:
        case T_PP_NUMBER:
            return true;

        default:
            break;
        }
        return false;
    }
// T_FLOATLIT
    inline bool
    handle_floatlit(boost::wave::token_id prev,
        boost::wave::token_id /*before*/)
    {
        using namespace boost::wave;
        switch (prev) {
        case T_IDENTIFIER:
        case T_NONREPLACABLE_IDENTIFIER:
        case T_INTLIT:
        case T_FLOATLIT:
        case T_FIXEDPOINTLIT:
        case T_PP_NUMBER:
            return true;

        default:
            break;
        }
        return false;
    }
// <% T_LEFTBRACE
    inline bool
    handle_alt_leftbrace(boost::wave::token_id prev,
        boost::wave::token_id /*before*/)
    {
        using namespace boost::wave;
        switch (prev) {
        case T_LESS:        // <<%
        case T_SHIFTLEFT:   // <<<%
            return true;

        default:
            break;
        }
        return false;
    }
// <: T_LEFTBRACKET
    inline bool
    handle_alt_leftbracket(boost::wave::token_id prev,
        boost::wave::token_id /*before*/)
    {
        using namespace boost::wave;
        switch (prev) {
        case T_LESS:        // <<:
        case T_SHIFTLEFT:   // <<<:
            return true;

        default:
            break;
        }
        return false;
    }
// T_FIXEDPOINTLIT
    inline bool
    handle_fixedpointlit(boost::wave::token_id prev,
        boost::wave::token_id /*before*/)
    {
        using namespace boost::wave;
        switch (prev) {
        case T_IDENTIFIER:
        case T_NONREPLACABLE_IDENTIFIER:
        case T_INTLIT:
        case T_FLOATLIT:
        case T_FIXEDPOINTLIT:
        case T_PP_NUMBER:
            return true;

        default:
            break;
        }
        return false;
    }
// T_DOT
    inline bool
    handle_dot(boost::wave::token_id prev, boost::wave::token_id before)
    {
        using namespace boost::wave;
        switch (prev) {
        case T_DOT:
            if (T_DOT == before)
                return true;    // ...
            break;

        default:
            break;
        }
        return false;
    }
// T_QUESTION_MARK
    inline bool
    handle_questionmark(boost::wave::token_id prev,
        boost::wave::token_id /*before*/)
    {
        using namespace boost::wave;
        switch(prev) {
        case T_UNKNOWN_UNIVERSALCHAR:     // \?
        case T_QUESTION_MARK:   // ??
            return true;

        default:
            break;
        }
        return false;
    }
// T_NEWLINE
    inline bool
    handle_newline(boost::wave::token_id prev,
        boost::wave::token_id before)
    {
        using namespace boost::wave;
        switch(prev) {
        case TOKEN_FROM_ID('\\', UnknownTokenType): // \ \n
        case T_DIVIDE:
            if (T_QUESTION_MARK == before)
                return true;    // ?/\n     // may be \\n
            break;

        default:
            break;
        }
        return false;
    }

    inline bool
    handle_parens(boost::wave::token_id prev)
    {
        switch (prev) {
        case T_LEFTPAREN:
        case T_RIGHTPAREN:
        case T_LEFTBRACKET:
        case T_RIGHTBRACKET:
        case T_LEFTBRACE:
        case T_RIGHTBRACE:
        case T_SEMICOLON:
        case T_COMMA:
        case T_COLON:
            // no insertion between parens/brackets/braces and operators
            return false;

        default:
            break;
        }
        return true;
    }

}   // namespace impl

class insert_whitespace_detection
{
public:
    insert_whitespace_detection(bool insert_whitespace_ = true)
    :   insert_whitespace(insert_whitespace_),
        prev(boost::wave::T_EOF), beforeprev(boost::wave::T_EOF)
    {}

    template <typename StringT>
    bool must_insert(boost::wave::token_id current, StringT const &value)
    {
        if (!insert_whitespace)
            return false;       // skip whitespace insertion alltogether

        using namespace boost::wave;
        switch (current) {
        case T_NONREPLACABLE_IDENTIFIER:
        case T_IDENTIFIER:
            return impl::handle_identifier(prev, beforeprev, value);
        case T_PP_NUMBER:
        case T_INTLIT:
            return impl::handle_intlit(prev, beforeprev);
        case T_FLOATLIT:
            return impl::handle_floatlit(prev, beforeprev);
        case T_STRINGLIT:
            if (TOKEN_FROM_ID('L', IdentifierTokenType) == prev)       // 'L'
                return true;
            break;
        case T_LEFTBRACE_ALT:
            return impl::handle_alt_leftbrace(prev, beforeprev);
        case T_LEFTBRACKET_ALT:
            return impl::handle_alt_leftbracket(prev, beforeprev);
        case T_FIXEDPOINTLIT:
            return impl::handle_fixedpointlit(prev, beforeprev);
        case T_DOT:
            return impl::handle_dot(prev, beforeprev);
        case T_QUESTION_MARK:
            return impl::handle_questionmark(prev, beforeprev);
        case T_NEWLINE:
            return impl::handle_newline(prev, beforeprev);

        case T_LEFTPAREN:
        case T_RIGHTPAREN:
        case T_LEFTBRACKET:
        case T_RIGHTBRACKET:
        case T_SEMICOLON:
        case T_COMMA:
        case T_COLON:
            switch (prev) {
            case T_LEFTPAREN:
            case T_RIGHTPAREN:
            case T_LEFTBRACKET:
            case T_RIGHTBRACKET:
            case T_LEFTBRACE:
            case T_RIGHTBRACE:
                return false;   // no insertion between parens/brackets/braces

            default:
                if (IS_CATEGORY(prev, OperatorTokenType))
                    return false;
                break;
            }
            break;

        case T_LEFTBRACE:
        case T_RIGHTBRACE:
            switch (prev) {
            case T_LEFTPAREN:
            case T_RIGHTPAREN:
            case T_LEFTBRACKET:
            case T_RIGHTBRACKET:
            case T_LEFTBRACE:
            case T_RIGHTBRACE:
            case T_SEMICOLON:
            case T_COMMA:
            case T_COLON:
                return false;   // no insertion between parens/brackets/braces

            case T_QUESTION_MARK:
                if (T_QUESTION_MARK == beforeprev)
                    return true;
                if (IS_CATEGORY(prev, OperatorTokenType))
                    return false;
                break;

            default:
                break;
            }
            break;

        case T_MINUS:
        case T_MINUSMINUS:
        case T_MINUSASSIGN:
            if (T_MINUS == prev || T_MINUSMINUS == prev)
                return true;
            if (!impl::handle_parens(prev))
                return false;
            if (T_QUESTION_MARK == prev && T_QUESTION_MARK == beforeprev)
                return true;
            break;

        case T_PLUS:
        case T_PLUSPLUS:
        case T_PLUSASSIGN:
            if (T_PLUS == prev || T_PLUSPLUS == prev)
                return true;
            if (!impl::handle_parens(prev))
                return false;
            if (T_QUESTION_MARK == prev && T_QUESTION_MARK == beforeprev)
                return true;
            break;

        case T_DIVIDE:
        case T_DIVIDEASSIGN:
            if (T_DIVIDE == prev)
                return true;
            if (!impl::handle_parens(prev))
                return false;
            if (T_QUESTION_MARK == prev && T_QUESTION_MARK == beforeprev)
                return true;
            break;

        case T_EQUAL:
        case T_ASSIGN:
            switch (prev) {
            case T_PLUSASSIGN:
            case T_MINUSASSIGN:
            case T_DIVIDEASSIGN:
            case T_STARASSIGN:
            case T_SHIFTRIGHTASSIGN:
            case T_SHIFTLEFTASSIGN:
            case T_EQUAL:
            case T_NOTEQUAL:
            case T_LESSEQUAL:
            case T_GREATEREQUAL:
            case T_LESS:
            case T_GREATER:
            case T_PLUS:
            case T_MINUS:
            case T_STAR:
            case T_DIVIDE:
            case T_ORASSIGN:
            case T_ANDASSIGN:
            case T_XORASSIGN:
            case T_OR:
            case T_AND:
            case T_XOR:
            case T_OROR:
            case T_ANDAND:
                return true;

            case T_QUESTION_MARK:
                if (T_QUESTION_MARK == beforeprev)
                    return true;
                break;

            default:
                if (!impl::handle_parens(prev))
                    return false;
                break;
            }
            break;

        case T_GREATER:
            if (T_MINUS == prev || T_GREATER == prev)
                return true;    // prevent -> or >>
            if (!impl::handle_parens(prev))
                return false;
            if (T_QUESTION_MARK == prev && T_QUESTION_MARK == beforeprev)
                return true;
            break;

        case T_LESS:
            if (T_LESS == prev)
                return true;    // prevent <<
            // fall through
        case T_CHARLIT:
        case T_NOT:
        case T_NOTEQUAL:
            if (!impl::handle_parens(prev))
                return false;
            if (T_QUESTION_MARK == prev && T_QUESTION_MARK == beforeprev)
                return true;
            break;

        case T_AND:
        case T_ANDAND:
            if (!impl::handle_parens(prev))
                return false;
            if (T_AND == prev || T_ANDAND == prev)
                return true;
            break;

        case T_OR:
            if (!impl::handle_parens(prev))
                return false;
            if (T_OR == prev)
                return true;
            break;

        case T_XOR:
            if (!impl::handle_parens(prev))
                return false;
            if (T_XOR == prev)
                return true;
            break;

        case T_COMPL_ALT:
        case T_OR_ALT:
        case T_AND_ALT:
        case T_NOT_ALT:
        case T_XOR_ALT:
        case T_ANDASSIGN_ALT:
        case T_ORASSIGN_ALT:
        case T_XORASSIGN_ALT:
        case T_NOTEQUAL_ALT:
            switch (prev) {
            case T_LEFTPAREN:
            case T_RIGHTPAREN:
            case T_LEFTBRACKET:
            case T_RIGHTBRACKET:
            case T_LEFTBRACE:
            case T_RIGHTBRACE:
            case T_SEMICOLON:
            case T_COMMA:
            case T_COLON:
                // no insertion between parens/brackets/braces and operators
                return false;

            case T_IDENTIFIER:
                if (T_NONREPLACABLE_IDENTIFIER == prev ||
                    IS_CATEGORY(prev, KeywordTokenType))
                {
                    return true;
                }
                break;

            default:
                break;
            }
            break;

        case T_STAR:
            if (T_STAR == prev)
                return false;     // '*****' do not need to be separated
            if (T_GREATER== prev &&
                (T_MINUS == beforeprev || T_MINUSMINUS == beforeprev)
               )
            {
                return true;    // prevent ->*
            }
            break;

        case T_POUND:
            if (T_POUND == prev)
                return true;
            break;

        default:
            break;
        }

    // FIXME: else, handle operators separately (will catch to many cases)
//         if (IS_CATEGORY(current, OperatorTokenType) &&
//             IS_CATEGORY(prev, OperatorTokenType))
//         {
//             return true;    // operators must be delimited always
//         }
        return false;
    }
    void shift_tokens (boost::wave::token_id next_id)
    {
        if (insert_whitespace) {
            beforeprev = prev;
            prev = next_id;
        }
    }

private:
    bool insert_whitespace;            // enable this component
    boost::wave::token_id prev;        // the previous analyzed token
    boost::wave::token_id beforeprev;  // the token before the previous
};

///////////////////////////////////////////////////////////////////////////////
}   //  namespace util
}   //  namespace wave
}   //  namespace boost

// the suffix header occurs after all of the code
#ifdef BOOST_HAS_ABI_HEADERS
#include BOOST_ABI_SUFFIX
#endif

#endif // !defined(BOOST_INSERT_WHITESPACE_DETECTION_HPP_765EF77B_0513_4967_BDD6_6A38148C4C96_INCLUDED)

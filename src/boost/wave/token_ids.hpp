/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library
    The definition of a default set of token identifiers and related
    functions.

    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#if !defined(BOOST_TOKEN_IDS_HPP_414E9A58_F079_4789_8AFF_513815CE475B_INCLUDED)
#define BOOST_TOKEN_IDS_HPP_414E9A58_F079_4789_8AFF_513815CE475B_INCLUDED

#include <string>

#include <boost/wave/wave_config.hpp>

// this must occur after all of the includes and before any code appears
#ifdef BOOST_HAS_ABI_HEADERS
#include BOOST_ABI_PREFIX
#endif

///////////////////////////////////////////////////////////////////////////////
//  Allow external redefinition of the token identifiers to use
#if !defined(BOOST_WAVE_TOKEN_IDS_DEFINED)
#define BOOST_WAVE_TOKEN_IDS_DEFINED

#if (defined (__FreeBSD__) || defined (__DragonFly__) || defined (__OpenBSD__)) && defined (T_DIVIDE)
#undef T_DIVIDE
#endif

///////////////////////////////////////////////////////////////////////////////
namespace boost {
namespace wave {

///////////////////////////////////////////////////////////////////////////////
//  assemble tokenids
#define TOKEN_FROM_ID(id, cat)   ((id) | (cat))
#define ID_FROM_TOKEN(tok)       ((tok) & ~TokenTypeMask)
#define BASEID_FROM_TOKEN(tok)   ((tok) & ~ExtTokenTypeMask)

///////////////////////////////////////////////////////////////////////////////
//  the token_category helps to classify the different token types
enum token_category {
    IdentifierTokenType         = 0x08040000,
    ParameterTokenType          = 0x08840000,
    ExtParameterTokenType       = 0x088C0000,
    OptParameterTokenType       = 0x08940000,
    KeywordTokenType            = 0x10040000,
    OperatorTokenType           = 0x18040000,
    LiteralTokenType            = 0x20040000,
    IntegerLiteralTokenType     = 0x20840000,
    FloatingLiteralTokenType    = 0x21040000,
    StringLiteralTokenType      = 0x21840000,
    CharacterLiteralTokenType   = 0x22040000,
    BoolLiteralTokenType        = 0x22840000,
    PPTokenType                 = 0x28040000,
    PPConditionalTokenType      = 0x28440000,

    UnknownTokenType            = 0x50000000,
    EOLTokenType                = 0x58000000,
    EOFTokenType                = 0x60000000,
    WhiteSpaceTokenType         = 0x68000000,
    InternalTokenType           = 0x70040000,

    TokenTypeMask               = 0x7F800000,
    AltTokenType                = 0x00080000,
    TriGraphTokenType           = 0x00100000,
    AltExtTokenType             = 0x00280000,   // and, bit_and etc.
    PPTokenFlag                 = 0x00040000,   // these are 'real' pp-tokens
    ExtTokenTypeMask            = 0x7FF80000,
    ExtTokenOnlyMask            = 0x00780000,
    TokenValueMask              = 0x0007FFFF,
    MainTokenMask               = 0x7F87FFFF    // TokenTypeMask|TokenValueMask
};

///////////////////////////////////////////////////////////////////////////////
//  the token_id assigns unique numbers to the different C++ lexemes
enum token_id {
    T_UNKNOWN      = 0,
    T_FIRST_TOKEN  = 256,
    T_AND          = TOKEN_FROM_ID(T_FIRST_TOKEN, OperatorTokenType),
    T_AND_ALT      = TOKEN_FROM_ID(T_FIRST_TOKEN, OperatorTokenType|AltExtTokenType),
    T_ANDAND       = TOKEN_FROM_ID(257, OperatorTokenType),
    T_ANDAND_ALT   = TOKEN_FROM_ID(257, OperatorTokenType|AltExtTokenType),
    T_ASSIGN       = TOKEN_FROM_ID(258, OperatorTokenType),
    T_ANDASSIGN    = TOKEN_FROM_ID(259, OperatorTokenType),
    T_ANDASSIGN_ALT     = TOKEN_FROM_ID(259, OperatorTokenType|AltExtTokenType),
    T_OR           = TOKEN_FROM_ID(260, OperatorTokenType),
    T_OR_ALT       = TOKEN_FROM_ID(260, OperatorTokenType|AltExtTokenType),
    T_OR_TRIGRAPH  = TOKEN_FROM_ID(260, OperatorTokenType|TriGraphTokenType),
    T_ORASSIGN     = TOKEN_FROM_ID(261, OperatorTokenType),
    T_ORASSIGN_ALT          = TOKEN_FROM_ID(261, OperatorTokenType|AltExtTokenType),
    T_ORASSIGN_TRIGRAPH     = TOKEN_FROM_ID(261, OperatorTokenType|TriGraphTokenType),
    T_XOR          = TOKEN_FROM_ID(262, OperatorTokenType),
    T_XOR_ALT      = TOKEN_FROM_ID(262, OperatorTokenType|AltExtTokenType),
    T_XOR_TRIGRAPH = TOKEN_FROM_ID(262, OperatorTokenType|TriGraphTokenType),
    T_XORASSIGN    = TOKEN_FROM_ID(263, OperatorTokenType),
    T_XORASSIGN_ALT         = TOKEN_FROM_ID(263, OperatorTokenType|AltExtTokenType),
    T_XORASSIGN_TRIGRAPH    = TOKEN_FROM_ID(263, OperatorTokenType|TriGraphTokenType),
    T_COMMA        = TOKEN_FROM_ID(264, OperatorTokenType),
    T_COLON        = TOKEN_FROM_ID(265, OperatorTokenType),
    T_DIVIDE       = TOKEN_FROM_ID(266, OperatorTokenType),
    T_DIVIDEASSIGN = TOKEN_FROM_ID(267, OperatorTokenType),
    T_DOT          = TOKEN_FROM_ID(268, OperatorTokenType),
    T_DOTSTAR      = TOKEN_FROM_ID(269, OperatorTokenType),
    T_ELLIPSIS     = TOKEN_FROM_ID(270, OperatorTokenType),
    T_EQUAL        = TOKEN_FROM_ID(271, OperatorTokenType),
    T_GREATER      = TOKEN_FROM_ID(272, OperatorTokenType),
    T_GREATEREQUAL = TOKEN_FROM_ID(273, OperatorTokenType),
    T_LEFTBRACE    = TOKEN_FROM_ID(274, OperatorTokenType),
    T_LEFTBRACE_ALT         = TOKEN_FROM_ID(274, OperatorTokenType|AltTokenType),
    T_LEFTBRACE_TRIGRAPH    = TOKEN_FROM_ID(274, OperatorTokenType|TriGraphTokenType),
    T_LESS         = TOKEN_FROM_ID(275, OperatorTokenType),
    T_LESSEQUAL    = TOKEN_FROM_ID(276, OperatorTokenType),
    T_LEFTPAREN    = TOKEN_FROM_ID(277, OperatorTokenType),
    T_LEFTBRACKET  = TOKEN_FROM_ID(278, OperatorTokenType),
    T_LEFTBRACKET_ALT       = TOKEN_FROM_ID(278, OperatorTokenType|AltTokenType),
    T_LEFTBRACKET_TRIGRAPH  = TOKEN_FROM_ID(278, OperatorTokenType|TriGraphTokenType),
    T_MINUS        = TOKEN_FROM_ID(279, OperatorTokenType),
    T_MINUSASSIGN  = TOKEN_FROM_ID(280, OperatorTokenType),
    T_MINUSMINUS   = TOKEN_FROM_ID(281, OperatorTokenType),
    T_PERCENT      = TOKEN_FROM_ID(282, OperatorTokenType),
    T_PERCENTASSIGN = TOKEN_FROM_ID(283, OperatorTokenType),
    T_NOT          = TOKEN_FROM_ID(284, OperatorTokenType),
    T_NOT_ALT      = TOKEN_FROM_ID(284, OperatorTokenType|AltExtTokenType),
    T_NOTEQUAL     = TOKEN_FROM_ID(285, OperatorTokenType),
    T_NOTEQUAL_ALT      = TOKEN_FROM_ID(285, OperatorTokenType|AltExtTokenType),
    T_OROR         = TOKEN_FROM_ID(286, OperatorTokenType),
    T_OROR_ALT     = TOKEN_FROM_ID(286, OperatorTokenType|AltExtTokenType),
    T_OROR_TRIGRAPH     = TOKEN_FROM_ID(286, OperatorTokenType|TriGraphTokenType),
    T_PLUS         = TOKEN_FROM_ID(287, OperatorTokenType),
    T_PLUSASSIGN   = TOKEN_FROM_ID(288, OperatorTokenType),
    T_PLUSPLUS     = TOKEN_FROM_ID(289, OperatorTokenType),
    T_ARROW        = TOKEN_FROM_ID(290, OperatorTokenType),
    T_ARROWSTAR    = TOKEN_FROM_ID(291, OperatorTokenType),
    T_QUESTION_MARK = TOKEN_FROM_ID(292, OperatorTokenType),
    T_RIGHTBRACE   = TOKEN_FROM_ID(293, OperatorTokenType),
    T_RIGHTBRACE_ALT        = TOKEN_FROM_ID(293, OperatorTokenType|AltTokenType),
    T_RIGHTBRACE_TRIGRAPH   = TOKEN_FROM_ID(293, OperatorTokenType|TriGraphTokenType),
    T_RIGHTPAREN   = TOKEN_FROM_ID(294, OperatorTokenType),
    T_RIGHTBRACKET = TOKEN_FROM_ID(295, OperatorTokenType),
    T_RIGHTBRACKET_ALT      = TOKEN_FROM_ID(295, OperatorTokenType|AltTokenType),
    T_RIGHTBRACKET_TRIGRAPH = TOKEN_FROM_ID(295, OperatorTokenType|TriGraphTokenType),
    T_COLON_COLON  = TOKEN_FROM_ID(296, OperatorTokenType),
    T_SEMICOLON    = TOKEN_FROM_ID(297, OperatorTokenType),
    T_SHIFTLEFT    = TOKEN_FROM_ID(298, OperatorTokenType),
    T_SHIFTLEFTASSIGN = TOKEN_FROM_ID(299, OperatorTokenType),
    T_SHIFTRIGHT   = TOKEN_FROM_ID(300, OperatorTokenType),
    T_SHIFTRIGHTASSIGN = TOKEN_FROM_ID(301, OperatorTokenType),
    T_STAR         = TOKEN_FROM_ID(302, OperatorTokenType),
    T_COMPL        = TOKEN_FROM_ID(303, OperatorTokenType),
    T_COMPL_ALT         = TOKEN_FROM_ID(303, OperatorTokenType|AltExtTokenType),
    T_COMPL_TRIGRAPH    = TOKEN_FROM_ID(303, OperatorTokenType|TriGraphTokenType),
    T_STARASSIGN   = TOKEN_FROM_ID(304, OperatorTokenType),
    T_ASM          = TOKEN_FROM_ID(305, KeywordTokenType),
    T_AUTO         = TOKEN_FROM_ID(306, KeywordTokenType),
    T_BOOL         = TOKEN_FROM_ID(307, KeywordTokenType),
    T_FALSE        = TOKEN_FROM_ID(308, BoolLiteralTokenType),
    T_TRUE         = TOKEN_FROM_ID(309, BoolLiteralTokenType),
    T_BREAK        = TOKEN_FROM_ID(310, KeywordTokenType),
    T_CASE         = TOKEN_FROM_ID(311, KeywordTokenType),
    T_CATCH        = TOKEN_FROM_ID(312, KeywordTokenType),
    T_CHAR         = TOKEN_FROM_ID(313, KeywordTokenType),
    T_CLASS        = TOKEN_FROM_ID(314, KeywordTokenType),
    T_CONST        = TOKEN_FROM_ID(315, KeywordTokenType),
    T_CONSTCAST    = TOKEN_FROM_ID(316, KeywordTokenType),
    T_CONTINUE     = TOKEN_FROM_ID(317, KeywordTokenType),
    T_DEFAULT      = TOKEN_FROM_ID(318, KeywordTokenType),
    T_DELETE       = TOKEN_FROM_ID(319, KeywordTokenType),
    T_DO           = TOKEN_FROM_ID(320, KeywordTokenType),
    T_DOUBLE       = TOKEN_FROM_ID(321, KeywordTokenType),
    T_DYNAMICCAST  = TOKEN_FROM_ID(322, KeywordTokenType),
    T_ELSE         = TOKEN_FROM_ID(323, KeywordTokenType),
    T_ENUM         = TOKEN_FROM_ID(324, KeywordTokenType),
    T_EXPLICIT     = TOKEN_FROM_ID(325, KeywordTokenType),
    T_EXPORT       = TOKEN_FROM_ID(326, KeywordTokenType),
    T_EXTERN       = TOKEN_FROM_ID(327, KeywordTokenType),
    T_FLOAT        = TOKEN_FROM_ID(328, KeywordTokenType),
    T_FOR          = TOKEN_FROM_ID(329, KeywordTokenType),
    T_FRIEND       = TOKEN_FROM_ID(330, KeywordTokenType),
    T_GOTO         = TOKEN_FROM_ID(331, KeywordTokenType),
    T_IF           = TOKEN_FROM_ID(332, KeywordTokenType),
    T_INLINE       = TOKEN_FROM_ID(333, KeywordTokenType),
    T_INT          = TOKEN_FROM_ID(334, KeywordTokenType),
    T_LONG         = TOKEN_FROM_ID(335, KeywordTokenType),
    T_MUTABLE      = TOKEN_FROM_ID(336, KeywordTokenType),
    T_NAMESPACE    = TOKEN_FROM_ID(337, KeywordTokenType),
    T_NEW          = TOKEN_FROM_ID(338, KeywordTokenType),
    T_OPERATOR     = TOKEN_FROM_ID(339, KeywordTokenType),
    T_PRIVATE      = TOKEN_FROM_ID(340, KeywordTokenType),
    T_PROTECTED    = TOKEN_FROM_ID(341, KeywordTokenType),
    T_PUBLIC       = TOKEN_FROM_ID(342, KeywordTokenType),
    T_REGISTER     = TOKEN_FROM_ID(343, KeywordTokenType),
    T_REINTERPRETCAST = TOKEN_FROM_ID(344, KeywordTokenType),
    T_RETURN       = TOKEN_FROM_ID(345, KeywordTokenType),
    T_SHORT        = TOKEN_FROM_ID(346, KeywordTokenType),
    T_SIGNED       = TOKEN_FROM_ID(347, KeywordTokenType),
    T_SIZEOF       = TOKEN_FROM_ID(348, KeywordTokenType),
    T_STATIC       = TOKEN_FROM_ID(349, KeywordTokenType),
    T_STATICCAST   = TOKEN_FROM_ID(350, KeywordTokenType),
    T_STRUCT       = TOKEN_FROM_ID(351, KeywordTokenType),
    T_SWITCH       = TOKEN_FROM_ID(352, KeywordTokenType),
    T_TEMPLATE     = TOKEN_FROM_ID(353, KeywordTokenType),
    T_THIS         = TOKEN_FROM_ID(354, KeywordTokenType),
    T_THROW        = TOKEN_FROM_ID(355, KeywordTokenType),
    T_TRY          = TOKEN_FROM_ID(356, KeywordTokenType),
    T_TYPEDEF      = TOKEN_FROM_ID(357, KeywordTokenType),
    T_TYPEID       = TOKEN_FROM_ID(358, KeywordTokenType),
    T_TYPENAME     = TOKEN_FROM_ID(359, KeywordTokenType),
    T_UNION        = TOKEN_FROM_ID(360, KeywordTokenType),
    T_UNSIGNED     = TOKEN_FROM_ID(361, KeywordTokenType),
    T_USING        = TOKEN_FROM_ID(362, KeywordTokenType),
    T_VIRTUAL      = TOKEN_FROM_ID(363, KeywordTokenType),
    T_VOID         = TOKEN_FROM_ID(364, KeywordTokenType),
    T_VOLATILE     = TOKEN_FROM_ID(365, KeywordTokenType),
    T_WCHART       = TOKEN_FROM_ID(366, KeywordTokenType),
    T_WHILE        = TOKEN_FROM_ID(367, KeywordTokenType),
    T_PP_DEFINE    = TOKEN_FROM_ID(368, PPTokenType),
    T_PP_IF        = TOKEN_FROM_ID(369, PPConditionalTokenType),
    T_PP_IFDEF     = TOKEN_FROM_ID(370, PPConditionalTokenType),
    T_PP_IFNDEF    = TOKEN_FROM_ID(371, PPConditionalTokenType),
    T_PP_ELSE      = TOKEN_FROM_ID(372, PPConditionalTokenType),
    T_PP_ELIF      = TOKEN_FROM_ID(373, PPConditionalTokenType),
    T_PP_ENDIF     = TOKEN_FROM_ID(374, PPConditionalTokenType),
    T_PP_ERROR     = TOKEN_FROM_ID(375, PPTokenType),
    T_PP_LINE      = TOKEN_FROM_ID(376, PPTokenType),
    T_PP_PRAGMA    = TOKEN_FROM_ID(377, PPTokenType),
    T_PP_UNDEF     = TOKEN_FROM_ID(378, PPTokenType),
    T_PP_WARNING   = TOKEN_FROM_ID(379, PPTokenType),
    T_IDENTIFIER   = TOKEN_FROM_ID(380, IdentifierTokenType),
    T_OCTALINT     = TOKEN_FROM_ID(381, IntegerLiteralTokenType),
    T_DECIMALINT   = TOKEN_FROM_ID(382, IntegerLiteralTokenType),
    T_HEXAINT      = TOKEN_FROM_ID(383, IntegerLiteralTokenType),
    T_INTLIT       = TOKEN_FROM_ID(384, IntegerLiteralTokenType),
    T_LONGINTLIT   = TOKEN_FROM_ID(385, IntegerLiteralTokenType),
    T_FLOATLIT     = TOKEN_FROM_ID(386, FloatingLiteralTokenType),
    T_FIXEDPOINTLIT = TOKEN_FROM_ID(386, FloatingLiteralTokenType|AltTokenType),  // IDL specific
    T_CCOMMENT     = TOKEN_FROM_ID(387, WhiteSpaceTokenType|AltTokenType),
    T_CPPCOMMENT   = TOKEN_FROM_ID(388, WhiteSpaceTokenType|AltTokenType),
    T_CHARLIT      = TOKEN_FROM_ID(389, CharacterLiteralTokenType),
    T_STRINGLIT    = TOKEN_FROM_ID(390, StringLiteralTokenType),
    T_CONTLINE     = TOKEN_FROM_ID(391, EOLTokenType),
    T_SPACE        = TOKEN_FROM_ID(392, WhiteSpaceTokenType),
    T_SPACE2       = TOKEN_FROM_ID(393, WhiteSpaceTokenType),
    T_NEWLINE      = TOKEN_FROM_ID(394, EOLTokenType),
    T_GENERATEDNEWLINE      = TOKEN_FROM_ID(394, EOLTokenType|AltTokenType),
    T_POUND_POUND           = TOKEN_FROM_ID(395, OperatorTokenType),
    T_POUND_POUND_ALT       = TOKEN_FROM_ID(395, OperatorTokenType|AltTokenType),
    T_POUND_POUND_TRIGRAPH  = TOKEN_FROM_ID(395, OperatorTokenType|TriGraphTokenType),
    T_POUND                 = TOKEN_FROM_ID(396, OperatorTokenType),
    T_POUND_ALT             = TOKEN_FROM_ID(396, OperatorTokenType|AltTokenType),
    T_POUND_TRIGRAPH        = TOKEN_FROM_ID(396, OperatorTokenType|TriGraphTokenType),
    T_ANY          = TOKEN_FROM_ID(397, UnknownTokenType),
    T_ANY_TRIGRAPH = TOKEN_FROM_ID(397, UnknownTokenType|TriGraphTokenType),
    T_PP_INCLUDE   = TOKEN_FROM_ID(398, PPTokenType),
    T_PP_QHEADER   = TOKEN_FROM_ID(399, PPTokenType),
    T_PP_HHEADER   = TOKEN_FROM_ID(400, PPTokenType),
    T_PP_INCLUDE_NEXT   = TOKEN_FROM_ID(398, PPTokenType|AltTokenType),
    T_PP_QHEADER_NEXT   = TOKEN_FROM_ID(399, PPTokenType|AltTokenType),
    T_PP_HHEADER_NEXT   = TOKEN_FROM_ID(400, PPTokenType|AltTokenType),
    T_EOF          = TOKEN_FROM_ID(401, EOFTokenType),      // end of file reached
    T_EOI          = TOKEN_FROM_ID(402, EOFTokenType),      // end of input reached
    T_PP_NUMBER    = TOKEN_FROM_ID(403, InternalTokenType),

// MS extensions
    T_MSEXT_INT8   = TOKEN_FROM_ID(404, KeywordTokenType),
    T_MSEXT_INT16  = TOKEN_FROM_ID(405, KeywordTokenType),
    T_MSEXT_INT32  = TOKEN_FROM_ID(406, KeywordTokenType),
    T_MSEXT_INT64  = TOKEN_FROM_ID(407, KeywordTokenType),
    T_MSEXT_BASED  = TOKEN_FROM_ID(408, KeywordTokenType),
    T_MSEXT_DECLSPEC = TOKEN_FROM_ID(409, KeywordTokenType),
    T_MSEXT_CDECL  = TOKEN_FROM_ID(410, KeywordTokenType),
    T_MSEXT_FASTCALL = TOKEN_FROM_ID(411, KeywordTokenType),
    T_MSEXT_STDCALL = TOKEN_FROM_ID(412, KeywordTokenType),
    T_MSEXT_TRY    = TOKEN_FROM_ID(413, KeywordTokenType),
    T_MSEXT_EXCEPT = TOKEN_FROM_ID(414, KeywordTokenType),
    T_MSEXT_FINALLY = TOKEN_FROM_ID(415, KeywordTokenType),
    T_MSEXT_LEAVE  = TOKEN_FROM_ID(416, KeywordTokenType),
    T_MSEXT_INLINE = TOKEN_FROM_ID(417, KeywordTokenType),
    T_MSEXT_ASM    = TOKEN_FROM_ID(418, KeywordTokenType),

    T_MSEXT_PP_REGION    = TOKEN_FROM_ID(419, PPTokenType),
    T_MSEXT_PP_ENDREGION = TOKEN_FROM_ID(420, PPTokenType),

// import is needed to be a keyword for the C++ module Standards proposal
    T_IMPORT       = TOKEN_FROM_ID(421, KeywordTokenType),

// C++11 keywords
    T_ALIGNAS      = TOKEN_FROM_ID(422, KeywordTokenType),
    T_ALIGNOF      = TOKEN_FROM_ID(423, KeywordTokenType),
    T_CHAR16_T     = TOKEN_FROM_ID(424, KeywordTokenType),
    T_CHAR32_T     = TOKEN_FROM_ID(425, KeywordTokenType),
    T_CONSTEXPR    = TOKEN_FROM_ID(426, KeywordTokenType),
    T_DECLTYPE     = TOKEN_FROM_ID(427, KeywordTokenType),
    T_NOEXCEPT     = TOKEN_FROM_ID(428, KeywordTokenType),
    T_NULLPTR      = TOKEN_FROM_ID(429, KeywordTokenType),
    T_STATICASSERT = TOKEN_FROM_ID(430, KeywordTokenType),
    T_THREADLOCAL  = TOKEN_FROM_ID(431, KeywordTokenType),
    T_RAWSTRINGLIT = TOKEN_FROM_ID(432, StringLiteralTokenType),

// C++20 keywords
    T_CHAR8_T      = TOKEN_FROM_ID(433, KeywordTokenType),
    T_CONCEPT      = TOKEN_FROM_ID(434, KeywordTokenType),
    T_CONSTEVAL    = TOKEN_FROM_ID(435, KeywordTokenType),
    T_CONSTINIT    = TOKEN_FROM_ID(436, KeywordTokenType),
    T_CO_AWAIT     = TOKEN_FROM_ID(437, KeywordTokenType),
    T_CO_RETURN    = TOKEN_FROM_ID(438, KeywordTokenType),
    T_CO_YIELD     = TOKEN_FROM_ID(439, KeywordTokenType),
    T_REQUIRES     = TOKEN_FROM_ID(440, KeywordTokenType),

// C++20 operators
    T_SPACESHIP    = TOKEN_FROM_ID(441, OperatorTokenType),

    T_LAST_TOKEN_ID,
    T_LAST_TOKEN = ID_FROM_TOKEN(T_LAST_TOKEN_ID & ~PPTokenFlag),

    T_UNKNOWN_UNIVERSALCHAR = TOKEN_FROM_ID('\\', UnknownTokenType),

// pseudo tokens to help streamlining macro replacement, these should not
// returned from the lexer nor should these be returned from the pp-iterator
    T_NONREPLACABLE_IDENTIFIER = TOKEN_FROM_ID(T_LAST_TOKEN+1, IdentifierTokenType),
    T_PLACEHOLDER = TOKEN_FROM_ID(T_LAST_TOKEN+2, WhiteSpaceTokenType),
    T_PLACEMARKER = TOKEN_FROM_ID(T_LAST_TOKEN+3, InternalTokenType),
    T_PARAMETERBASE = TOKEN_FROM_ID(T_LAST_TOKEN+4, ParameterTokenType),
    T_EXTPARAMETERBASE = TOKEN_FROM_ID(T_LAST_TOKEN+4, ExtParameterTokenType),
    T_OPTPARAMETERBASE = TOKEN_FROM_ID(T_LAST_TOKEN+4, OptParameterTokenType)
};

///////////////////////////////////////////////////////////////////////////////
//  redefine the TOKEN_FROM_ID macro to be more type safe
#undef TOKEN_FROM_ID
#define TOKEN_FROM_ID(id, cat)   boost::wave::token_id((id) | (cat))

#undef ID_FROM_TOKEN
#define ID_FROM_TOKEN(tok)                                                    \
    boost::wave::token_id((tok) &                                             \
        ~(boost::wave::TokenTypeMask|boost::wave::PPTokenFlag))               \
    /**/

#undef BASEID_FROM_TOKEN
#define BASEID_FROM_TOKEN(tok)                                                \
    boost::wave::token_id((tok) &                                             \
         ~(boost::wave::ExtTokenTypeMask|boost::wave::PPTokenFlag))           \
    /**/
#define BASE_TOKEN(tok)                                                       \
    boost::wave::token_id((tok) & boost::wave::MainTokenMask)                 \
    /**/
#define CATEGORY_FROM_TOKEN(tok) ((tok) & boost::wave::TokenTypeMask)
#define EXTCATEGORY_FROM_TOKEN(tok) ((tok) & boost::wave::ExtTokenTypeMask)
#define IS_CATEGORY(tok, cat)                                                 \
    ((CATEGORY_FROM_TOKEN(tok) == CATEGORY_FROM_TOKEN(cat)) ? true : false)   \
    /**/
#define IS_EXTCATEGORY(tok, cat)                                              \
    ((EXTCATEGORY_FROM_TOKEN(tok) == EXTCATEGORY_FROM_TOKEN(cat)) ? true : false) \
    /**/

///////////////////////////////////////////////////////////////////////////////
// verify whether the given token(-id) represents a valid pp_token
inline bool is_pp_token(boost::wave::token_id id)
{
    return (id & boost::wave::PPTokenFlag) ? true : false;
}

template <typename TokenT>
inline bool is_pp_token(TokenT const& tok)
{
    return is_pp_token(boost::wave::token_id(tok));
}

///////////////////////////////////////////////////////////////////////////////
//  return a token name
BOOST_WAVE_DECL
BOOST_WAVE_STRINGTYPE get_token_name(token_id tokid);

///////////////////////////////////////////////////////////////////////////////
//  return a token name
BOOST_WAVE_DECL
char const *get_token_value(token_id tokid);

///////////////////////////////////////////////////////////////////////////////
}   // namespace wave
}   // namespace boost

#endif // #if !defined(BOOST_WAVE_TOKEN_IDS_DEFINED)

// the suffix header occurs after all of the code
#ifdef BOOST_HAS_ABI_HEADERS
#include BOOST_ABI_SUFFIX
#endif

#endif // !defined(BOOST_TOKEN_IDS_HPP_414E9A58_F079_4789_8AFF_513815CE475B_INCLUDED)


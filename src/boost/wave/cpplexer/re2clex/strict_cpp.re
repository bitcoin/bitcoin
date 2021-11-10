/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library

    Copyright (c) 2001 Daniel C. Nuffer
    Copyright (c) 2001-2011 Hartmut Kaiser.
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

    This is a strict lexer conforming to the Standard as close as possible.
    It does not allow the '$' to be part of identifiers. If you need the '$'
    character in identifiers please include the lexer definition provided
    in the cpp.re file.

    TODO:
        handle errors better.
=============================================================================*/

/*!re2c
re2c:indent:string = "    ";
any                = [\t\v\f\r\n\040-\377];
anyctrl            = [\001-\037];
OctalDigit         = [0-7];
Digit              = [0-9];
HexDigit           = [a-fA-F0-9];
Integer            = (("0" [xX] HexDigit+) | ("0" OctalDigit*) | ([1-9] Digit*));
ExponentStart      = [Ee] [+-];
ExponentPart       = [Ee] [+-]? Digit+;
FractionalConstant = (Digit* "." Digit+) | (Digit+ ".");
FloatingSuffix     = [fF] [lL]? | [lL] [fF]?;
IntegerSuffix      = [uU] [lL]? | [lL] [uU]?;
LongIntegerSuffix  = [uU] ("ll" | "LL") | ("ll" | "LL") [uU]?;
Backslash          = [\\] | "??/";
EscapeSequence     = Backslash ([abfnrtv?'"] | Backslash | "x" HexDigit+ | OctalDigit OctalDigit? OctalDigit?);
HexQuad            = HexDigit HexDigit HexDigit HexDigit;
UniversalChar      = Backslash ("u" HexQuad | "U" HexQuad HexQuad);
Newline            = "\r\n" | "\n" | "\r";
PPSpace            = ([ \t\f\v]|("/*"(any\[*]|Newline|("*"+(any\[*/]|Newline)))*"*"+"/"))*;
Pound              = "#" | "??=" | "%:";
NonDigit           = [a-zA-Z_] | UniversalChar;
*/

/*!re2c
    "/*"            { goto ccomment; }
    "//"            { goto cppcomment; }
    "."? Digit      { goto pp_number; }

    "alignas"       { BOOST_WAVE_RET(s->act_in_cpp0x_mode ? T_ALIGNAS : T_IDENTIFIER); }
    "alignof"       { BOOST_WAVE_RET(s->act_in_cpp0x_mode ? T_ALIGNOF : T_IDENTIFIER); }
    "asm"           { BOOST_WAVE_RET(T_ASM); }
    "auto"          { BOOST_WAVE_RET(T_AUTO); }
    "bool"          { BOOST_WAVE_RET(T_BOOL); }
    "break"         { BOOST_WAVE_RET(T_BREAK); }
    "case"          { BOOST_WAVE_RET(T_CASE); }
    "catch"         { BOOST_WAVE_RET(T_CATCH); }
    "char"          { BOOST_WAVE_RET(T_CHAR); }
    "char8_t"       { BOOST_WAVE_RET(s->act_in_cpp2a_mode ? T_CHAR8_T : T_IDENTIFIER); }
    "char16_t"      { BOOST_WAVE_RET(s->act_in_cpp0x_mode ? T_CHAR16_T : T_IDENTIFIER); }
    "char32_t"      { BOOST_WAVE_RET(s->act_in_cpp0x_mode ? T_CHAR32_T : T_IDENTIFIER); }
    "class"         { BOOST_WAVE_RET(T_CLASS); }
    "concept"       { BOOST_WAVE_RET(s->act_in_cpp2a_mode ? T_CONCEPT : T_IDENTIFIER); }
    "const"         { BOOST_WAVE_RET(T_CONST); }
    "consteval"     { BOOST_WAVE_RET(s->act_in_cpp2a_mode ? T_CONSTEVAL : T_IDENTIFIER); }
    "constexpr"     { BOOST_WAVE_RET(s->act_in_cpp0x_mode ? T_CONSTEXPR : T_IDENTIFIER); }
    "constinit"     { BOOST_WAVE_RET(s->act_in_cpp2a_mode ? T_CONSTINIT : T_IDENTIFIER); }
    "const_cast"    { BOOST_WAVE_RET(T_CONSTCAST); }
    "continue"      { BOOST_WAVE_RET(T_CONTINUE); }
    "co_await"      { BOOST_WAVE_RET(s->act_in_cpp2a_mode ? T_CO_AWAIT : T_IDENTIFIER); }
    "co_return"     { BOOST_WAVE_RET(s->act_in_cpp2a_mode ? T_CO_RETURN : T_IDENTIFIER); }
    "co_yield"      { BOOST_WAVE_RET(s->act_in_cpp2a_mode ? T_CO_YIELD : T_IDENTIFIER); }
    "decltype"      { BOOST_WAVE_RET(s->act_in_cpp0x_mode ? T_DECLTYPE : T_IDENTIFIER); }
    "default"       { BOOST_WAVE_RET(T_DEFAULT); }
    "delete"        { BOOST_WAVE_RET(T_DELETE); }
    "do"            { BOOST_WAVE_RET(T_DO); }
    "double"        { BOOST_WAVE_RET(T_DOUBLE); }
    "dynamic_cast"  { BOOST_WAVE_RET(T_DYNAMICCAST); }
    "else"          { BOOST_WAVE_RET(T_ELSE); }
    "enum"          { BOOST_WAVE_RET(T_ENUM); }
    "explicit"      { BOOST_WAVE_RET(T_EXPLICIT); }
    "export"        { BOOST_WAVE_RET(T_EXPORT); }
    "extern"        { BOOST_WAVE_RET(T_EXTERN); }
    "false"         { BOOST_WAVE_RET(T_FALSE); }
    "float"         { BOOST_WAVE_RET(T_FLOAT); }
    "for"           { BOOST_WAVE_RET(T_FOR); }
    "friend"        { BOOST_WAVE_RET(T_FRIEND); }
    "goto"          { BOOST_WAVE_RET(T_GOTO); }
    "if"            { BOOST_WAVE_RET(T_IF); }
    "import"        { BOOST_WAVE_RET(s->enable_import_keyword ? T_IMPORT : T_IDENTIFIER); }
    "inline"        { BOOST_WAVE_RET(T_INLINE); }
    "int"           { BOOST_WAVE_RET(T_INT); }
    "long"          { BOOST_WAVE_RET(T_LONG); }
    "mutable"       { BOOST_WAVE_RET(T_MUTABLE); }
    "namespace"     { BOOST_WAVE_RET(T_NAMESPACE); }
    "new"           { BOOST_WAVE_RET(T_NEW); }
    "noexcept"      { BOOST_WAVE_RET(s->act_in_cpp0x_mode ? T_NOEXCEPT : T_IDENTIFIER); }
    "nullptr"       { BOOST_WAVE_RET(s->act_in_cpp0x_mode ? T_NULLPTR : T_IDENTIFIER); }
    "operator"      { BOOST_WAVE_RET(T_OPERATOR); }
    "private"       { BOOST_WAVE_RET(T_PRIVATE); }
    "protected"     { BOOST_WAVE_RET(T_PROTECTED); }
    "public"        { BOOST_WAVE_RET(T_PUBLIC); }
    "register"      { BOOST_WAVE_RET(T_REGISTER); }
    "reinterpret_cast" { BOOST_WAVE_RET(T_REINTERPRETCAST); }
    "requires"      { BOOST_WAVE_RET(s->act_in_cpp2a_mode ? T_REQUIRES : T_IDENTIFIER); }
    "return"        { BOOST_WAVE_RET(T_RETURN); }
    "short"         { BOOST_WAVE_RET(T_SHORT); }
    "signed"        { BOOST_WAVE_RET(T_SIGNED); }
    "sizeof"        { BOOST_WAVE_RET(T_SIZEOF); }
    "static"        { BOOST_WAVE_RET(T_STATIC); }
    "static_cast"   { BOOST_WAVE_RET(T_STATICCAST); }
    "static_assert" { BOOST_WAVE_RET(s->act_in_cpp0x_mode ? T_STATICASSERT : T_IDENTIFIER); }
    "struct"        { BOOST_WAVE_RET(T_STRUCT); }
    "switch"        { BOOST_WAVE_RET(T_SWITCH); }
    "template"      { BOOST_WAVE_RET(T_TEMPLATE); }
    "this"          { BOOST_WAVE_RET(T_THIS); }
    "thread_local"  { BOOST_WAVE_RET(s->act_in_cpp0x_mode ? T_THREADLOCAL : T_IDENTIFIER); }
    "throw"         { BOOST_WAVE_RET(T_THROW); }
    "true"          { BOOST_WAVE_RET(T_TRUE); }
    "try"           { BOOST_WAVE_RET(T_TRY); }
    "typedef"       { BOOST_WAVE_RET(T_TYPEDEF); }
    "typeid"        { BOOST_WAVE_RET(T_TYPEID); }
    "typename"      { BOOST_WAVE_RET(T_TYPENAME); }
    "union"         { BOOST_WAVE_RET(T_UNION); }
    "unsigned"      { BOOST_WAVE_RET(T_UNSIGNED); }
    "using"         { BOOST_WAVE_RET(T_USING); }
    "virtual"       { BOOST_WAVE_RET(T_VIRTUAL); }
    "void"          { BOOST_WAVE_RET(T_VOID); }
    "volatile"      { BOOST_WAVE_RET(T_VOLATILE); }
    "wchar_t"       { BOOST_WAVE_RET(T_WCHART); }
    "while"         { BOOST_WAVE_RET(T_WHILE); }

    "__int8"        { BOOST_WAVE_RET(s->enable_ms_extensions ? T_MSEXT_INT8 : T_IDENTIFIER); }
    "__int16"       { BOOST_WAVE_RET(s->enable_ms_extensions ? T_MSEXT_INT16 : T_IDENTIFIER); }
    "__int32"       { BOOST_WAVE_RET(s->enable_ms_extensions ? T_MSEXT_INT32 : T_IDENTIFIER); }
    "__int64"       { BOOST_WAVE_RET(s->enable_ms_extensions ? T_MSEXT_INT64 : T_IDENTIFIER); }
    "_"? "_based"   { BOOST_WAVE_RET(s->enable_ms_extensions ? T_MSEXT_BASED : T_IDENTIFIER); }
    "_"? "_declspec" { BOOST_WAVE_RET(s->enable_ms_extensions ? T_MSEXT_DECLSPEC : T_IDENTIFIER); }
    "_"? "_cdecl"   { BOOST_WAVE_RET(s->enable_ms_extensions ? T_MSEXT_CDECL : T_IDENTIFIER); }
    "_"? "_fastcall" { BOOST_WAVE_RET(s->enable_ms_extensions ? T_MSEXT_FASTCALL : T_IDENTIFIER); }
    "_"? "_stdcall" { BOOST_WAVE_RET(s->enable_ms_extensions ? T_MSEXT_STDCALL : T_IDENTIFIER); }
    "__try"         { BOOST_WAVE_RET(s->enable_ms_extensions ? T_MSEXT_TRY : T_IDENTIFIER); }
    "__except"      { BOOST_WAVE_RET(s->enable_ms_extensions ? T_MSEXT_EXCEPT : T_IDENTIFIER); }
    "__finally"     { BOOST_WAVE_RET(s->enable_ms_extensions ? T_MSEXT_FINALLY : T_IDENTIFIER); }
    "__leave"       { BOOST_WAVE_RET(s->enable_ms_extensions ? T_MSEXT_LEAVE : T_IDENTIFIER); }
    "_"? "_inline"  { BOOST_WAVE_RET(s->enable_ms_extensions ? T_MSEXT_INLINE : T_IDENTIFIER); }
    "_"? "_asm"     { BOOST_WAVE_RET(s->enable_ms_extensions ? T_MSEXT_ASM : T_IDENTIFIER); }

    "{"             { BOOST_WAVE_RET(T_LEFTBRACE); }
    "??<"           { BOOST_WAVE_RET(T_LEFTBRACE_TRIGRAPH); }
    "<%"            { BOOST_WAVE_RET(T_LEFTBRACE_ALT); }
    "}"             { BOOST_WAVE_RET(T_RIGHTBRACE); }
    "??>"           { BOOST_WAVE_RET(T_RIGHTBRACE_TRIGRAPH); }
    "%>"            { BOOST_WAVE_RET(T_RIGHTBRACE_ALT); }
    "["             { BOOST_WAVE_RET(T_LEFTBRACKET); }
    "??("           { BOOST_WAVE_RET(T_LEFTBRACKET_TRIGRAPH); }
    "<:"            { BOOST_WAVE_RET(T_LEFTBRACKET_ALT); }
    "]"             { BOOST_WAVE_RET(T_RIGHTBRACKET); }
    "??)"           { BOOST_WAVE_RET(T_RIGHTBRACKET_TRIGRAPH); }
    ":>"            { BOOST_WAVE_RET(T_RIGHTBRACKET_ALT); }
    "#"             { BOOST_WAVE_RET(T_POUND); }
    "%:"            { BOOST_WAVE_RET(T_POUND_ALT); }
    "??="           { BOOST_WAVE_RET(T_POUND_TRIGRAPH); }
    "##"            { BOOST_WAVE_RET(T_POUND_POUND); }
    "#??="          { BOOST_WAVE_RET(T_POUND_POUND_TRIGRAPH); }
    "??=#"          { BOOST_WAVE_RET(T_POUND_POUND_TRIGRAPH); }
    "??=??="        { BOOST_WAVE_RET(T_POUND_POUND_TRIGRAPH); }
    "%:%:"          { BOOST_WAVE_RET(T_POUND_POUND_ALT); }
    "("             { BOOST_WAVE_RET(T_LEFTPAREN); }
    ")"             { BOOST_WAVE_RET(T_RIGHTPAREN); }
    ";"             { BOOST_WAVE_RET(T_SEMICOLON); }
    ":"             { BOOST_WAVE_RET(T_COLON); }
    "..."           { BOOST_WAVE_RET(T_ELLIPSIS); }
    "?"             { BOOST_WAVE_RET(T_QUESTION_MARK); }
    "::"
        {
            if (s->act_in_c99_mode) {
                --YYCURSOR;
                BOOST_WAVE_RET(T_COLON);
            }
            else {
                BOOST_WAVE_RET(T_COLON_COLON);
            }
        }
    "."             { BOOST_WAVE_RET(T_DOT); }
    ".*"
        {
            if (s->act_in_c99_mode) {
                --YYCURSOR;
                BOOST_WAVE_RET(T_DOT);
            }
            else {
                BOOST_WAVE_RET(T_DOTSTAR);
            }
        }
    "+"             { BOOST_WAVE_RET(T_PLUS); }
    "-"             { BOOST_WAVE_RET(T_MINUS); }
    "*"             { BOOST_WAVE_RET(T_STAR); }
    "/"             { BOOST_WAVE_RET(T_DIVIDE); }
    "%"             { BOOST_WAVE_RET(T_PERCENT); }
    "^"             { BOOST_WAVE_RET(T_XOR); }
    "??'"           { BOOST_WAVE_RET(T_XOR_TRIGRAPH); }
    "xor"           { BOOST_WAVE_RET(s->act_in_c99_mode ? T_IDENTIFIER : T_XOR_ALT); }
    "&"             { BOOST_WAVE_RET(T_AND); }
    "bitand"        { BOOST_WAVE_RET(s->act_in_c99_mode ? T_IDENTIFIER : T_AND_ALT); }
    "|"             { BOOST_WAVE_RET(T_OR); }
    "bitor"         { BOOST_WAVE_RET(s->act_in_c99_mode ? T_IDENTIFIER : T_OR_ALT); }
    "??!"           { BOOST_WAVE_RET(T_OR_TRIGRAPH); }
    "~"             { BOOST_WAVE_RET(T_COMPL); }
    "??-"           { BOOST_WAVE_RET(T_COMPL_TRIGRAPH); }
    "compl"         { BOOST_WAVE_RET(s->act_in_c99_mode ? T_IDENTIFIER : T_COMPL_ALT); }
    "!"             { BOOST_WAVE_RET(T_NOT); }
    "not"           { BOOST_WAVE_RET(s->act_in_c99_mode ? T_IDENTIFIER : T_NOT_ALT); }
    "="             { BOOST_WAVE_RET(T_ASSIGN); }
    "<"             { BOOST_WAVE_RET(T_LESS); }
    ">"             { BOOST_WAVE_RET(T_GREATER); }
    "+="            { BOOST_WAVE_RET(T_PLUSASSIGN); }
    "-="            { BOOST_WAVE_RET(T_MINUSASSIGN); }
    "*="            { BOOST_WAVE_RET(T_STARASSIGN); }
    "/="            { BOOST_WAVE_RET(T_DIVIDEASSIGN); }
    "%="            { BOOST_WAVE_RET(T_PERCENTASSIGN); }
    "^="            { BOOST_WAVE_RET(T_XORASSIGN); }
    "xor_eq"        { BOOST_WAVE_RET(s->act_in_c99_mode ? T_IDENTIFIER : T_XORASSIGN_ALT); }
    "??'="          { BOOST_WAVE_RET(T_XORASSIGN_TRIGRAPH); }
    "&="            { BOOST_WAVE_RET(T_ANDASSIGN); }
    "and_eq"        { BOOST_WAVE_RET(s->act_in_c99_mode ? T_IDENTIFIER : T_ANDASSIGN_ALT); }
    "|="            { BOOST_WAVE_RET(T_ORASSIGN); }
    "or_eq"         { BOOST_WAVE_RET(s->act_in_c99_mode ? T_IDENTIFIER : T_ORASSIGN_ALT); }
    "??!="          { BOOST_WAVE_RET(T_ORASSIGN_TRIGRAPH); }
    "<<"            { BOOST_WAVE_RET(T_SHIFTLEFT); }
    ">>"            { BOOST_WAVE_RET(T_SHIFTRIGHT); }
    ">>="           { BOOST_WAVE_RET(T_SHIFTRIGHTASSIGN); }
    "<<="           { BOOST_WAVE_RET(T_SHIFTLEFTASSIGN); }
    "=="            { BOOST_WAVE_RET(T_EQUAL); }
    "!="            { BOOST_WAVE_RET(T_NOTEQUAL); }
    "not_eq"        { BOOST_WAVE_RET(s->act_in_c99_mode ? T_IDENTIFIER : T_NOTEQUAL_ALT); }
    "<=>"
        {
            if (s->act_in_cpp2a_mode) {
                BOOST_WAVE_RET(T_SPACESHIP);
            }
            else {
                --YYCURSOR;
                BOOST_WAVE_RET(T_LESSEQUAL);
            }
        }
    "<="            { BOOST_WAVE_RET(T_LESSEQUAL); }
    ">="            { BOOST_WAVE_RET(T_GREATEREQUAL); }
    "&&"            { BOOST_WAVE_RET(T_ANDAND); }
    "and"           { BOOST_WAVE_RET(s->act_in_c99_mode ? T_IDENTIFIER : T_ANDAND_ALT); }
    "||"            { BOOST_WAVE_RET(T_OROR); }
    "??!|"          { BOOST_WAVE_RET(T_OROR_TRIGRAPH); }
    "|??!"          { BOOST_WAVE_RET(T_OROR_TRIGRAPH); }
    "or"            { BOOST_WAVE_RET(s->act_in_c99_mode ? T_IDENTIFIER : T_OROR_ALT); }
    "??!??!"        { BOOST_WAVE_RET(T_OROR_TRIGRAPH); }
    "++"            { BOOST_WAVE_RET(T_PLUSPLUS); }
    "--"            { BOOST_WAVE_RET(T_MINUSMINUS); }
    ","             { BOOST_WAVE_RET(T_COMMA); }
    "->*"
        {
            if (s->act_in_c99_mode) {
                --YYCURSOR;
                BOOST_WAVE_RET(T_ARROW);
            }
            else {
                BOOST_WAVE_RET(T_ARROWSTAR);
            }
        }
    "->"            { BOOST_WAVE_RET(T_ARROW); }
    "??/"           { BOOST_WAVE_RET(T_ANY_TRIGRAPH); }

    "L"? (['] (EscapeSequence | UniversalChar | any\[\n\r\\'])+ ['])
        { BOOST_WAVE_RET(T_CHARLIT); }

    "L"? (["] (EscapeSequence | UniversalChar | any\[\n\r\\"])* ["])
        { BOOST_WAVE_RET(T_STRINGLIT); }

    "L"? "R" ["]
        {
            if (s->act_in_cpp0x_mode)
            {
                rawstringdelim = "";
                goto extrawstringlit;
            }
            --YYCURSOR;
            BOOST_WAVE_RET(T_IDENTIFIER);
        }

    [uU] [']
        {
            if (s->act_in_cpp0x_mode)
                goto extcharlit;
            --YYCURSOR;
            BOOST_WAVE_RET(T_IDENTIFIER);
        }

    ([uU] | "u8") ["]
        {
            if (s->act_in_cpp0x_mode)
                goto extstringlit;
            --YYCURSOR;
            BOOST_WAVE_RET(T_IDENTIFIER);
        }

    ([uU] | "u8") "R" ["]
        {
            if (s->act_in_cpp0x_mode)
            {
                rawstringdelim = "";
                goto extrawstringlit;
            }
            --YYCURSOR;
            BOOST_WAVE_RET(T_IDENTIFIER);
        }

    ([a-zA-Z_] | UniversalChar) ([a-zA-Z_0-9] | UniversalChar)*
        { BOOST_WAVE_RET(T_IDENTIFIER); }

    Pound PPSpace ( "include" | "include_next") PPSpace "<" (any\[\n\r>])+ ">"
        { BOOST_WAVE_RET(T_PP_HHEADER); }

    Pound PPSpace ( "include" | "include_next") PPSpace "\"" (any\[\n\r"])+ "\""
        { BOOST_WAVE_RET(T_PP_QHEADER); }

    Pound PPSpace ( "include" | "include_next") PPSpace
        { BOOST_WAVE_RET(T_PP_INCLUDE); }

    Pound PPSpace "if"        { BOOST_WAVE_RET(T_PP_IF); }
    Pound PPSpace "ifdef"     { BOOST_WAVE_RET(T_PP_IFDEF); }
    Pound PPSpace "ifndef"    { BOOST_WAVE_RET(T_PP_IFNDEF); }
    Pound PPSpace "else"      { BOOST_WAVE_RET(T_PP_ELSE); }
    Pound PPSpace "elif"      { BOOST_WAVE_RET(T_PP_ELIF); }
    Pound PPSpace "endif"     { BOOST_WAVE_RET(T_PP_ENDIF); }
    Pound PPSpace "define"    { BOOST_WAVE_RET(T_PP_DEFINE); }
    Pound PPSpace "undef"     { BOOST_WAVE_RET(T_PP_UNDEF); }
    Pound PPSpace "line"      { BOOST_WAVE_RET(T_PP_LINE); }
    Pound PPSpace "error"     { BOOST_WAVE_RET(T_PP_ERROR); }
    Pound PPSpace "pragma"    { BOOST_WAVE_RET(T_PP_PRAGMA); }

    Pound PPSpace "warning"   { BOOST_WAVE_RET(T_PP_WARNING); }

    Pound PPSpace "region"    { BOOST_WAVE_RET(T_MSEXT_PP_REGION); }
    Pound PPSpace "endregion" { BOOST_WAVE_RET(T_MSEXT_PP_ENDREGION); }

    [ \t\v\f]+
        { BOOST_WAVE_RET(T_SPACE); }

    Newline
    {
        s->line++;
        cursor.column = 1;
        BOOST_WAVE_RET(T_NEWLINE);
    }

    "\000"
    {
        if (s->eof && cursor != s->eof)
        {
            BOOST_WAVE_UPDATE_CURSOR();     // adjust the input cursor
            (*s->error_proc)(s, lexing_exception::generic_lexing_error,
                "invalid character '\\000' in input stream");
        }
        BOOST_WAVE_RET(T_EOF);
    }

    any        { BOOST_WAVE_RET(TOKEN_FROM_ID(*s->tok, UnknownTokenType)); }

    anyctrl
    {
        // flag the error
        BOOST_WAVE_UPDATE_CURSOR();     // adjust the input cursor
        (*s->error_proc)(s, lexing_exception::generic_lexing_error,
            "invalid character '\\%03o' in input stream", *--YYCURSOR);
    }
*/

ccomment:
/*!re2c
    "*/"            { BOOST_WAVE_RET(T_CCOMMENT); }

    Newline
    {
        /*if(cursor == s->eof) BOOST_WAVE_RET(T_EOF);*/
        /*s->tok = cursor; */
        s->line += count_backslash_newlines(s, cursor) +1;
        cursor.column = 1;
        goto ccomment;
    }

    any            { goto ccomment; }

    "\000"
    {
        if(cursor == s->eof)
        {
            BOOST_WAVE_UPDATE_CURSOR();   // adjust the input cursor
            (*s->error_proc)(s, lexing_exception::generic_lexing_warning,
                "Unterminated 'C' style comment");
        }
        else
        {
            --YYCURSOR;                   // next call returns T_EOF
            BOOST_WAVE_UPDATE_CURSOR();   // adjust the input cursor
            (*s->error_proc)(s, lexing_exception::generic_lexing_error,
                "invalid character: '\\000' in input stream");
        }
    }

    anyctrl
    {
        // flag the error
        BOOST_WAVE_UPDATE_CURSOR();   // adjust the input cursor
        (*s->error_proc)(s, lexing_exception::generic_lexing_error,
            "invalid character '\\%03o' in input stream", *--YYCURSOR);
    }
*/

cppcomment:
/*!re2c
    Newline
    {
        /*if(cursor == s->eof) BOOST_WAVE_RET(T_EOF); */
        /*s->tok = cursor; */
        s->line++;
        cursor.column = 1;
        BOOST_WAVE_RET(T_CPPCOMMENT);
    }

    any            { goto cppcomment; }

    "\000"
    {
        if (s->eof && cursor != s->eof)
        {
            --YYCURSOR;                     // next call returns T_EOF
            BOOST_WAVE_UPDATE_CURSOR();     // adjust the input cursor
            (*s->error_proc)(s, lexing_exception::generic_lexing_error,
                "invalid character '\\000' in input stream");
        }

        --YYCURSOR;                         // next call returns T_EOF
        if (!s->single_line_only)
        {
            BOOST_WAVE_UPDATE_CURSOR();     // adjust the input cursor
            (*s->error_proc)(s, lexing_exception::generic_lexing_warning,
                "Unterminated 'C++' style comment");
        }
        BOOST_WAVE_RET(T_CPPCOMMENT);
    }

    anyctrl
    {
        // flag the error
        BOOST_WAVE_UPDATE_CURSOR();     // adjust the input cursor
        (*s->error_proc)(s, lexing_exception::generic_lexing_error,
            "invalid character '\\%03o' in input stream", *--YYCURSOR);
    }
*/

/* this subscanner is called whenever a pp_number has been started */
pp_number:
{
    cursor = uchar_wrapper(s->tok = s->cur, s->column = s->curr_column);
    marker = uchar_wrapper(s->ptr);
    limit = uchar_wrapper(s->lim);

    if (s->detect_pp_numbers) {
    /*!re2c
        "."? Digit (Digit | NonDigit | ExponentStart | ".")*
            { BOOST_WAVE_RET(T_PP_NUMBER); }

        * { BOOST_ASSERT(false); }
    */
    }
    else {
    /*!re2c
        ((FractionalConstant ExponentPart?) | (Digit+ ExponentPart)) FloatingSuffix?
            { BOOST_WAVE_RET(T_FLOATLIT); }

        Integer { goto integer_suffix; }

        * { BOOST_ASSERT(false); }
    */
    }
}

/* this subscanner is called, whenever an Integer was recognized */
integer_suffix:
{
    if (s->enable_ms_extensions) {
    /*!re2c
        LongIntegerSuffix | "i64"
            { BOOST_WAVE_RET(T_LONGINTLIT); }

        IntegerSuffix?
            { BOOST_WAVE_RET(T_INTLIT); }
    */
    }
    else {
    /*!re2c
        LongIntegerSuffix
            { BOOST_WAVE_RET(T_LONGINTLIT); }

        IntegerSuffix?
            { BOOST_WAVE_RET(T_INTLIT); }
    */
    }

    // re2c will complain about -Wmatch-empty-string above
    // it's OK because we've already matched an integer
    // and will return T_INTLIT
}

/* this subscanner is invoked for C++0x extended character literals */
extcharlit:
{
    /*!re2c
        * {
            (*s->error_proc)(s, lexing_exception::generic_lexing_error,
                "Invalid character in raw string delimiter ('%c')", yych);
        }

        ((EscapeSequence | UniversalChar | any\[\n\r\\']) ['])
            { BOOST_WAVE_RET(T_CHARLIT); }

        any
            { BOOST_WAVE_RET(TOKEN_FROM_ID(*s->tok, UnknownTokenType)); }
    */
}

/* this subscanner is invoked for C++0x extended character string literals */
extstringlit:
{
    /*!re2c
        * {
            (*s->error_proc)(s, lexing_exception::generic_lexing_error,
                "Invalid character in raw string delimiter ('%c')", yych);
        }

        ((EscapeSequence | UniversalChar | any\[\n\r\\"])* ["])
            { BOOST_WAVE_RET(T_STRINGLIT); }

        any
            { BOOST_WAVE_RET(TOKEN_FROM_ID(*s->tok, UnknownTokenType)); }
    */
}

extrawstringlit:
{
    // we have consumed the double quote but not the lparen
    // at this point we may see a delimiter

    /*!re2c
        * {
            (*s->error_proc)(s, lexing_exception::generic_lexing_error,
                "Invalid character in raw string delimiter ('%c')", yych);
        }

        // delimiters are any character but parentheses, backslash, and whitespace
        any\[()\\\t\v\f\r\n]
        {
            rawstringdelim += yych;
            if (rawstringdelim.size() > 16)
            {
                (*s->error_proc)(s, lexing_exception::generic_lexing_error,
                    "Raw string delimiter of excessive length (\"%s\") in input stream",
                    rawstringdelim.c_str());
            }
            goto extrawstringlit;
        }

        "("
        {
            rawstringdelim = ")" + rawstringdelim;
            goto extrawstringbody;
        }

    */
}

extrawstringbody:
{
    /*!re2c

        * {
            (*s->error_proc)(s, lexing_exception::generic_lexing_error,
                "Invalid character in raw string body ('%c')", yych);
        }

        Newline
        {
            s->line += count_backslash_newlines(s, cursor) +1;
            cursor.column = 1;
            goto extrawstringbody;
        }

        (EscapeSequence | UniversalChar | any\["])
        {
            goto extrawstringbody;
        }

        ["]
        {
            // check to see if we have completed a delimiter
            if (string_type((char *)(YYCURSOR - rawstringdelim.size() - 1),
                             (char *)(YYCURSOR - 1)) == rawstringdelim)
            {
                 BOOST_WAVE_RET(T_RAWSTRINGLIT);
            } else {
                goto extrawstringbody;
            }
        }
    */
}

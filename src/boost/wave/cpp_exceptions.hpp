/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library

    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#if !defined(BOOST_CPP_EXCEPTIONS_HPP_5190E447_A781_4521_A275_5134FF9917D7_INCLUDED)
#define BOOST_CPP_EXCEPTIONS_HPP_5190E447_A781_4521_A275_5134FF9917D7_INCLUDED

#include <exception>
#include <string>
#include <limits>

#include <boost/assert.hpp>
#include <boost/config.hpp>
#include <boost/throw_exception.hpp>
#include <boost/wave/wave_config.hpp>
#include <boost/wave/cpp_throw.hpp>

// this must occur after all of the includes and before any code appears
#ifdef BOOST_HAS_ABI_HEADERS
#include BOOST_ABI_PREFIX
#endif

///////////////////////////////////////////////////////////////////////////////
namespace boost {
namespace wave {

///////////////////////////////////////////////////////////////////////////////
// exception severity
namespace util {

    enum severity {
        severity_remark = 0,
        severity_warning,
        severity_error,
        severity_fatal,
        severity_commandline_error,
        last_severity_code = severity_commandline_error
    };

    inline char const *
    get_severity(int level)
    {
        static char const *severity_text[] =
        {
            "remark",               // severity_remark
            "warning",              // severity_warning
            "error",                // severity_error
            "fatal error",          // severity_fatal
            "command line error"    // severity_commandline_error
        };
        BOOST_ASSERT(severity_remark <= level &&
            level <= last_severity_code);
        return severity_text[level];
    }
}

///////////////////////////////////////////////////////////////////////////////
//  cpp_exception, the base class for all specific C preprocessor exceptions
class BOOST_SYMBOL_VISIBLE cpp_exception
:   public std::exception
{
public:
    cpp_exception(std::size_t line_, std::size_t column_, char const *filename_) throw()
    :   line(line_), column(column_)
    {
        unsigned int off = 0;
        while (off < sizeof(filename)-1 && *filename_)
            filename[off++] = *filename_++;
        filename[off] = 0;
    }
    ~cpp_exception() throw() {}

    char const *what() const throw() BOOST_OVERRIDE = 0;    // to be overloaded
    virtual char const *description() const throw() = 0;
    virtual int get_errorcode() const throw() = 0;
    virtual int get_severity() const throw() = 0;
    virtual char const* get_related_name() const throw() = 0;
    virtual bool is_recoverable() const throw() = 0;

    std::size_t line_no() const throw() { return line; }
    std::size_t column_no() const throw() { return column; }
    char const *file_name() const throw() { return filename; }

protected:
    char filename[512];
    std::size_t line;
    std::size_t column;
};

///////////////////////////////////////////////////////////////////////////////
// preprocessor error
class BOOST_SYMBOL_VISIBLE preprocess_exception :
    public cpp_exception
{
public:
    enum error_code {
        no_error = 0,
        unexpected_error,
        macro_redefinition,
        macro_insertion_error,
        bad_include_file,
        bad_include_statement,
        bad_has_include_expression,
        ill_formed_directive,
        error_directive,
        warning_directive,
        ill_formed_expression,
        missing_matching_if,
        missing_matching_endif,
        ill_formed_operator,
        bad_define_statement,
        bad_define_statement_va_args,
        bad_define_statement_va_opt,
        bad_define_statement_va_opt_parens,
        bad_define_statement_va_opt_recurse,
        too_few_macroarguments,
        too_many_macroarguments,
        empty_macroarguments,
        improperly_terminated_macro,
        bad_line_statement,
        bad_line_number,
        bad_line_filename,
        bad_undefine_statement,
        bad_macro_definition,
        illegal_redefinition,
        duplicate_parameter_name,
        invalid_concat,
        last_line_not_terminated,
        ill_formed_pragma_option,
        include_nesting_too_deep,
        misplaced_operator,
        alreadydefined_name,
        undefined_macroname,
        invalid_macroname,
        unexpected_qualified_name,
        division_by_zero,
        integer_overflow,
        illegal_operator_redefinition,
        ill_formed_integer_literal,
        ill_formed_character_literal,
        unbalanced_if_endif,
        character_literal_out_of_range,
        could_not_open_output_file,
        incompatible_config,
        ill_formed_pragma_message,
        pragma_message_directive,
        last_error_number = pragma_message_directive
    };

    preprocess_exception(char const *what_, error_code code, std::size_t line_,
        std::size_t column_, char const *filename_) throw()
    :   cpp_exception(line_, column_, filename_),
        code(code)
    {
        unsigned int off = 0;
        while (off < sizeof(buffer) - 1 && *what_)
            buffer[off++] = *what_++;
        buffer[off] = 0;
    }
    ~preprocess_exception() throw() {}

    char const *what() const throw() BOOST_OVERRIDE
    {
        return "boost::wave::preprocess_exception";
    }
    char const *description() const throw() BOOST_OVERRIDE
    {
        return buffer;
    }
    int get_severity() const throw() BOOST_OVERRIDE
    {
        return severity_level(code);
    }
    int get_errorcode() const throw() BOOST_OVERRIDE
    {
        return code;
    }
    char const* get_related_name() const throw() BOOST_OVERRIDE
    {
        return "<unknown>";
    }
    bool is_recoverable() const throw() BOOST_OVERRIDE
    {
        switch (get_errorcode()) {
        // these are the exceptions thrown during processing not supposed to
        // produce any tokens on the context::iterator level
        case preprocess_exception::no_error:        // just a placeholder
        case preprocess_exception::macro_redefinition:
        case preprocess_exception::macro_insertion_error:
        case preprocess_exception::bad_macro_definition:
        case preprocess_exception::illegal_redefinition:
        case preprocess_exception::duplicate_parameter_name:
        case preprocess_exception::invalid_macroname:
        case preprocess_exception::bad_include_file:
        case preprocess_exception::bad_include_statement:
        case preprocess_exception::bad_has_include_expression:
        case preprocess_exception::ill_formed_directive:
        case preprocess_exception::error_directive:
        case preprocess_exception::warning_directive:
        case preprocess_exception::ill_formed_expression:
        case preprocess_exception::missing_matching_if:
        case preprocess_exception::missing_matching_endif:
        case preprocess_exception::unbalanced_if_endif:
        case preprocess_exception::bad_define_statement:
        case preprocess_exception::bad_define_statement_va_args:
        case preprocess_exception::bad_define_statement_va_opt:
        case preprocess_exception::bad_define_statement_va_opt_parens:
        case preprocess_exception::bad_define_statement_va_opt_recurse:
        case preprocess_exception::bad_line_statement:
        case preprocess_exception::bad_line_number:
        case preprocess_exception::bad_line_filename:
        case preprocess_exception::bad_undefine_statement:
        case preprocess_exception::division_by_zero:
        case preprocess_exception::integer_overflow:
        case preprocess_exception::ill_formed_integer_literal:
        case preprocess_exception::ill_formed_character_literal:
        case preprocess_exception::character_literal_out_of_range:
        case preprocess_exception::last_line_not_terminated:
        case preprocess_exception::include_nesting_too_deep:
        case preprocess_exception::illegal_operator_redefinition:
        case preprocess_exception::incompatible_config:
        case preprocess_exception::ill_formed_pragma_option:
        case preprocess_exception::ill_formed_pragma_message:
        case preprocess_exception::pragma_message_directive:
            return true;

        case preprocess_exception::unexpected_error:
        case preprocess_exception::ill_formed_operator:
        case preprocess_exception::too_few_macroarguments:
        case preprocess_exception::too_many_macroarguments:
        case preprocess_exception::empty_macroarguments:
        case preprocess_exception::improperly_terminated_macro:
        case preprocess_exception::invalid_concat:
        case preprocess_exception::could_not_open_output_file:
            break;
        }
        return false;
    }

    static char const *error_text(int code)
    {
        // error texts in this array must appear in the same order as the items in
        // the error enum above
        static char const *preprocess_exception_errors[] = {
            "no error",                                 // no_error
            "unexpected error (should not happen)",     // unexpected_error
            "illegal macro redefinition",               // macro_redefinition
            "macro definition failed (out of memory?)", // macro_insertion_error
            "could not find include file",              // bad_include_file
            "ill formed #include directive",            // bad_include_statement
            "ill formed __has_include expression",      // bad_has_include_expression
            "ill formed preprocessor directive",        // ill_formed_directive
            "encountered #error directive or #pragma wave stop()", // error_directive
            "encountered #warning directive",           // warning_directive
            "ill formed preprocessor expression",       // ill_formed_expression
            "the #if for this directive is missing",    // missing_matching_if
            "detected at least one missing #endif directive",   // missing_matching_endif
            "ill formed preprocessing operator",        // ill_formed_operator
            "ill formed #define directive",             // bad_define_statement
            "__VA_ARGS__ can only appear in the "
            "expansion of a C99 variadic macro",        // bad_define_statement_va_args
            "__VA_OPT__ can only appear in the "
            "expansion of a C++20 variadic macro",      // bad_define_statement_va_opt
            "__VA_OPT__ must be followed by a left "
            "paren in a C++20 variadic macro",          // bad_define_statement_va_opt_parens
            "__VA_OPT__() may not contain __VA_OPT__",  // bad_define_statement_va_opt_recurse
            "too few macro arguments",                  // too_few_macroarguments
            "too many macro arguments",                 // too_many_macroarguments
            "empty macro arguments are not supported in pure C++ mode, "
            "use variadics mode to allow these",        // empty_macroarguments
            "improperly terminated macro invocation "
            "or replacement-list terminates in partial "
            "macro expansion (not supported yet)",      // improperly_terminated_macro
            "ill formed #line directive",               // bad_line_statement
            "line number argument of #line directive "
            "should consist out of decimal digits "
            "only and must be in range of [1..INT_MAX]", // bad_line_number
            "filename argument of #line directive should "
            "be a narrow string literal",               // bad_line_filename
            "#undef may not be used on this predefined name",   // bad_undefine_statement
            "invalid macro definition",                 // bad_macro_definition
            "this predefined name may not be redefined",        // illegal_redefinition
            "duplicate macro parameter name",           // duplicate_parameter_name
            "pasting the following two tokens does not "
            "give a valid preprocessing token",         // invalid_concat
            "last line of file ends without a newline", // last_line_not_terminated
            "unknown or illformed pragma option",       // ill_formed_pragma_option
            "include files nested too deep",            // include_nesting_too_deep
            "misplaced operator defined()",             // misplaced_operator
            "the name is already used in this scope as "
            "a macro or scope name",                    // alreadydefined_name
            "undefined macro or scope name may not be imported", // undefined_macroname
            "ill formed macro name",                    // invalid_macroname
            "qualified names are supported in C++11 mode only",  // unexpected_qualified_name
            "division by zero in preprocessor expression",       // division_by_zero
            "integer overflow in preprocessor expression",       // integer_overflow
            "this cannot be used as a macro name as it is "
            "an operator in C++",                       // illegal_operator_redefinition
            "ill formed integer literal or integer constant too large",   // ill_formed_integer_literal
            "ill formed character literal",             // ill_formed_character_literal
            "unbalanced #if/#endif in include file",    // unbalanced_if_endif
            "expression contains out of range character literal", // character_literal_out_of_range
            "could not open output file",               // could_not_open_output_file
            "incompatible state information",           // incompatible_config
            "illformed pragma message",                 // ill_formed_pragma_message
            "encountered #pragma message directive"     // pragma_message_directive
        };
        BOOST_ASSERT(no_error <= code && code <= last_error_number);
        return preprocess_exception_errors[code];
    }

    static util::severity severity_level(int code)
    {
        static util::severity preprocess_exception_severity[] = {
            util::severity_remark,             // no_error
            util::severity_fatal,              // unexpected_error
            util::severity_warning,            // macro_redefinition
            util::severity_fatal,              // macro_insertion_error
            util::severity_error,              // bad_include_file
            util::severity_error,              // bad_include_statement
            util::severity_error,              // bad_has_include_expression
            util::severity_error,              // ill_formed_directive
            util::severity_fatal,              // error_directive
            util::severity_warning,            // warning_directive
            util::severity_error,              // ill_formed_expression
            util::severity_error,              // missing_matching_if
            util::severity_error,              // missing_matching_endif
            util::severity_error,              // ill_formed_operator
            util::severity_error,              // bad_define_statement
            util::severity_error,              // bad_define_statement_va_args
            util::severity_error,              // bad_define_statement_va_opt
            util::severity_error,              // bad_define_statement_va_opt_parens
            util::severity_error,              // bad_define_statement_va_opt_recurse
            util::severity_warning,            // too_few_macroarguments
            util::severity_warning,            // too_many_macroarguments
            util::severity_warning,            // empty_macroarguments
            util::severity_error,              // improperly_terminated_macro
            util::severity_warning,            // bad_line_statement
            util::severity_warning,            // bad_line_number
            util::severity_warning,            // bad_line_filename
            util::severity_warning,            // bad_undefine_statement
            util::severity_commandline_error,  // bad_macro_definition
            util::severity_warning,            // illegal_redefinition
            util::severity_error,              // duplicate_parameter_name
            util::severity_error,              // invalid_concat
            util::severity_warning,            // last_line_not_terminated
            util::severity_warning,            // ill_formed_pragma_option
            util::severity_fatal,              // include_nesting_too_deep
            util::severity_error,              // misplaced_operator
            util::severity_error,              // alreadydefined_name
            util::severity_error,              // undefined_macroname
            util::severity_error,              // invalid_macroname
            util::severity_error,              // unexpected_qualified_name
            util::severity_fatal,              // division_by_zero
            util::severity_error,              // integer_overflow
            util::severity_error,              // illegal_operator_redefinition
            util::severity_error,              // ill_formed_integer_literal
            util::severity_error,              // ill_formed_character_literal
            util::severity_warning,            // unbalanced_if_endif
            util::severity_warning,            // character_literal_out_of_range
            util::severity_error,              // could_not_open_output_file
            util::severity_remark,             // incompatible_config
            util::severity_warning,            // ill_formed_pragma_message
            util::severity_remark,             // pragma_message_directive
        };
        BOOST_ASSERT(no_error <= code && code <= last_error_number);
        return preprocess_exception_severity[code];
    }
    static char const *severity_text(int code)
    {
        return util::get_severity(severity_level(code));
    }

private:
    char buffer[512];
    error_code code;
};

///////////////////////////////////////////////////////////////////////////////
//  Error during macro handling, this exception contains the related macro name
class BOOST_SYMBOL_VISIBLE macro_handling_exception :
    public preprocess_exception
{
public:
    macro_handling_exception(char const *what_, error_code code, std::size_t line_,
        std::size_t column_, char const *filename_, char const *macroname) throw()
    :   preprocess_exception(what_, code, line_, column_, filename_)
    {
        unsigned int off = 0;
        while (off < sizeof(name) && *macroname)
            name[off++] = *macroname++;
        name[off] = 0;
    }
    ~macro_handling_exception() throw() {}

    char const *what() const throw() BOOST_OVERRIDE
    {
        return "boost::wave::macro_handling_exception";
    }
    char const* get_related_name() const throw() BOOST_OVERRIDE
    {
        return name;
    }

private:
    char name[512];
};

///////////////////////////////////////////////////////////////////////////////
//
//  The is_recoverable() function allows to decide, whether it is possible
//  simply to continue after a given exception was thrown by Wave.
//
//  This is kind of a hack to allow to recover from certain errors as long as
//  Wave doesn't provide better means of error recovery.
//
///////////////////////////////////////////////////////////////////////////////
inline bool
is_recoverable(cpp_exception const& e)
{
    return e.is_recoverable();
}

///////////////////////////////////////////////////////////////////////////////
}   // namespace wave
}   // namespace boost

// the suffix header occurs after all of the code
#ifdef BOOST_HAS_ABI_HEADERS
#include BOOST_ABI_SUFFIX
#endif

#endif // !defined(BOOST_CPP_EXCEPTIONS_HPP_5190E447_A781_4521_A275_5134FF9917D7_INCLUDED)

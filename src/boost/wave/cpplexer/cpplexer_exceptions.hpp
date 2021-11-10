/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library

    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#if !defined(BOOST_CPPLEXER_EXCEPTIONS_HPP_1A09DE1A_6D1F_4091_AF7F_5F13AB0D31AB_INCLUDED)
#define BOOST_CPPLEXER_EXCEPTIONS_HPP_1A09DE1A_6D1F_4091_AF7F_5F13AB0D31AB_INCLUDED

#include <exception>
#include <string>

#include <boost/assert.hpp>
#include <boost/config.hpp>
#include <boost/throw_exception.hpp>

#include <boost/wave/wave_config.hpp>

// this must occur after all of the includes and before any code appears
#ifdef BOOST_HAS_ABI_HEADERS
#include BOOST_ABI_PREFIX
#endif

///////////////////////////////////////////////////////////////////////////////
// helper macro for throwing exceptions
#if !defined(BOOST_WAVE_LEXER_THROW)
#ifdef BOOST_NO_STRINGSTREAM
#include <strstream>
#define BOOST_WAVE_LEXER_THROW(cls, code, msg, line, column, name)            \
    {                                                                         \
        using namespace boost::wave;                                          \
        std::strstream stream;                                                \
        stream << cls::severity_text(cls::code) << ": "                       \
               << cls::error_text(cls::code);                                 \
        if ((msg)[0] != 0) stream << ": " << (msg);                           \
        stream << std::ends;                                                  \
        std::string throwmsg = stream.str(); stream.freeze(false);            \
        boost::throw_exception(cls(throwmsg.c_str(), cls::code, line, column, \
            name));                                                           \
    }                                                                         \
    /**/
#else
#include <sstream>
#define BOOST_WAVE_LEXER_THROW(cls, code, msg, line, column, name)                \
    {                                                                             \
        using namespace boost::wave;                                              \
        std::stringstream stream;                                                 \
        stream << cls::severity_text(cls::code) << ": "                           \
               << cls::error_text(cls::code);                                     \
        if ((msg)[0] != 0) stream << ": " << (msg);                               \
        stream << std::ends;                                                      \
        boost::throw_exception(cls(stream.str().c_str(), cls::code, line, column, \
            name));                                                               \
    }                                                                             \
    /**/
#endif // BOOST_NO_STRINGSTREAM
#endif // BOOST_WAVE_LEXER_THROW

#if !defined(BOOST_WAVE_LEXER_THROW_VAR)
#ifdef BOOST_NO_STRINGSTREAM
#include <strstream>
#define BOOST_WAVE_LEXER_THROW_VAR(cls, codearg, msg, line, column, name) \
    {                                                                     \
        using namespace boost::wave;                                      \
        cls::error_code code = static_cast<cls::error_code>(codearg);     \
        std::strstream stream;                                            \
        stream << cls::severity_text(code) << ": "                        \
               << cls::error_text(code);                                  \
        if ((msg)[0] != 0) stream << ": " << (msg);                       \
        stream << std::ends;                                              \
        std::string throwmsg = stream.str(); stream.freeze(false);        \
        boost::throw_exception(cls(throwmsg.c_str(), code, line, column,  \
            name));                                                       \
    }                                                                     \
    /**/
#else
#include <sstream>
#define BOOST_WAVE_LEXER_THROW_VAR(cls, codearg, msg, line, column, name)    \
    {                                                                        \
        using namespace boost::wave;                                         \
        cls::error_code code = static_cast<cls::error_code>(codearg);        \
        std::stringstream stream;                                            \
        stream << cls::severity_text(code) << ": "                           \
               << cls::error_text(code);                                     \
        if ((msg)[0] != 0) stream << ": " << (msg);                          \
        stream << std::ends;                                                 \
        boost::throw_exception(cls(stream.str().c_str(), code, line, column, \
            name));                                                          \
    }                                                                        \
    /**/
#endif // BOOST_NO_STRINGSTREAM
#endif // BOOST_WAVE_LEXER_THROW

///////////////////////////////////////////////////////////////////////////////
namespace boost {
namespace wave {
namespace cpplexer {

///////////////////////////////////////////////////////////////////////////////
// exception severity
namespace util {

    enum severity {
        severity_remark = 0,
        severity_warning,
        severity_error,
        severity_fatal
    };

    inline char const *
    get_severity(severity level)
    {
        static char const *severity_text[] =
        {
            "remark",           // severity_remark
            "warning",          // severity_warning
            "error",            // severity_error
            "fatal error"       // severity_fatal
        };
        BOOST_ASSERT(severity_remark <= level && level <= severity_fatal);
        return severity_text[level];
    }
}

///////////////////////////////////////////////////////////////////////////////
//  cpplexer_exception, the base class for all specific C++ lexer exceptions
class BOOST_SYMBOL_VISIBLE cpplexer_exception
:   public std::exception
{
public:
    cpplexer_exception(std::size_t line_, std::size_t column_, char const *filename_) throw()
    :   line(line_), column(column_)
    {
        unsigned int off = 0;
        while (off < sizeof(filename)-1 && *filename_)
            filename[off++] = *filename_++;
        filename[off] = 0;
    }
    ~cpplexer_exception() throw() {}

    char const *what() const throw() BOOST_OVERRIDE = 0;   // to be overloaded
    virtual char const *description() const throw() = 0;
    virtual int get_errorcode() const throw() = 0;
    virtual int get_severity() const throw() = 0;
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
// lexing_exception error
class BOOST_SYMBOL_VISIBLE lexing_exception :
    public cpplexer_exception
{
public:
    enum error_code {
        unexpected_error = 0,
        universal_char_invalid = 1,
        universal_char_base_charset = 2,
        universal_char_not_allowed = 3,
        invalid_long_long_literal = 4,
        generic_lexing_error = 5,
        generic_lexing_warning = 6
    };

    lexing_exception(char const *what_, error_code code, std::size_t line_,
        std::size_t column_, char const *filename_) throw()
    :   cpplexer_exception(line_, column_, filename_),
        level(severity_level(code)), code(code)
    {
        unsigned int off = 0;
        while (off < sizeof(buffer)-1 && *what_)
            buffer[off++] = *what_++;
        buffer[off] = 0;
    }
    ~lexing_exception() throw() {}

    char const *what() const throw() BOOST_OVERRIDE
    {
        return "boost::wave::lexing_exception";
    }
    char const *description() const throw() BOOST_OVERRIDE
    {
        return buffer;
    }
    int get_severity() const throw() BOOST_OVERRIDE
    {
        return level;
    }
    int get_errorcode() const throw() BOOST_OVERRIDE
    {
        return code;
    }
    bool is_recoverable() const throw() BOOST_OVERRIDE
    {
        switch (get_errorcode()) {
        case lexing_exception::universal_char_invalid:
        case lexing_exception::universal_char_base_charset:
        case lexing_exception::universal_char_not_allowed:
        case lexing_exception::invalid_long_long_literal:
        case lexing_exception::generic_lexing_warning:
        case lexing_exception::generic_lexing_error:
            return true;    // for now allow all exceptions to be recoverable

        case lexing_exception::unexpected_error:
        default:
            break;
        }
        return false;
    }

    static char const *error_text(int code)
    {
    // error texts in this array must appear in the same order as the items in
    // the error enum above
        static char const *preprocess_exception_errors[] = {
            "unexpected error (should not happen)",     // unexpected_error
            "universal character name specifies an invalid character",  // universal_char_invalid
            "a universal character name cannot designate a character in the "
                "basic character set",                  // universal_char_base_charset
            "this universal character is not allowed in an identifier", // universal_char_not_allowed
            "long long suffixes are not allowed in pure C++ mode, "
            "enable long_long mode to allow these",     // invalid_long_long_literal
            "generic lexer error",                      // generic_lexing_error
            "generic lexer warning"                     // generic_lexing_warning
        };
        return preprocess_exception_errors[code];
    }

    static util::severity severity_level(int code)
    {
        static util::severity preprocess_exception_severity[] = {
            util::severity_fatal,               // unexpected_error
            util::severity_error,               // universal_char_invalid
            util::severity_error,               // universal_char_base_charset
            util::severity_error,               // universal_char_not_allowed
            util::severity_warning,             // invalid_long_long_literal
            util::severity_error,               // generic_lexing_error
            util::severity_warning              // invalid_long_long_literal
        };
        return preprocess_exception_severity[code];
    }
    static char const *severity_text(int code)
    {
        return util::get_severity(severity_level(code));
    }

private:
    char buffer[512];
    util::severity level;
    error_code code;
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
is_recoverable(lexing_exception const& e)
{
    return e.is_recoverable();
}

///////////////////////////////////////////////////////////////////////////////
}   // namespace cpplexer
}   // namespace wave
}   // namespace boost

// the suffix header occurs after all of the code
#ifdef BOOST_HAS_ABI_HEADERS
#include BOOST_ABI_SUFFIX
#endif

#endif // !defined(BOOST_CPPLEXER_EXCEPTIONS_HPP_1A09DE1A_6D1F_4091_AF7F_5F13AB0D31AB_INCLUDED)

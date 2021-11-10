/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library

    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#if !defined(BOOST_WAVE_CPP_THROW_HPP_INCLUDED)
#define BOOST_WAVE_CPP_THROW_HPP_INCLUDED

#include <string>
#include <boost/throw_exception.hpp>

#ifdef BOOST_NO_STRINGSTREAM
#include <strstream>
#else
#include <sstream>
#endif

namespace boost { namespace wave { namespace util
{
#ifdef BOOST_NO_STRINGSTREAM
    template <typename Exception, typename S1, typename Pos>
    void throw_(typename Exception::error_code code, S1 msg, Pos const& pos)
    {
        std::strstream stream;
        stream << Exception::severity_text(code) << ": "
               << Exception::error_text(code);
        if (msg[0] != 0)
            stream << ": " << msg;
        stream << std::ends;
        std::string throwmsg = stream.str(); stream.freeze(false);
        boost::throw_exception(Exception(throwmsg.c_str(), code,
            pos.get_line(), pos.get_column(), pos.get_file().c_str()));
    }

    template <typename Exception, typename Context, typename S1, typename Pos>
    void throw_(Context& ctx, typename Exception::error_code code,
        S1 msg, Pos const& pos)
    {
        std::strstream stream;
        stream << Exception::severity_text(code) << ": "
               << Exception::error_text(code);
        if (msg[0] != 0)
            stream << ": " << msg;
        stream << std::ends;
        std::string throwmsg = stream.str(); stream.freeze(false);
        ctx.get_hooks().throw_exception(ctx.derived(),
            Exception(throwmsg.c_str(), code, pos.get_line(), pos.get_column(),
                pos.get_file().c_str()));
    }

    template <typename Exception, typename S1, typename Pos, typename S2>
    void throw_(typename Exception::error_code code, S1 msg, Pos const& pos,
        S2 name)
    {
        std::strstream stream;
        stream << Exception::severity_text(code) << ": "
               << Exception::error_text(code);
        if (msg[0] != 0)
            stream << ": " << msg;
        stream << std::ends;
        std::string throwmsg = stream.str(); stream.freeze(false);
        boost::throw_exception(Exception(throwmsg.c_str(), code,
            pos.get_line(), pos.get_column(), pos.get_file().c_str(), name));
    }

    template <typename Exception, typename Context, typename S1, typename Pos,
        typename S2>
    void throw_(Context& ctx, typename Exception::error_code code,
        S1 msg, Pos const& pos, S2 name)
    {
        std::strstream stream;
        stream << Exception::severity_text(code) << ": "
               << Exception::error_text(code);
        if (msg[0] != 0)
            stream << ": " << msg;
        stream << std::ends;
        std::string throwmsg = stream.str(); stream.freeze(false);
        ctx.get_hooks().throw_exception(ctx.derived(),
            Exception(throwmsg.c_str(), code, pos.get_line(), pos.get_column(),
                pos.get_file().c_str(), name));
    }
#else
    template <typename Exception, typename S1, typename Pos>
    void throw_(typename Exception::error_code code, S1 msg, Pos const& pos)
    {
        std::stringstream stream;
        stream << Exception::severity_text(code) << ": "
               << Exception::error_text(code);
        if (msg[0] != 0)
            stream << ": " << msg;
        stream << std::ends;
        std::string throwmsg = stream.str();
        boost::throw_exception(Exception(throwmsg.c_str(), code,
            pos.get_line(), pos.get_column(), pos.get_file().c_str()));
    }

    template <typename Exception, typename Context, typename S1, typename Pos>
    void throw_(Context& ctx, typename Exception::error_code code,
        S1 msg, Pos const& pos)
    {
        std::stringstream stream;
        stream << Exception::severity_text(code) << ": "
               << Exception::error_text(code);
        if (msg[0] != 0)
            stream << ": " << msg;
        stream << std::ends;
        std::string throwmsg = stream.str();
        ctx.get_hooks().throw_exception(ctx.derived(),
            Exception(throwmsg.c_str(), code, pos.get_line(), pos.get_column(),
                pos.get_file().c_str()));
    }

    template <typename Exception, typename S1, typename Pos, typename S2>
    void throw_(typename Exception::error_code code, S1 msg, Pos const& pos,
        S2 name)
    {
        std::stringstream stream;
        stream << Exception::severity_text(code) << ": "
               << Exception::error_text(code);
        if (msg[0] != 0)
            stream << ": " << msg;
        stream << std::ends;
        std::string throwmsg = stream.str();
        boost::throw_exception(Exception(throwmsg.c_str(), code,
            pos.get_line(), pos.get_column(), pos.get_file().c_str(), name));
    }

    template <typename Exception, typename Context, typename S1, typename Pos1,
        typename S2>
    void throw_(Context& ctx, typename Exception::error_code code,
        S1 msg, Pos1 const& pos, S2 name)
    {
        std::stringstream stream;
        stream << Exception::severity_text(code) << ": "
               << Exception::error_text(code);
        if (msg[0] != 0)
            stream << ": " << msg;
        stream << std::ends;
        std::string throwmsg = stream.str();
        ctx.get_hooks().throw_exception(ctx.derived(),
            Exception(throwmsg.c_str(), code, pos.get_line(), pos.get_column(),
                pos.get_file().c_str(), name));
    }
#endif
}}}

///////////////////////////////////////////////////////////////////////////////
// helper macro for throwing exceptions
#if !defined(BOOST_WAVE_THROW)
#define BOOST_WAVE_THROW(cls, code, msg, act_pos)                             \
    boost::wave::util::throw_<cls>(cls::code, msg, act_pos)                   \
    /**/

#define BOOST_WAVE_THROW_CTX(ctx, cls, code, msg, act_pos)                    \
    boost::wave::util::throw_<cls>(ctx, cls::code, msg, act_pos)              \
    /**/
#endif // BOOST_WAVE_THROW

///////////////////////////////////////////////////////////////////////////////
// helper macro for throwing exceptions with additional parameter
#if !defined(BOOST_WAVE_THROW_NAME_CTX)
#define BOOST_WAVE_THROW_NAME_CTX(ctx, cls, code, msg, act_pos, name)         \
    boost::wave::util::throw_<cls>(cls::code, msg, act_pos, name)             \
    /**/
#endif // BOOST_WAVE_THROW_NAME_CTX

///////////////////////////////////////////////////////////////////////////////
// helper macro for throwing exceptions with additional parameter
#if !defined(BOOST_WAVE_THROW_VAR_CTX)
#define BOOST_WAVE_THROW_VAR_CTX(ctx, cls, code, msg, act_pos)                \
    boost::wave::util::throw_<cls>(ctx, code, msg, act_pos)                   \
    /**/
#endif // BOOST_WAVE_THROW_VAR_CTX

#endif // !defined(BOOST_WAVE_CPP_THROW_HPP_INCLUDED)

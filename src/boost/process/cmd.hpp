// Copyright (c) 2006, 2007 Julio M. Merino Vidal
// Copyright (c) 2008 Ilya Sokolov, Boris Schaeling
// Copyright (c) 2009 Boris Schaeling
// Copyright (c) 2010 Felipe Tanus, Boris Schaeling
// Copyright (c) 2011, 2012 Jeff Flinn, Boris Schaeling
// Copyright (c) 2016 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PROCESS_DETAIL_CMD_LINE_HPP
#define BOOST_PROCESS_DETAIL_CMD_LINE_HPP

#include <boost/winapi/config.hpp>
#include <boost/process/detail/config.hpp>
#include <boost/process/detail/handler_base.hpp>
#include <boost/process/detail/traits/cmd_or_exe.hpp>
#include <boost/process/detail/traits/wchar_t.hpp>

#if defined(BOOST_POSIX_API)
#include <boost/process/detail/posix/cmd.hpp>
#elif defined(BOOST_WINDOWS_API)
#include <boost/process/detail/windows/cmd.hpp>
#endif

/** \file boost/process/cmd.hpp
 *
 *    This header provides the \xmlonly <globalname alt="boost::process::cmd">cmd</globalname>\endxmlonly property.
 *
\xmlonly
<programlisting>
namespace boost {
  namespace process {
    <emphasis>unspecified</emphasis> <globalname alt="boost::process::cmd">cmd</globalname>;
  }
}
</programlisting>
\endxmlonly
*/

namespace boost { namespace process { namespace detail {


struct cmd_
{
    constexpr cmd_() = default;

    template<typename Char>
    inline api::cmd_setter_<Char> operator()(const Char *s) const
    {
        return api::cmd_setter_<Char>(s);
    }
    template<typename Char>
    inline api::cmd_setter_<Char> operator= (const Char *s) const
    {
        return api::cmd_setter_<Char>(s);
    }

    template<typename Char>
    inline api::cmd_setter_<Char> operator()(const std::basic_string<Char> &s) const
    {
        return api::cmd_setter_<Char>(s);
    }
    template<typename Char>
    inline api::cmd_setter_<Char> operator= (const std::basic_string<Char> &s) const
    {
        return api::cmd_setter_<Char>(s);
    }
};

template<> struct is_wchar_t<api::cmd_setter_<wchar_t>> : std::true_type {};



template<>
struct char_converter<char, api::cmd_setter_<wchar_t>>
{
    static api::cmd_setter_<char> conv(const api::cmd_setter_<wchar_t> & in)
    {
        return { ::boost::process::detail::convert(in.str()) };
    }
};

template<>
struct char_converter<wchar_t, api::cmd_setter_<char>>
{
    static api::cmd_setter_<wchar_t> conv(const api::cmd_setter_<char> & in)
    {
        return { ::boost::process::detail::convert(in.str()) };
    }
};






}


/** The cmd property allows to explicitly set commands for the execution.

The overload form applies when only one string is passed to a launching function.
The string will be internally parsed and split at spaces.

The following expressions are valid, with `value` being either a C-String or
a `std::basic_string` with `char` or `wchar_t`.

\code{.cpp}
cmd="value";
cmd(value);
\endcode

The property can only be used for assignments.


 */
constexpr static ::boost::process::detail::cmd_ cmd;

}}

#endif

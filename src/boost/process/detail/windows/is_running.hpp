// Copyright (c) 2106 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PROCESS_WINDOWS_IS_RUNNING_HPP
#define BOOST_PROCESS_WINDOWS_IS_RUNNING_HPP

#include <boost/process/detail/config.hpp>
#include <system_error>
#include <cstdlib>
#include <boost/winapi/process.hpp>

namespace boost { namespace process { namespace detail { namespace windows {

constexpr static ::boost::winapi::DWORD_ still_active = 259;


struct child_handle;

inline bool is_running(const child_handle &p, int & exit_code, std::error_code &ec) noexcept
{
    ::boost::winapi::DWORD_ code;
    //single value, not needed in the winapi.
    if (!::boost::winapi::GetExitCodeProcess(p.process_handle(), &code))
        ec = ::boost::process::detail::get_last_error();
    else
        ec.clear();

    if (code == still_active)
        return true;
    else
    {
        exit_code = code;
        return false;
    }
}

inline bool is_running(const child_handle &p, int & exit_code)
{
    std::error_code ec;
    bool b = is_running(p, exit_code, ec);
    boost::process::detail::throw_error(ec, "GetExitCodeProcess() failed in is_running");
    return b;
}

inline bool is_running(int code)
{
    return code == still_active;
}

inline int eval_exit_status(int in ) {return in;}

}}}}

#endif

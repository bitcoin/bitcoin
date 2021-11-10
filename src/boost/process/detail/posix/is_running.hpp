// Copyright (c) 2106 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PROCESS_DETAIL_POSIX_IS_RUNNING_HPP
#define BOOST_PROCESS_DETAIL_POSIX_IS_RUNNING_HPP

#include <boost/process/detail/config.hpp>
#include <boost/process/detail/posix/child_handle.hpp>
#include <system_error>
#include <sys/wait.h>

namespace boost { namespace process { namespace detail { namespace posix {

// Use the "stopped" state (WIFSTOPPED) to indicate "not terminated".
// This bit arrangement of status codes is not guaranteed by POSIX, but (according to comments in
// the glibc <bits/waitstatus.h> header) is the same across systems in practice.
constexpr int still_active = 0x017f;
static_assert(WIFSTOPPED(still_active), "Expected still_active to indicate WIFSTOPPED");
static_assert(!WIFEXITED(still_active), "Expected still_active to not indicate WIFEXITED");
static_assert(!WIFSIGNALED(still_active), "Expected still_active to not indicate WIFSIGNALED");
static_assert(!WIFCONTINUED(still_active), "Expected still_active to not indicate WIFCONTINUED");

inline bool is_running(int code)
{
    return !WIFEXITED(code) && !WIFSIGNALED(code);
}

inline bool is_running(const child_handle &p, int & exit_code, std::error_code &ec) noexcept
{
    int status;
    auto ret = ::waitpid(p.pid, &status, WNOHANG);

    if (ret == -1)
    {
        if (errno != ECHILD) //because it no child is running, than this one isn't either, obviously.
            ec = ::boost::process::detail::get_last_error();
        return false;
    }
    else if (ret == 0)
        return true;
    else
    {
        ec.clear();

        if (!is_running(status))
            exit_code = status;

        return false;
    }
}

inline bool is_running(const child_handle &p, int & exit_code)
{
    std::error_code ec;
    bool b = is_running(p, exit_code, ec);
    boost::process::detail::throw_error(ec, "waitpid(2) failed in is_running");
    return b;
}

inline int eval_exit_status(int code)
{
    if (WIFEXITED(code))
    {
        return WEXITSTATUS(code);
    }
    else if (WIFSIGNALED(code))
    {
        return WTERMSIG(code);
    }
    else
    {
        return code;
    }
}

}}}}

#endif

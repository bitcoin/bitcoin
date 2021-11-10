// Copyright (c) 2006, 2007 Julio M. Merino Vidal
// Copyright (c) 2008 Ilya Sokolov, Boris Schaeling
// Copyright (c) 2009 Boris Schaeling
// Copyright (c) 2010 Felipe Tanus, Boris Schaeling
// Copyright (c) 2011, 2012 Jeff Flinn, Boris Schaeling
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PROCESS_DETAIL_POSIX_TERMINATE_HPP
#define BOOST_PROCESS_DETAIL_POSIX_TERMINATE_HPP

#include <boost/process/detail/config.hpp>
#include <boost/process/detail/posix/child_handle.hpp>
#include <system_error>
#include <signal.h>
#include <sys/wait.h>


namespace boost { namespace process { namespace detail { namespace posix {

inline void terminate(const child_handle &p, std::error_code &ec) noexcept
{
    if (::kill(p.pid, SIGKILL) == -1)
        ec = boost::process::detail::get_last_error();
    else
        ec.clear();

    int status;
    ::waitpid(p.pid, &status, WNOHANG); //just to clean it up
}

inline void terminate(const child_handle &p)
{
    std::error_code ec;
    terminate(p, ec);
    boost::process::detail::throw_error(ec, "kill(2) failed");
}

}}}}

#endif

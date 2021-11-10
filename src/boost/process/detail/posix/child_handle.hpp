// Copyright (c) 2006, 2007 Julio M. Merino Vidal
// Copyright (c) 2008 Ilya Sokolov, Boris Schaeling
// Copyright (c) 2009 Boris Schaeling
// Copyright (c) 2010 Felipe Tanus, Boris Schaeling
// Copyright (c) 2011, 2012 Jeff Flinn, Boris Schaeling
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PROCESS_POSIX_CHILD_HPP
#define BOOST_PROCESS_POSIX_CHILD_HPP

#include <utility>
#include <system_error>

namespace boost { namespace process { namespace detail { namespace posix {

typedef ::pid_t pid_t;

struct child_handle
{
    int pid {-1};
    explicit child_handle(int pid) : pid(pid)
    {}

    child_handle()  = default;
    ~child_handle() = default;

    child_handle(const child_handle & c) = delete;
    child_handle(child_handle && c) : pid(c.pid)
    {
        c.pid = -1;
    }
    child_handle &operator=(const child_handle & c) = delete;
    child_handle &operator=(child_handle && c)
    {
        pid = c.pid;
        c.pid = -1;
        return *this;
    }

    int id() const
    {
        return pid;
    }
    bool in_group() const {return true;}
    bool in_group(std::error_code&) const noexcept {return true;}

    typedef int process_handle_t;
    process_handle_t process_handle() const { return pid; }

    bool valid() const
    {
        return pid != -1;
    }
};

}}}}

#endif

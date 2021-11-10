// Copyright (c) 2006, 2007 Julio M. Merino Vidal
// Copyright (c) 2008 Ilya Sokolov, Boris Schaeling
// Copyright (c) 2009 Boris Schaeling
// Copyright (c) 2010 Felipe Tanus, Boris Schaeling
// Copyright (c) 2011, 2012 Jeff Flinn, Boris Schaeling
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PROCESS_DETAIL_POSIX_FILE_IN_HPP
#define BOOST_PROCESS_DETAIL_POSIX_FILE_IN_HPP

#include <boost/process/pipe.hpp>
#include <boost/process/detail/posix/handler.hpp>
#include <boost/process/detail/posix/file_descriptor.hpp>
#include <boost/process/detail/used_handles.hpp>
#include <cstdio>
#include <unistd.h>

namespace boost { namespace process { namespace detail { namespace posix {

struct file_in : handler_base_ext, ::boost::process::detail::uses_handles
{
    file_descriptor file;
    int handle = file.handle();

    std::array<int, 2> get_used_handles()
    {
        return {{STDIN_FILENO, handle}};
    }

    template<typename T>
    file_in(T&& t) : file(std::forward<T>(t)) {}
    file_in(FILE * f) : handle(fileno(f)) {}

    template <class WindowsExecutor>
    void on_exec_setup(WindowsExecutor &e) const
    {
        if (::dup2(handle, STDIN_FILENO) == -1)
             e.set_error(::boost::process::detail::get_last_error(), "dup2() failed");
    }
};

}}}}

#endif

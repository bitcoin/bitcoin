// Copyright (c) 2006, 2007 Julio M. Merino Vidal
// Copyright (c) 2008 Ilya Sokolov, Boris Schaeling
// Copyright (c) 2009 Boris Schaeling
// Copyright (c) 2010 Felipe Tanus, Boris Schaeling
// Copyright (c) 2011, 2012 Jeff Flinn, Boris Schaeling
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PROCESS_DETAIL_WINDOWS_FILE_IN_HPP
#define BOOST_PROCESS_DETAIL_WINDOWS_FILE_IN_HPP

#include <boost/winapi/process.hpp>
#include <boost/winapi/handles.hpp>
#include <boost/process/detail/handler_base.hpp>
#include <boost/process/detail/used_handles.hpp>
#include <boost/process/detail/windows/file_descriptor.hpp>
#include <io.h>

namespace boost { namespace process { namespace detail { namespace windows {

struct file_in : public ::boost::process::detail::handler_base,
                        ::boost::process::detail::uses_handles
{
    file_descriptor file;
    ::boost::winapi::HANDLE_ handle = file.handle();

    ::boost::winapi::HANDLE_ get_used_handles() const { return handle; }

    template<typename T>
    file_in(T&& t) : file(std::forward<T>(t), file_descriptor::read) {}
    file_in(FILE * f) : handle(reinterpret_cast<::boost::winapi::HANDLE_>(_get_osfhandle(_fileno(f)))) {}

    template <class WindowsExecutor>
    void on_setup(WindowsExecutor &e) const
    {
        boost::winapi::SetHandleInformation(handle,
                boost::winapi::HANDLE_FLAG_INHERIT_,
                boost::winapi::HANDLE_FLAG_INHERIT_);
        e.startup_info.hStdInput = handle;
        e.startup_info.dwFlags  |= boost::winapi::STARTF_USESTDHANDLES_;
        e.inherit_handles = true;
    }
};

}}}}

#endif

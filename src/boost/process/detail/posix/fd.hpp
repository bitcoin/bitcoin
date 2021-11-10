// Copyright (c) 2006, 2007 Julio M. Merino Vidal
// Copyright (c) 2008 Ilya Sokolov, Boris Schaeling
// Copyright (c) 2009 Boris Schaeling
// Copyright (c) 2010 Felipe Tanus, Boris Schaeling
// Copyright (c) 2011, 2012 Jeff Flinn, Boris Schaeling
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PROCESS_DETAIL_POSIX_FD_HPP
#define BOOST_PROCESS_DETAIL_POSIX_FD_HPP

#include <boost/process/detail/posix/handler.hpp>
#include <unistd.h>
#include <boost/process/detail/used_handles.hpp>
#include <array>

namespace boost { namespace process { namespace detail { namespace posix {


struct close_fd_ : handler_base_ext, ::boost::process::detail::uses_handles
{
    close_fd_(int fd) : fd_(fd) {}

    template <class PosixExecutor>
    void on_exec_setup(PosixExecutor& e) const
    {
        if (::close(fd_) == -1)
            e.set_error(::boost::process::detail::get_last_error(), "close() failed");
    }

    int get_used_handles() {return fd_;}


private:
    int fd_;
};

template <class Range>
struct close_fds_ : handler_base_ext, ::boost::process::detail::uses_handles
{
public:
    close_fds_(const Range &fds) : fds_(fds) {}

    template <class PosixExecutor>
    void on_exec_setup(PosixExecutor& e) const
    {
        for (auto & fd_ : fds_)
            if (::close(fd_) == -1)
            {
                 e.set_error(::boost::process::detail::get_last_error(), "close() failed");
                 break;
            }
    }

    Range& get_used_handles() {return fds_;}

private:
    Range fds_;
};



template <class FileDescriptor>
struct bind_fd_ : handler_base_ext, ::boost::process::detail::uses_handles
{
public:
    bind_fd_(int id, const FileDescriptor &fd) : id_(id), fd_(fd) {}

    template <class PosixExecutor>
    void on_exec_setup(PosixExecutor& e) const
    {
        if (::dup2(fd_, id_) == -1)
             e.set_error(::boost::process::detail::get_last_error(), "dup2() failed");
    }

    std::array<int, 2> get_used_handles() {return {id_, fd_};}


private:
    int id_;
    FileDescriptor fd_;
};


struct fd_
{
    constexpr fd_() {};
    close_fd_ close(int _fd) const {return close_fd_(_fd);}
    close_fds_<std::vector<int>> close(const std::initializer_list<int> & vec) const {return std::vector<int>(vec);}
    template<typename Range>
    close_fds_<Range> close(const Range & r) const {return r;}

    template <class FileDescriptor>
    bind_fd_<FileDescriptor> bind(int id, const FileDescriptor & fd) const {return {id, fd};}

};


}}}}

#endif

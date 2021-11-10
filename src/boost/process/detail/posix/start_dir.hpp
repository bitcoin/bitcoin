// Copyright (c) 2006, 2007 Julio M. Merino Vidal
// Copyright (c) 2008 Ilya Sokolov, Boris Schaeling
// Copyright (c) 2009 Boris Schaeling
// Copyright (c) 2010 Felipe Tanus, Boris Schaeling
// Copyright (c) 2011, 2012 Jeff Flinn, Boris Schaeling
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PROCESS_DETAIL_POSIX_START_DIR_HPP
#define BOOST_PROCESS_DETAIL_POSIX_START_DIR_HPP

#include <boost/process/detail/posix/handler.hpp>
#include <string>
#include <unistd.h>

namespace boost { namespace process { namespace detail { namespace posix {

template<typename Char>
struct start_dir_init : handler_base_ext
{
    typedef Char value_type;
    typedef std::basic_string<value_type> string_type;
    start_dir_init(string_type s) : s_(std::move(s)) {}

    template <class PosixExecutor>
    void on_exec_setup(PosixExecutor&) const
    {
        ::chdir(s_.c_str());
    }
    const string_type & str() const {return s_;}
private:
    string_type s_;
};

}}}}

#endif

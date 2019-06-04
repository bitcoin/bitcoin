//
// Copyright (C) 2011-2019 Ben Key
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//



#if !( \
    BOOST_OS_ANDROID || BOOST_OS_BSD_DRAGONFLY || BOOST_OS_BSD_FREE || BOOST_OS_BSD_NET \
    || BOOST_OS_CYGWIN || BOOST_OS_HPUX || BOOST_OS_IOS || BOOST_OS_LINUX || BOOST_OS_MACOS \
    || BOOST_OS_QNX || BOOST_OS_SOLARIS || BOOST_OS_UNIX || BOOST_OS_WINDOWS)

#include <string>

#include <util/executable_path/include/boost/detail/executable_path_internals.hpp>

namespace boost {
namespace detail {

boost::filesystem::path executable_path_worker()
{
    boost::filesystem::path ret;
    return ret;
}

} // namespace detail
} // namespace boost

#endif

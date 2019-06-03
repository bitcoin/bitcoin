//
// Copyright (C) 2011-2019 Ben Key
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//


#if (BOOST_OS_ANDROID || BOOST_OS_HPUX || BOOST_OS_LINUX || BOOST_OS_UNIX)

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include <util/executable_path/include/boost/detail/executable_path_internals.hpp>

namespace boost {
namespace detail {

boost::filesystem::path executable_path_worker()
{
    boost::filesystem::path ret;
    boost::system::error_code ec;
    auto linkPath = boost::filesystem::read_symlink("/proc/self/exe", ec);
    if (ec.value() != boost::system::errc::success)
    {
        return ret;
    }
    ret = boost::filesystem::canonical(linkPath, boost::filesystem::current_path(), ec);
    if (ec.value() != boost::system::errc::success)
    {
        ret.clear();
    }
    return ret;
}

} // namespace detail
} // namespace boost

#endif

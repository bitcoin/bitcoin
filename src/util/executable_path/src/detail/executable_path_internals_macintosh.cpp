//
// Copyright (C) 2011-2019 Ben Key
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//



#if (BOOST_OS_MACOS || BOOST_OS_IOS)

#include <algorithm>
#include <iterator>
#include <string>
#include <vector>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include <util/executable_path/include/boost/detail/executable_path_internals.hpp>

#include <mach-o/dyld.h>

namespace boost {
namespace detail {

boost::filesystem::path executable_path_worker()
{
    using char_vector = std::vector<char>;
    boost::filesystem::path ret;
    char_vector buf(1024, 0);
    uint32_t size = static_cast<uint32_t>(buf.size());
    bool havePath = false;
    bool shouldContinue = true;
    do
    {
        int result = _NSGetExecutablePath(buf.data(), &size);
        if (result == -1)
        {
            buf.resize(size + 1);
            std::fill(std::begin(buf), std::end(buf), 0);
        }
        else
        {
            shouldContinue = false;
            if (buf.at(0) != 0)
            {
                havePath = true;
            }
        }
    } while (shouldContinue);
    if (!havePath)
    {
        return ret;
    }
    std::string pathString(buf.data(), size);
    boost::system::error_code ec;
    ret = boost::filesystem::canonical(pathString, boost::filesystem::current_path(), ec);
    if (ec.value() != boost::system::errc::success)
    {
        ret.clear();
    }
    return ret;
}

} // namespace detail
} // namespace boost

#endif

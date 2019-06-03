//
// Copyright (C) 2011-2019 Ben Key
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include <string>


#include <util/executable_path/include/boost/executable_path.hpp>
#include <util/executable_path/include/boost/detail/executable_path_internals.hpp>

namespace boost {

std::string executable_path(const char* argv0)
{
    boost::filesystem::path ret = detail::executable_path_worker();
    if (ret.empty())
    {
        ret = detail::executable_path_fallback(argv0);
    }
    return ret.make_preferred().string();
}

std::wstring executable_path(const wchar_t* argv0)
{
    boost::filesystem::path ret = detail::executable_path_worker();
    if (ret.empty())
    {
        ret = detail::executable_path_fallback(argv0);
    }
    return ret.make_preferred().wstring();
}

} // namespace boost


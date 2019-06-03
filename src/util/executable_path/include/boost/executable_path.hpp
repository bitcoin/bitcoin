//
// Copyright (C) 2011-2019 Ben Key
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SYSCOIN_UTIL_EXECUTABLE_PATH_INCLUDE_BOOST_EXECUTABLE_PATH_HPP
#define SYSCOIN_UTIL_EXECUTABLE_PATH_INCLUDE_BOOST_EXECUTABLE_PATH_HPP

#pragma once

#include <string>

namespace boost {
std::string executable_path(const char* argv0);
std::wstring executable_path(const wchar_t* argv0);
} // namespace boost

#endif // SYSCOIN_UTIL_EXECUTABLE_PATH_INCLUDE_BOOST_EXECUTABLE_PATH_HPP

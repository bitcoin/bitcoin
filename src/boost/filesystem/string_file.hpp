//  filesystem/string_file.hpp  --------------------------------------------------------//

//  Copyright Beman Dawes 2015

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt

//  Library home page: http://www.boost.org/libs/filesystem

#ifndef BOOST_FILESYSTEM_STRING_FILE_HPP
#define BOOST_FILESYSTEM_STRING_FILE_HPP

#include <cstddef>
#include <string>
#include <ios>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/operations.hpp>

#include <boost/filesystem/detail/header.hpp> // must be the last #include

namespace boost {
namespace filesystem {

inline void save_string_file(path const& p, std::string const& str)
{
    filesystem::ofstream file;
    file.exceptions(std::ios_base::failbit | std::ios_base::badbit);
    file.open(p, std::ios_base::binary);
    file.write(str.c_str(), str.size());
}

inline void load_string_file(path const& p, std::string& str)
{
    filesystem::ifstream file;
    file.exceptions(std::ios_base::failbit | std::ios_base::badbit);
    file.open(p, std::ios_base::binary);
    std::size_t sz = static_cast< std::size_t >(filesystem::file_size(p));
    str.resize(sz, '\0');
    file.read(&str[0], sz);
}

} // namespace filesystem
} // namespace boost

#include <boost/filesystem/detail/footer.hpp>

#endif // BOOST_FILESYSTEM_STRING_FILE_HPP

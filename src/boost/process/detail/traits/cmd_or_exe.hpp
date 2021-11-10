// Copyright (c) 2016 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#ifndef BOOST_PROCESS_DETAIL_TRAITS_CMD_OR_EXE_HPP_
#define BOOST_PROCESS_DETAIL_TRAITS_CMD_OR_EXE_HPP_

#include <string>
#include <vector>
#include <type_traits>
#include <initializer_list>
#include <boost/filesystem/path.hpp>
#include <boost/process/detail/traits/decl.hpp>
namespace boost { namespace process { namespace detail {

template<typename Char>
struct cmd_or_exe_tag {};

struct shell_;


template<> struct initializer_tag<const char*    > { typedef cmd_or_exe_tag<char>    type;};
template<> struct initializer_tag<const wchar_t* > { typedef cmd_or_exe_tag<wchar_t> type;};

template<> struct initializer_tag<char*    > { typedef cmd_or_exe_tag<char>     type;};
template<> struct initializer_tag<wchar_t* > { typedef cmd_or_exe_tag<wchar_t>  type;};

template<std::size_t Size> struct initializer_tag<const char    [Size]> { typedef cmd_or_exe_tag<char>     type;};
template<std::size_t Size> struct initializer_tag<const wchar_t [Size]> { typedef cmd_or_exe_tag<wchar_t>  type;};

template<std::size_t Size> struct initializer_tag<const char    (&)[Size]> { typedef cmd_or_exe_tag<char>     type;};
template<std::size_t Size> struct initializer_tag<const wchar_t (&)[Size]> { typedef cmd_or_exe_tag<wchar_t>  type;};

template<> struct initializer_tag<std::basic_string<char    >> { typedef cmd_or_exe_tag<char>     type;};
template<> struct initializer_tag<std::basic_string<wchar_t >> { typedef cmd_or_exe_tag<wchar_t>  type;};

template<> struct initializer_tag<std::vector<std::basic_string<char    >>> { typedef cmd_or_exe_tag<char>     type;};
template<> struct initializer_tag<std::vector<std::basic_string<wchar_t >>> { typedef cmd_or_exe_tag<wchar_t>  type;};

template<> struct initializer_tag<std::initializer_list<std::basic_string<char    >>> { typedef cmd_or_exe_tag<char>     type;};
template<> struct initializer_tag<std::initializer_list<std::basic_string<wchar_t >>> { typedef cmd_or_exe_tag<wchar_t>  type;};

template<> struct initializer_tag<std::vector<char    *>> { typedef cmd_or_exe_tag<char>     type;};
template<> struct initializer_tag<std::vector<wchar_t *>> { typedef cmd_or_exe_tag<wchar_t>  type;};

template<> struct initializer_tag<std::initializer_list<char    *>> { typedef cmd_or_exe_tag<char>     type;};
template<> struct initializer_tag<std::initializer_list<wchar_t *>> { typedef cmd_or_exe_tag<wchar_t>  type;};

template<> struct initializer_tag<std::initializer_list<const char    *>> { typedef cmd_or_exe_tag<char>     type;};
template<> struct initializer_tag<std::initializer_list<const wchar_t *>> { typedef cmd_or_exe_tag<wchar_t>  type;};

template<> struct initializer_tag<shell_>
{
    typedef cmd_or_exe_tag<typename boost::filesystem::path::value_type> type;
};

template<> struct initializer_tag<boost::filesystem::path>
{
    typedef cmd_or_exe_tag<typename boost::filesystem::path::value_type> type;
};

template <typename Char>
struct exe_setter_;
template <typename Char, bool Append = false>
struct arg_setter_;

template <typename Char, bool Append>
struct initializer_tag<arg_setter_<Char, Append>> { typedef cmd_or_exe_tag<Char> type;};

template<typename Char> struct initializer_tag<exe_setter_<Char>> { typedef cmd_or_exe_tag<Char> type;};

template<>
struct initializer_builder<cmd_or_exe_tag<char>>;

template<>
struct initializer_builder<cmd_or_exe_tag<wchar_t>>;


}}}



#endif /* BOOST_PROCESS_DETAIL_STRING_TRAITS_HPP_ */

// Copyright (c) 2006, 2007 Julio M. Merino Vidal
// Copyright (c) 2008 Ilya Sokolov, Boris Schaeling
// Copyright (c) 2009 Boris Schaeling
// Copyright (c) 2010 Felipe Tanus, Boris Schaeling
// Copyright (c) 2011, 2012 Jeff Flinn, Boris Schaeling
// Copyright (c) 2016 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PROCESS_ARGS_HPP
#define BOOST_PROCESS_ARGS_HPP

/** \file boost/process/args.hpp
 *
 *    This header provides the \xmlonly <globalname alt="boost::process::args">args</globalname>\endxmlonly property. It also provides the
 *    alternative name \xmlonly <globalname alt="boost::process::argv">argv</globalname>\endxmlonly .
 *
 *
\xmlonly
<programlisting>
namespace boost {
  namespace process {
    <emphasis>unspecified</emphasis> <globalname alt="boost::process::args">args</globalname>;
    <emphasis>unspecified</emphasis> <globalname alt="boost::process::argv">argv</globalname>;
  }
}
</programlisting>
\endxmlonly
 */


#include <boost/process/detail/basic_cmd.hpp>
#include <iterator>

namespace boost { namespace process { namespace detail {

struct args_
{
    template<typename T>
    using remove_reference_t = typename std::remove_reference<T>::type;
    template<typename T>
    using value_type = typename remove_reference_t<T>::value_type;

    template<typename T>
    using vvalue_type = value_type<value_type<T>>;

    template <class Range>
    arg_setter_<vvalue_type<Range>, true>     operator()(Range &&range) const
    {
        return arg_setter_<vvalue_type<Range>, true>(std::forward<Range>(range));
    }
    template <class Range>
    arg_setter_<vvalue_type<Range>, true>     operator+=(Range &&range) const
    {
        return arg_setter_<vvalue_type<Range>, true>(std::forward<Range>(range));
    }
    template <class Range>
    arg_setter_<vvalue_type<Range>, false>    operator= (Range &&range) const
    {
        return arg_setter_<vvalue_type<Range>, false>(std::forward<Range>(range));
    }
    template<typename Char>
    arg_setter_<Char, true>     operator()(std::basic_string<Char> && str) const
    {
        return arg_setter_<Char, true> (str);
    }
    template<typename Char>
    arg_setter_<Char, true>     operator+=(std::basic_string<Char> && str) const
    {
        return arg_setter_<Char, true> (str);
    }
    template<typename Char>
    arg_setter_<Char, false>    operator= (std::basic_string<Char> && str) const
    {
        return arg_setter_<Char, false>(str);
    }
    template<typename Char>
    arg_setter_<Char, true>     operator()(const std::basic_string<Char> & str) const
    {
        return arg_setter_<Char, true> (str);
    }
    template<typename Char>
    arg_setter_<Char, true>     operator+=(const std::basic_string<Char> & str) const
    {
        return arg_setter_<Char, true> (str);
    }
    template<typename Char>
    arg_setter_<Char, false>    operator= (const std::basic_string<Char> & str) const
    {
        return arg_setter_<Char, false>(str);
    }
    template<typename Char>
    arg_setter_<Char, true>     operator()(std::basic_string<Char> & str) const
    {
        return arg_setter_<Char, true> (str);
    }
    template<typename Char>
    arg_setter_<Char, true>     operator+=(std::basic_string<Char> & str) const
    {
        return arg_setter_<Char, true> (str);
    }
    template<typename Char>
    arg_setter_<Char, false>    operator= (std::basic_string<Char> & str) const
    {
        return arg_setter_<Char, false>(str);
    }
    template<typename Char>
    arg_setter_<Char, true>     operator()(const Char* str) const
    {
        return arg_setter_<Char, true> (str);
    }
    template<typename Char>
    arg_setter_<Char, true>     operator+=(const Char* str) const
    {
        return arg_setter_<Char, true> (str);
    }
    template<typename Char>
    arg_setter_<Char, false>    operator= (const Char* str) const
    {
        return arg_setter_<Char, false>(str);
    }
//    template<typename Char, std::size_t Size>
//    arg_setter_<Char, true>     operator()(const Char (&str) [Size]) const
//    {
//        return arg_setter_<Char, true> (str);
//    }
//    template<typename Char, std::size_t Size>
//    arg_setter_<Char, true>     operator+=(const Char (&str) [Size]) const
//    {
//        return arg_setter_<Char, true> (str);
//    }
//    template<typename Char, std::size_t Size>
//    arg_setter_<Char, false>    operator= (const Char (&str) [Size]) const
//    {
//        return arg_setter_<Char, false>(str);
//    }

    arg_setter_<char, true> operator()(std::initializer_list<const char*> &&range) const
    {
        return arg_setter_<char, true>(range.begin(), range.end());
    }
    arg_setter_<char, true> operator+=(std::initializer_list<const char*> &&range) const
    {
        return arg_setter_<char, true>(range.begin(), range.end());
    }
    arg_setter_<char, false> operator= (std::initializer_list<const char*> &&range) const
    {
        return arg_setter_<char, false>(range.begin(), range.end());
    }
    arg_setter_<char, true> operator()(std::initializer_list<std::string> &&range) const
    {
        return arg_setter_<char, true>(range.begin(), range.end());
    }
    arg_setter_<char, true> operator+=(std::initializer_list<std::string> &&range) const
    {
        return arg_setter_<char, true>(range.begin(), range.end());
    }
    arg_setter_<char, false> operator= (std::initializer_list<std::string> &&range) const
    {
        return arg_setter_<char, false>(range.begin(), range.end());
    }

    arg_setter_<wchar_t, true> operator()(std::initializer_list<const wchar_t*> &&range) const
    {
        return arg_setter_<wchar_t, true>(range.begin(), range.end());
    }
    arg_setter_<wchar_t, true> operator+=(std::initializer_list<const wchar_t*> &&range) const
    {
        return arg_setter_<wchar_t, true>(range.begin(), range.end());
    }
    arg_setter_<wchar_t, false> operator= (std::initializer_list<const wchar_t*> &&range) const
    {
        return arg_setter_<wchar_t, false>(range.begin(), range.end());
    }
    arg_setter_<wchar_t, true> operator()(std::initializer_list<std::wstring> &&range) const
    {
        return arg_setter_<wchar_t, true>(range.begin(), range.end());
    }
    arg_setter_<wchar_t, true> operator+=(std::initializer_list<std::wstring> &&range) const
    {
        return arg_setter_<wchar_t, true>(range.begin(), range.end());
    }
    arg_setter_<wchar_t, false> operator= (std::initializer_list<std::wstring> &&range) const
    {
        return arg_setter_<wchar_t, false>(range.begin(), range.end());
    }
};


}
/**

The `args` property allows to explicitly set arguments for the execution. The
name of the executable will always be the first element in the arg-vector.

\section args_details Details

\subsection args_operations Operations

\subsubsection args_set_var Setting values

To set a the argument vector the following syntax can be used.

\code{.cpp}
args = value;
args(value);
\endcode

`std::initializer_list` is among the allowed types, so the following syntax is also possible.

\code{.cpp}
args = {value1, value2};
args({value1, value2});
\endcode

Below the possible types for `value` are listed, with `char_type` being either `char` or `wchar_t`.

\paragraph args_set_var_value value

 - `std::basic_string<char_type>`
 - `const char_type * `
 - `std::initializer_list<const char_type *>`
 - `std::vector<std::basic_string<char_type>>`

Additionally any range of `std::basic_string<char_type>` can be passed.

\subsubsection args_append_var Appending values

To append a the argument vector the following syntax can be used.

\code{.cpp}
args += value;
\endcode

`std::initializer_list` is among the allowed types, so the following syntax is also possible.

\code{.cpp}
args += {value1, value2};
\endcode

Below the possible types for `value` are listed, with `char_type` being either `char` or `wchar_t`.

\paragraph args_append_var_value value

 - `std::basic_string<char_type>`
 - `const char_type * `
 - `std::initializer_list<const char_type *>`
 - `std::vector<std::basic_string<char_type>>`

Additionally any range of `std::basic_string<char_type>` can be passed.


\subsection args_example Example

The overload form is used when more than one string is passed, from the second one forward.
I.e. the following expressions have the same results:

\code{.cpp}
spawn("gcc", "--version");
spawn("gcc", args ="--version");
spawn("gcc", args+="--version");
spawn("gcc", args ={"--version"});
spawn("gcc", args+={"--version"});
\endcode

\note A string will be parsed and set in quotes if it has none and contains spaces.


 */
constexpr boost::process::detail::args_ args{};

///Alias for \xmlonly <globalname alt="boost::process::args">args</globalname> \endxmlonly .
constexpr boost::process::detail::args_ argv{};


}}

#endif

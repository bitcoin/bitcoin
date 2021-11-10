// Copyright (c) 2016 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PROCESS_IO_HPP_
#define BOOST_PROCESS_IO_HPP_

#include <iosfwd>
#include <cstdio>
#include <functional>
#include <utility>
#include <boost/process/detail/config.hpp>
#include <boost/process/pipe.hpp>

#include <future>

#if defined(BOOST_POSIX_API)
#include <boost/process/detail/posix/asio_fwd.hpp>
#include <boost/process/detail/posix/close_in.hpp>
#include <boost/process/detail/posix/close_out.hpp>
#include <boost/process/detail/posix/null_in.hpp>
#include <boost/process/detail/posix/null_out.hpp>
#include <boost/process/detail/posix/file_in.hpp>
#include <boost/process/detail/posix/file_out.hpp>
#include <boost/process/detail/posix/pipe_in.hpp>
#include <boost/process/detail/posix/pipe_out.hpp>
#elif defined(BOOST_WINDOWS_API)
#include <boost/process/detail/windows/asio_fwd.hpp>
#include <boost/process/detail/windows/close_in.hpp>
#include <boost/process/detail/windows/close_out.hpp>
#include <boost/process/detail/windows/null_in.hpp>
#include <boost/process/detail/windows/null_out.hpp>
#include <boost/process/detail/windows/file_in.hpp>
#include <boost/process/detail/windows/file_out.hpp>
#include <boost/process/detail/windows/pipe_in.hpp>
#include <boost/process/detail/windows/pipe_out.hpp>
#endif

/** \file boost/process/io.hpp
 *
 *    Header which provides the io properties. It provides the following properties:
 *
\xmlonly
<programlisting>
namespace boost {
  namespace process {
    <emphasis>unspecified</emphasis> <globalname alt="boost::process::close">close</globalname>;
    <emphasis>unspecified</emphasis> <globalname alt="boost::process::null">null</globalname>;
    <emphasis>unspecified</emphasis> <globalname alt="boost::process::std_in">std_in</globalname>;
    <emphasis>unspecified</emphasis> <globalname alt="boost::process::std_out">std_out</globalname>;
    <emphasis>unspecified</emphasis> <globalname alt="boost::process::std_err">std_err</globalname>;
  }
}
</programlisting>
\endxmlonly

\par File I/O

The library allows full redirection of streams to files as shown below.

\code{.cpp}
boost::filesystem::path log    = "my_log_file.txt";
boost::filesystem::path input  = "input.txt";
boost::filesystem::path output = "output.txt";
system("my_prog", std_out>output, std_in<input, std_err>log);
\endcode

\par Synchronous Pipe I/O

Another way is to communicate through pipes.

\code{.cpp}
pstream str;
child c("my_prog", std_out > str);

int i;
str >> i;
\endcode

Note that the pipe may also be used between several processes, like this:

\code{.cpp}
pipe p;
child c1("nm", "a.out", std_out>p);
child c2("c++filt", std_in<p);
\endcode

\par Asynchronous I/O

Utilizing `boost.asio` asynchronous I/O is provided.

\code
boost::asio::io_context ios;
std::future<std::string> output;
system("ls", std_out > output, ios);

auto res = fut.get();
\endcode

\note `boost/process/async.hpp` must also be included for this to work.

\par Closing

Stream can be closed, so nothing can be read or written.

\code{.cpp}
system("foo", std_in.close());
\endcode

\par Null

Streams can be redirected to null, which means, that written date will be
discarded and read data will only contain `EOF`.

\code{.cpp}
system("b2", std_out > null);
\endcode

 *
 */

namespace boost { namespace process { namespace detail {


template<typename T> using is_streambuf    = typename std::is_same<T, boost::asio::streambuf>::type;
template<typename T> using is_const_buffer =
        std::integral_constant<bool,
            std::is_same<   boost::asio::const_buffer, T>::value |
            std::is_base_of<boost::asio::const_buffer, T>::value
        >;
template<typename T> using is_mutable_buffer =
        std::integral_constant<bool,
            std::is_same<   boost::asio::mutable_buffer, T>::value |
            std::is_base_of<boost::asio::mutable_buffer, T>::value
        >;


struct null_t  {constexpr null_t() = default;};
struct close_t;

template<class>
struct std_in_
{
    constexpr std_in_() = default;

    api::close_in close() const {return api::close_in(); }
    api::close_in operator=(const close_t &) const {return api::close_in();}
    api::close_in operator<(const close_t &) const {return api::close_in();}

    api::null_in null() const {return api::null_in();}
    api::null_in operator=(const null_t &) const {return api::null_in();}
    api::null_in operator<(const null_t &) const {return api::null_in();}

    api::file_in operator=(const boost::filesystem::path &p) const {return p;}
    api::file_in operator=(const std::string & p)            const {return p;}
    api::file_in operator=(const std::wstring &p)            const {return p;}
    api::file_in operator=(const char * p)                   const {return p;}
    api::file_in operator=(const wchar_t * p)                const {return p;}

    api::file_in operator<(const boost::filesystem::path &p) const {return p;}
    api::file_in operator<(const std::string &p)             const {return p;}
    api::file_in operator<(const std::wstring &p)            const {return p;}
    api::file_in operator<(const char*p)                     const {return p;}
    api::file_in operator<(const wchar_t * p)                const {return p;}

    api::file_in operator=(FILE * f)                         const {return f;}
    api::file_in operator<(FILE * f)                         const {return f;}

    template<typename Char, typename Traits> api::pipe_in operator=(basic_pipe<Char, Traits> & p)      const {return p;}
    template<typename Char, typename Traits> api::pipe_in operator<(basic_pipe<Char, Traits> & p)      const {return p;}
    template<typename Char, typename Traits> api::pipe_in operator=(basic_opstream<Char, Traits> & p)  const {return p.pipe();}
    template<typename Char, typename Traits> api::pipe_in operator<(basic_opstream<Char, Traits> & p)  const {return p.pipe();}
    template<typename Char, typename Traits> api::pipe_in operator=(basic_pstream <Char, Traits> & p)  const {return p.pipe();}
    template<typename Char, typename Traits> api::pipe_in operator<(basic_pstream <Char, Traits> & p)  const {return p.pipe();}

    api::async_pipe_in operator=(async_pipe & p) const {return p;}
    api::async_pipe_in operator<(async_pipe & p) const {return p;}

    template<typename T, typename = typename std::enable_if<
            is_const_buffer<T>::value || is_mutable_buffer<T>::value
            >::type>
    api::async_in_buffer<const T> operator=(const T & buf) const {return buf;}
    template<typename T, typename = typename std::enable_if<is_streambuf<T>::value>::type >
    api::async_in_buffer<T>       operator=(T       & buf) const {return buf;}

    template<typename T, typename = typename std::enable_if<
            is_const_buffer<T>::value || is_mutable_buffer<T>::value
            >::type>
    api::async_in_buffer<const T> operator<(const T & buf) const {return buf;}
    template<typename T, typename = typename std::enable_if<is_streambuf<T>::value>::type >
    api::async_in_buffer<T>       operator<(T       & buf) const {return buf;}

};

//-1 == empty.
//1 == stdout
//2 == stderr
template<int p1, int p2 = -1>
struct std_out_
{
    constexpr std_out_() = default;

    api::close_out<p1,p2> close() const {return api::close_out<p1,p2>(); }
    api::close_out<p1,p2> operator=(const close_t &) const {return api::close_out<p1,p2>();}
    api::close_out<p1,p2> operator>(const close_t &) const {return api::close_out<p1,p2>();}

    api::null_out<p1,p2> null() const {return api::null_out<p1,p2>();}
    api::null_out<p1,p2> operator=(const null_t &) const {return api::null_out<p1,p2>();}
    api::null_out<p1,p2> operator>(const null_t &) const {return api::null_out<p1,p2>();}

    api::file_out<p1,p2> operator=(const boost::filesystem::path &p) const {return api::file_out<p1,p2>(p);}
    api::file_out<p1,p2> operator=(const std::string &p)             const {return api::file_out<p1,p2>(p);}
    api::file_out<p1,p2> operator=(const std::wstring &p)            const {return api::file_out<p1,p2>(p);}
    api::file_out<p1,p2> operator=(const char * p)                   const {return api::file_out<p1,p2>(p);}
    api::file_out<p1,p2> operator=(const wchar_t * p)                const {return api::file_out<p1,p2>(p);}

    api::file_out<p1,p2> operator>(const boost::filesystem::path &p) const {return api::file_out<p1,p2>(p);}
    api::file_out<p1,p2> operator>(const std::string &p)             const {return api::file_out<p1,p2>(p);}
    api::file_out<p1,p2> operator>(const std::wstring &p)            const {return api::file_out<p1,p2>(p);}
    api::file_out<p1,p2> operator>(const char * p)                   const {return api::file_out<p1,p2>(p);}
    api::file_out<p1,p2> operator>(const wchar_t * p)                const {return api::file_out<p1,p2>(p);}

    api::file_out<p1,p2> operator=(FILE * f)  const {return f;}
    api::file_out<p1,p2> operator>(FILE * f)  const {return f;}

    template<typename Char, typename Traits> api::pipe_out<p1,p2> operator=(basic_pipe<Char, Traits> & p)      const {return p;}
    template<typename Char, typename Traits> api::pipe_out<p1,p2> operator>(basic_pipe<Char, Traits> & p)      const {return p;}
    template<typename Char, typename Traits> api::pipe_out<p1,p2> operator=(basic_ipstream<Char, Traits> & p)  const {return p.pipe();}
    template<typename Char, typename Traits> api::pipe_out<p1,p2> operator>(basic_ipstream<Char, Traits> & p)  const {return p.pipe();}
    template<typename Char, typename Traits> api::pipe_out<p1,p2> operator=(basic_pstream <Char, Traits> & p)  const {return p.pipe();}
    template<typename Char, typename Traits> api::pipe_out<p1,p2> operator>(basic_pstream <Char, Traits> & p)  const {return p.pipe();}

    api::async_pipe_out<p1, p2> operator=(async_pipe & p) const {return p;}
    api::async_pipe_out<p1, p2> operator>(async_pipe & p) const {return p;}

    api::async_out_buffer<p1, p2, const asio::mutable_buffer>     operator=(const asio::mutable_buffer & buf)     const {return buf;}
    api::async_out_buffer<p1, p2, const asio::mutable_buffers_1> operator=(const asio::mutable_buffers_1 & buf) const {return buf;}
    api::async_out_buffer<p1, p2, asio::streambuf>               operator=(asio::streambuf & os)                   const {return os ;}

    api::async_out_buffer<p1, p2, const asio::mutable_buffer>     operator>(const asio::mutable_buffer & buf)     const {return buf;}
    api::async_out_buffer<p1, p2, const asio::mutable_buffers_1> operator>(const asio::mutable_buffers_1 & buf) const {return buf;}
    api::async_out_buffer<p1, p2, asio::streambuf>               operator>(asio::streambuf & os)                   const {return os ;}

    api::async_out_future<p1,p2, std::string>       operator=(std::future<std::string> & fut)       const { return fut;}
    api::async_out_future<p1,p2, std::string>       operator>(std::future<std::string> & fut)       const { return fut;}
    api::async_out_future<p1,p2, std::vector<char>> operator=(std::future<std::vector<char>> & fut) const { return fut;}
    api::async_out_future<p1,p2, std::vector<char>> operator>(std::future<std::vector<char>> & fut) const { return fut;}

    template<int pin, typename = typename std::enable_if<
            (((p1 == 1) && (pin == 2)) ||
             ((p1 == 2) && (pin == 1)))
             && (p2 == -1)>::type>
    constexpr std_out_<1, 2> operator& (const std_out_<pin>&) const
    {
        return std_out_<1, 2> ();
    }

};

struct close_t
{
    constexpr close_t() = default;
    template<int T, int U>
    api::close_out<T,U> operator()(std_out_<T,U>) {return api::close_out<T,U>();}
};



}
///This constant is a utility to allow syntax like `std_out > close` for closing I/O streams.
constexpr boost::process::detail::close_t close;
///This constant is a utility to redirect streams to the null-device.
constexpr boost::process::detail::null_t  null;

/**
This property allows to set the input stream for the child process.

\section stdin_details Details

\subsection stdin_file File Input

The file I/O simple redirects the stream to a file, for which the possible types are

 - `boost::filesystem::path`
 - `std::basic_string<char_type>`
 - `const char_type*`
 - `FILE*`

with `char_type` being either `char` or `wchar_t`.

FILE* is explicitly added, so the process can easily redirect the output stream
of the child to another output stream of the process. That is:

\code{.cpp}
system("ls", std_in < stdin);
\endcode

\warning If the launching and the child process use the input, this leads to undefined behaviour.

A syntax like `system("ls", std_out > std::cerr)` is not possible, due to the C++
implementation not providing access to the handle.

The valid expressions for this property are

\code{.cpp}
std_in < file;
std_in = file;
\endcode

\subsection stdin_pipe Pipe Input

As explained in the corresponding section, the boost.process library provides a
@ref boost::process::async_pipe "async_pipe" class which can be
used to communicate with child processes.

\note Technically the @ref boost::process::async_pipe "async_pipe"
works synchronous here, since no asio implementation is used by the library here.
The async-operation will then however not end if the process is finished, since
the pipe remains open. You can use the async_close function with on_exit to fix that.

Valid expressions with pipes are these:

\code{.cpp}
std_in < pipe;
std_in = pipe;
\endcode

Where the valid types for `pipe` are the following:

 - `basic_pipe`
 - `async_pipe`
 - `basic_opstream`
 - `basic_pstream`

Note that the pipe may also be used between several processes, like this:

\code{.cpp}
pipe p;
child c1("nm", "a.out", std_out>p);
child c2("c++filt", std_in<p);
\endcode

\subsection stdin_async_pipe Asynchronous Pipe Input

Asynchronous Pipe I/O classifies communication which has automatically handling
of the asynchronous operations by the process library. This means, that a pipe will be
constructed, the async_read/-write will be automatically started, and that the
end of the child process will also close the pipe.

Valid types for pipe I/O are the following:

 - `boost::asio::const_buffer`   \xmlonly <footnote><para> Constructed with <code>boost::asio::buffer</code></para></footnote> \endxmlonly
 - `boost::asio::mutable_buffer` \xmlonly <footnote><para> Constructed with <code>boost::asio::buffer</code></para></footnote> \endxmlonly
 - `boost::asio::streambuf`

Valid expressions with pipes are these:

\code{.cpp}
std_in < buffer;
std_in = buffer;
std_out > buffer;
std_out = buffer;
std_err > buffer;
std_err = buffer;
(std_out & std_err) > buffer;
(std_out & std_err) = buffer;
\endcode

\note  It is also possible to get a future for std_in, by chaining another `std::future<void>` onto it,
so you can wait for the input to be completed. It looks like this:
\code{.cpp}
std::future<void> fut;
boost::asio::io_context ios;
std::string data;
child c("prog", std_in < buffer(data) >  fut, ios);
fut.get();
\endcode


\note `boost::asio::buffer` is also available in the `boost::process` namespace.

\warning This feature requires `boost/process/async.hpp` to be included and a reference to `boost::asio::io_context` to be passed to the launching function.


\subsection stdin_close Close

The input stream can be closed, so it cannot be read from. This will lead to an error when attempted.

This can be achieved by the following syntax.

\code{.cpp}
std_in < close;
std_in = close;
std_in.close();
\endcode

\subsection stdin_null Null

The input stream can be redirected to read from the null-device, which means that only `EOF` is read.

The syntax to achieve that has the following variants:

\code{.cpp}
std_in < null;
std_in = null;
std_in.null();
\endcode

*/

constexpr boost::process::detail::std_in_<void>   std_in;

/**
This property allows to set the output stream for the child process.

\note The Semantic is the same as for \xmlonly <globalname alt="boost::process::std_err">std_err</globalname> \endxmlonly

\note `std_err` and `std_out` can be combined into one stream, with the `operator &`, i.e. `std_out & std_err`.

\section stdout_details Details

\subsection stdout_file File Input

The file I/O simple redirects the stream to a file, for which the possible types are

 - `boost::filesystem::path`
 - `std::basic_string<char_type>`
 - `const char_type*`
 - `FILE*`

with `char_type` being either `char` or `wchar_t`.

FILE* is explicitly added, so the process can easily redirect the output stream
of the child to another output stream of the process. That is:

\code{.cpp}
system("ls", std_out < stdin);
\endcode

\warning If the launching and the child process use the input, this leads to undefined behaviour.

A syntax like `system("ls", std_out > std::cerr)` is not possible, due to the C++
implementation not providing access to the handle.

The valid expressions for this property are

\code{.cpp}
std_out < file;
std_out = file;
\endcode

\subsection stdout_pipe Pipe Output

As explained in the corresponding section, the boost.process library provides a
@ref boost::process::async_pipe "async_pipe" class which can be
used to communicate with child processes.

\note Technically the @ref boost::process::async_pipe "async_pipe"
works like a synchronous pipe here, since no asio implementation is used by the library here.
The asynchronous operation will then however not end if the process is finished, since
the pipe remains open. You can use the async_close function with on_exit to fix that.

Valid expressions with pipes are these:

\code{.cpp}
std_out > pipe;
std_out = pipe;
\endcode

Where the valid types for `pipe` are the following:

 - `basic_pipe`
 - `async_pipe`
 - `basic_ipstream`
 - `basic_pstream`

Note that the pipe may also be used between several processes, like this:

\code{.cpp}
pipe p;
child c1("nm", "a.out", std_out>p);
child c2("c++filt", std_in<p);
\endcode

\subsection stdout_async_pipe Asynchronous Pipe Output

Asynchronous Pipe I/O classifies communication which has automatically handling
of the async operations by the process library. This means, that a pipe will be
constructed, the async_read/-write will be automatically started, and that the
end of the child process will also close the pipe.

Valid types for pipe I/O are the following:

 - `boost::asio::mutable_buffer` \xmlonly <footnote><para> Constructed with <code>boost::asio::buffer</code></para></footnote> \endxmlonly
 - `boost::asio::streambuf`
 - `std::future<std::vector<char>>`
 - `std::future<std::string>`

Valid expressions with pipes are these:

\code{.cpp}
std_out > buffer;
std_out = buffer;
std_err > buffer;
std_err = buffer;
(std_out & std_err) > buffer;
(std_out & std_err) = buffer;
\endcode

\note `boost::asio::buffer` is also available in the `boost::process` namespace.

\warning This feature requires `boost/process/async.hpp` to be included and a reference to `boost::asio::io_context` to be passed to the launching function.


\subsection stdout_close Close

The out stream can be closed, so it cannot be write from.
This will lead to an error when attempted.

This can be achieved by the following syntax.

\code{.cpp}
std_out > close;
std_out = close;
std_out.close();
\endcode

\subsection stdout_null Null

The output stream can be redirected to write to the null-device,
which means that all output is discarded.

The syntax to achieve that has the following variants:

\code{.cpp}
std_out > null;
std_out = null;
std_out.null();
\endcode

*/

constexpr boost::process::detail::std_out_<1> std_out;
/**This property allows setting the `stderr` stream. The semantic and syntax is the same as for
 * \xmlonly <globalname alt="boost::process::std_out">std_out</globalname> \endxmlonly .
 */
constexpr boost::process::detail::std_out_<2> std_err;

}}
#endif /* INCLUDE_BOOST_PROCESS_IO_HPP_ */

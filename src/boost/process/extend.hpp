// Copyright (c) 2016 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PROCESS_EXTENSIONS_HPP_
#define BOOST_PROCESS_EXTENSIONS_HPP_

#include <boost/process/detail/handler.hpp>
#include <boost/process/detail/used_handles.hpp>

#if defined(BOOST_WINDOWS_API)
#include <boost/process/detail/windows/executor.hpp>
#include <boost/process/detail/windows/async_handler.hpp>
#include <boost/process/detail/windows/asio_fwd.hpp>
#else
#include <boost/process/detail/posix/executor.hpp>
#include <boost/process/detail/posix/async_handler.hpp>
#include <boost/process/detail/posix/asio_fwd.hpp>
#endif


/** \file boost/process/extend.hpp
 *
 * This header which provides the types and functions provided for custom extensions.
 *
 * \xmlonly
   Please refer to the <link linkend="boost_process.extend">tutorial</link> for more details.
   \endxmlonly
 */


namespace boost {
namespace process {
namespace detail {
template<typename Tuple>
inline asio::io_context& get_io_context(const Tuple & tup);
}


///Namespace for extensions \attention This is experimental.
namespace extend {

#if defined(BOOST_WINDOWS_API)

template<typename Char, typename Sequence>
using windows_executor = ::boost::process::detail::windows::executor<Char, Sequence>;
template<typename Sequence>
struct posix_executor;

#elif defined(BOOST_POSIX_API)

template<typename Sequence>
using posix_executor = ::boost::process::detail::posix::executor<Sequence>;
template<typename Char, typename Sequence>
struct windows_executor;

#endif

using ::boost::process::detail::handler;
using ::boost::process::detail::api::require_io_context;
using ::boost::process::detail::api::async_handler;
using ::boost::process::detail::get_io_context;
using ::boost::process::detail::get_last_error;
using ::boost::process::detail::throw_last_error;
using ::boost::process::detail::uses_handles;
using ::boost::process::detail::foreach_used_handle;
using ::boost::process::detail::get_used_handles;

///This handler is invoked before the process in launched, to setup parameters. The required signature is `void(Exec &)`, where `Exec` is a template parameter.
constexpr boost::process::detail::make_handler_t<boost::process::detail::on_setup_>   on_setup;
///This handler is invoked if an error occured. The required signature is `void(auto & exec, const std::error_code&)`, where `Exec` is a template parameter.
constexpr boost::process::detail::make_handler_t<boost::process::detail::on_error_>   on_error;
///This handler is invoked if launching the process has succeeded. The required signature is `void(auto & exec)`, where `Exec` is a template parameter.
constexpr boost::process::detail::make_handler_t<boost::process::detail::on_success_> on_success;

#if defined(BOOST_POSIX_API) || defined(BOOST_PROCESS_DOXYGEN)
///This handler is invoked if the fork failed. The required signature is `void(auto & exec)`, where `Exec` is a template parameter. \note Only available on posix.
constexpr ::boost::process::detail::make_handler_t<::boost::process::detail::posix::on_fork_error_  >   on_fork_error;
///This handler is invoked if the fork succeeded. The required signature is `void(Exec &)`, where `Exec` is a template parameter. \note Only available on posix.
constexpr ::boost::process::detail::make_handler_t<::boost::process::detail::posix::on_exec_setup_  >   on_exec_setup;
///This handler is invoked if the exec call errored. The required signature is `void(auto & exec)`, where `Exec` is a template parameter. \note Only available on posix.
constexpr ::boost::process::detail::make_handler_t<::boost::process::detail::posix::on_exec_error_  >   on_exec_error;
#endif

#if defined(BOOST_PROCESS_DOXYGEN)
///Helper function to get the last error code system-independent
inline std::error_code get_last_error();

///Helper function to get and throw the last system error.
/// \throws boost::process::process_error
/// \param msg A message to add to the error code.
inline void throw_last_error(const std::string & msg);
///\overload void throw_last_error(const std::string & msg)
inline void throw_last_error();


/** This function gets the io_context from the initializer sequence.
 *
 * \attention Yields a compile-time error if no `io_context` is provided.
 * \param seq The Sequence of the initializer.
 */
template<typename Sequence>
inline asio::io_context& get_io_context(const Sequence & seq);

/** This class is the base for every initializer, to be used for extensions.
 *
 *  The usage is done through compile-time polymorphism, so that the required
 *  functions can be overloaded.
 *
 * \note None of the function need to be `const`.
 *
 */
struct handler
{
    ///This function is invoked before the process launch. \note It is not required to be const.
    template <class Executor>
    void on_setup(Executor&) const {}

    /** This function is invoked if an error occured while trying to launch the process.
     * \note It is not required to be const.
     */
    template <class Executor>
    void on_error(Executor&, const std::error_code &) const {}

    /** This function is invoked if the process was successfully launched.
     * \note It is not required to be const.
     */
    template <class Executor>
    void on_success(Executor&) const {}

    /**This function is invoked if an error occured during the call of `fork`.
     * \note This function will only be called on posix.
     */
    template<typename Executor>
    void on_fork_error  (Executor &, const std::error_code&) const {}

    /**This function is invoked if the call of `fork` was successful, before
     * calling `execve`.
     * \note This function will only be called on posix.
     * \attention It will be invoked from the new process.
     */
    template<typename Executor>
    void on_exec_setup  (Executor &) const {}

    /**This function is invoked if the call of `execve` failed.
     * \note This function will only be called on posix.
     * \attention It will be invoked from the new process.
     */
    template<typename Executor>
    void on_exec_error  (Executor &, const std::error_code&) const {}

};


/** Inheriting the class will tell the launching process that an `io_context` is
 * needed. This should always be used when \ref get_io_context is used.
 *
 */
struct require_io_context {};
/** Inheriting this class will tell the launching function, that an event handler
 * shall be invoked when the process exits. This automatically does also inherit
 * \ref require_io_context.
 *
 * You must add the following function to your implementation:
 *
 \code{.cpp}
template<typename Executor>
std::function<void(int, const std::error_code&)> on_exit_handler(Executor & exec)
{
    auto handler_ = this->handler;
    return [handler_](int exit_code, const std::error_code & ec)
           {
                handler_(static_cast<int>(exit_code), ec);
           };

}
 \endcode

 The callback will be obtained by calling this function on setup and it will be
 invoked when the process exits.

 *
 * \warning Cannot be used with \ref boost::process::spawn
 */
struct async_handler : handler, require_io_context
{

};

///The posix executor type.
/** This type represents the posix executor and can be used for overloading in a custom handler.
 * \note It is an alias for the implementation on posix, and a forward-declaration on windows.
 *
 * \tparam Sequence The used initializer-sequence, it is fulfills the boost.fusion [sequence](http://www.boost.org/doc/libs/master/libs/fusion/doc/html/fusion/sequence.html) concept.


\xmlonly
As information for extension development, here is the structure of the process launching (in pseudo-code and uml)
<xi:include href="posix_pseudocode.xml" xmlns:xi="http://www.w3.org/2001/XInclude"/>

<mediaobject>
<caption>
<para>The sequence if when no error occurs.</para>
</caption>
<imageobject>
<imagedata fileref="boost_process/posix_success.svg"/>
</imageobject>
</mediaobject>

<mediaobject>
<caption>
<para>The sequence if the execution fails.</para>
</caption>
<imageobject>
<imagedata fileref="boost_process/posix_exec_err.svg"/>
</imageobject>
</mediaobject>

<mediaobject>
<caption>
<para>The sequence if the fork fails.</para>
</caption>
<imageobject>
<imagedata fileref="boost_process/posix_fork_err.svg"/>
</imageobject>
</mediaobject>

\endxmlonly


\note Error handling if execve fails is done through a pipe, unless \ref ignore_error is used.

 */
template<typename Sequence>
struct posix_executor
{
    ///A reference to the actual initializer-sequence
     Sequence & seq;
    ///A pointer to the name of the executable.
     const char * exe      = nullptr;
     ///A pointer to the argument-vector.
     char *const* cmd_line = nullptr;
     ///A pointer to the environment variables, as default it is set to [environ](http://pubs.opengroup.org/onlinepubs/009695399/basedefs/xbd_chap08.html)
     char **env      = ::environ;
     ///The pid of the process - it will be -1 before invoking [fork](http://pubs.opengroup.org/onlinepubs/009695399/functions/fork.html), and after forking either 0 for the new process or a positive value if in the current process. */
     pid_t pid = -1;
     ///This shared-pointer holds the exit code. It's done this way, so it can be shared between an `asio::io_context` and \ref child.
     std::shared_ptr<std::atomic<int>> exit_status = std::make_shared<std::atomic<int>>(still_active);

     ///This function returns a const reference to the error state of the executor.
     const std::error_code & error() const;

     ///This function can be used to report an error to the executor. This will be handled according to the configuration of the executor, i.e. it
     /// might throw an exception. \note This is the required way to handle errors in initializers.
     void set_error(const std::error_code &ec, const std::string &msg);
     ///\overload void set_error(const std::error_code &ec, const std::string &msg);
     void set_error(const std::error_code &ec, const char* msg);
};

///The windows executor type.
/** This type represents the posix executor and can be used for overloading in a custom handler.
 *
 * \note It is an alias for the implementation on posix, and a forward-declaration on windows.
 * \tparam Sequence The used initializer-sequence, it is fulfills the boost.fusion [sequence](http://www.boost.org/doc/libs/master/libs/fusion/doc/html/fusion/sequence.html) concept.
 * \tparam Char The used char-type, either `char` or `wchar_t`.
 *

\xmlonly
As information for extension development, here is the structure of the process launching (in pseudo-code and uml)<xi:include href="windows_pseudocode.xml" xmlns:xi="http://www.w3.org/2001/XInclude"/>
<mediaobject>
<caption>
<para>The sequence for windows process creation.</para>
</caption>
<imageobject>
<imagedata fileref="boost_process/windows_exec.svg"/>
</imageobject>
</mediaobject>
\endxmlonly

 */

template<typename Char, typename Sequence>
struct windows_executor
{
    ///A reference to the actual initializer-sequence
     Sequence & seq;

     ///A pointer to the name of the executable. It's null by default.
     const Char * exe      = nullptr;
     ///A pointer to the argument-vector. Must be set by some initializer.
     char  Char* cmd_line = nullptr;
     ///A pointer to the environment variables. It's null by default.
     char  Char* env      = nullptr;
     ///A pointer to the working directory. It's null by default.
     const Char * work_dir = nullptr;

     ///A pointer to the process-attributes of type [SECURITY_ATTRIBUTES](https://msdn.microsoft.com/en-us/library/windows/desktop/aa379560.aspx). It's null by default.
     ::boost::detail::winapi::LPSECURITY_ATTRIBUTES_ proc_attrs   = nullptr;
     ///A pointer to the thread-attributes of type [SECURITY_ATTRIBUTES](https://msdn.microsoft.com/en-us/library/windows/desktop/aa379560.aspx). It' null by default.
     ::boost::detail::winapi::LPSECURITY_ATTRIBUTES_ thread_attrs = nullptr;
     ///A logical bool value setting whether handles shall be inherited or not.
     ::boost::detail::winapi::BOOL_ inherit_handles = false;

     ///The element holding the process-information after process creation. The type is [PROCESS_INFORMATION](https://msdn.microsoft.com/en-us/library/windows/desktop/ms684873.aspx)
     ::boost::detail::winapi::PROCESS_INFORMATION_ proc_info{nullptr, nullptr, 0,0};


     ///This shared-pointer holds the exit code. It's done this way, so it can be shared between an `asio::io_context` and \ref child.
     std::shared_ptr<std::atomic<int>> exit_status = std::make_shared<std::atomic<int>>(still_active);

     ///This function returns a const reference to the error state of the executor.
     const std::error_code & error() const;

     ///This function can be used to report an error to the executor. This will be handled according to the configuration of the executor, i.e. it
     /// might throw an exception. \note This is the required way to handle errors in initializers.
     void set_error(const std::error_code &ec, const std::string &msg);
     ///\overload void set_error(const std::error_code &ec, const std::string &msg);
     void set_error(const std::error_code &ec, const char* msg);

     ///The creation flags of the process
    ::boost::detail::winapi::DWORD_ creation_flags;
    ///The type of the [startup-info](https://msdn.microsoft.com/en-us/library/windows/desktop/ms686331.aspx), depending on the char-type.
    typedef typename detail::startup_info<Char>::type    startup_info_t;
    ///The type of the [extended startup-info](https://msdn.microsoft.com/de-de/library/windows/desktop/ms686329.aspx), depending the char-type; only defined with winapi-version equal or higher than 6.
    typedef typename detail::startup_info_ex<Char>::type startup_info_ex_t;
    ///This function switches the information, so that the extended structure is used. \note It's only defined with winapi-version equal or higher than 6.
    void set_startup_info_ex();
    ///This element is an instance or a reference (if \ref startup_info_ex exists) to the [startup-info](https://msdn.microsoft.com/en-us/library/windows/desktop/ms686331.aspx) for the process.
    startup_info_t startup_info;
    ///This element is the instance of the  [extended startup-info](https://msdn.microsoft.com/de-de/library/windows/desktop/ms686329.aspx). It is only available with a winapi-version equal or highter than 6.
    startup_info_ex_t  startup_info_ex;
};



#endif

}
}
}

#endif

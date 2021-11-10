// Copyright (c) 2016 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_PROCESS_POSIX_HPP_
#define BOOST_PROCESS_POSIX_HPP_

#include <boost/process/detail/posix/fd.hpp>
#include <boost/process/detail/posix/handler.hpp>
#include <boost/process/detail/posix/use_vfork.hpp>
#include <boost/process/detail/posix/signal.hpp>


/** \file boost/process/posix.hpp
 *
 *    Header which provides the posix extensions.
\xmlonly
<programlisting>
namespace boost {
  namespace process {
    namespace posix {
      <emphasis>unspecified</emphasis> <globalname alt="boost::process::posix::fd">fd</globalname>;
      <emphasis>unspecified</emphasis> <globalname alt="boost::process::posix::sig">sig</globalname>;
      <emphasis>unspecified</emphasis> <globalname alt="boost::process::posix::use_vfork">use_vfork</globalname>;
    }
  }
}
</programlisting>
 *  \endxmlonly
 *   \warning Only available on posix. See the documentation of [fork](http://pubs.opengroup.org/onlinepubs/009695399/functions/fork.html),
 *   [execve](http://pubs.opengroup.org/onlinepubs/009695399/functions/execve.html) and
 *   [vfork](http://pubs.opengroup.org/onlinepubs/009695399/functions/vfork.html).
 *
 */

namespace boost { namespace process {

///Namespace containing the posix exensions.
namespace posix {

/** This property lets you modify file-descriptors other than the standard ones (0,1,2).
 *
 * It provides the functions `bind`, which implements [dup2](http://pubs.opengroup.org/onlinepubs/9699919799/functions/dup.html)
 * and [close](http://pubs.opengroup.org/onlinepubs/9699919799/functions/close.html).
 *
 * Close can also be called with a range of file-descriptors to be closed.
 *
 */
constexpr ::boost::process::detail::posix::fd_ fd;

/** This property lets you modify the handling of `SIGCHLD` for this call. It will be reset afterwards.

It can be set to default, by the expression `sig.dfl()`, set to ignore with `sig.ign()` or
assigned a custom handler. A custom handler must have the type `sighandler_t`and can be assigned with the following syntax:

\code{.cpp}
sig = handler;
sig(handler);
\endcode

\warning @ref spawn will automatically use `sig.ign()`, which will override if you pass a custom handler.
 */
constexpr ::boost::process::detail::posix::sig_       sig;
/** This property will replace the usage of [fork](http://pubs.opengroup.org/onlinepubs/9699919799/functions/fork.html) by [vfork](http://pubs.opengroup.org/onlinepubs/009695399/functions/vfork.html).
 \note `vfork` is no longer an official part of the posix standard.

 */
constexpr ::boost::process::detail::posix::use_vfork_ use_vfork;


using ::boost::process::detail::posix::sighandler_t;

}}}

#endif /* BOOST_PROCESS_POSIX_HPP_ */

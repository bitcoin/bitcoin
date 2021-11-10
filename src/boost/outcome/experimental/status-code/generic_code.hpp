/* Proposed SG14 status_code
(C) 2018 - 2020 Niall Douglas <http://www.nedproductions.biz/> (5 commits)
File Created: Feb 2018


Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License in the accompanying file
Licence.txt or at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.


Distributed under the Boost Software License, Version 1.0.
(See accompanying file Licence.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef BOOST_OUTCOME_SYSTEM_ERROR2_GENERIC_CODE_HPP
#define BOOST_OUTCOME_SYSTEM_ERROR2_GENERIC_CODE_HPP

#include "status_error.hpp"

#include <cerrno>  // for error constants

BOOST_OUTCOME_SYSTEM_ERROR2_NAMESPACE_BEGIN

//! The generic error coding (POSIX)
enum class errc : int
{
  success = 0,
  unknown = -1,

  address_family_not_supported = EAFNOSUPPORT,
  address_in_use = EADDRINUSE,
  address_not_available = EADDRNOTAVAIL,
  already_connected = EISCONN,
  argument_list_too_long = E2BIG,
  argument_out_of_domain = EDOM,
  bad_address = EFAULT,
  bad_file_descriptor = EBADF,
  bad_message = EBADMSG,
  broken_pipe = EPIPE,
  connection_aborted = ECONNABORTED,
  connection_already_in_progress = EALREADY,
  connection_refused = ECONNREFUSED,
  connection_reset = ECONNRESET,
  cross_device_link = EXDEV,
  destination_address_required = EDESTADDRREQ,
  device_or_resource_busy = EBUSY,
  directory_not_empty = ENOTEMPTY,
  executable_format_error = ENOEXEC,
  file_exists = EEXIST,
  file_too_large = EFBIG,
  filename_too_long = ENAMETOOLONG,
  function_not_supported = ENOSYS,
  host_unreachable = EHOSTUNREACH,
  identifier_removed = EIDRM,
  illegal_byte_sequence = EILSEQ,
  inappropriate_io_control_operation = ENOTTY,
  interrupted = EINTR,
  invalid_argument = EINVAL,
  invalid_seek = ESPIPE,
  io_error = EIO,
  is_a_directory = EISDIR,
  message_size = EMSGSIZE,
  network_down = ENETDOWN,
  network_reset = ENETRESET,
  network_unreachable = ENETUNREACH,
  no_buffer_space = ENOBUFS,
  no_child_process = ECHILD,
  no_link = ENOLINK,
  no_lock_available = ENOLCK,
  no_message = ENOMSG,
  no_protocol_option = ENOPROTOOPT,
  no_space_on_device = ENOSPC,
  no_stream_resources = ENOSR,
  no_such_device_or_address = ENXIO,
  no_such_device = ENODEV,
  no_such_file_or_directory = ENOENT,
  no_such_process = ESRCH,
  not_a_directory = ENOTDIR,
  not_a_socket = ENOTSOCK,
  not_a_stream = ENOSTR,
  not_connected = ENOTCONN,
  not_enough_memory = ENOMEM,
  not_supported = ENOTSUP,
  operation_canceled = ECANCELED,
  operation_in_progress = EINPROGRESS,
  operation_not_permitted = EPERM,
  operation_not_supported = EOPNOTSUPP,
  operation_would_block = EWOULDBLOCK,
  owner_dead = EOWNERDEAD,
  permission_denied = EACCES,
  protocol_error = EPROTO,
  protocol_not_supported = EPROTONOSUPPORT,
  read_only_file_system = EROFS,
  resource_deadlock_would_occur = EDEADLK,
  resource_unavailable_try_again = EAGAIN,
  result_out_of_range = ERANGE,
  state_not_recoverable = ENOTRECOVERABLE,
  stream_timeout = ETIME,
  text_file_busy = ETXTBSY,
  timed_out = ETIMEDOUT,
  too_many_files_open_in_system = ENFILE,
  too_many_files_open = EMFILE,
  too_many_links = EMLINK,
  too_many_symbolic_link_levels = ELOOP,
  value_too_large = EOVERFLOW,
  wrong_protocol_type = EPROTOTYPE
};

namespace detail
{
  BOOST_OUTCOME_SYSTEM_ERROR2_CONSTEXPR14 inline const char *generic_code_message(errc code) noexcept
  {
    switch(code)
    {
    case errc::success:
      return "Success";
    case errc::address_family_not_supported:
      return "Address family not supported by protocol";
    case errc::address_in_use:
      return "Address already in use";
    case errc::address_not_available:
      return "Cannot assign requested address";
    case errc::already_connected:
      return "Transport endpoint is already connected";
    case errc::argument_list_too_long:
      return "Argument list too long";
    case errc::argument_out_of_domain:
      return "Numerical argument out of domain";
    case errc::bad_address:
      return "Bad address";
    case errc::bad_file_descriptor:
      return "Bad file descriptor";
    case errc::bad_message:
      return "Bad message";
    case errc::broken_pipe:
      return "Broken pipe";
    case errc::connection_aborted:
      return "Software caused connection abort";
    case errc::connection_already_in_progress:
      return "Operation already in progress";
    case errc::connection_refused:
      return "Connection refused";
    case errc::connection_reset:
      return "Connection reset by peer";
    case errc::cross_device_link:
      return "Invalid cross-device link";
    case errc::destination_address_required:
      return "Destination address required";
    case errc::device_or_resource_busy:
      return "Device or resource busy";
    case errc::directory_not_empty:
      return "Directory not empty";
    case errc::executable_format_error:
      return "Exec format error";
    case errc::file_exists:
      return "File exists";
    case errc::file_too_large:
      return "File too large";
    case errc::filename_too_long:
      return "File name too long";
    case errc::function_not_supported:
      return "Function not implemented";
    case errc::host_unreachable:
      return "No route to host";
    case errc::identifier_removed:
      return "Identifier removed";
    case errc::illegal_byte_sequence:
      return "Invalid or incomplete multibyte or wide character";
    case errc::inappropriate_io_control_operation:
      return "Inappropriate ioctl for device";
    case errc::interrupted:
      return "Interrupted system call";
    case errc::invalid_argument:
      return "Invalid argument";
    case errc::invalid_seek:
      return "Illegal seek";
    case errc::io_error:
      return "Input/output error";
    case errc::is_a_directory:
      return "Is a directory";
    case errc::message_size:
      return "Message too long";
    case errc::network_down:
      return "Network is down";
    case errc::network_reset:
      return "Network dropped connection on reset";
    case errc::network_unreachable:
      return "Network is unreachable";
    case errc::no_buffer_space:
      return "No buffer space available";
    case errc::no_child_process:
      return "No child processes";
    case errc::no_link:
      return "Link has been severed";
    case errc::no_lock_available:
      return "No locks available";
    case errc::no_message:
      return "No message of desired type";
    case errc::no_protocol_option:
      return "Protocol not available";
    case errc::no_space_on_device:
      return "No space left on device";
    case errc::no_stream_resources:
      return "Out of streams resources";
    case errc::no_such_device_or_address:
      return "No such device or address";
    case errc::no_such_device:
      return "No such device";
    case errc::no_such_file_or_directory:
      return "No such file or directory";
    case errc::no_such_process:
      return "No such process";
    case errc::not_a_directory:
      return "Not a directory";
    case errc::not_a_socket:
      return "Socket operation on non-socket";
    case errc::not_a_stream:
      return "Device not a stream";
    case errc::not_connected:
      return "Transport endpoint is not connected";
    case errc::not_enough_memory:
      return "Cannot allocate memory";
#if ENOTSUP != EOPNOTSUPP
    case errc::not_supported:
      return "Operation not supported";
#endif
    case errc::operation_canceled:
      return "Operation canceled";
    case errc::operation_in_progress:
      return "Operation now in progress";
    case errc::operation_not_permitted:
      return "Operation not permitted";
    case errc::operation_not_supported:
      return "Operation not supported";
#if EAGAIN != EWOULDBLOCK
    case errc::operation_would_block:
      return "Resource temporarily unavailable";
#endif
    case errc::owner_dead:
      return "Owner died";
    case errc::permission_denied:
      return "Permission denied";
    case errc::protocol_error:
      return "Protocol error";
    case errc::protocol_not_supported:
      return "Protocol not supported";
    case errc::read_only_file_system:
      return "Read-only file system";
    case errc::resource_deadlock_would_occur:
      return "Resource deadlock avoided";
    case errc::resource_unavailable_try_again:
      return "Resource temporarily unavailable";
    case errc::result_out_of_range:
      return "Numerical result out of range";
    case errc::state_not_recoverable:
      return "State not recoverable";
    case errc::stream_timeout:
      return "Timer expired";
    case errc::text_file_busy:
      return "Text file busy";
    case errc::timed_out:
      return "Connection timed out";
    case errc::too_many_files_open_in_system:
      return "Too many open files in system";
    case errc::too_many_files_open:
      return "Too many open files";
    case errc::too_many_links:
      return "Too many links";
    case errc::too_many_symbolic_link_levels:
      return "Too many levels of symbolic links";
    case errc::value_too_large:
      return "Value too large for defined data type";
    case errc::wrong_protocol_type:
      return "Protocol wrong type for socket";
    default:
      return "unknown";
    }
  }
}  // namespace detail

/*! The implementation of the domain for generic status codes, those mapped by `errc` (POSIX).
 */
class _generic_code_domain : public status_code_domain
{
  template <class> friend class status_code;
  template <class StatusCode> friend class detail::indirecting_domain;
  using _base = status_code_domain;

public:
  //! The value type of the generic code, which is an `errc` as per POSIX.
  using value_type = errc;
  using string_ref = _base::string_ref;

public:
  //! Default constructor
  constexpr explicit _generic_code_domain(typename _base::unique_id_type id = 0x746d6354f4f733e9) noexcept
      : _base(id)
  {
  }
  _generic_code_domain(const _generic_code_domain &) = default;
  _generic_code_domain(_generic_code_domain &&) = default;
  _generic_code_domain &operator=(const _generic_code_domain &) = default;
  _generic_code_domain &operator=(_generic_code_domain &&) = default;
  ~_generic_code_domain() = default;

  //! Constexpr singleton getter. Returns the constexpr generic_code_domain variable.
  static inline constexpr const _generic_code_domain &get();

  virtual _base::string_ref name() const noexcept override { return string_ref("generic domain"); }  // NOLINT
protected:
  virtual bool _do_failure(const status_code<void> &code) const noexcept override  // NOLINT
  {
    assert(code.domain() == *this);                                           // NOLINT
    return static_cast<const generic_code &>(code).value() != errc::success;  // NOLINT
  }
  virtual bool _do_equivalent(const status_code<void> &code1, const status_code<void> &code2) const noexcept override  // NOLINT
  {
    assert(code1.domain() == *this);                            // NOLINT
    const auto &c1 = static_cast<const generic_code &>(code1);  // NOLINT
    if(code2.domain() == *this)
    {
      const auto &c2 = static_cast<const generic_code &>(code2);  // NOLINT
      return c1.value() == c2.value();
    }
    return false;
  }
  virtual generic_code _generic_code(const status_code<void> &code) const noexcept override  // NOLINT
  {
    assert(code.domain() == *this);                  // NOLINT
    return static_cast<const generic_code &>(code);  // NOLINT
  }
  virtual _base::string_ref _do_message(const status_code<void> &code) const noexcept override  // NOLINT
  {
    assert(code.domain() == *this);                           // NOLINT
    const auto &c = static_cast<const generic_code &>(code);  // NOLINT
    return string_ref(detail::generic_code_message(c.value()));
  }
#if defined(_CPPUNWIND) || defined(__EXCEPTIONS) || defined(BOOST_OUTCOME_STANDARDESE_IS_IN_THE_HOUSE)
  BOOST_OUTCOME_SYSTEM_ERROR2_NORETURN virtual void _do_throw_exception(const status_code<void> &code) const override  // NOLINT
  {
    assert(code.domain() == *this);                           // NOLINT
    const auto &c = static_cast<const generic_code &>(code);  // NOLINT
    throw status_error<_generic_code_domain>(c);
  }
#endif
};
//! A specialisation of `status_error` for the generic code domain.
using generic_error = status_error<_generic_code_domain>;
//! A constexpr source variable for the generic code domain, which is that of `errc` (POSIX). Returned by `_generic_code_domain::get()`.
constexpr _generic_code_domain generic_code_domain;
inline constexpr const _generic_code_domain &_generic_code_domain::get()
{
  return generic_code_domain;
}
// Enable implicit construction of generic_code from errc
BOOST_OUTCOME_SYSTEM_ERROR2_CONSTEXPR14 inline generic_code make_status_code(errc c) noexcept
{
  return generic_code(in_place, c);
}


/*************************************************************************************************************/


template <class T> inline bool status_code<void>::equivalent(const status_code<T> &o) const noexcept
{
  if(_domain && o._domain)
  {
    if(_domain->_do_equivalent(*this, o))
    {
      return true;
    }
    if(o._domain->_do_equivalent(o, *this))
    {
      return true;
    }
    generic_code c1 = o._domain->_generic_code(o);
    if(c1.value() != errc::unknown && _domain->_do_equivalent(*this, c1))
    {
      return true;
    }
    generic_code c2 = _domain->_generic_code(*this);
    if(c2.value() != errc::unknown && o._domain->_do_equivalent(o, c2))
    {
      return true;
    }
  }
  // If we are both empty, we are equivalent, otherwise not equivalent
  return (!_domain && !o._domain);
}
//! True if the status code's are semantically equal via `equivalent()`.
template <class DomainType1, class DomainType2> inline bool operator==(const status_code<DomainType1> &a, const status_code<DomainType2> &b) noexcept
{
  return a.equivalent(b);
}
//! True if the status code's are not semantically equal via `equivalent()`.
template <class DomainType1, class DomainType2> inline bool operator!=(const status_code<DomainType1> &a, const status_code<DomainType2> &b) noexcept
{
  return !a.equivalent(b);
}
//! True if the status code's are semantically equal via `equivalent()` to `make_status_code(T)`.
template <class DomainType1, class T,                                                                       //
          class MakeStatusCodeResult = typename detail::safe_get_make_status_code_result<const T &>::type,  // Safe ADL lookup of make_status_code(), returns void if not found
          typename std::enable_if<is_status_code<MakeStatusCodeResult>::value, bool>::type = true>          // ADL makes a status code
inline bool operator==(const status_code<DomainType1> &a, const T &b)
{
  return a.equivalent(make_status_code(b));
}
//! True if the status code's are semantically equal via `equivalent()` to `make_status_code(T)`.
template <class T, class DomainType1,                                                                       //
          class MakeStatusCodeResult = typename detail::safe_get_make_status_code_result<const T &>::type,  // Safe ADL lookup of make_status_code(), returns void if not found
          typename std::enable_if<is_status_code<MakeStatusCodeResult>::value, bool>::type = true>          // ADL makes a status code
inline bool operator==(const T &a, const status_code<DomainType1> &b)
{
  return b.equivalent(make_status_code(a));
}
//! True if the status code's are not semantically equal via `equivalent()` to `make_status_code(T)`.
template <class DomainType1, class T,                                                                       //
          class MakeStatusCodeResult = typename detail::safe_get_make_status_code_result<const T &>::type,  // Safe ADL lookup of make_status_code(), returns void if not found
          typename std::enable_if<is_status_code<MakeStatusCodeResult>::value, bool>::type = true>          // ADL makes a status code
inline bool operator!=(const status_code<DomainType1> &a, const T &b)
{
  return !a.equivalent(make_status_code(b));
}
//! True if the status code's are semantically equal via `equivalent()` to `make_status_code(T)`.
template <class T, class DomainType1,                                                                       //
          class MakeStatusCodeResult = typename detail::safe_get_make_status_code_result<const T &>::type,  // Safe ADL lookup of make_status_code(), returns void if not found
          typename std::enable_if<is_status_code<MakeStatusCodeResult>::value, bool>::type = true>          // ADL makes a status code
inline bool operator!=(const T &a, const status_code<DomainType1> &b)
{
  return !b.equivalent(make_status_code(a));
}
//! True if the status code's are semantically equal via `equivalent()` to `quick_status_code_from_enum<T>::code_type(b)`.
template <class DomainType1, class T,                                                     //
          class QuickStatusCodeType = typename quick_status_code_from_enum<T>::code_type  // Enumeration has been activated
          >
inline bool operator==(const status_code<DomainType1> &a, const T &b)
{
  return a.equivalent(QuickStatusCodeType(b));
}
//! True if the status code's are semantically equal via `equivalent()` to `quick_status_code_from_enum<T>::code_type(a)`.
template <class T, class DomainType1,                                                     //
          class QuickStatusCodeType = typename quick_status_code_from_enum<T>::code_type  // Enumeration has been activated
          >
inline bool operator==(const T &a, const status_code<DomainType1> &b)
{
  return b.equivalent(QuickStatusCodeType(a));
}
//! True if the status code's are not semantically equal via `equivalent()` to `quick_status_code_from_enum<T>::code_type(b)`.
template <class DomainType1, class T,                                                     //
          class QuickStatusCodeType = typename quick_status_code_from_enum<T>::code_type  // Enumeration has been activated
          >
inline bool operator!=(const status_code<DomainType1> &a, const T &b)
{
  return !a.equivalent(QuickStatusCodeType(b));
}
//! True if the status code's are not semantically equal via `equivalent()` to `quick_status_code_from_enum<T>::code_type(a)`.
template <class T, class DomainType1,                                                     //
          class QuickStatusCodeType = typename quick_status_code_from_enum<T>::code_type  // Enumeration has been activated
          >
inline bool operator!=(const T &a, const status_code<DomainType1> &b)
{
  return !b.equivalent(QuickStatusCodeType(a));
}


BOOST_OUTCOME_SYSTEM_ERROR2_NAMESPACE_END

#endif

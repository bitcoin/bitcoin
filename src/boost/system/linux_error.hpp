//  boost/system/linux_error.hpp  -------------------------------------------//

//  Copyright Beman Dawes 2007

//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See library home page at http://www.boost.org/libs/system

#ifndef BOOST_SYSTEM_LINUX_ERROR_HPP
#define BOOST_SYSTEM_LINUX_ERROR_HPP

#include <boost/config/pragma_message.hpp>

#if !defined(BOOST_ALLOW_DEPRECATED_HEADERS)
  BOOST_PRAGMA_MESSAGE("This header is deprecated and is slated for removal."
  " If you want it retained, please open an issue in github.com/boostorg/system.")
#endif

//  This header is effectively empty for compiles on operating systems where
//  it is not applicable.

#if defined(linux) || defined(__linux) || defined(__linux__)

#include <boost/system/error_code.hpp>

namespace boost
{
  namespace system
  {
    //  To construct an error_code after a API error:
    //
    //      error_code( errno, system_category() )

    //  User code should use the portable "posix" enums for POSIX errors; this
    //  allows such code to be portable to non-POSIX systems. For the non-POSIX
    //  errno values that POSIX-based systems typically provide in addition to 
    //  POSIX values, use the system specific enums below.

    namespace linux_error
    {
      enum linux_errno
      {
        advertise_error = EADV,
        bad_exchange = EBADE,
        bad_file_number = EBADFD,
        bad_font_format = EBFONT,
        bad_request_code = EBADRQC,
        bad_request_descriptor = EBADR,
        bad_slot = EBADSLT,
        channel_range = ECHRNG,
        communication_error = ECOMM,
        dot_dot_error = EDOTDOT,
        exchange_full = EXFULL,
        host_down = EHOSTDOWN,
        is_named_file_type= EISNAM,
        key_expired = EKEYEXPIRED,
        key_rejected = EKEYREJECTED,
        key_revoked = EKEYREVOKED,
        level2_halt= EL2HLT,
        level2_no_syncronized= EL2NSYNC,
        level3_halt = EL3HLT,
        level3_reset = EL3RST,
        link_range = ELNRNG,
        medium_type = EMEDIUMTYPE,
        no_anode= ENOANO,
        no_block_device = ENOTBLK,
        no_csi = ENOCSI,
        no_key = ENOKEY,
        no_medium = ENOMEDIUM,
        no_network = ENONET,
        no_package = ENOPKG,
        not_avail = ENAVAIL,
        not_named_file_type= ENOTNAM,
        not_recoverable = ENOTRECOVERABLE,
        not_unique = ENOTUNIQ,
        owner_dead = EOWNERDEAD,
        protocol_no_supported = EPFNOSUPPORT,
        remote_address_changed = EREMCHG,
        remote_io_error = EREMOTEIO,
        remote_object = EREMOTE,
        restart_needed = ERESTART,
        shared_library_access = ELIBACC,
        shared_library_bad = ELIBBAD,
        shared_library_execute = ELIBEXEC,
        shared_library_max_ = ELIBMAX,
        shared_library_section= ELIBSCN,
        shutdown = ESHUTDOWN,
        socket_type_not_supported = ESOCKTNOSUPPORT,
        srmount_error = ESRMNT,
        stream_pipe_error = ESTRPIPE,
        too_many_references = ETOOMANYREFS,
        too_many_users = EUSERS,
        unattached = EUNATCH,
        unclean = EUCLEAN
      };
    }  // namespace linux_error

# ifdef BOOST_SYSTEM_ENABLE_DEPRECATED
    namespace Linux = linux_error;
# endif

    template<> struct is_error_code_enum<linux_error::linux_errno>
      { static const bool value = true; };

    namespace linux_error
    {
      inline error_code make_error_code( linux_errno e )
        { return error_code( e, system_category() ); }
    }

  }  // namespace system
}  // namespace boost 

#endif  // Linux

#endif  // BOOST_SYSTEM_LINUX_ERROR_HPP

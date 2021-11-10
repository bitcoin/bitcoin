//
// detail/socket_ops.hpp
// ~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_DETAIL_SOCKET_OPS_HPP
#define BOOST_ASIO_DETAIL_SOCKET_OPS_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>

#include <boost/system/error_code.hpp>
#include <boost/asio/detail/memory.hpp>
#include <boost/asio/detail/socket_types.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace detail {
namespace socket_ops {

// Socket state bits.
enum
{
  // The user wants a non-blocking socket.
  user_set_non_blocking = 1,

  // The socket has been set non-blocking.
  internal_non_blocking = 2,

  // Helper "state" used to determine whether the socket is non-blocking.
  non_blocking = user_set_non_blocking | internal_non_blocking,

  // User wants connection_aborted errors, which are disabled by default.
  enable_connection_aborted = 4,

  // The user set the linger option. Needs to be checked when closing.
  user_set_linger = 8,

  // The socket is stream-oriented.
  stream_oriented = 16,

  // The socket is datagram-oriented.
  datagram_oriented = 32,

  // The socket may have been dup()-ed.
  possible_dup = 64
};

typedef unsigned char state_type;

struct noop_deleter { void operator()(void*) {} };
typedef shared_ptr<void> shared_cancel_token_type;
typedef weak_ptr<void> weak_cancel_token_type;

#if !defined(BOOST_ASIO_WINDOWS_RUNTIME)

BOOST_ASIO_DECL socket_type accept(socket_type s, socket_addr_type* addr,
    std::size_t* addrlen, boost::system::error_code& ec);

BOOST_ASIO_DECL socket_type sync_accept(socket_type s,
    state_type state, socket_addr_type* addr,
    std::size_t* addrlen, boost::system::error_code& ec);

#if defined(BOOST_ASIO_HAS_IOCP)

BOOST_ASIO_DECL void complete_iocp_accept(socket_type s,
    void* output_buffer, DWORD address_length,
    socket_addr_type* addr, std::size_t* addrlen,
    socket_type new_socket, boost::system::error_code& ec);

#else // defined(BOOST_ASIO_HAS_IOCP)

BOOST_ASIO_DECL bool non_blocking_accept(socket_type s,
    state_type state, socket_addr_type* addr, std::size_t* addrlen,
    boost::system::error_code& ec, socket_type& new_socket);

#endif // defined(BOOST_ASIO_HAS_IOCP)

BOOST_ASIO_DECL int bind(socket_type s, const socket_addr_type* addr,
    std::size_t addrlen, boost::system::error_code& ec);

BOOST_ASIO_DECL int close(socket_type s, state_type& state,
    bool destruction, boost::system::error_code& ec);

BOOST_ASIO_DECL bool set_user_non_blocking(socket_type s,
    state_type& state, bool value, boost::system::error_code& ec);

BOOST_ASIO_DECL bool set_internal_non_blocking(socket_type s,
    state_type& state, bool value, boost::system::error_code& ec);

BOOST_ASIO_DECL int shutdown(socket_type s,
    int what, boost::system::error_code& ec);

BOOST_ASIO_DECL int connect(socket_type s, const socket_addr_type* addr,
    std::size_t addrlen, boost::system::error_code& ec);

BOOST_ASIO_DECL void sync_connect(socket_type s, const socket_addr_type* addr,
    std::size_t addrlen, boost::system::error_code& ec);

#if defined(BOOST_ASIO_HAS_IOCP)

BOOST_ASIO_DECL void complete_iocp_connect(socket_type s,
    boost::system::error_code& ec);

#endif // defined(BOOST_ASIO_HAS_IOCP)

BOOST_ASIO_DECL bool non_blocking_connect(socket_type s,
    boost::system::error_code& ec);

BOOST_ASIO_DECL int socketpair(int af, int type, int protocol,
    socket_type sv[2], boost::system::error_code& ec);

BOOST_ASIO_DECL bool sockatmark(socket_type s, boost::system::error_code& ec);

BOOST_ASIO_DECL size_t available(socket_type s, boost::system::error_code& ec);

BOOST_ASIO_DECL int listen(socket_type s,
    int backlog, boost::system::error_code& ec);

#if defined(BOOST_ASIO_WINDOWS) || defined(__CYGWIN__)
typedef WSABUF buf;
#else // defined(BOOST_ASIO_WINDOWS) || defined(__CYGWIN__)
typedef iovec buf;
#endif // defined(BOOST_ASIO_WINDOWS) || defined(__CYGWIN__)

BOOST_ASIO_DECL void init_buf(buf& b, void* data, size_t size);

BOOST_ASIO_DECL void init_buf(buf& b, const void* data, size_t size);

BOOST_ASIO_DECL signed_size_type recv(socket_type s, buf* bufs,
    size_t count, int flags, boost::system::error_code& ec);

BOOST_ASIO_DECL signed_size_type recv1(socket_type s,
    void* data, size_t size, int flags, boost::system::error_code& ec);

BOOST_ASIO_DECL size_t sync_recv(socket_type s, state_type state, buf* bufs,
    size_t count, int flags, bool all_empty, boost::system::error_code& ec);

BOOST_ASIO_DECL size_t sync_recv1(socket_type s, state_type state,
    void* data, size_t size, int flags, boost::system::error_code& ec);

#if defined(BOOST_ASIO_HAS_IOCP)

BOOST_ASIO_DECL void complete_iocp_recv(state_type state,
    const weak_cancel_token_type& cancel_token, bool all_empty,
    boost::system::error_code& ec, size_t bytes_transferred);

#else // defined(BOOST_ASIO_HAS_IOCP)

BOOST_ASIO_DECL bool non_blocking_recv(socket_type s,
    buf* bufs, size_t count, int flags, bool is_stream,
    boost::system::error_code& ec, size_t& bytes_transferred);

BOOST_ASIO_DECL bool non_blocking_recv1(socket_type s,
    void* data, size_t size, int flags, bool is_stream,
    boost::system::error_code& ec, size_t& bytes_transferred);

#endif // defined(BOOST_ASIO_HAS_IOCP)

BOOST_ASIO_DECL signed_size_type recvfrom(socket_type s, buf* bufs,
    size_t count, int flags, socket_addr_type* addr,
    std::size_t* addrlen, boost::system::error_code& ec);

BOOST_ASIO_DECL signed_size_type recvfrom1(socket_type s, void* data,
    size_t size, int flags, socket_addr_type* addr,
    std::size_t* addrlen, boost::system::error_code& ec);

BOOST_ASIO_DECL size_t sync_recvfrom(socket_type s, state_type state,
    buf* bufs, size_t count, int flags, socket_addr_type* addr,
    std::size_t* addrlen, boost::system::error_code& ec);

BOOST_ASIO_DECL size_t sync_recvfrom1(socket_type s, state_type state,
    void* data, size_t size, int flags, socket_addr_type* addr,
    std::size_t* addrlen, boost::system::error_code& ec);

#if defined(BOOST_ASIO_HAS_IOCP)

BOOST_ASIO_DECL void complete_iocp_recvfrom(
    const weak_cancel_token_type& cancel_token,
    boost::system::error_code& ec);

#else // defined(BOOST_ASIO_HAS_IOCP)

BOOST_ASIO_DECL bool non_blocking_recvfrom(socket_type s,
    buf* bufs, size_t count, int flags,
    socket_addr_type* addr, std::size_t* addrlen,
    boost::system::error_code& ec, size_t& bytes_transferred);

BOOST_ASIO_DECL bool non_blocking_recvfrom1(socket_type s,
    void* data, size_t size, int flags,
    socket_addr_type* addr, std::size_t* addrlen,
    boost::system::error_code& ec, size_t& bytes_transferred);

#endif // defined(BOOST_ASIO_HAS_IOCP)

BOOST_ASIO_DECL signed_size_type recvmsg(socket_type s, buf* bufs,
    size_t count, int in_flags, int& out_flags,
    boost::system::error_code& ec);

BOOST_ASIO_DECL size_t sync_recvmsg(socket_type s, state_type state,
    buf* bufs, size_t count, int in_flags, int& out_flags,
    boost::system::error_code& ec);

#if defined(BOOST_ASIO_HAS_IOCP)

BOOST_ASIO_DECL void complete_iocp_recvmsg(
    const weak_cancel_token_type& cancel_token,
    boost::system::error_code& ec);

#else // defined(BOOST_ASIO_HAS_IOCP)

BOOST_ASIO_DECL bool non_blocking_recvmsg(socket_type s,
    buf* bufs, size_t count, int in_flags, int& out_flags,
    boost::system::error_code& ec, size_t& bytes_transferred);

#endif // defined(BOOST_ASIO_HAS_IOCP)

BOOST_ASIO_DECL signed_size_type send(socket_type s, const buf* bufs,
    size_t count, int flags, boost::system::error_code& ec);

BOOST_ASIO_DECL signed_size_type send1(socket_type s,
    const void* data, size_t size, int flags, boost::system::error_code& ec);

BOOST_ASIO_DECL size_t sync_send(socket_type s, state_type state,
    const buf* bufs, size_t count, int flags,
    bool all_empty, boost::system::error_code& ec);

BOOST_ASIO_DECL size_t sync_send1(socket_type s, state_type state,
    const void* data, size_t size, int flags, boost::system::error_code& ec);

#if defined(BOOST_ASIO_HAS_IOCP)

BOOST_ASIO_DECL void complete_iocp_send(
    const weak_cancel_token_type& cancel_token,
    boost::system::error_code& ec);

#else // defined(BOOST_ASIO_HAS_IOCP)

BOOST_ASIO_DECL bool non_blocking_send(socket_type s,
    const buf* bufs, size_t count, int flags,
    boost::system::error_code& ec, size_t& bytes_transferred);

BOOST_ASIO_DECL bool non_blocking_send1(socket_type s,
    const void* data, size_t size, int flags,
    boost::system::error_code& ec, size_t& bytes_transferred);

#endif // defined(BOOST_ASIO_HAS_IOCP)

BOOST_ASIO_DECL signed_size_type sendto(socket_type s, const buf* bufs,
    size_t count, int flags, const socket_addr_type* addr,
    std::size_t addrlen, boost::system::error_code& ec);

BOOST_ASIO_DECL signed_size_type sendto1(socket_type s, const void* data,
    size_t size, int flags, const socket_addr_type* addr,
    std::size_t addrlen, boost::system::error_code& ec);

BOOST_ASIO_DECL size_t sync_sendto(socket_type s, state_type state,
    const buf* bufs, size_t count, int flags, const socket_addr_type* addr,
    std::size_t addrlen, boost::system::error_code& ec);

BOOST_ASIO_DECL size_t sync_sendto1(socket_type s, state_type state,
    const void* data, size_t size, int flags, const socket_addr_type* addr,
    std::size_t addrlen, boost::system::error_code& ec);

#if !defined(BOOST_ASIO_HAS_IOCP)

BOOST_ASIO_DECL bool non_blocking_sendto(socket_type s,
    const buf* bufs, size_t count, int flags,
    const socket_addr_type* addr, std::size_t addrlen,
    boost::system::error_code& ec, size_t& bytes_transferred);

BOOST_ASIO_DECL bool non_blocking_sendto1(socket_type s,
    const void* data, size_t size, int flags,
    const socket_addr_type* addr, std::size_t addrlen,
    boost::system::error_code& ec, size_t& bytes_transferred);

#endif // !defined(BOOST_ASIO_HAS_IOCP)

BOOST_ASIO_DECL socket_type socket(int af, int type, int protocol,
    boost::system::error_code& ec);

BOOST_ASIO_DECL int setsockopt(socket_type s, state_type& state,
    int level, int optname, const void* optval,
    std::size_t optlen, boost::system::error_code& ec);

BOOST_ASIO_DECL int getsockopt(socket_type s, state_type state,
    int level, int optname, void* optval,
    size_t* optlen, boost::system::error_code& ec);

BOOST_ASIO_DECL int getpeername(socket_type s, socket_addr_type* addr,
    std::size_t* addrlen, bool cached, boost::system::error_code& ec);

BOOST_ASIO_DECL int getsockname(socket_type s, socket_addr_type* addr,
    std::size_t* addrlen, boost::system::error_code& ec);

BOOST_ASIO_DECL int ioctl(socket_type s, state_type& state,
    int cmd, ioctl_arg_type* arg, boost::system::error_code& ec);

BOOST_ASIO_DECL int select(int nfds, fd_set* readfds, fd_set* writefds,
    fd_set* exceptfds, timeval* timeout, boost::system::error_code& ec);

BOOST_ASIO_DECL int poll_read(socket_type s,
    state_type state, int msec, boost::system::error_code& ec);

BOOST_ASIO_DECL int poll_write(socket_type s,
    state_type state, int msec, boost::system::error_code& ec);

BOOST_ASIO_DECL int poll_error(socket_type s,
    state_type state, int msec, boost::system::error_code& ec);

BOOST_ASIO_DECL int poll_connect(socket_type s,
    int msec, boost::system::error_code& ec);

#endif // !defined(BOOST_ASIO_WINDOWS_RUNTIME)

BOOST_ASIO_DECL const char* inet_ntop(int af, const void* src, char* dest,
    size_t length, unsigned long scope_id, boost::system::error_code& ec);

BOOST_ASIO_DECL int inet_pton(int af, const char* src, void* dest,
    unsigned long* scope_id, boost::system::error_code& ec);

BOOST_ASIO_DECL int gethostname(char* name,
    int namelen, boost::system::error_code& ec);

#if !defined(BOOST_ASIO_WINDOWS_RUNTIME)

BOOST_ASIO_DECL boost::system::error_code getaddrinfo(const char* host,
    const char* service, const addrinfo_type& hints,
    addrinfo_type** result, boost::system::error_code& ec);

BOOST_ASIO_DECL boost::system::error_code background_getaddrinfo(
    const weak_cancel_token_type& cancel_token, const char* host,
    const char* service, const addrinfo_type& hints,
    addrinfo_type** result, boost::system::error_code& ec);

BOOST_ASIO_DECL void freeaddrinfo(addrinfo_type* ai);

BOOST_ASIO_DECL boost::system::error_code getnameinfo(
    const socket_addr_type* addr, std::size_t addrlen,
    char* host, std::size_t hostlen, char* serv,
    std::size_t servlen, int flags, boost::system::error_code& ec);

BOOST_ASIO_DECL boost::system::error_code sync_getnameinfo(
    const socket_addr_type* addr, std::size_t addrlen,
    char* host, std::size_t hostlen, char* serv,
    std::size_t servlen, int sock_type, boost::system::error_code& ec);

BOOST_ASIO_DECL boost::system::error_code background_getnameinfo(
    const weak_cancel_token_type& cancel_token,
    const socket_addr_type* addr, std::size_t addrlen,
    char* host, std::size_t hostlen, char* serv,
    std::size_t servlen, int sock_type, boost::system::error_code& ec);

#endif // !defined(BOOST_ASIO_WINDOWS_RUNTIME)

BOOST_ASIO_DECL u_long_type network_to_host_long(u_long_type value);

BOOST_ASIO_DECL u_long_type host_to_network_long(u_long_type value);

BOOST_ASIO_DECL u_short_type network_to_host_short(u_short_type value);

BOOST_ASIO_DECL u_short_type host_to_network_short(u_short_type value);

} // namespace socket_ops
} // namespace detail
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#if defined(BOOST_ASIO_HEADER_ONLY)
# include <boost/asio/detail/impl/socket_ops.ipp>
#endif // defined(BOOST_ASIO_HEADER_ONLY)

#endif // BOOST_ASIO_DETAIL_SOCKET_OPS_HPP

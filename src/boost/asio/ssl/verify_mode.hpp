//
// ssl/verify_mode.hpp
// ~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_SSL_VERIFY_MODE_HPP
#define BOOST_ASIO_SSL_VERIFY_MODE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <boost/asio/ssl/detail/openssl_types.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace ssl {

/// Bitmask type for peer verification.
/**
 * Possible values are:
 *
 * @li @ref verify_none
 * @li @ref verify_peer
 * @li @ref verify_fail_if_no_peer_cert
 * @li @ref verify_client_once
 */
typedef int verify_mode;

#if defined(GENERATING_DOCUMENTATION)
/// No verification.
const int verify_none = implementation_defined;

/// Verify the peer.
const int verify_peer = implementation_defined;

/// Fail verification if the peer has no certificate. Ignored unless
/// @ref verify_peer is set.
const int verify_fail_if_no_peer_cert = implementation_defined;

/// Do not request client certificate on renegotiation. Ignored unless
/// @ref verify_peer is set.
const int verify_client_once = implementation_defined;
#else
const int verify_none = SSL_VERIFY_NONE;
const int verify_peer = SSL_VERIFY_PEER;
const int verify_fail_if_no_peer_cert = SSL_VERIFY_FAIL_IF_NO_PEER_CERT;
const int verify_client_once = SSL_VERIFY_CLIENT_ONCE;
#endif

} // namespace ssl
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_SSL_VERIFY_MODE_HPP

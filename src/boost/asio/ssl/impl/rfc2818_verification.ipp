//
// ssl/impl/rfc2818_verification.ipp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_SSL_IMPL_RFC2818_VERIFICATION_IPP
#define BOOST_ASIO_SSL_IMPL_RFC2818_VERIFICATION_IPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>

#if !defined(BOOST_ASIO_NO_DEPRECATED)

#include <cctype>
#include <cstring>
#include <boost/asio/ip/address.hpp>
#include <boost/asio/ssl/rfc2818_verification.hpp>
#include <boost/asio/ssl/detail/openssl_types.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace ssl {

bool rfc2818_verification::operator()(
    bool preverified, verify_context& ctx) const
{
  using namespace std; // For memcmp.

  // Don't bother looking at certificates that have failed pre-verification.
  if (!preverified)
    return false;

  // We're only interested in checking the certificate at the end of the chain.
  int depth = X509_STORE_CTX_get_error_depth(ctx.native_handle());
  if (depth > 0)
    return true;

  // Try converting the host name to an address. If it is an address then we
  // need to look for an IP address in the certificate rather than a host name.
  boost::system::error_code ec;
  ip::address address = ip::make_address(host_, ec);
  bool is_address = !ec;

  X509* cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());

  // Go through the alternate names in the certificate looking for matching DNS
  // or IP address entries.
  GENERAL_NAMES* gens = static_cast<GENERAL_NAMES*>(
      X509_get_ext_d2i(cert, NID_subject_alt_name, 0, 0));
  for (int i = 0; i < sk_GENERAL_NAME_num(gens); ++i)
  {
    GENERAL_NAME* gen = sk_GENERAL_NAME_value(gens, i);
    if (gen->type == GEN_DNS && !is_address)
    {
      ASN1_IA5STRING* domain = gen->d.dNSName;
      if (domain->type == V_ASN1_IA5STRING && domain->data && domain->length)
      {
        const char* pattern = reinterpret_cast<const char*>(domain->data);
        std::size_t pattern_length = domain->length;
        if (match_pattern(pattern, pattern_length, host_.c_str()))
        {
          GENERAL_NAMES_free(gens);
          return true;
        }
      }
    }
    else if (gen->type == GEN_IPADD && is_address)
    {
      ASN1_OCTET_STRING* ip_address = gen->d.iPAddress;
      if (ip_address->type == V_ASN1_OCTET_STRING && ip_address->data)
      {
        if (address.is_v4() && ip_address->length == 4)
        {
          ip::address_v4::bytes_type bytes = address.to_v4().to_bytes();
          if (memcmp(bytes.data(), ip_address->data, 4) == 0)
          {
            GENERAL_NAMES_free(gens);
            return true;
          }
        }
        else if (address.is_v6() && ip_address->length == 16)
        {
          ip::address_v6::bytes_type bytes = address.to_v6().to_bytes();
          if (memcmp(bytes.data(), ip_address->data, 16) == 0)
          {
            GENERAL_NAMES_free(gens);
            return true;
          }
        }
      }
    }
  }
  GENERAL_NAMES_free(gens);

  // No match in the alternate names, so try the common names. We should only
  // use the "most specific" common name, which is the last one in the list.
  X509_NAME* name = X509_get_subject_name(cert);
  int i = -1;
  ASN1_STRING* common_name = 0;
  while ((i = X509_NAME_get_index_by_NID(name, NID_commonName, i)) >= 0)
  {
    X509_NAME_ENTRY* name_entry = X509_NAME_get_entry(name, i);
    common_name = X509_NAME_ENTRY_get_data(name_entry);
  }
  if (common_name && common_name->data && common_name->length)
  {
    const char* pattern = reinterpret_cast<const char*>(common_name->data);
    std::size_t pattern_length = common_name->length;
    if (match_pattern(pattern, pattern_length, host_.c_str()))
      return true;
  }

  return false;
}

bool rfc2818_verification::match_pattern(const char* pattern,
    std::size_t pattern_length, const char* host)
{
  using namespace std; // For tolower.

  const char* p = pattern;
  const char* p_end = p + pattern_length;
  const char* h = host;

  while (p != p_end && *h)
  {
    if (*p == '*')
    {
      ++p;
      while (*h && *h != '.')
        if (match_pattern(p, p_end - p, h++))
          return true;
    }
    else if (tolower(*p) == tolower(*h))
    {
      ++p;
      ++h;
    }
    else
    {
      return false;
    }
  }

  return p == p_end && !*h;
}

} // namespace ssl
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // !defined(BOOST_ASIO_NO_DEPRECATED)

#endif // BOOST_ASIO_SSL_IMPL_RFC2818_VERIFICATION_IPP

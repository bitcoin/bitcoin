//
// detail/impl/buffer_sequence_adapter.ipp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_DETAIL_IMPL_BUFFER_SEQUENCE_ADAPTER_IPP
#define BOOST_ASIO_DETAIL_IMPL_BUFFER_SEQUENCE_ADAPTER_IPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>

#if defined(BOOST_ASIO_WINDOWS_RUNTIME)

#include <robuffer.h>
#include <windows.storage.streams.h>
#include <wrl/implements.h>
#include <boost/asio/detail/buffer_sequence_adapter.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace detail {

class winrt_buffer_impl :
  public Microsoft::WRL::RuntimeClass<
    Microsoft::WRL::RuntimeClassFlags<
      Microsoft::WRL::RuntimeClassType::WinRtClassicComMix>,
    ABI::Windows::Storage::Streams::IBuffer,
    Windows::Storage::Streams::IBufferByteAccess>
{
public:
  explicit winrt_buffer_impl(const boost::asio::const_buffer& b)
  {
    bytes_ = const_cast<byte*>(static_cast<const byte*>(b.data()));
    length_ = b.size();
    capacity_ = b.size();
  }

  explicit winrt_buffer_impl(const boost::asio::mutable_buffer& b)
  {
    bytes_ = static_cast<byte*>(b.data());
    length_ = 0;
    capacity_ = b.size();
  }

  ~winrt_buffer_impl()
  {
  }

  STDMETHODIMP Buffer(byte** value)
  {
    *value = bytes_;
    return S_OK;
  }

  STDMETHODIMP get_Capacity(UINT32* value)
  {
    *value = capacity_;
    return S_OK;
  }

  STDMETHODIMP get_Length(UINT32 *value)
  {
    *value = length_;
    return S_OK;
  }

  STDMETHODIMP put_Length(UINT32 value)
  {
    if (value > capacity_)
      return E_INVALIDARG;
    length_ = value;
    return S_OK;
  }

private:
  byte* bytes_;
  UINT32 length_;
  UINT32 capacity_;
};

void buffer_sequence_adapter_base::init_native_buffer(
    buffer_sequence_adapter_base::native_buffer_type& buf,
    const boost::asio::mutable_buffer& buffer)
{
  std::memset(&buf, 0, sizeof(native_buffer_type));
  Microsoft::WRL::ComPtr<IInspectable> insp
    = Microsoft::WRL::Make<winrt_buffer_impl>(buffer);
  buf = reinterpret_cast<Windows::Storage::Streams::IBuffer^>(insp.Get());
}

void buffer_sequence_adapter_base::init_native_buffer(
    buffer_sequence_adapter_base::native_buffer_type& buf,
    const boost::asio::const_buffer& buffer)
{
  std::memset(&buf, 0, sizeof(native_buffer_type));
  Microsoft::WRL::ComPtr<IInspectable> insp
    = Microsoft::WRL::Make<winrt_buffer_impl>(buffer);
  Platform::Object^ buf_obj = reinterpret_cast<Platform::Object^>(insp.Get());
  buf = reinterpret_cast<Windows::Storage::Streams::IBuffer^>(insp.Get());
}

} // namespace detail
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // defined(BOOST_ASIO_WINDOWS_RUNTIME)

#endif // BOOST_ASIO_DETAIL_IMPL_BUFFER_SEQUENCE_ADAPTER_IPP

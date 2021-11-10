//
// impl/buffered_write_stream.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_IMPL_BUFFERED_WRITE_STREAM_HPP
#define BOOST_ASIO_IMPL_BUFFERED_WRITE_STREAM_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/associator.hpp>
#include <boost/asio/detail/handler_alloc_helpers.hpp>
#include <boost/asio/detail/handler_cont_helpers.hpp>
#include <boost/asio/detail/handler_invoke_helpers.hpp>
#include <boost/asio/detail/handler_type_requirements.hpp>
#include <boost/asio/detail/non_const_lvalue.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {

template <typename Stream>
std::size_t buffered_write_stream<Stream>::flush()
{
  std::size_t bytes_written = write(next_layer_,
      buffer(storage_.data(), storage_.size()));
  storage_.consume(bytes_written);
  return bytes_written;
}

template <typename Stream>
std::size_t buffered_write_stream<Stream>::flush(boost::system::error_code& ec)
{
  std::size_t bytes_written = write(next_layer_,
      buffer(storage_.data(), storage_.size()),
      transfer_all(), ec);
  storage_.consume(bytes_written);
  return bytes_written;
}

namespace detail
{
  template <typename WriteHandler>
  class buffered_flush_handler
  {
  public:
    buffered_flush_handler(detail::buffered_stream_storage& storage,
        WriteHandler& handler)
      : storage_(storage),
        handler_(BOOST_ASIO_MOVE_CAST(WriteHandler)(handler))
    {
    }

#if defined(BOOST_ASIO_HAS_MOVE)
    buffered_flush_handler(const buffered_flush_handler& other)
      : storage_(other.storage_),
        handler_(other.handler_)
    {
    }

    buffered_flush_handler(buffered_flush_handler&& other)
      : storage_(other.storage_),
        handler_(BOOST_ASIO_MOVE_CAST(WriteHandler)(other.handler_))
    {
    }
#endif // defined(BOOST_ASIO_HAS_MOVE)

    void operator()(const boost::system::error_code& ec,
        const std::size_t bytes_written)
    {
      storage_.consume(bytes_written);
      BOOST_ASIO_MOVE_OR_LVALUE(WriteHandler)(handler_)(ec, bytes_written);
    }

  //private:
    detail::buffered_stream_storage& storage_;
    WriteHandler handler_;
  };

  template <typename WriteHandler>
  inline asio_handler_allocate_is_deprecated
  asio_handler_allocate(std::size_t size,
      buffered_flush_handler<WriteHandler>* this_handler)
  {
#if defined(BOOST_ASIO_NO_DEPRECATED)
    boost_asio_handler_alloc_helpers::allocate(size, this_handler->handler_);
    return asio_handler_allocate_is_no_longer_used();
#else // defined(BOOST_ASIO_NO_DEPRECATED)
    return boost_asio_handler_alloc_helpers::allocate(
        size, this_handler->handler_);
#endif // defined(BOOST_ASIO_NO_DEPRECATED)
  }

  template <typename WriteHandler>
  inline asio_handler_deallocate_is_deprecated
  asio_handler_deallocate(void* pointer, std::size_t size,
      buffered_flush_handler<WriteHandler>* this_handler)
  {
    boost_asio_handler_alloc_helpers::deallocate(
        pointer, size, this_handler->handler_);
#if defined(BOOST_ASIO_NO_DEPRECATED)
    return asio_handler_deallocate_is_no_longer_used();
#endif // defined(BOOST_ASIO_NO_DEPRECATED)
  }

  template <typename WriteHandler>
  inline bool asio_handler_is_continuation(
      buffered_flush_handler<WriteHandler>* this_handler)
  {
    return boost_asio_handler_cont_helpers::is_continuation(
          this_handler->handler_);
  }

  template <typename Function, typename WriteHandler>
  inline asio_handler_invoke_is_deprecated
  asio_handler_invoke(Function& function,
      buffered_flush_handler<WriteHandler>* this_handler)
  {
    boost_asio_handler_invoke_helpers::invoke(
        function, this_handler->handler_);
#if defined(BOOST_ASIO_NO_DEPRECATED)
    return asio_handler_invoke_is_no_longer_used();
#endif // defined(BOOST_ASIO_NO_DEPRECATED)
  }

  template <typename Function, typename WriteHandler>
  inline asio_handler_invoke_is_deprecated
  asio_handler_invoke(const Function& function,
      buffered_flush_handler<WriteHandler>* this_handler)
  {
    boost_asio_handler_invoke_helpers::invoke(
        function, this_handler->handler_);
#if defined(BOOST_ASIO_NO_DEPRECATED)
    return asio_handler_invoke_is_no_longer_used();
#endif // defined(BOOST_ASIO_NO_DEPRECATED)
  }

  template <typename Stream>
  class initiate_async_buffered_flush
  {
  public:
    typedef typename remove_reference<
      Stream>::type::lowest_layer_type::executor_type executor_type;

    explicit initiate_async_buffered_flush(
        typename remove_reference<Stream>::type& next_layer)
      : next_layer_(next_layer)
    {
    }

    executor_type get_executor() const BOOST_ASIO_NOEXCEPT
    {
      return next_layer_.lowest_layer().get_executor();
    }

    template <typename WriteHandler>
    void operator()(BOOST_ASIO_MOVE_ARG(WriteHandler) handler,
        buffered_stream_storage* storage) const
    {
      // If you get an error on the following line it means that your handler
      // does not meet the documented type requirements for a WriteHandler.
      BOOST_ASIO_WRITE_HANDLER_CHECK(WriteHandler, handler) type_check;

      non_const_lvalue<WriteHandler> handler2(handler);
      async_write(next_layer_, buffer(storage->data(), storage->size()),
          buffered_flush_handler<typename decay<WriteHandler>::type>(
            *storage, handler2.value));
    }

  private:
    typename remove_reference<Stream>::type& next_layer_;
  };
} // namespace detail

#if !defined(GENERATING_DOCUMENTATION)

template <template <typename, typename> class Associator,
    typename WriteHandler, typename DefaultCandidate>
struct associator<Associator,
    detail::buffered_flush_handler<WriteHandler>,
    DefaultCandidate>
  : Associator<WriteHandler, DefaultCandidate>
{
  static typename Associator<WriteHandler, DefaultCandidate>::type get(
      const detail::buffered_flush_handler<WriteHandler>& h,
      const DefaultCandidate& c = DefaultCandidate()) BOOST_ASIO_NOEXCEPT
  {
    return Associator<WriteHandler, DefaultCandidate>::get(h.handler_, c);
  }
};

#endif // !defined(GENERATING_DOCUMENTATION)

template <typename Stream>
template <
    BOOST_ASIO_COMPLETION_TOKEN_FOR(void (boost::system::error_code,
      std::size_t)) WriteHandler>
BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(WriteHandler,
    void (boost::system::error_code, std::size_t))
buffered_write_stream<Stream>::async_flush(
    BOOST_ASIO_MOVE_ARG(WriteHandler) handler)
{
  return async_initiate<WriteHandler,
    void (boost::system::error_code, std::size_t)>(
      detail::initiate_async_buffered_flush<Stream>(next_layer_),
      handler, &storage_);
}

template <typename Stream>
template <typename ConstBufferSequence>
std::size_t buffered_write_stream<Stream>::write_some(
    const ConstBufferSequence& buffers)
{
  using boost::asio::buffer_size;
  if (buffer_size(buffers) == 0)
    return 0;

  if (storage_.size() == storage_.capacity())
    this->flush();

  return this->copy(buffers);
}

template <typename Stream>
template <typename ConstBufferSequence>
std::size_t buffered_write_stream<Stream>::write_some(
    const ConstBufferSequence& buffers, boost::system::error_code& ec)
{
  ec = boost::system::error_code();

  using boost::asio::buffer_size;
  if (buffer_size(buffers) == 0)
    return 0;

  if (storage_.size() == storage_.capacity() && !flush(ec))
    return 0;

  return this->copy(buffers);
}

namespace detail
{
  template <typename ConstBufferSequence, typename WriteHandler>
  class buffered_write_some_handler
  {
  public:
    buffered_write_some_handler(detail::buffered_stream_storage& storage,
        const ConstBufferSequence& buffers, WriteHandler& handler)
      : storage_(storage),
        buffers_(buffers),
        handler_(BOOST_ASIO_MOVE_CAST(WriteHandler)(handler))
    {
    }

#if defined(BOOST_ASIO_HAS_MOVE)
      buffered_write_some_handler(const buffered_write_some_handler& other)
        : storage_(other.storage_),
          buffers_(other.buffers_),
          handler_(other.handler_)
      {
      }

      buffered_write_some_handler(buffered_write_some_handler&& other)
        : storage_(other.storage_),
          buffers_(other.buffers_),
          handler_(BOOST_ASIO_MOVE_CAST(WriteHandler)(other.handler_))
      {
      }
#endif // defined(BOOST_ASIO_HAS_MOVE)

    void operator()(const boost::system::error_code& ec, std::size_t)
    {
      if (ec)
      {
        const std::size_t length = 0;
        BOOST_ASIO_MOVE_OR_LVALUE(WriteHandler)(handler_)(ec, length);
      }
      else
      {
        using boost::asio::buffer_size;
        std::size_t orig_size = storage_.size();
        std::size_t space_avail = storage_.capacity() - orig_size;
        std::size_t bytes_avail = buffer_size(buffers_);
        std::size_t length = bytes_avail < space_avail
          ? bytes_avail : space_avail;
        storage_.resize(orig_size + length);
        const std::size_t bytes_copied = boost::asio::buffer_copy(
            storage_.data() + orig_size, buffers_, length);
        BOOST_ASIO_MOVE_OR_LVALUE(WriteHandler)(handler_)(ec, bytes_copied);
      }
    }

  //private:
    detail::buffered_stream_storage& storage_;
    ConstBufferSequence buffers_;
    WriteHandler handler_;
  };

  template <typename ConstBufferSequence, typename WriteHandler>
  inline asio_handler_allocate_is_deprecated
  asio_handler_allocate(std::size_t size,
      buffered_write_some_handler<
        ConstBufferSequence, WriteHandler>* this_handler)
  {
#if defined(BOOST_ASIO_NO_DEPRECATED)
    boost_asio_handler_alloc_helpers::allocate(size, this_handler->handler_);
    return asio_handler_allocate_is_no_longer_used();
#else // defined(BOOST_ASIO_NO_DEPRECATED)
    return boost_asio_handler_alloc_helpers::allocate(
        size, this_handler->handler_);
#endif // defined(BOOST_ASIO_NO_DEPRECATED)
  }

  template <typename ConstBufferSequence, typename WriteHandler>
  inline asio_handler_deallocate_is_deprecated
  asio_handler_deallocate(void* pointer, std::size_t size,
      buffered_write_some_handler<
        ConstBufferSequence, WriteHandler>* this_handler)
  {
    boost_asio_handler_alloc_helpers::deallocate(
        pointer, size, this_handler->handler_);
#if defined(BOOST_ASIO_NO_DEPRECATED)
    return asio_handler_deallocate_is_no_longer_used();
#endif // defined(BOOST_ASIO_NO_DEPRECATED)
  }

  template <typename ConstBufferSequence, typename WriteHandler>
  inline bool asio_handler_is_continuation(
      buffered_write_some_handler<
        ConstBufferSequence, WriteHandler>* this_handler)
  {
    return boost_asio_handler_cont_helpers::is_continuation(
          this_handler->handler_);
  }

  template <typename Function, typename ConstBufferSequence,
      typename WriteHandler>
  inline asio_handler_invoke_is_deprecated
  asio_handler_invoke(Function& function,
      buffered_write_some_handler<
        ConstBufferSequence, WriteHandler>* this_handler)
  {
    boost_asio_handler_invoke_helpers::invoke(
        function, this_handler->handler_);
#if defined(BOOST_ASIO_NO_DEPRECATED)
    return asio_handler_invoke_is_no_longer_used();
#endif // defined(BOOST_ASIO_NO_DEPRECATED)
  }

  template <typename Function, typename ConstBufferSequence,
      typename WriteHandler>
  inline asio_handler_invoke_is_deprecated
  asio_handler_invoke(const Function& function,
      buffered_write_some_handler<
        ConstBufferSequence, WriteHandler>* this_handler)
  {
    boost_asio_handler_invoke_helpers::invoke(
        function, this_handler->handler_);
#if defined(BOOST_ASIO_NO_DEPRECATED)
    return asio_handler_invoke_is_no_longer_used();
#endif // defined(BOOST_ASIO_NO_DEPRECATED)
  }

  template <typename Stream>
  class initiate_async_buffered_write_some
  {
  public:
    typedef typename remove_reference<
      Stream>::type::lowest_layer_type::executor_type executor_type;

    explicit initiate_async_buffered_write_some(
        typename remove_reference<Stream>::type& next_layer)
      : next_layer_(next_layer)
    {
    }

    executor_type get_executor() const BOOST_ASIO_NOEXCEPT
    {
      return next_layer_.lowest_layer().get_executor();
    }

    template <typename WriteHandler, typename ConstBufferSequence>
    void operator()(BOOST_ASIO_MOVE_ARG(WriteHandler) handler,
        buffered_stream_storage* storage,
        const ConstBufferSequence& buffers) const
    {
      // If you get an error on the following line it means that your handler
      // does not meet the documented type requirements for a WriteHandler.
      BOOST_ASIO_WRITE_HANDLER_CHECK(WriteHandler, handler) type_check;

      using boost::asio::buffer_size;
      non_const_lvalue<WriteHandler> handler2(handler);
      if (buffer_size(buffers) == 0 || storage->size() < storage->capacity())
      {
        next_layer_.async_write_some(BOOST_ASIO_CONST_BUFFER(0, 0),
            buffered_write_some_handler<ConstBufferSequence,
              typename decay<WriteHandler>::type>(
                *storage, buffers, handler2.value));
      }
      else
      {
        initiate_async_buffered_flush<Stream>(this->next_layer_)(
            buffered_write_some_handler<ConstBufferSequence,
              typename decay<WriteHandler>::type>(
                *storage, buffers, handler2.value),
            storage);
      }
    }

  private:
    typename remove_reference<Stream>::type& next_layer_;
  };
} // namespace detail

#if !defined(GENERATING_DOCUMENTATION)

template <template <typename, typename> class Associator,
    typename ConstBufferSequence, typename WriteHandler,
    typename DefaultCandidate>
struct associator<Associator,
    detail::buffered_write_some_handler<ConstBufferSequence, WriteHandler>,
    DefaultCandidate>
  : Associator<WriteHandler, DefaultCandidate>
{
  static typename Associator<WriteHandler, DefaultCandidate>::type get(
      const detail::buffered_write_some_handler<
        ConstBufferSequence, WriteHandler>& h,
      const DefaultCandidate& c = DefaultCandidate()) BOOST_ASIO_NOEXCEPT
  {
    return Associator<WriteHandler, DefaultCandidate>::get(h.handler_, c);
  }
};

#endif // !defined(GENERATING_DOCUMENTATION)

template <typename Stream>
template <typename ConstBufferSequence,
    BOOST_ASIO_COMPLETION_TOKEN_FOR(void (boost::system::error_code,
      std::size_t)) WriteHandler>
BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(WriteHandler,
    void (boost::system::error_code, std::size_t))
buffered_write_stream<Stream>::async_write_some(
    const ConstBufferSequence& buffers,
    BOOST_ASIO_MOVE_ARG(WriteHandler) handler)
{
  return async_initiate<WriteHandler,
    void (boost::system::error_code, std::size_t)>(
      detail::initiate_async_buffered_write_some<Stream>(next_layer_),
      handler, &storage_, buffers);
}

template <typename Stream>
template <typename ConstBufferSequence>
std::size_t buffered_write_stream<Stream>::copy(
    const ConstBufferSequence& buffers)
{
  using boost::asio::buffer_size;
  std::size_t orig_size = storage_.size();
  std::size_t space_avail = storage_.capacity() - orig_size;
  std::size_t bytes_avail = buffer_size(buffers);
  std::size_t length = bytes_avail < space_avail ? bytes_avail : space_avail;
  storage_.resize(orig_size + length);
  return boost::asio::buffer_copy(
      storage_.data() + orig_size, buffers, length);
}

} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_IMPL_BUFFERED_WRITE_STREAM_HPP

//
// impl/write.hpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_IMPL_WRITE_HPP
#define BOOST_ASIO_IMPL_WRITE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/associator.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/completion_condition.hpp>
#include <boost/asio/detail/array_fwd.hpp>
#include <boost/asio/detail/base_from_cancellation_state.hpp>
#include <boost/asio/detail/base_from_completion_cond.hpp>
#include <boost/asio/detail/bind_handler.hpp>
#include <boost/asio/detail/consuming_buffers.hpp>
#include <boost/asio/detail/dependent_type.hpp>
#include <boost/asio/detail/handler_alloc_helpers.hpp>
#include <boost/asio/detail/handler_cont_helpers.hpp>
#include <boost/asio/detail/handler_invoke_helpers.hpp>
#include <boost/asio/detail/handler_tracking.hpp>
#include <boost/asio/detail/handler_type_requirements.hpp>
#include <boost/asio/detail/non_const_lvalue.hpp>
#include <boost/asio/detail/throw_error.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {

namespace detail
{
  template <typename SyncWriteStream, typename ConstBufferSequence,
      typename ConstBufferIterator, typename CompletionCondition>
  std::size_t write_buffer_sequence(SyncWriteStream& s,
      const ConstBufferSequence& buffers, const ConstBufferIterator&,
      CompletionCondition completion_condition, boost::system::error_code& ec)
  {
    ec = boost::system::error_code();
    boost::asio::detail::consuming_buffers<const_buffer,
        ConstBufferSequence, ConstBufferIterator> tmp(buffers);
    while (!tmp.empty())
    {
      if (std::size_t max_size = detail::adapt_completion_condition_result(
            completion_condition(ec, tmp.total_consumed())))
        tmp.consume(s.write_some(tmp.prepare(max_size), ec));
      else
        break;
    }
    return tmp.total_consumed();
  }
} // namespace detail

template <typename SyncWriteStream, typename ConstBufferSequence,
    typename CompletionCondition>
inline std::size_t write(SyncWriteStream& s, const ConstBufferSequence& buffers,
    CompletionCondition completion_condition, boost::system::error_code& ec,
    typename constraint<
      is_const_buffer_sequence<ConstBufferSequence>::value
    >::type)
{
  return detail::write_buffer_sequence(s, buffers,
      boost::asio::buffer_sequence_begin(buffers),
      BOOST_ASIO_MOVE_CAST(CompletionCondition)(completion_condition), ec);
}

template <typename SyncWriteStream, typename ConstBufferSequence>
inline std::size_t write(SyncWriteStream& s, const ConstBufferSequence& buffers,
    typename constraint<
      is_const_buffer_sequence<ConstBufferSequence>::value
    >::type)
{
  boost::system::error_code ec;
  std::size_t bytes_transferred = write(s, buffers, transfer_all(), ec);
  boost::asio::detail::throw_error(ec, "write");
  return bytes_transferred;
}

template <typename SyncWriteStream, typename ConstBufferSequence>
inline std::size_t write(SyncWriteStream& s, const ConstBufferSequence& buffers,
    boost::system::error_code& ec,
    typename constraint<
      is_const_buffer_sequence<ConstBufferSequence>::value
    >::type)
{
  return write(s, buffers, transfer_all(), ec);
}

template <typename SyncWriteStream, typename ConstBufferSequence,
    typename CompletionCondition>
inline std::size_t write(SyncWriteStream& s, const ConstBufferSequence& buffers,
    CompletionCondition completion_condition,
    typename constraint<
      is_const_buffer_sequence<ConstBufferSequence>::value
    >::type)
{
  boost::system::error_code ec;
  std::size_t bytes_transferred = write(s, buffers,
      BOOST_ASIO_MOVE_CAST(CompletionCondition)(completion_condition), ec);
  boost::asio::detail::throw_error(ec, "write");
  return bytes_transferred;
}

#if !defined(BOOST_ASIO_NO_DYNAMIC_BUFFER_V1)

template <typename SyncWriteStream, typename DynamicBuffer_v1,
    typename CompletionCondition>
std::size_t write(SyncWriteStream& s,
    BOOST_ASIO_MOVE_ARG(DynamicBuffer_v1) buffers,
    CompletionCondition completion_condition, boost::system::error_code& ec,
    typename constraint<
      is_dynamic_buffer_v1<typename decay<DynamicBuffer_v1>::type>::value
    >::type,
    typename constraint<
      !is_dynamic_buffer_v2<typename decay<DynamicBuffer_v1>::type>::value
    >::type)
{
  typename decay<DynamicBuffer_v1>::type b(
      BOOST_ASIO_MOVE_CAST(DynamicBuffer_v1)(buffers));

  std::size_t bytes_transferred = write(s, b.data(),
      BOOST_ASIO_MOVE_CAST(CompletionCondition)(completion_condition), ec);
  b.consume(bytes_transferred);
  return bytes_transferred;
}

template <typename SyncWriteStream, typename DynamicBuffer_v1>
inline std::size_t write(SyncWriteStream& s,
    BOOST_ASIO_MOVE_ARG(DynamicBuffer_v1) buffers,
    typename constraint<
      is_dynamic_buffer_v1<typename decay<DynamicBuffer_v1>::type>::value
    >::type,
    typename constraint<
      !is_dynamic_buffer_v2<typename decay<DynamicBuffer_v1>::type>::value
    >::type)
{
  boost::system::error_code ec;
  std::size_t bytes_transferred = write(s,
      BOOST_ASIO_MOVE_CAST(DynamicBuffer_v1)(buffers),
      transfer_all(), ec);
  boost::asio::detail::throw_error(ec, "write");
  return bytes_transferred;
}

template <typename SyncWriteStream, typename DynamicBuffer_v1>
inline std::size_t write(SyncWriteStream& s,
    BOOST_ASIO_MOVE_ARG(DynamicBuffer_v1) buffers,
    boost::system::error_code& ec,
    typename constraint<
      is_dynamic_buffer_v1<typename decay<DynamicBuffer_v1>::type>::value
    >::type,
    typename constraint<
      !is_dynamic_buffer_v2<typename decay<DynamicBuffer_v1>::type>::value
    >::type)
{
  return write(s, BOOST_ASIO_MOVE_CAST(DynamicBuffer_v1)(buffers),
      transfer_all(), ec);
}

template <typename SyncWriteStream, typename DynamicBuffer_v1,
    typename CompletionCondition>
inline std::size_t write(SyncWriteStream& s,
    BOOST_ASIO_MOVE_ARG(DynamicBuffer_v1) buffers,
    CompletionCondition completion_condition,
    typename constraint<
      is_dynamic_buffer_v1<typename decay<DynamicBuffer_v1>::type>::value
    >::type,
    typename constraint<
      !is_dynamic_buffer_v2<typename decay<DynamicBuffer_v1>::type>::value
    >::type)
{
  boost::system::error_code ec;
  std::size_t bytes_transferred = write(s,
      BOOST_ASIO_MOVE_CAST(DynamicBuffer_v1)(buffers),
      BOOST_ASIO_MOVE_CAST(CompletionCondition)(completion_condition), ec);
  boost::asio::detail::throw_error(ec, "write");
  return bytes_transferred;
}

#if !defined(BOOST_ASIO_NO_EXTENSIONS)
#if !defined(BOOST_ASIO_NO_IOSTREAM)

template <typename SyncWriteStream, typename Allocator,
    typename CompletionCondition>
inline std::size_t write(SyncWriteStream& s,
    boost::asio::basic_streambuf<Allocator>& b,
    CompletionCondition completion_condition, boost::system::error_code& ec)
{
  return write(s, basic_streambuf_ref<Allocator>(b),
      BOOST_ASIO_MOVE_CAST(CompletionCondition)(completion_condition), ec);
}

template <typename SyncWriteStream, typename Allocator>
inline std::size_t write(SyncWriteStream& s,
    boost::asio::basic_streambuf<Allocator>& b)
{
  return write(s, basic_streambuf_ref<Allocator>(b));
}

template <typename SyncWriteStream, typename Allocator>
inline std::size_t write(SyncWriteStream& s,
    boost::asio::basic_streambuf<Allocator>& b,
    boost::system::error_code& ec)
{
  return write(s, basic_streambuf_ref<Allocator>(b), ec);
}

template <typename SyncWriteStream, typename Allocator,
    typename CompletionCondition>
inline std::size_t write(SyncWriteStream& s,
    boost::asio::basic_streambuf<Allocator>& b,
    CompletionCondition completion_condition)
{
  return write(s, basic_streambuf_ref<Allocator>(b),
      BOOST_ASIO_MOVE_CAST(CompletionCondition)(completion_condition));
}

#endif // !defined(BOOST_ASIO_NO_IOSTREAM)
#endif // !defined(BOOST_ASIO_NO_EXTENSIONS)
#endif // !defined(BOOST_ASIO_NO_DYNAMIC_BUFFER_V1)

template <typename SyncWriteStream, typename DynamicBuffer_v2,
    typename CompletionCondition>
std::size_t write(SyncWriteStream& s, DynamicBuffer_v2 buffers,
    CompletionCondition completion_condition, boost::system::error_code& ec,
    typename constraint<
      is_dynamic_buffer_v2<DynamicBuffer_v2>::value
    >::type)
{
  std::size_t bytes_transferred = write(s, buffers.data(0, buffers.size()),
      BOOST_ASIO_MOVE_CAST(CompletionCondition)(completion_condition), ec);
  buffers.consume(bytes_transferred);
  return bytes_transferred;
}

template <typename SyncWriteStream, typename DynamicBuffer_v2>
inline std::size_t write(SyncWriteStream& s, DynamicBuffer_v2 buffers,
    typename constraint<
      is_dynamic_buffer_v2<DynamicBuffer_v2>::value
    >::type)
{
  boost::system::error_code ec;
  std::size_t bytes_transferred = write(s,
      BOOST_ASIO_MOVE_CAST(DynamicBuffer_v2)(buffers),
      transfer_all(), ec);
  boost::asio::detail::throw_error(ec, "write");
  return bytes_transferred;
}

template <typename SyncWriteStream, typename DynamicBuffer_v2>
inline std::size_t write(SyncWriteStream& s, DynamicBuffer_v2 buffers,
    boost::system::error_code& ec,
    typename constraint<
      is_dynamic_buffer_v2<DynamicBuffer_v2>::value
    >::type)
{
  return write(s, BOOST_ASIO_MOVE_CAST(DynamicBuffer_v2)(buffers),
      transfer_all(), ec);
}

template <typename SyncWriteStream, typename DynamicBuffer_v2,
    typename CompletionCondition>
inline std::size_t write(SyncWriteStream& s, DynamicBuffer_v2 buffers,
    CompletionCondition completion_condition,
    typename constraint<
      is_dynamic_buffer_v2<DynamicBuffer_v2>::value
    >::type)
{
  boost::system::error_code ec;
  std::size_t bytes_transferred = write(s,
      BOOST_ASIO_MOVE_CAST(DynamicBuffer_v2)(buffers),
      BOOST_ASIO_MOVE_CAST(CompletionCondition)(completion_condition), ec);
  boost::asio::detail::throw_error(ec, "write");
  return bytes_transferred;
}

namespace detail
{
  template <typename AsyncWriteStream, typename ConstBufferSequence,
      typename ConstBufferIterator, typename CompletionCondition,
      typename WriteHandler>
  class write_op
    : public base_from_cancellation_state<WriteHandler>,
      base_from_completion_cond<CompletionCondition>
  {
  public:
    write_op(AsyncWriteStream& stream, const ConstBufferSequence& buffers,
        CompletionCondition& completion_condition, WriteHandler& handler)
      : base_from_cancellation_state<WriteHandler>(
          handler, enable_partial_cancellation()),
        base_from_completion_cond<CompletionCondition>(completion_condition),
        stream_(stream),
        buffers_(buffers),
        start_(0),
        handler_(BOOST_ASIO_MOVE_CAST(WriteHandler)(handler))
    {
    }

#if defined(BOOST_ASIO_HAS_MOVE)
    write_op(const write_op& other)
      : base_from_cancellation_state<WriteHandler>(other),
        base_from_completion_cond<CompletionCondition>(other),
        stream_(other.stream_),
        buffers_(other.buffers_),
        start_(other.start_),
        handler_(other.handler_)
    {
    }

    write_op(write_op&& other)
      : base_from_cancellation_state<WriteHandler>(
          BOOST_ASIO_MOVE_CAST(base_from_cancellation_state<
            WriteHandler>)(other)),
        base_from_completion_cond<CompletionCondition>(
          BOOST_ASIO_MOVE_CAST(base_from_completion_cond<
            CompletionCondition>)(other)),
        stream_(other.stream_),
        buffers_(BOOST_ASIO_MOVE_CAST(buffers_type)(other.buffers_)),
        start_(other.start_),
        handler_(BOOST_ASIO_MOVE_CAST(WriteHandler)(other.handler_))
    {
    }
#endif // defined(BOOST_ASIO_HAS_MOVE)

    void operator()(boost::system::error_code ec,
        std::size_t bytes_transferred, int start = 0)
    {
      std::size_t max_size;
      switch (start_ = start)
      {
        case 1:
        max_size = this->check_for_completion(ec, buffers_.total_consumed());
        for (;;)
        {
          {
            BOOST_ASIO_HANDLER_LOCATION((__FILE__, __LINE__, "async_write"));
            stream_.async_write_some(buffers_.prepare(max_size),
                BOOST_ASIO_MOVE_CAST(write_op)(*this));
          }
          return; default:
          buffers_.consume(bytes_transferred);
          if ((!ec && bytes_transferred == 0) || buffers_.empty())
            break;
          max_size = this->check_for_completion(ec, buffers_.total_consumed());
          if (max_size == 0)
            break;
          if (this->cancelled() != cancellation_type::none)
          {
            ec = error::operation_aborted;
            break;
          }
        }

        BOOST_ASIO_MOVE_OR_LVALUE(WriteHandler)(handler_)(
            static_cast<const boost::system::error_code&>(ec),
            static_cast<const std::size_t&>(buffers_.total_consumed()));
      }
    }

  //private:
    typedef boost::asio::detail::consuming_buffers<const_buffer,
        ConstBufferSequence, ConstBufferIterator> buffers_type;

    AsyncWriteStream& stream_;
    buffers_type buffers_;
    int start_;
    WriteHandler handler_;
  };

  template <typename AsyncWriteStream, typename ConstBufferSequence,
      typename ConstBufferIterator, typename CompletionCondition,
      typename WriteHandler>
  inline asio_handler_allocate_is_deprecated
  asio_handler_allocate(std::size_t size,
      write_op<AsyncWriteStream, ConstBufferSequence, ConstBufferIterator,
        CompletionCondition, WriteHandler>* this_handler)
  {
#if defined(BOOST_ASIO_NO_DEPRECATED)
    boost_asio_handler_alloc_helpers::allocate(size, this_handler->handler_);
    return asio_handler_allocate_is_no_longer_used();
#else // defined(BOOST_ASIO_NO_DEPRECATED)
    return boost_asio_handler_alloc_helpers::allocate(
        size, this_handler->handler_);
#endif // defined(BOOST_ASIO_NO_DEPRECATED)
  }

  template <typename AsyncWriteStream, typename ConstBufferSequence,
      typename ConstBufferIterator, typename CompletionCondition,
      typename WriteHandler>
  inline asio_handler_deallocate_is_deprecated
  asio_handler_deallocate(void* pointer, std::size_t size,
      write_op<AsyncWriteStream, ConstBufferSequence, ConstBufferIterator,
        CompletionCondition, WriteHandler>* this_handler)
  {
    boost_asio_handler_alloc_helpers::deallocate(
        pointer, size, this_handler->handler_);
#if defined(BOOST_ASIO_NO_DEPRECATED)
    return asio_handler_deallocate_is_no_longer_used();
#endif // defined(BOOST_ASIO_NO_DEPRECATED)
  }

  template <typename AsyncWriteStream, typename ConstBufferSequence,
      typename ConstBufferIterator, typename CompletionCondition,
      typename WriteHandler>
  inline bool asio_handler_is_continuation(
      write_op<AsyncWriteStream, ConstBufferSequence, ConstBufferIterator,
        CompletionCondition, WriteHandler>* this_handler)
  {
    return this_handler->start_ == 0 ? true
      : boost_asio_handler_cont_helpers::is_continuation(
          this_handler->handler_);
  }

  template <typename Function, typename AsyncWriteStream,
      typename ConstBufferSequence, typename ConstBufferIterator,
      typename CompletionCondition, typename WriteHandler>
  inline asio_handler_invoke_is_deprecated
  asio_handler_invoke(Function& function,
      write_op<AsyncWriteStream, ConstBufferSequence, ConstBufferIterator,
        CompletionCondition, WriteHandler>* this_handler)
  {
    boost_asio_handler_invoke_helpers::invoke(
        function, this_handler->handler_);
#if defined(BOOST_ASIO_NO_DEPRECATED)
    return asio_handler_invoke_is_no_longer_used();
#endif // defined(BOOST_ASIO_NO_DEPRECATED)
  }

  template <typename Function, typename AsyncWriteStream,
      typename ConstBufferSequence, typename ConstBufferIterator,
      typename CompletionCondition, typename WriteHandler>
  inline asio_handler_invoke_is_deprecated
  asio_handler_invoke(const Function& function,
      write_op<AsyncWriteStream, ConstBufferSequence, ConstBufferIterator,
        CompletionCondition, WriteHandler>* this_handler)
  {
    boost_asio_handler_invoke_helpers::invoke(
        function, this_handler->handler_);
#if defined(BOOST_ASIO_NO_DEPRECATED)
    return asio_handler_invoke_is_no_longer_used();
#endif // defined(BOOST_ASIO_NO_DEPRECATED)
  }

  template <typename AsyncWriteStream, typename ConstBufferSequence,
      typename ConstBufferIterator, typename CompletionCondition,
      typename WriteHandler>
  inline void start_write_buffer_sequence_op(AsyncWriteStream& stream,
      const ConstBufferSequence& buffers, const ConstBufferIterator&,
      CompletionCondition& completion_condition, WriteHandler& handler)
  {
    detail::write_op<AsyncWriteStream, ConstBufferSequence,
      ConstBufferIterator, CompletionCondition, WriteHandler>(
        stream, buffers, completion_condition, handler)(
          boost::system::error_code(), 0, 1);
  }

  template <typename AsyncWriteStream>
  class initiate_async_write_buffer_sequence
  {
  public:
    typedef typename AsyncWriteStream::executor_type executor_type;

    explicit initiate_async_write_buffer_sequence(AsyncWriteStream& stream)
      : stream_(stream)
    {
    }

    executor_type get_executor() const BOOST_ASIO_NOEXCEPT
    {
      return stream_.get_executor();
    }

    template <typename WriteHandler, typename ConstBufferSequence,
        typename CompletionCondition>
    void operator()(BOOST_ASIO_MOVE_ARG(WriteHandler) handler,
        const ConstBufferSequence& buffers,
        BOOST_ASIO_MOVE_ARG(CompletionCondition) completion_cond) const
    {
      // If you get an error on the following line it means that your handler
      // does not meet the documented type requirements for a WriteHandler.
      BOOST_ASIO_WRITE_HANDLER_CHECK(WriteHandler, handler) type_check;

      non_const_lvalue<WriteHandler> handler2(handler);
      non_const_lvalue<CompletionCondition> completion_cond2(completion_cond);
      start_write_buffer_sequence_op(stream_, buffers,
          boost::asio::buffer_sequence_begin(buffers),
          completion_cond2.value, handler2.value);
    }

  private:
    AsyncWriteStream& stream_;
  };
} // namespace detail

#if !defined(GENERATING_DOCUMENTATION)

template <template <typename, typename> class Associator,
    typename AsyncWriteStream, typename ConstBufferSequence,
    typename ConstBufferIterator, typename CompletionCondition,
    typename WriteHandler, typename DefaultCandidate>
struct associator<Associator,
    detail::write_op<AsyncWriteStream, ConstBufferSequence,
      ConstBufferIterator, CompletionCondition, WriteHandler>,
    DefaultCandidate>
  : Associator<WriteHandler, DefaultCandidate>
{
  static typename Associator<WriteHandler, DefaultCandidate>::type get(
      const detail::write_op<AsyncWriteStream, ConstBufferSequence,
        ConstBufferIterator, CompletionCondition, WriteHandler>& h,
      const DefaultCandidate& c = DefaultCandidate()) BOOST_ASIO_NOEXCEPT
  {
    return Associator<WriteHandler, DefaultCandidate>::get(h.handler_, c);
  }
};

#endif // !defined(GENERATING_DOCUMENTATION)

template <typename AsyncWriteStream,
    typename ConstBufferSequence, typename CompletionCondition,
    BOOST_ASIO_COMPLETION_TOKEN_FOR(void (boost::system::error_code,
      std::size_t)) WriteHandler>
inline BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(WriteHandler,
    void (boost::system::error_code, std::size_t))
async_write(AsyncWriteStream& s, const ConstBufferSequence& buffers,
    CompletionCondition completion_condition,
    BOOST_ASIO_MOVE_ARG(WriteHandler) handler,
    typename constraint<
      is_const_buffer_sequence<ConstBufferSequence>::value
    >::type)
{
  return async_initiate<WriteHandler,
    void (boost::system::error_code, std::size_t)>(
      detail::initiate_async_write_buffer_sequence<AsyncWriteStream>(s),
      handler, buffers,
      BOOST_ASIO_MOVE_CAST(CompletionCondition)(completion_condition));
}

template <typename AsyncWriteStream, typename ConstBufferSequence,
    BOOST_ASIO_COMPLETION_TOKEN_FOR(void (boost::system::error_code,
      std::size_t)) WriteHandler>
inline BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(WriteHandler,
    void (boost::system::error_code, std::size_t))
async_write(AsyncWriteStream& s, const ConstBufferSequence& buffers,
    BOOST_ASIO_MOVE_ARG(WriteHandler) handler,
    typename constraint<
      is_const_buffer_sequence<ConstBufferSequence>::value
    >::type)
{
  return async_initiate<WriteHandler,
    void (boost::system::error_code, std::size_t)>(
      detail::initiate_async_write_buffer_sequence<AsyncWriteStream>(s),
      handler, buffers, transfer_all());
}

#if !defined(BOOST_ASIO_NO_DYNAMIC_BUFFER_V1)

namespace detail
{
  template <typename AsyncWriteStream, typename DynamicBuffer_v1,
      typename CompletionCondition, typename WriteHandler>
  class write_dynbuf_v1_op
  {
  public:
    template <typename BufferSequence>
    write_dynbuf_v1_op(AsyncWriteStream& stream,
        BOOST_ASIO_MOVE_ARG(BufferSequence) buffers,
        CompletionCondition& completion_condition, WriteHandler& handler)
      : stream_(stream),
        buffers_(BOOST_ASIO_MOVE_CAST(BufferSequence)(buffers)),
        completion_condition_(
          BOOST_ASIO_MOVE_CAST(CompletionCondition)(completion_condition)),
        handler_(BOOST_ASIO_MOVE_CAST(WriteHandler)(handler))
    {
    }

#if defined(BOOST_ASIO_HAS_MOVE)
    write_dynbuf_v1_op(const write_dynbuf_v1_op& other)
      : stream_(other.stream_),
        buffers_(other.buffers_),
        completion_condition_(other.completion_condition_),
        handler_(other.handler_)
    {
    }

    write_dynbuf_v1_op(write_dynbuf_v1_op&& other)
      : stream_(other.stream_),
        buffers_(BOOST_ASIO_MOVE_CAST(DynamicBuffer_v1)(other.buffers_)),
        completion_condition_(
          BOOST_ASIO_MOVE_CAST(CompletionCondition)(
            other.completion_condition_)),
        handler_(BOOST_ASIO_MOVE_CAST(WriteHandler)(other.handler_))
    {
    }
#endif // defined(BOOST_ASIO_HAS_MOVE)

    void operator()(const boost::system::error_code& ec,
        std::size_t bytes_transferred, int start = 0)
    {
      switch (start)
      {
        case 1:
        async_write(stream_, buffers_.data(),
            BOOST_ASIO_MOVE_CAST(CompletionCondition)(completion_condition_),
            BOOST_ASIO_MOVE_CAST(write_dynbuf_v1_op)(*this));
        return; default:
        buffers_.consume(bytes_transferred);
        BOOST_ASIO_MOVE_OR_LVALUE(WriteHandler)(handler_)(ec,
            static_cast<const std::size_t&>(bytes_transferred));
      }
    }

  //private:
    AsyncWriteStream& stream_;
    DynamicBuffer_v1 buffers_;
    CompletionCondition completion_condition_;
    WriteHandler handler_;
  };

  template <typename AsyncWriteStream, typename DynamicBuffer_v1,
      typename CompletionCondition, typename WriteHandler>
  inline asio_handler_allocate_is_deprecated
  asio_handler_allocate(std::size_t size,
      write_dynbuf_v1_op<AsyncWriteStream, DynamicBuffer_v1,
        CompletionCondition, WriteHandler>* this_handler)
  {
#if defined(BOOST_ASIO_NO_DEPRECATED)
    boost_asio_handler_alloc_helpers::allocate(size, this_handler->handler_);
    return asio_handler_allocate_is_no_longer_used();
#else // defined(BOOST_ASIO_NO_DEPRECATED)
    return boost_asio_handler_alloc_helpers::allocate(
        size, this_handler->handler_);
#endif // defined(BOOST_ASIO_NO_DEPRECATED)
  }

  template <typename AsyncWriteStream, typename DynamicBuffer_v1,
      typename CompletionCondition, typename WriteHandler>
  inline asio_handler_deallocate_is_deprecated
  asio_handler_deallocate(void* pointer, std::size_t size,
      write_dynbuf_v1_op<AsyncWriteStream, DynamicBuffer_v1,
        CompletionCondition, WriteHandler>* this_handler)
  {
    boost_asio_handler_alloc_helpers::deallocate(
        pointer, size, this_handler->handler_);
#if defined(BOOST_ASIO_NO_DEPRECATED)
    return asio_handler_deallocate_is_no_longer_used();
#endif // defined(BOOST_ASIO_NO_DEPRECATED)
  }

  template <typename AsyncWriteStream, typename DynamicBuffer_v1,
      typename CompletionCondition, typename WriteHandler>
  inline bool asio_handler_is_continuation(
      write_dynbuf_v1_op<AsyncWriteStream, DynamicBuffer_v1,
        CompletionCondition, WriteHandler>* this_handler)
  {
    return boost_asio_handler_cont_helpers::is_continuation(
        this_handler->handler_);
  }

  template <typename Function, typename AsyncWriteStream,
      typename DynamicBuffer_v1, typename CompletionCondition,
      typename WriteHandler>
  inline asio_handler_invoke_is_deprecated
  asio_handler_invoke(Function& function,
      write_dynbuf_v1_op<AsyncWriteStream, DynamicBuffer_v1,
        CompletionCondition, WriteHandler>* this_handler)
  {
    boost_asio_handler_invoke_helpers::invoke(
        function, this_handler->handler_);
#if defined(BOOST_ASIO_NO_DEPRECATED)
    return asio_handler_invoke_is_no_longer_used();
#endif // defined(BOOST_ASIO_NO_DEPRECATED)
  }

  template <typename Function, typename AsyncWriteStream,
      typename DynamicBuffer_v1, typename CompletionCondition,
      typename WriteHandler>
  inline asio_handler_invoke_is_deprecated
  asio_handler_invoke(const Function& function,
      write_dynbuf_v1_op<AsyncWriteStream, DynamicBuffer_v1,
        CompletionCondition, WriteHandler>* this_handler)
  {
    boost_asio_handler_invoke_helpers::invoke(
        function, this_handler->handler_);
#if defined(BOOST_ASIO_NO_DEPRECATED)
    return asio_handler_invoke_is_no_longer_used();
#endif // defined(BOOST_ASIO_NO_DEPRECATED)
  }

  template <typename AsyncWriteStream>
  class initiate_async_write_dynbuf_v1
  {
  public:
    typedef typename AsyncWriteStream::executor_type executor_type;

    explicit initiate_async_write_dynbuf_v1(AsyncWriteStream& stream)
      : stream_(stream)
    {
    }

    executor_type get_executor() const BOOST_ASIO_NOEXCEPT
    {
      return stream_.get_executor();
    }

    template <typename WriteHandler, typename DynamicBuffer_v1,
        typename CompletionCondition>
    void operator()(BOOST_ASIO_MOVE_ARG(WriteHandler) handler,
        BOOST_ASIO_MOVE_ARG(DynamicBuffer_v1) buffers,
        BOOST_ASIO_MOVE_ARG(CompletionCondition) completion_cond) const
    {
      // If you get an error on the following line it means that your handler
      // does not meet the documented type requirements for a WriteHandler.
      BOOST_ASIO_WRITE_HANDLER_CHECK(WriteHandler, handler) type_check;

      non_const_lvalue<WriteHandler> handler2(handler);
      non_const_lvalue<CompletionCondition> completion_cond2(completion_cond);
      write_dynbuf_v1_op<AsyncWriteStream,
        typename decay<DynamicBuffer_v1>::type,
          CompletionCondition, typename decay<WriteHandler>::type>(
            stream_, BOOST_ASIO_MOVE_CAST(DynamicBuffer_v1)(buffers),
              completion_cond2.value, handler2.value)(
                boost::system::error_code(), 0, 1);
    }

  private:
    AsyncWriteStream& stream_;
  };
} // namespace detail

#if !defined(GENERATING_DOCUMENTATION)

template <template <typename, typename> class Associator,
    typename AsyncWriteStream, typename DynamicBuffer_v1,
    typename CompletionCondition, typename WriteHandler,
    typename DefaultCandidate>
struct associator<Associator,
    detail::write_dynbuf_v1_op<AsyncWriteStream,
      DynamicBuffer_v1, CompletionCondition, WriteHandler>,
    DefaultCandidate>
  : Associator<WriteHandler, DefaultCandidate>
{
  static typename Associator<WriteHandler, DefaultCandidate>::type get(
      const detail::write_dynbuf_v1_op<AsyncWriteStream,
        DynamicBuffer_v1, CompletionCondition, WriteHandler>& h,
      const DefaultCandidate& c = DefaultCandidate()) BOOST_ASIO_NOEXCEPT
  {
    return Associator<WriteHandler, DefaultCandidate>::get(h.handler_, c);
  }
};

#endif // !defined(GENERATING_DOCUMENTATION)

template <typename AsyncWriteStream, typename DynamicBuffer_v1,
    BOOST_ASIO_COMPLETION_TOKEN_FOR(void (boost::system::error_code,
      std::size_t)) WriteHandler>
inline BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(WriteHandler,
    void (boost::system::error_code, std::size_t))
async_write(AsyncWriteStream& s,
    BOOST_ASIO_MOVE_ARG(DynamicBuffer_v1) buffers,
    BOOST_ASIO_MOVE_ARG(WriteHandler) handler,
    typename constraint<
      is_dynamic_buffer_v1<typename decay<DynamicBuffer_v1>::type>::value
    >::type,
    typename constraint<
      !is_dynamic_buffer_v2<typename decay<DynamicBuffer_v1>::type>::value
    >::type)
{
  return async_write(s,
      BOOST_ASIO_MOVE_CAST(DynamicBuffer_v1)(buffers),
      transfer_all(), BOOST_ASIO_MOVE_CAST(WriteHandler)(handler));
}

template <typename AsyncWriteStream,
    typename DynamicBuffer_v1, typename CompletionCondition,
    BOOST_ASIO_COMPLETION_TOKEN_FOR(void (boost::system::error_code,
      std::size_t)) WriteHandler>
inline BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(WriteHandler,
    void (boost::system::error_code, std::size_t))
async_write(AsyncWriteStream& s,
    BOOST_ASIO_MOVE_ARG(DynamicBuffer_v1) buffers,
    CompletionCondition completion_condition,
    BOOST_ASIO_MOVE_ARG(WriteHandler) handler,
    typename constraint<
      is_dynamic_buffer_v1<typename decay<DynamicBuffer_v1>::type>::value
    >::type,
    typename constraint<
      !is_dynamic_buffer_v2<typename decay<DynamicBuffer_v1>::type>::value
    >::type)
{
  return async_initiate<WriteHandler,
    void (boost::system::error_code, std::size_t)>(
      detail::initiate_async_write_dynbuf_v1<AsyncWriteStream>(s),
      handler, BOOST_ASIO_MOVE_CAST(DynamicBuffer_v1)(buffers),
      BOOST_ASIO_MOVE_CAST(CompletionCondition)(completion_condition));
}

#if !defined(BOOST_ASIO_NO_EXTENSIONS)
#if !defined(BOOST_ASIO_NO_IOSTREAM)

template <typename AsyncWriteStream, typename Allocator,
    BOOST_ASIO_COMPLETION_TOKEN_FOR(void (boost::system::error_code,
      std::size_t)) WriteHandler>
inline BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(WriteHandler,
    void (boost::system::error_code, std::size_t))
async_write(AsyncWriteStream& s,
    boost::asio::basic_streambuf<Allocator>& b,
    BOOST_ASIO_MOVE_ARG(WriteHandler) handler)
{
  return async_write(s, basic_streambuf_ref<Allocator>(b),
      BOOST_ASIO_MOVE_CAST(WriteHandler)(handler));
}

template <typename AsyncWriteStream,
    typename Allocator, typename CompletionCondition,
    BOOST_ASIO_COMPLETION_TOKEN_FOR(void (boost::system::error_code,
      std::size_t)) WriteHandler>
inline BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(WriteHandler,
    void (boost::system::error_code, std::size_t))
async_write(AsyncWriteStream& s,
    boost::asio::basic_streambuf<Allocator>& b,
    CompletionCondition completion_condition,
    BOOST_ASIO_MOVE_ARG(WriteHandler) handler)
{
  return async_write(s, basic_streambuf_ref<Allocator>(b),
      BOOST_ASIO_MOVE_CAST(CompletionCondition)(completion_condition),
      BOOST_ASIO_MOVE_CAST(WriteHandler)(handler));
}

#endif // !defined(BOOST_ASIO_NO_IOSTREAM)
#endif // !defined(BOOST_ASIO_NO_EXTENSIONS)
#endif // !defined(BOOST_ASIO_NO_DYNAMIC_BUFFER_V1)

namespace detail
{
  template <typename AsyncWriteStream, typename DynamicBuffer_v2,
      typename CompletionCondition, typename WriteHandler>
  class write_dynbuf_v2_op
  {
  public:
    template <typename BufferSequence>
    write_dynbuf_v2_op(AsyncWriteStream& stream,
        BOOST_ASIO_MOVE_ARG(BufferSequence) buffers,
        CompletionCondition& completion_condition, WriteHandler& handler)
      : stream_(stream),
        buffers_(BOOST_ASIO_MOVE_CAST(BufferSequence)(buffers)),
        completion_condition_(
          BOOST_ASIO_MOVE_CAST(CompletionCondition)(completion_condition)),
        handler_(BOOST_ASIO_MOVE_CAST(WriteHandler)(handler))
    {
    }

#if defined(BOOST_ASIO_HAS_MOVE)
    write_dynbuf_v2_op(const write_dynbuf_v2_op& other)
      : stream_(other.stream_),
        buffers_(other.buffers_),
        completion_condition_(other.completion_condition_),
        handler_(other.handler_)
    {
    }

    write_dynbuf_v2_op(write_dynbuf_v2_op&& other)
      : stream_(other.stream_),
        buffers_(BOOST_ASIO_MOVE_CAST(DynamicBuffer_v2)(other.buffers_)),
        completion_condition_(
          BOOST_ASIO_MOVE_CAST(CompletionCondition)(
            other.completion_condition_)),
        handler_(BOOST_ASIO_MOVE_CAST(WriteHandler)(other.handler_))
    {
    }
#endif // defined(BOOST_ASIO_HAS_MOVE)

    void operator()(const boost::system::error_code& ec,
        std::size_t bytes_transferred, int start = 0)
    {
      switch (start)
      {
        case 1:
        async_write(stream_, buffers_.data(0, buffers_.size()),
            BOOST_ASIO_MOVE_CAST(CompletionCondition)(completion_condition_),
            BOOST_ASIO_MOVE_CAST(write_dynbuf_v2_op)(*this));
        return; default:
        buffers_.consume(bytes_transferred);
        BOOST_ASIO_MOVE_OR_LVALUE(WriteHandler)(handler_)(ec,
            static_cast<const std::size_t&>(bytes_transferred));
      }
    }

  //private:
    AsyncWriteStream& stream_;
    DynamicBuffer_v2 buffers_;
    CompletionCondition completion_condition_;
    WriteHandler handler_;
  };

  template <typename AsyncWriteStream, typename DynamicBuffer_v2,
      typename CompletionCondition, typename WriteHandler>
  inline asio_handler_allocate_is_deprecated
  asio_handler_allocate(std::size_t size,
      write_dynbuf_v2_op<AsyncWriteStream, DynamicBuffer_v2,
        CompletionCondition, WriteHandler>* this_handler)
  {
#if defined(BOOST_ASIO_NO_DEPRECATED)
    boost_asio_handler_alloc_helpers::allocate(size, this_handler->handler_);
    return asio_handler_allocate_is_no_longer_used();
#else // defined(BOOST_ASIO_NO_DEPRECATED)
    return boost_asio_handler_alloc_helpers::allocate(
        size, this_handler->handler_);
#endif // defined(BOOST_ASIO_NO_DEPRECATED)
  }

  template <typename AsyncWriteStream, typename DynamicBuffer_v2,
      typename CompletionCondition, typename WriteHandler>
  inline asio_handler_deallocate_is_deprecated
  asio_handler_deallocate(void* pointer, std::size_t size,
      write_dynbuf_v2_op<AsyncWriteStream, DynamicBuffer_v2,
        CompletionCondition, WriteHandler>* this_handler)
  {
    boost_asio_handler_alloc_helpers::deallocate(
        pointer, size, this_handler->handler_);
#if defined(BOOST_ASIO_NO_DEPRECATED)
    return asio_handler_deallocate_is_no_longer_used();
#endif // defined(BOOST_ASIO_NO_DEPRECATED)
  }

  template <typename AsyncWriteStream, typename DynamicBuffer_v2,
      typename CompletionCondition, typename WriteHandler>
  inline bool asio_handler_is_continuation(
      write_dynbuf_v2_op<AsyncWriteStream, DynamicBuffer_v2,
        CompletionCondition, WriteHandler>* this_handler)
  {
    return boost_asio_handler_cont_helpers::is_continuation(
        this_handler->handler_);
  }

  template <typename Function, typename AsyncWriteStream,
      typename DynamicBuffer_v2, typename CompletionCondition,
      typename WriteHandler>
  inline asio_handler_invoke_is_deprecated
  asio_handler_invoke(Function& function,
      write_dynbuf_v2_op<AsyncWriteStream, DynamicBuffer_v2,
        CompletionCondition, WriteHandler>* this_handler)
  {
    boost_asio_handler_invoke_helpers::invoke(
        function, this_handler->handler_);
#if defined(BOOST_ASIO_NO_DEPRECATED)
    return asio_handler_invoke_is_no_longer_used();
#endif // defined(BOOST_ASIO_NO_DEPRECATED)
  }

  template <typename Function, typename AsyncWriteStream,
      typename DynamicBuffer_v2, typename CompletionCondition,
      typename WriteHandler>
  inline asio_handler_invoke_is_deprecated
  asio_handler_invoke(const Function& function,
      write_dynbuf_v2_op<AsyncWriteStream, DynamicBuffer_v2,
        CompletionCondition, WriteHandler>* this_handler)
  {
    boost_asio_handler_invoke_helpers::invoke(
        function, this_handler->handler_);
#if defined(BOOST_ASIO_NO_DEPRECATED)
    return asio_handler_invoke_is_no_longer_used();
#endif // defined(BOOST_ASIO_NO_DEPRECATED)
  }

  template <typename AsyncWriteStream>
  class initiate_async_write_dynbuf_v2
  {
  public:
    typedef typename AsyncWriteStream::executor_type executor_type;

    explicit initiate_async_write_dynbuf_v2(AsyncWriteStream& stream)
      : stream_(stream)
    {
    }

    executor_type get_executor() const BOOST_ASIO_NOEXCEPT
    {
      return stream_.get_executor();
    }

    template <typename WriteHandler, typename DynamicBuffer_v2,
        typename CompletionCondition>
    void operator()(BOOST_ASIO_MOVE_ARG(WriteHandler) handler,
        BOOST_ASIO_MOVE_ARG(DynamicBuffer_v2) buffers,
        BOOST_ASIO_MOVE_ARG(CompletionCondition) completion_cond) const
    {
      // If you get an error on the following line it means that your handler
      // does not meet the documented type requirements for a WriteHandler.
      BOOST_ASIO_WRITE_HANDLER_CHECK(WriteHandler, handler) type_check;

      non_const_lvalue<WriteHandler> handler2(handler);
      non_const_lvalue<CompletionCondition> completion_cond2(completion_cond);
      write_dynbuf_v2_op<AsyncWriteStream,
        typename decay<DynamicBuffer_v2>::type,
          CompletionCondition, typename decay<WriteHandler>::type>(
            stream_, BOOST_ASIO_MOVE_CAST(DynamicBuffer_v2)(buffers),
              completion_cond2.value, handler2.value)(
                boost::system::error_code(), 0, 1);
    }

  private:
    AsyncWriteStream& stream_;
  };
} // namespace detail

#if !defined(GENERATING_DOCUMENTATION)

template <template <typename, typename> class Associator,
    typename AsyncWriteStream, typename DynamicBuffer_v2,
    typename CompletionCondition, typename WriteHandler,
    typename DefaultCandidate>
struct associator<Associator,
    detail::write_dynbuf_v2_op<AsyncWriteStream,
      DynamicBuffer_v2, CompletionCondition, WriteHandler>,
    DefaultCandidate>
  : Associator<WriteHandler, DefaultCandidate>
{
  static typename Associator<WriteHandler, DefaultCandidate>::type get(
      const detail::write_dynbuf_v2_op<AsyncWriteStream,
        DynamicBuffer_v2, CompletionCondition, WriteHandler>& h,
      const DefaultCandidate& c = DefaultCandidate()) BOOST_ASIO_NOEXCEPT
  {
    return Associator<WriteHandler, DefaultCandidate>::get(h.handler_, c);
  }
};

template <typename AsyncWriteStream, typename DynamicBuffer_v2,
    typename CompletionCondition, typename WriteHandler,
    typename CancellationSlot>
struct associated_cancellation_slot<
    detail::write_dynbuf_v2_op<AsyncWriteStream,
      DynamicBuffer_v2, CompletionCondition, WriteHandler>,
    CancellationSlot>
  : detail::associated_cancellation_slot_forwarding_base<
      WriteHandler, CancellationSlot>
{
  typedef typename associated_cancellation_slot<
      WriteHandler, CancellationSlot>::type type;

  static type get(
      const detail::write_dynbuf_v2_op<AsyncWriteStream,
        DynamicBuffer_v2, CompletionCondition, WriteHandler>& h,
      const CancellationSlot& s = CancellationSlot()) BOOST_ASIO_NOEXCEPT
  {
    return associated_cancellation_slot<WriteHandler,
        CancellationSlot>::get(h.handler_, s);
  }
};

#endif // !defined(GENERATING_DOCUMENTATION)

template <typename AsyncWriteStream, typename DynamicBuffer_v2,
    BOOST_ASIO_COMPLETION_TOKEN_FOR(void (boost::system::error_code,
      std::size_t)) WriteHandler>
inline BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(WriteHandler,
    void (boost::system::error_code, std::size_t))
async_write(AsyncWriteStream& s, DynamicBuffer_v2 buffers,
    BOOST_ASIO_MOVE_ARG(WriteHandler) handler,
    typename constraint<
      is_dynamic_buffer_v2<DynamicBuffer_v2>::value
    >::type)
{
  return async_write(s,
      BOOST_ASIO_MOVE_CAST(DynamicBuffer_v2)(buffers),
      transfer_all(), BOOST_ASIO_MOVE_CAST(WriteHandler)(handler));
}

template <typename AsyncWriteStream,
    typename DynamicBuffer_v2, typename CompletionCondition,
    BOOST_ASIO_COMPLETION_TOKEN_FOR(void (boost::system::error_code,
      std::size_t)) WriteHandler>
inline BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(WriteHandler,
    void (boost::system::error_code, std::size_t))
async_write(AsyncWriteStream& s, DynamicBuffer_v2 buffers,
    CompletionCondition completion_condition,
    BOOST_ASIO_MOVE_ARG(WriteHandler) handler,
    typename constraint<
      is_dynamic_buffer_v2<DynamicBuffer_v2>::value
    >::type)
{
  return async_initiate<WriteHandler,
    void (boost::system::error_code, std::size_t)>(
      detail::initiate_async_write_dynbuf_v2<AsyncWriteStream>(s),
      handler, BOOST_ASIO_MOVE_CAST(DynamicBuffer_v2)(buffers),
      BOOST_ASIO_MOVE_CAST(CompletionCondition)(completion_condition));
}

} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_IMPL_WRITE_HPP

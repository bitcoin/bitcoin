#ifndef BOOST_THREAD_QUEUE_OP_STATUS_HPP
#define BOOST_THREAD_QUEUE_OP_STATUS_HPP

//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Vicente J. Botet Escriba 2014. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/thread for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#include <exception>
#include <boost/core/scoped_enum.hpp>
#include <boost/thread/detail/config.hpp>
#include <boost/thread/detail/move.hpp>

#include <boost/config/abi_prefix.hpp>

namespace boost
{
namespace concurrent
{

  BOOST_SCOPED_ENUM_DECLARE_BEGIN(queue_op_status)
  { success = 0, empty, full, closed, busy, timeout, not_ready }
  BOOST_SCOPED_ENUM_DECLARE_END(queue_op_status)

  struct BOOST_SYMBOL_VISIBLE sync_queue_is_closed : std::exception
  {
  };

}

#ifndef BOOST_THREAD_QUEUE_DEPRECATE_OLD
  struct no_block_tag{};
  BOOST_CONSTEXPR_OR_CONST no_block_tag no_block = {};
#endif

  using concurrent::queue_op_status;
  using concurrent::sync_queue_is_closed;

}

#include <boost/config/abi_suffix.hpp>

#endif

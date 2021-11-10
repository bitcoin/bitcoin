//
// execution.hpp
// ~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_EXECUTION_HPP
#define BOOST_ASIO_EXECUTION_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/execution/allocator.hpp>
#include <boost/asio/execution/any_executor.hpp>
#include <boost/asio/execution/bad_executor.hpp>
#include <boost/asio/execution/blocking.hpp>
#include <boost/asio/execution/blocking_adaptation.hpp>
#include <boost/asio/execution/bulk_execute.hpp>
#include <boost/asio/execution/bulk_guarantee.hpp>
#include <boost/asio/execution/connect.hpp>
#include <boost/asio/execution/context.hpp>
#include <boost/asio/execution/context_as.hpp>
#include <boost/asio/execution/execute.hpp>
#include <boost/asio/execution/executor.hpp>
#include <boost/asio/execution/invocable_archetype.hpp>
#include <boost/asio/execution/mapping.hpp>
#include <boost/asio/execution/occupancy.hpp>
#include <boost/asio/execution/operation_state.hpp>
#include <boost/asio/execution/outstanding_work.hpp>
#include <boost/asio/execution/prefer_only.hpp>
#include <boost/asio/execution/receiver.hpp>
#include <boost/asio/execution/receiver_invocation_error.hpp>
#include <boost/asio/execution/relationship.hpp>
#include <boost/asio/execution/schedule.hpp>
#include <boost/asio/execution/scheduler.hpp>
#include <boost/asio/execution/sender.hpp>
#include <boost/asio/execution/set_done.hpp>
#include <boost/asio/execution/set_error.hpp>
#include <boost/asio/execution/set_value.hpp>
#include <boost/asio/execution/start.hpp>
#include <boost/asio/execution/submit.hpp>

#endif // BOOST_ASIO_EXECUTION_HPP

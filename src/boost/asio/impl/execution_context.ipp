//
// impl/execution_context.ipp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_IMPL_EXECUTION_CONTEXT_IPP
#define BOOST_ASIO_IMPL_EXECUTION_CONTEXT_IPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <boost/asio/execution_context.hpp>
#include <boost/asio/detail/service_registry.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {

execution_context::execution_context()
  : service_registry_(new boost::asio::detail::service_registry(*this))
{
}

execution_context::~execution_context()
{
  shutdown();
  destroy();
  delete service_registry_;
}

void execution_context::shutdown()
{
  service_registry_->shutdown_services();
}

void execution_context::destroy()
{
  service_registry_->destroy_services();
}

void execution_context::notify_fork(
    boost::asio::execution_context::fork_event event)
{
  service_registry_->notify_fork(event);
}

execution_context::service::service(execution_context& owner)
  : owner_(owner),
    next_(0)
{
}

execution_context::service::~service()
{
}

void execution_context::service::notify_fork(execution_context::fork_event)
{
}

service_already_exists::service_already_exists()
  : std::logic_error("Service already exists.")
{
}

invalid_service_owner::invalid_service_owner()
  : std::logic_error("Invalid service owner.")
{
}

} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_IMPL_EXECUTION_CONTEXT_IPP

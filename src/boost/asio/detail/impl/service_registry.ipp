//
// detail/impl/service_registry.ipp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_DETAIL_IMPL_SERVICE_REGISTRY_IPP
#define BOOST_ASIO_DETAIL_IMPL_SERVICE_REGISTRY_IPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <vector>
#include <boost/asio/detail/service_registry.hpp>
#include <boost/asio/detail/throw_exception.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace detail {

service_registry::service_registry(execution_context& owner)
  : owner_(owner),
    first_service_(0)
{
}

service_registry::~service_registry()
{
}

void service_registry::shutdown_services()
{
  execution_context::service* service = first_service_;
  while (service)
  {
    service->shutdown();
    service = service->next_;
  }
}

void service_registry::destroy_services()
{
  while (first_service_)
  {
    execution_context::service* next_service = first_service_->next_;
    destroy(first_service_);
    first_service_ = next_service;
  }
}

void service_registry::notify_fork(execution_context::fork_event fork_ev)
{
  // Make a copy of all of the services while holding the lock. We don't want
  // to hold the lock while calling into each service, as it may try to call
  // back into this class.
  std::vector<execution_context::service*> services;
  {
    boost::asio::detail::mutex::scoped_lock lock(mutex_);
    execution_context::service* service = first_service_;
    while (service)
    {
      services.push_back(service);
      service = service->next_;
    }
  }

  // If processing the fork_prepare event, we want to go in reverse order of
  // service registration, which happens to be the existing order of the
  // services in the vector. For the other events we want to go in the other
  // direction.
  std::size_t num_services = services.size();
  if (fork_ev == execution_context::fork_prepare)
    for (std::size_t i = 0; i < num_services; ++i)
      services[i]->notify_fork(fork_ev);
  else
    for (std::size_t i = num_services; i > 0; --i)
      services[i - 1]->notify_fork(fork_ev);
}

void service_registry::init_key_from_id(execution_context::service::key& key,
    const execution_context::id& id)
{
  key.type_info_ = 0;
  key.id_ = &id;
}

bool service_registry::keys_match(
    const execution_context::service::key& key1,
    const execution_context::service::key& key2)
{
  if (key1.id_ && key2.id_)
    if (key1.id_ == key2.id_)
      return true;
  if (key1.type_info_ && key2.type_info_)
    if (*key1.type_info_ == *key2.type_info_)
      return true;
  return false;
}

void service_registry::destroy(execution_context::service* service)
{
  delete service;
}

execution_context::service* service_registry::do_use_service(
    const execution_context::service::key& key,
    factory_type factory, void* owner)
{
  boost::asio::detail::mutex::scoped_lock lock(mutex_);

  // First see if there is an existing service object with the given key.
  execution_context::service* service = first_service_;
  while (service)
  {
    if (keys_match(service->key_, key))
      return service;
    service = service->next_;
  }

  // Create a new service object. The service registry's mutex is not locked
  // at this time to allow for nested calls into this function from the new
  // service's constructor.
  lock.unlock();
  auto_service_ptr new_service = { factory(owner) };
  new_service.ptr_->key_ = key;
  lock.lock();

  // Check that nobody else created another service object of the same type
  // while the lock was released.
  service = first_service_;
  while (service)
  {
    if (keys_match(service->key_, key))
      return service;
    service = service->next_;
  }

  // Service was successfully initialised, pass ownership to registry.
  new_service.ptr_->next_ = first_service_;
  first_service_ = new_service.ptr_;
  new_service.ptr_ = 0;
  return first_service_;
}

void service_registry::do_add_service(
    const execution_context::service::key& key,
    execution_context::service* new_service)
{
  if (&owner_ != &new_service->context())
    boost::asio::detail::throw_exception(invalid_service_owner());

  boost::asio::detail::mutex::scoped_lock lock(mutex_);

  // Check if there is an existing service object with the given key.
  execution_context::service* service = first_service_;
  while (service)
  {
    if (keys_match(service->key_, key))
      boost::asio::detail::throw_exception(service_already_exists());
    service = service->next_;
  }

  // Take ownership of the service object.
  new_service->key_ = key;
  new_service->next_ = first_service_;
  first_service_ = new_service;
}

bool service_registry::do_has_service(
    const execution_context::service::key& key) const
{
  boost::asio::detail::mutex::scoped_lock lock(mutex_);

  execution_context::service* service = first_service_;
  while (service)
  {
    if (keys_match(service->key_, key))
      return true;
    service = service->next_;
  }

  return false;
}

} // namespace detail
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_DETAIL_IMPL_SERVICE_REGISTRY_IPP

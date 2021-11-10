//
// execution_context.hpp
// ~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_EXECUTION_CONTEXT_HPP
#define BOOST_ASIO_EXECUTION_CONTEXT_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <cstddef>
#include <stdexcept>
#include <typeinfo>
#include <boost/asio/detail/noncopyable.hpp>
#include <boost/asio/detail/variadic_templates.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {

class execution_context;
class io_context;

#if !defined(GENERATING_DOCUMENTATION)
template <typename Service> Service& use_service(execution_context&);
template <typename Service> Service& use_service(io_context&);
template <typename Service> void add_service(execution_context&, Service*);
template <typename Service> bool has_service(execution_context&);
#endif // !defined(GENERATING_DOCUMENTATION)

namespace detail { class service_registry; }

/// A context for function object execution.
/**
 * An execution context represents a place where function objects will be
 * executed. An @c io_context is an example of an execution context.
 *
 * @par The execution_context class and services
 *
 * Class execution_context implements an extensible, type-safe, polymorphic set
 * of services, indexed by service type.
 *
 * Services exist to manage the resources that are shared across an execution
 * context. For example, timers may be implemented in terms of a single timer
 * queue, and this queue would be stored in a service.
 *
 * Access to the services of an execution_context is via three function
 * templates, use_service(), add_service() and has_service().
 *
 * In a call to @c use_service<Service>(), the type argument chooses a service,
 * making available all members of the named type. If @c Service is not present
 * in an execution_context, an object of type @c Service is created and added
 * to the execution_context. A C++ program can check if an execution_context
 * implements a particular service with the function template @c
 * has_service<Service>().
 *
 * Service objects may be explicitly added to an execution_context using the
 * function template @c add_service<Service>(). If the @c Service is already
 * present, the service_already_exists exception is thrown. If the owner of the
 * service is not the same object as the execution_context parameter, the
 * invalid_service_owner exception is thrown.
 *
 * Once a service reference is obtained from an execution_context object by
 * calling use_service(), that reference remains usable as long as the owning
 * execution_context object exists.
 *
 * All service implementations have execution_context::service as a public base
 * class. Custom services may be implemented by deriving from this class and
 * then added to an execution_context using the facilities described above.
 *
 * @par The execution_context as a base class
 *
 * Class execution_context may be used only as a base class for concrete
 * execution context types. The @c io_context is an example of such a derived
 * type.
 *
 * On destruction, a class that is derived from execution_context must perform
 * <tt>execution_context::shutdown()</tt> followed by
 * <tt>execution_context::destroy()</tt>.
 *
 * This destruction sequence permits programs to simplify their resource
 * management by using @c shared_ptr<>. Where an object's lifetime is tied to
 * the lifetime of a connection (or some other sequence of asynchronous
 * operations), a @c shared_ptr to the object would be bound into the handlers
 * for all asynchronous operations associated with it. This works as follows:
 *
 * @li When a single connection ends, all associated asynchronous operations
 * complete. The corresponding handler objects are destroyed, and all @c
 * shared_ptr references to the objects are destroyed.
 *
 * @li To shut down the whole program, the io_context function stop() is called
 * to terminate any run() calls as soon as possible. The io_context destructor
 * calls @c shutdown() and @c destroy() to destroy all pending handlers,
 * causing all @c shared_ptr references to all connection objects to be
 * destroyed.
 */
class execution_context
  : private noncopyable
{
public:
  class id;
  class service;

public:
  /// Constructor.
  BOOST_ASIO_DECL execution_context();

  /// Destructor.
  BOOST_ASIO_DECL ~execution_context();

protected:
  /// Shuts down all services in the context.
  /**
   * This function is implemented as follows:
   *
   * @li For each service object @c svc in the execution_context set, in
   * reverse order of the beginning of service object lifetime, performs @c
   * svc->shutdown().
   */
  BOOST_ASIO_DECL void shutdown();

  /// Destroys all services in the context.
  /**
   * This function is implemented as follows:
   *
   * @li For each service object @c svc in the execution_context set, in
   * reverse order * of the beginning of service object lifetime, performs
   * <tt>delete static_cast<execution_context::service*>(svc)</tt>.
   */
  BOOST_ASIO_DECL void destroy();

public:
  /// Fork-related event notifications.
  enum fork_event
  {
    /// Notify the context that the process is about to fork.
    fork_prepare,

    /// Notify the context that the process has forked and is the parent.
    fork_parent,

    /// Notify the context that the process has forked and is the child.
    fork_child
  };

  /// Notify the execution_context of a fork-related event.
  /**
   * This function is used to inform the execution_context that the process is
   * about to fork, or has just forked. This allows the execution_context, and
   * the services it contains, to perform any necessary housekeeping to ensure
   * correct operation following a fork.
   *
   * This function must not be called while any other execution_context
   * function, or any function associated with the execution_context's derived
   * class, is being called in another thread. It is, however, safe to call
   * this function from within a completion handler, provided no other thread
   * is accessing the execution_context or its derived class.
   *
   * @param event A fork-related event.
   *
   * @throws boost::system::system_error Thrown on failure. If the notification
   * fails the execution_context object should no longer be used and should be
   * destroyed.
   *
   * @par Example
   * The following code illustrates how to incorporate the notify_fork()
   * function:
   * @code my_execution_context.notify_fork(execution_context::fork_prepare);
   * if (fork() == 0)
   * {
   *   // This is the child process.
   *   my_execution_context.notify_fork(execution_context::fork_child);
   * }
   * else
   * {
   *   // This is the parent process.
   *   my_execution_context.notify_fork(execution_context::fork_parent);
   * } @endcode
   *
   * @note For each service object @c svc in the execution_context set,
   * performs <tt>svc->notify_fork();</tt>. When processing the fork_prepare
   * event, services are visited in reverse order of the beginning of service
   * object lifetime. Otherwise, services are visited in order of the beginning
   * of service object lifetime.
   */
  BOOST_ASIO_DECL void notify_fork(fork_event event);

  /// Obtain the service object corresponding to the given type.
  /**
   * This function is used to locate a service object that corresponds to the
   * given service type. If there is no existing implementation of the service,
   * then the execution_context will create a new instance of the service.
   *
   * @param e The execution_context object that owns the service.
   *
   * @return The service interface implementing the specified service type.
   * Ownership of the service interface is not transferred to the caller.
   */
  template <typename Service>
  friend Service& use_service(execution_context& e);

  /// Obtain the service object corresponding to the given type.
  /**
   * This function is used to locate a service object that corresponds to the
   * given service type. If there is no existing implementation of the service,
   * then the io_context will create a new instance of the service.
   *
   * @param ioc The io_context object that owns the service.
   *
   * @return The service interface implementing the specified service type.
   * Ownership of the service interface is not transferred to the caller.
   *
   * @note This overload is preserved for backwards compatibility with services
   * that inherit from io_context::service.
   */
  template <typename Service>
  friend Service& use_service(io_context& ioc);

#if defined(GENERATING_DOCUMENTATION)

  /// Creates a service object and adds it to the execution_context.
  /**
   * This function is used to add a service to the execution_context.
   *
   * @param e The execution_context object that owns the service.
   *
   * @param args Zero or more arguments to be passed to the service
   * constructor.
   *
   * @throws boost::asio::service_already_exists Thrown if a service of the
   * given type is already present in the execution_context.
   */
  template <typename Service, typename... Args>
  friend Service& make_service(execution_context& e, Args&&... args);

#elif defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)

  template <typename Service, typename... Args>
  friend Service& make_service(execution_context& e,
      BOOST_ASIO_MOVE_ARG(Args)... args);

#else // defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)

  template <typename Service>
  friend Service& make_service(execution_context& e);

#define BOOST_ASIO_PRIVATE_MAKE_SERVICE_DEF(n) \
  template <typename Service, BOOST_ASIO_VARIADIC_TPARAMS(n)> \
  friend Service& make_service(execution_context& e, \
      BOOST_ASIO_VARIADIC_MOVE_PARAMS(n)); \
  /**/
  BOOST_ASIO_VARIADIC_GENERATE(BOOST_ASIO_PRIVATE_MAKE_SERVICE_DEF)
#undef BOOST_ASIO_PRIVATE_MAKE_SERVICE_DEF

#endif // defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)

  /// (Deprecated: Use make_service().) Add a service object to the
  /// execution_context.
  /**
   * This function is used to add a service to the execution_context.
   *
   * @param e The execution_context object that owns the service.
   *
   * @param svc The service object. On success, ownership of the service object
   * is transferred to the execution_context. When the execution_context object
   * is destroyed, it will destroy the service object by performing: @code
   * delete static_cast<execution_context::service*>(svc) @endcode
   *
   * @throws boost::asio::service_already_exists Thrown if a service of the
   * given type is already present in the execution_context.
   *
   * @throws boost::asio::invalid_service_owner Thrown if the service's owning
   * execution_context is not the execution_context object specified by the
   * @c e parameter.
   */
  template <typename Service>
  friend void add_service(execution_context& e, Service* svc);

  /// Determine if an execution_context contains a specified service type.
  /**
   * This function is used to determine whether the execution_context contains a
   * service object corresponding to the given service type.
   *
   * @param e The execution_context object that owns the service.
   *
   * @return A boolean indicating whether the execution_context contains the
   * service.
   */
  template <typename Service>
  friend bool has_service(execution_context& e);

private:
  // The service registry.
  boost::asio::detail::service_registry* service_registry_;
};

/// Class used to uniquely identify a service.
class execution_context::id
  : private noncopyable
{
public:
  /// Constructor.
  id() {}
};

/// Base class for all io_context services.
class execution_context::service
  : private noncopyable
{
public:
  /// Get the context object that owns the service.
  execution_context& context();

protected:
  /// Constructor.
  /**
   * @param owner The execution_context object that owns the service.
   */
  BOOST_ASIO_DECL service(execution_context& owner);

  /// Destructor.
  BOOST_ASIO_DECL virtual ~service();

private:
  /// Destroy all user-defined handler objects owned by the service.
  virtual void shutdown() = 0;

  /// Handle notification of a fork-related event to perform any necessary
  /// housekeeping.
  /**
   * This function is not a pure virtual so that services only have to
   * implement it if necessary. The default implementation does nothing.
   */
  BOOST_ASIO_DECL virtual void notify_fork(
      execution_context::fork_event event);

  friend class boost::asio::detail::service_registry;
  struct key
  {
    key() : type_info_(0), id_(0) {}
    const std::type_info* type_info_;
    const execution_context::id* id_;
  } key_;

  execution_context& owner_;
  service* next_;
};

/// Exception thrown when trying to add a duplicate service to an
/// execution_context.
class service_already_exists
  : public std::logic_error
{
public:
  BOOST_ASIO_DECL service_already_exists();
};

/// Exception thrown when trying to add a service object to an
/// execution_context where the service has a different owner.
class invalid_service_owner
  : public std::logic_error
{
public:
  BOOST_ASIO_DECL invalid_service_owner();
};

namespace detail {

// Special derived service id type to keep classes header-file only.
template <typename Type>
class service_id
  : public execution_context::id
{
};

// Special service base class to keep classes header-file only.
template <typename Type>
class execution_context_service_base
  : public execution_context::service
{
public:
  static service_id<Type> id;

  // Constructor.
  execution_context_service_base(execution_context& e)
    : execution_context::service(e)
  {
  }
};

template <typename Type>
service_id<Type> execution_context_service_base<Type>::id;

} // namespace detail
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#include <boost/asio/impl/execution_context.hpp>
#if defined(BOOST_ASIO_HEADER_ONLY)
# include <boost/asio/impl/execution_context.ipp>
#endif // defined(BOOST_ASIO_HEADER_ONLY)

#endif // BOOST_ASIO_EXECUTION_CONTEXT_HPP

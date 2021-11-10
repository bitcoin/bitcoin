//
// coroutine.hpp
// ~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_COROUTINE_HPP
#define BOOST_ASIO_COROUTINE_HPP

namespace boost {
namespace asio {
namespace detail {

class coroutine_ref;

} // namespace detail

/// Provides support for implementing stackless coroutines.
/**
 * The @c coroutine class may be used to implement stackless coroutines. The
 * class itself is used to store the current state of the coroutine.
 *
 * Coroutines are copy-constructible and assignable, and the space overhead is
 * a single int. They can be used as a base class:
 *
 * @code class session : coroutine
 * {
 *   ...
 * }; @endcode
 *
 * or as a data member:
 *
 * @code class session
 * {
 *   ...
 *   coroutine coro_;
 * }; @endcode
 *
 * or even bound in as a function argument using lambdas or @c bind(). The
 * important thing is that as the application maintains a copy of the object
 * for as long as the coroutine must be kept alive.
 *
 * @par Pseudo-keywords
 *
 * A coroutine is used in conjunction with certain "pseudo-keywords", which
 * are implemented as macros. These macros are defined by a header file:
 *
 * @code #include <boost/asio/yield.hpp>@endcode
 *
 * and may conversely be undefined as follows:
 *
 * @code #include <boost/asio/unyield.hpp>@endcode
 *
 * <b>reenter</b>
 *
 * The @c reenter macro is used to define the body of a coroutine. It takes a
 * single argument: a pointer or reference to a coroutine object. For example,
 * if the base class is a coroutine object you may write:
 *
 * @code reenter (this)
 * {
 *   ... coroutine body ...
 * } @endcode
 *
 * and if a data member or other variable you can write:
 *
 * @code reenter (coro_)
 * {
 *   ... coroutine body ...
 * } @endcode
 *
 * When @c reenter is executed at runtime, control jumps to the location of the
 * last @c yield or @c fork.
 *
 * The coroutine body may also be a single statement, such as:
 *
 * @code reenter (this) for (;;)
 * {
 *   ...
 * } @endcode
 *
 * @b Limitation: The @c reenter macro is implemented using a switch. This
 * means that you must take care when using local variables within the
 * coroutine body. The local variable is not allowed in a position where
 * reentering the coroutine could bypass the variable definition.
 *
 * <b>yield <em>statement</em></b>
 *
 * This form of the @c yield keyword is often used with asynchronous operations:
 *
 * @code yield socket_->async_read_some(buffer(*buffer_), *this); @endcode
 *
 * This divides into four logical steps:
 *
 * @li @c yield saves the current state of the coroutine.
 * @li The statement initiates the asynchronous operation.
 * @li The resume point is defined immediately following the statement.
 * @li Control is transferred to the end of the coroutine body.
 *
 * When the asynchronous operation completes, the function object is invoked
 * and @c reenter causes control to transfer to the resume point. It is
 * important to remember to carry the coroutine state forward with the
 * asynchronous operation. In the above snippet, the current class is a
 * function object object with a coroutine object as base class or data member.
 *
 * The statement may also be a compound statement, and this permits us to
 * define local variables with limited scope:
 *
 * @code yield
 * {
 *   mutable_buffers_1 b = buffer(*buffer_);
 *   socket_->async_read_some(b, *this);
 * } @endcode
 *
 * <b>yield return <em>expression</em> ;</b>
 *
 * This form of @c yield is often used in generators or coroutine-based parsers.
 * For example, the function object:
 *
 * @code struct interleave : coroutine
 * {
 *   istream& is1;
 *   istream& is2;
 *   char operator()(char c)
 *   {
 *     reenter (this) for (;;)
 *     {
 *       yield return is1.get();
 *       yield return is2.get();
 *     }
 *   }
 * }; @endcode
 *
 * defines a trivial coroutine that interleaves the characters from two input
 * streams.
 *
 * This type of @c yield divides into three logical steps:
 *
 * @li @c yield saves the current state of the coroutine.
 * @li The resume point is defined immediately following the semicolon.
 * @li The value of the expression is returned from the function.
 *
 * <b>yield ;</b>
 *
 * This form of @c yield is equivalent to the following steps:
 *
 * @li @c yield saves the current state of the coroutine.
 * @li The resume point is defined immediately following the semicolon.
 * @li Control is transferred to the end of the coroutine body.
 *
 * This form might be applied when coroutines are used for cooperative
 * threading and scheduling is explicitly managed. For example:
 *
 * @code struct task : coroutine
 * {
 *   ...
 *   void operator()()
 *   {
 *     reenter (this)
 *     {
 *       while (... not finished ...)
 *       {
 *         ... do something ...
 *         yield;
 *         ... do some more ...
 *         yield;
 *       }
 *     }
 *   }
 *   ...
 * };
 * ...
 * task t1, t2;
 * for (;;)
 * {
 *   t1();
 *   t2();
 * } @endcode
 *
 * <b>yield break ;</b>
 *
 * The final form of @c yield is used to explicitly terminate the coroutine.
 * This form is comprised of two steps:
 *
 * @li @c yield sets the coroutine state to indicate termination.
 * @li Control is transferred to the end of the coroutine body.
 *
 * Once terminated, calls to is_complete() return true and the coroutine cannot
 * be reentered.
 *
 * Note that a coroutine may also be implicitly terminated if the coroutine
 * body is exited without a yield, e.g. by return, throw or by running to the
 * end of the body.
 *
 * <b>fork <em>statement</em></b>
 *
 * The @c fork pseudo-keyword is used when "forking" a coroutine, i.e. splitting
 * it into two (or more) copies. One use of @c fork is in a server, where a new
 * coroutine is created to handle each client connection:
 * 
 * @code reenter (this)
 * {
 *   do
 *   {
 *     socket_.reset(new tcp::socket(my_context_));
 *     yield acceptor->async_accept(*socket_, *this);
 *     fork server(*this)();
 *   } while (is_parent());
 *   ... client-specific handling follows ...
 * } @endcode
 * 
 * The logical steps involved in a @c fork are:
 * 
 * @li @c fork saves the current state of the coroutine.
 * @li The statement creates a copy of the coroutine and either executes it
 *     immediately or schedules it for later execution.
 * @li The resume point is defined immediately following the semicolon.
 * @li For the "parent", control immediately continues from the next line.
 *
 * The functions is_parent() and is_child() can be used to differentiate
 * between parent and child. You would use these functions to alter subsequent
 * control flow.
 *
 * Note that @c fork doesn't do the actual forking by itself. It is the
 * application's responsibility to create a clone of the coroutine and call it.
 * The clone can be called immediately, as above, or scheduled for delayed
 * execution using something like boost::asio::post().
 *
 * @par Alternate macro names
 *
 * If preferred, an application can use macro names that follow a more typical
 * naming convention, rather than the pseudo-keywords. These are:
 *
 * @li @c BOOST_ASIO_CORO_REENTER instead of @c reenter
 * @li @c BOOST_ASIO_CORO_YIELD instead of @c yield
 * @li @c BOOST_ASIO_CORO_FORK instead of @c fork
 */
class coroutine
{
public:
  /// Constructs a coroutine in its initial state.
  coroutine() : value_(0) {}

  /// Returns true if the coroutine is the child of a fork.
  bool is_child() const { return value_ < 0; }

  /// Returns true if the coroutine is the parent of a fork.
  bool is_parent() const { return !is_child(); }

  /// Returns true if the coroutine has reached its terminal state.
  bool is_complete() const { return value_ == -1; }

private:
  friend class detail::coroutine_ref;
  int value_;
};


namespace detail {

class coroutine_ref
{
public:
  coroutine_ref(coroutine& c) : value_(c.value_), modified_(false) {}
  coroutine_ref(coroutine* c) : value_(c->value_), modified_(false) {}
  ~coroutine_ref() { if (!modified_) value_ = -1; }
  operator int() const { return value_; }
  int& operator=(int v) { modified_ = true; return value_ = v; }
private:
  void operator=(const coroutine_ref&);
  int& value_;
  bool modified_;
};

} // namespace detail
} // namespace asio
} // namespace boost

#define BOOST_ASIO_CORO_REENTER(c) \
  switch (::boost::asio::detail::coroutine_ref _coro_value = c) \
    case -1: if (_coro_value) \
    { \
      goto terminate_coroutine; \
      terminate_coroutine: \
      _coro_value = -1; \
      goto bail_out_of_coroutine; \
      bail_out_of_coroutine: \
      break; \
    } \
    else /* fall-through */ case 0:

#define BOOST_ASIO_CORO_YIELD_IMPL(n) \
  for (_coro_value = (n);;) \
    if (_coro_value == 0) \
    { \
      case (n): ; \
      break; \
    } \
    else \
      switch (_coro_value ? 0 : 1) \
        for (;;) \
          /* fall-through */ case -1: if (_coro_value) \
            goto terminate_coroutine; \
          else for (;;) \
            /* fall-through */ case 1: if (_coro_value) \
              goto bail_out_of_coroutine; \
            else /* fall-through */ case 0:

#define BOOST_ASIO_CORO_FORK_IMPL(n) \
  for (_coro_value = -(n);; _coro_value = (n)) \
    if (_coro_value == (n)) \
    { \
      case -(n): ; \
      break; \
    } \
    else

#if defined(_MSC_VER)
# define BOOST_ASIO_CORO_YIELD BOOST_ASIO_CORO_YIELD_IMPL(__COUNTER__ + 1)
# define BOOST_ASIO_CORO_FORK BOOST_ASIO_CORO_FORK_IMPL(__COUNTER__ + 1)
#else // defined(_MSC_VER)
# define BOOST_ASIO_CORO_YIELD BOOST_ASIO_CORO_YIELD_IMPL(__LINE__)
# define BOOST_ASIO_CORO_FORK BOOST_ASIO_CORO_FORK_IMPL(__LINE__)
#endif // defined(_MSC_VER)

#endif // BOOST_ASIO_COROUTINE_HPP

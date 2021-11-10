//  (C) Copyright 2008-10 Anthony Williams
//  (C) Copyright 2011-2015 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_THREAD_FUTURE_HPP
#define BOOST_THREAD_FUTURE_HPP

#include <boost/thread/detail/config.hpp>

// boost::thread::future requires exception handling
// due to boost::exception::exception_ptr dependency

//#define BOOST_THREAD_CONTINUATION_SYNC

#ifdef BOOST_NO_EXCEPTIONS
namespace boost
{
namespace detail {
struct shared_state_base {
    void notify_deferred() {}
};
}
}
#else

#include <boost/thread/condition_variable.hpp>
#include <boost/thread/detail/move.hpp>
#include <boost/thread/detail/invoker.hpp>
#include <boost/thread/detail/invoke.hpp>
#include <boost/thread/detail/is_convertible.hpp>
#include <boost/thread/exceptional_ptr.hpp>
#include <boost/thread/futures/future_error.hpp>
#include <boost/thread/futures/future_error_code.hpp>
#include <boost/thread/futures/future_status.hpp>
#include <boost/thread/futures/is_future_type.hpp>
#include <boost/thread/futures/launch.hpp>
#include <boost/thread/futures/wait_for_all.hpp>
#include <boost/thread/futures/wait_for_any.hpp>
#include <boost/thread/lock_algorithms.hpp>
#include <boost/thread/lock_types.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread_only.hpp>
#include <boost/thread/thread_time.hpp>
#include <boost/thread/executor.hpp>
#include <boost/thread/executors/generic_executor_ref.hpp>

#if defined BOOST_THREAD_FUTURE_USES_OPTIONAL
#include <boost/optional.hpp>
#else
#include <boost/thread/csbl/memory/unique_ptr.hpp>
#endif

#include <boost/assert.hpp>
#include <boost/bind/bind.hpp>
#ifdef BOOST_THREAD_USES_CHRONO
#include <boost/chrono/system_clocks.hpp>
#endif
#include <boost/core/enable_if.hpp>
#include <boost/core/ref.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/exception_ptr.hpp>
#include <boost/function.hpp>
#include <boost/next_prior.hpp>
#include <boost/scoped_array.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/smart_ptr/make_shared.hpp>
#include <boost/throw_exception.hpp>
#include <boost/type_traits/conditional.hpp>
#include <boost/type_traits/decay.hpp>
#include <boost/type_traits/is_copy_constructible.hpp>
#include <boost/type_traits/is_fundamental.hpp>
#include <boost/type_traits/is_void.hpp>
#include <boost/utility/result_of.hpp>


#if defined BOOST_THREAD_PROVIDES_FUTURE_CTOR_ALLOCATORS
#include <boost/thread/detail/memory.hpp>
#include <boost/container/scoped_allocator.hpp>
#if ! defined  BOOST_NO_CXX11_ALLOCATOR
#include <memory>
#endif
#endif

#if defined BOOST_THREAD_PROVIDES_FUTURE_WHEN_ALL_WHEN_ANY
#include <boost/thread/csbl/tuple.hpp>
#include <boost/thread/csbl/vector.hpp>
#endif

#include <algorithm>
#include <list>
#include <vector>
#include <utility>

#if defined BOOST_THREAD_PROVIDES_FUTURE
#define BOOST_THREAD_FUTURE future
#else
#define BOOST_THREAD_FUTURE unique_future
#endif

namespace boost
{
  template <class T>
  shared_ptr<T> static_shared_from_this(T* that)
  {
    return static_pointer_cast<T>(that->shared_from_this());
  }
  template <class T>
  shared_ptr<T const> static_shared_from_this(T const* that)
  {
    return static_pointer_cast<T const>(that->shared_from_this());
  }

#ifdef BOOST_THREAD_PROVIDES_EXECUTORS
#else
    namespace executors {
        class executor;
    }
    using executors::executor;
#endif
    typedef shared_ptr<executor> executor_ptr_type;

    namespace detail
    {

        struct relocker
        {
            boost::unique_lock<boost::mutex>& lock_;

            relocker(boost::unique_lock<boost::mutex>& lk):
                lock_(lk)
            {
                lock_.unlock();
            }
            ~relocker()
            {
              if (! lock_.owns_lock()) {
                lock_.lock();
              }
            }
            void lock() {
              if (! lock_.owns_lock()) {
                lock_.lock();
              }
            }
        private:
            relocker& operator=(relocker const&);
        };

        struct shared_state_base : enable_shared_from_this<shared_state_base>
        {
            typedef std::list<boost::condition_variable_any*> waiter_list;
            typedef waiter_list::iterator notify_when_ready_handle;
            // This type should be only included conditionally if interruptions are allowed, but is included to maintain the same layout.
            typedef shared_ptr<shared_state_base> continuation_ptr_type;
            typedef std::vector<continuation_ptr_type> continuations_type;

            boost::exception_ptr exception;
            bool done;
            bool is_valid_;
            bool is_deferred_;
            bool is_constructed;
            launch policy_;
            mutable boost::mutex mutex;
            boost::condition_variable waiters;
            waiter_list external_waiters;
            boost::function<void()> callback;
            // This declaration should be only included conditionally, but is included to maintain the same layout.
            continuations_type continuations;
            executor_ptr_type ex_;

            // This declaration should be only included conditionally, but is included to maintain the same layout.
            virtual void launch_continuation()
            {
            }

            shared_state_base():
                done(false),
                is_valid_(true),
                is_deferred_(false),
                is_constructed(false),
                policy_(launch::none),
                continuations(),
                ex_()
            {}

            shared_state_base(exceptional_ptr const& ex):
                exception(ex.ptr_),
                done(true),
                is_valid_(true),
                is_deferred_(false),
                is_constructed(false),
                policy_(launch::none),
                continuations(),
                ex_()
            {}


            virtual ~shared_state_base()
            {
            }

            bool is_done()
            {
                return done;
            }

            executor_ptr_type get_executor()
            {
              return ex_;
            }

            void set_executor_policy(executor_ptr_type aex)
            {
              set_executor();
              ex_ = aex;
            }
            void set_executor_policy(executor_ptr_type aex, boost::lock_guard<boost::mutex>&)
            {
              set_executor();
              ex_ = aex;
            }
            void set_executor_policy(executor_ptr_type aex, boost::unique_lock<boost::mutex>&)
            {
              set_executor();
              ex_ = aex;
            }

            bool valid(boost::unique_lock<boost::mutex>&) { return is_valid_; }
            bool valid() {
              boost::unique_lock<boost::mutex> lk(this->mutex);
              return valid(lk);
            }
            void invalidate(boost::unique_lock<boost::mutex>&) { is_valid_ = false; }
            void invalidate() {
              boost::unique_lock<boost::mutex> lk(this->mutex);
              invalidate(lk);
            }
            void validate(boost::unique_lock<boost::mutex>&) { is_valid_ = true; }
            void validate() {
              boost::unique_lock<boost::mutex> lk(this->mutex);
              validate(lk);
            }

            void set_deferred()
            {
              is_deferred_ = true;
              policy_ = launch::deferred;
            }
            void set_async()
            {
              is_deferred_ = false;
              policy_ = launch::async;
            }
#ifdef BOOST_THREAD_PROVIDES_EXECUTORS
            void set_executor()
            {
              is_deferred_ = false;
              policy_ = launch::executor;
            }
#else
            void set_executor()
            {
            }
#endif
            notify_when_ready_handle notify_when_ready(boost::condition_variable_any& cv)
            {
                boost::unique_lock<boost::mutex> lock(this->mutex);
                do_callback(lock);
                return external_waiters.insert(external_waiters.end(),&cv);
            }

            void unnotify_when_ready(notify_when_ready_handle it)
            {
                boost::lock_guard<boost::mutex> lock(this->mutex);
                external_waiters.erase(it);
            }

#if 0
            // this inline definition results in ODR. See https://github.com/boostorg/thread/issues/193
            // to avoid it, we define the function on the derived templates using the macro BOOST_THREAD_DO_CONTINUATION
#define BOOST_THREAD_DO_CONTINUATION
#if defined BOOST_THREAD_PROVIDES_FUTURE_CONTINUATION
            void do_continuation(boost::unique_lock<boost::mutex>& lock)
            {
                if (! continuations.empty()) {
                  continuations_type the_continuations = continuations;
                  continuations.clear();
                  relocker rlk(lock);
                  for (continuations_type::iterator it = the_continuations.begin(); it != the_continuations.end(); ++it) {
                    (*it)->launch_continuation();
                  }
                }
            }
#else
            void do_continuation(boost::unique_lock<boost::mutex>&)
            {
            }
#endif

#else
#if defined BOOST_THREAD_PROVIDES_FUTURE_CONTINUATION
#define BOOST_THREAD_DO_CONTINUATION \
            void do_continuation(boost::unique_lock<boost::mutex>& lock) \
            { \
                if (! this->continuations.empty()) { \
                  continuations_type the_continuations = this->continuations; \
                  this->continuations.clear(); \
                  relocker rlk(lock); \
                  for (continuations_type::iterator it = the_continuations.begin(); it != the_continuations.end(); ++it) { \
                    (*it)->launch_continuation(); \
                  } \
                } \
            }
#else
#define BOOST_THREAD_DO_CONTINUATION \
            void do_continuation(boost::unique_lock<boost::mutex>&) \
            { \
            }
#endif

            virtual void do_continuation(boost::unique_lock<boost::mutex>&) = 0;
#endif

#if defined BOOST_THREAD_PROVIDES_FUTURE_CONTINUATION
            virtual void set_continuation_ptr(continuation_ptr_type continuation, boost::unique_lock<boost::mutex>& lock)
            {
              continuations.push_back(continuation);
              if (done) {
                do_continuation(lock);
              }
            }
#endif
            void mark_finished_internal(boost::unique_lock<boost::mutex>& lock)
            {
                done=true;
                waiters.notify_all();
                for(waiter_list::const_iterator it=external_waiters.begin(),
                        end=external_waiters.end();it!=end;++it)
                {
                    (*it)->notify_all();
                }
                do_continuation(lock);
            }
            void notify_deferred()
            {
              boost::unique_lock<boost::mutex> lock(this->mutex);
              mark_finished_internal(lock);
            }

            void do_callback(boost::unique_lock<boost::mutex>& lock)
            {
                if(callback && !done)
                {
                    boost::function<void()> local_callback=callback;
                    relocker relock(lock);
                    local_callback();
                }
            }

            virtual bool run_if_is_deferred()
            {
              boost::unique_lock<boost::mutex> lk(this->mutex);
              if (is_deferred_)
              {
                is_deferred_=false;
                execute(lk);
                return true;
              }
              else
                return false;
            }
            virtual bool run_if_is_deferred_or_ready()
            {
              boost::unique_lock<boost::mutex> lk(this->mutex);
              if (is_deferred_)
              {
                is_deferred_=false;
                execute(lk);

                return true;
              }
              else
                return done;
            }
            void wait_internal(boost::unique_lock<boost::mutex> &lk, bool rethrow=true)
            {
              do_callback(lk);
              if (is_deferred_)
              {
                is_deferred_=false;
                execute(lk);
              }
              waiters.wait(lk, boost::bind(&shared_state_base::is_done, boost::ref(*this)));
              if(rethrow && exception)
              {
                  boost::rethrow_exception(exception);
              }
            }

            virtual void wait(boost::unique_lock<boost::mutex>& lock, bool rethrow=true)
            {
                wait_internal(lock, rethrow);
            }

            void wait(bool rethrow=true)
            {
                boost::unique_lock<boost::mutex> lock(this->mutex);
                wait(lock, rethrow);
            }

#if defined BOOST_THREAD_USES_DATETIME
            template<typename Duration>
            bool timed_wait(Duration const& rel_time)
            {
                boost::unique_lock<boost::mutex> lock(this->mutex);
                if (is_deferred_)
                    return false;

                do_callback(lock);
                return waiters.timed_wait(lock, rel_time, boost::bind(&shared_state_base::is_done, boost::ref(*this)));
            }

            bool timed_wait_until(boost::system_time const& target_time)
            {
                boost::unique_lock<boost::mutex> lock(this->mutex);
                if (is_deferred_)
                    return false;

                do_callback(lock);
                return waiters.timed_wait(lock, target_time, boost::bind(&shared_state_base::is_done, boost::ref(*this)));
            }
#endif
#ifdef BOOST_THREAD_USES_CHRONO

            template <class Clock, class Duration>
            future_status
            wait_until(const chrono::time_point<Clock, Duration>& abs_time)
            {
              boost::unique_lock<boost::mutex> lock(this->mutex);
              if (is_deferred_)
                  return future_status::deferred;
              do_callback(lock);
              if(!waiters.wait_until(lock, abs_time, boost::bind(&shared_state_base::is_done, boost::ref(*this))))
              {
                  return future_status::timeout;
              }
              return future_status::ready;
            }
#endif
            void mark_exceptional_finish_internal(boost::exception_ptr const& e, boost::unique_lock<boost::mutex>& lock)
            {
                exception=e;
                mark_finished_internal(lock);
            }

            void mark_exceptional_finish()
            {
                boost::unique_lock<boost::mutex> lock(this->mutex);
                mark_exceptional_finish_internal(boost::current_exception(), lock);
            }

            void set_exception_deferred(exception_ptr e)
            {
              unique_lock<boost::mutex> lk(this->mutex);
              if (has_value(lk))
              {
                  throw_exception(promise_already_satisfied());
              }
              exception=e;
              this->is_constructed = true;
            }
            void set_exception_at_thread_exit(exception_ptr e)
            {
              set_exception_deferred(e);
//              unique_lock<boost::mutex> lk(this->mutex);
//              if (has_value(lk))
//              {
//                  throw_exception(promise_already_satisfied());
//              }
//              exception=e;
//              this->is_constructed = true;
              detail::make_ready_at_thread_exit(shared_from_this());
            }

            bool has_value() const
            {
                boost::lock_guard<boost::mutex> lock(this->mutex);
                return done && ! exception;
            }

            bool has_value(unique_lock<boost::mutex>& )  const
            {
                return done && ! exception;
            }

            bool has_exception()  const
            {
                boost::lock_guard<boost::mutex> lock(this->mutex);
                return done && exception;
            }

            launch launch_policy(boost::unique_lock<boost::mutex>&) const
            {
                return policy_;
            }

            future_state::state get_state(boost::unique_lock<boost::mutex>&) const
            {
                if(!done)
                {
                    return future_state::waiting;
                }
                else
                {
                    return future_state::ready;
                }
            }
            future_state::state get_state() const
            {
                boost::lock_guard<boost::mutex> guard(this->mutex);
                if(!done)
                {
                    return future_state::waiting;
                }
                else
                {
                    return future_state::ready;
                }
            }

            exception_ptr get_exception_ptr()
            {
                boost::unique_lock<boost::mutex> lock(this->mutex);
                wait_internal(lock, false);
                return exception;
            }

            template<typename F,typename U>
            void set_wait_callback(F f,U* u)
            {
                boost::lock_guard<boost::mutex> lock(this->mutex);
                callback=boost::bind(f,boost::ref(*u));
            }

            virtual void execute(boost::unique_lock<boost::mutex>&) {}

        private:
            shared_state_base(shared_state_base const&);
            shared_state_base& operator=(shared_state_base const&);
        };

        // Used to create stand-alone futures
        template<typename T>
        struct shared_state:
            detail::shared_state_base
        {
#if defined BOOST_THREAD_FUTURE_USES_OPTIONAL
              typedef boost::optional<T> storage_type;
#else
              typedef boost::csbl::unique_ptr<T> storage_type;
#endif
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
            typedef T const& source_reference_type;
            typedef BOOST_THREAD_RV_REF(T) rvalue_source_type;
            typedef T move_dest_type;
#elif defined BOOST_THREAD_USES_MOVE
            typedef typename conditional<boost::is_fundamental<T>::value,T,T const&>::type source_reference_type;
            typedef BOOST_THREAD_RV_REF(T) rvalue_source_type;
            typedef T move_dest_type;
#else
            typedef T& source_reference_type;
            typedef typename conditional<boost::thread_detail::is_convertible<T&,BOOST_THREAD_RV_REF(T) >::value, BOOST_THREAD_RV_REF(T),T const&>::type rvalue_source_type;
            typedef typename conditional<boost::thread_detail::is_convertible<T&,BOOST_THREAD_RV_REF(T) >::value, BOOST_THREAD_RV_REF(T),T>::type move_dest_type;
#endif

            typedef const T& shared_future_get_result_type;

            storage_type result;

            shared_state():
                result()
            {}
            shared_state(exceptional_ptr const& ex):
              detail::shared_state_base(ex), result()
            {}

            // locating this definition on the template avoid the ODR issue. See https://github.com/boostorg/thread/issues/193
            BOOST_THREAD_DO_CONTINUATION

            void mark_finished_with_result_internal(source_reference_type result_, boost::unique_lock<boost::mutex>& lock)
            {
#if defined BOOST_THREAD_FUTURE_USES_OPTIONAL
                result = result_;
#else
                result.reset(new T(result_));
#endif
                this->mark_finished_internal(lock);
            }

            void mark_finished_with_result_internal(rvalue_source_type result_, boost::unique_lock<boost::mutex>& lock)
            {
#if defined BOOST_THREAD_FUTURE_USES_OPTIONAL
                result = boost::move(result_);
#elif ! defined  BOOST_NO_CXX11_RVALUE_REFERENCES
                result.reset(new T(boost::move(result_)));
#else
                result.reset(new T(static_cast<rvalue_source_type>(result_)));
#endif
                this->mark_finished_internal(lock);
            }


#if ! defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)
            template <class ...Args>
            void mark_finished_with_result_internal(boost::unique_lock<boost::mutex>& lock, BOOST_THREAD_FWD_REF(Args)... args)
            {
#if defined BOOST_THREAD_FUTURE_USES_OPTIONAL
                result.emplace(boost::forward<Args>(args)...);
#else
                result.reset(new T(boost::forward<Args>(args)...));
#endif
                this->mark_finished_internal(lock);
            }
#endif

            void mark_finished_with_result(source_reference_type result_)
            {
                boost::unique_lock<boost::mutex> lock(this->mutex);
                this->mark_finished_with_result_internal(result_, lock);
            }

            void mark_finished_with_result(rvalue_source_type result_)
            {
                boost::unique_lock<boost::mutex> lock(this->mutex);

#if ! defined  BOOST_NO_CXX11_RVALUE_REFERENCES
                mark_finished_with_result_internal(boost::move(result_), lock);
#else
                mark_finished_with_result_internal(static_cast<rvalue_source_type>(result_), lock);
#endif
            }

            storage_type& get_storage(boost::unique_lock<boost::mutex>& lk)
            {
                wait_internal(lk);
                return result;
            }
            virtual move_dest_type get(boost::unique_lock<boost::mutex>& lk)
            {
                return boost::move(*get_storage(lk));
            }
            move_dest_type get()
            {
                boost::unique_lock<boost::mutex> lk(this->mutex);
                return this->get(lk);
            }

            virtual shared_future_get_result_type get_sh(boost::unique_lock<boost::mutex>& lk)
            {
                return *get_storage(lk);
            }
            shared_future_get_result_type get_sh()
            {
                boost::unique_lock<boost::mutex> lk(this->mutex);
                return this->get_sh(lk);
            }
            void set_value_deferred(source_reference_type result_)
            {
              unique_lock<boost::mutex> lk(this->mutex);
              if (this->has_value(lk))
              {
                  throw_exception(promise_already_satisfied());
              }
#if defined BOOST_THREAD_FUTURE_USES_OPTIONAL
              result = result_;
#else
              result.reset(new T(result_));
#endif

              this->is_constructed = true;
            }
            void set_value_deferred(rvalue_source_type result_)
            {
              unique_lock<boost::mutex> lk(this->mutex);
              if (this->has_value(lk))
              {
                  throw_exception(promise_already_satisfied());
              }

#if ! defined  BOOST_NO_CXX11_RVALUE_REFERENCES
#if defined BOOST_THREAD_FUTURE_USES_OPTIONAL
                result = boost::move(result_);
#else
                result.reset(new T(boost::move(result_)));
#endif
#else
#if defined BOOST_THREAD_FUTURE_USES_OPTIONAL
                result = boost::move(result_);
#else
                result.reset(new T(static_cast<rvalue_source_type>(result_)));
#endif
#endif
              this->is_constructed = true;
            }

            void set_value_at_thread_exit(source_reference_type result_)
            {
                set_value_deferred(result_);
//              unique_lock<boost::mutex> lk(this->mutex);
//              if (this->has_value(lk))
//              {
//                  throw_exception(promise_already_satisfied());
//              }
//#if defined BOOST_THREAD_FUTURE_USES_OPTIONAL
//              result = result_;
//#else
//              result.reset(new T(result_));
//#endif
//
//              this->is_constructed = true;
              detail::make_ready_at_thread_exit(shared_from_this());
            }
            void set_value_at_thread_exit(rvalue_source_type result_)
            {
                set_value_deferred(boost::move(result_));
//              unique_lock<boost::mutex> lk(this->mutex);
//              if (this->has_value(lk))
//                  throw_exception(promise_already_satisfied());
//
//#if ! defined  BOOST_NO_CXX11_RVALUE_REFERENCES
//#if defined BOOST_THREAD_FUTURE_USES_OPTIONAL
//                result = boost::move(result_);
//#else
//                result.reset(new T(boost::move(result_)));
//#endif
//#else
//#if defined BOOST_THREAD_FUTURE_USES_OPTIONAL
//                result = boost::move(result_);
//#else
//                result.reset(new T(static_cast<rvalue_source_type>(result_)));
//#endif
//#endif
//              this->is_constructed = true;
              detail::make_ready_at_thread_exit(shared_from_this());
            }

        private:
            shared_state(shared_state const&);
            shared_state& operator=(shared_state const&);
        };

        template<typename T>
        struct shared_state<T&>:
            detail::shared_state_base
        {
            typedef T* storage_type;
            typedef T& source_reference_type;
            typedef T& move_dest_type;
            typedef T& shared_future_get_result_type;

            T* result;

            shared_state():
                result(0)
            {}

            shared_state(exceptional_ptr const& ex):
              detail::shared_state_base(ex), result(0)
            {}

            // locating this definition on the template avoid the ODR issue. See https://github.com/boostorg/thread/issues/193
            BOOST_THREAD_DO_CONTINUATION

            void mark_finished_with_result_internal(source_reference_type result_, boost::unique_lock<boost::mutex>& lock)
            {
                result= &result_;
                mark_finished_internal(lock);
            }

            void mark_finished_with_result(source_reference_type result_)
            {
                boost::unique_lock<boost::mutex> lock(this->mutex);
                mark_finished_with_result_internal(result_, lock);
            }

            virtual T& get(boost::unique_lock<boost::mutex>& lock)
            {
                wait_internal(lock);
                return *result;
            }
            T& get()
            {
                boost::unique_lock<boost::mutex> lk(this->mutex);
                return get(lk);
            }

            virtual T& get_sh(boost::unique_lock<boost::mutex>& lock)
            {
                wait_internal(lock);
                return *result;
            }
            T& get_sh()
            {
                boost::unique_lock<boost::mutex> lock(this->mutex);
                return get_sh(lock);
            }

            void set_value_deferred(T& result_)
            {
              unique_lock<boost::mutex> lk(this->mutex);
              if (this->has_value(lk))
              {
                  throw_exception(promise_already_satisfied());
              }
              result= &result_;
              this->is_constructed = true;
            }

            void set_value_at_thread_exit(T& result_)
            {
              set_value_deferred(result_);
//              unique_lock<boost::mutex> lk(this->mutex);
//              if (this->has_value(lk))
//                  throw_exception(promise_already_satisfied());
//              result= &result_;
//              this->is_constructed = true;
              detail::make_ready_at_thread_exit(shared_from_this());
            }

        private:
            shared_state(shared_state const&);
            shared_state& operator=(shared_state const&);
        };

        template<>
        struct shared_state<void>:
            detail::shared_state_base
        {
            typedef void shared_future_get_result_type;
            typedef void move_dest_type;

            shared_state()
            {}

            shared_state(exceptional_ptr const& ex):
              detail::shared_state_base(ex)
            {}

            // locating this definition on the template avoid the ODR issue. See https://github.com/boostorg/thread/issues/193
            BOOST_THREAD_DO_CONTINUATION

            void mark_finished_with_result_internal(boost::unique_lock<boost::mutex>& lock)
            {
                mark_finished_internal(lock);
            }

            void mark_finished_with_result()
            {
                boost::unique_lock<boost::mutex> lock(this->mutex);
                mark_finished_with_result_internal(lock);
            }

            virtual void get(boost::unique_lock<boost::mutex>& lock)
            {
                this->wait_internal(lock);
            }
            void get()
            {
                boost::unique_lock<boost::mutex> lock(this->mutex);
                this->get(lock);
            }

            virtual void get_sh(boost::unique_lock<boost::mutex>& lock)
            {
                this->wait_internal(lock);
            }
            void get_sh()
            {
                boost::unique_lock<boost::mutex> lock(this->mutex);
                this->get_sh(lock);
            }

            void set_value_deferred()
            {
              unique_lock<boost::mutex> lk(this->mutex);
              if (this->has_value(lk))
              {
                  throw_exception(promise_already_satisfied());
              }
              this->is_constructed = true;
            }
            void set_value_at_thread_exit()
            {
              set_value_deferred();
//              unique_lock<boost::mutex> lk(this->mutex);
//              if (this->has_value(lk))
//              {
//                  throw_exception(promise_already_satisfied());
//              }
//              this->is_constructed = true;
              detail::make_ready_at_thread_exit(shared_from_this());
            }
        private:
            shared_state(shared_state const&);
            shared_state& operator=(shared_state const&);
        };

        /////////////////////////
        /// future_async_shared_state_base
        /////////////////////////
        template<typename Rp>
        struct future_async_shared_state_base: shared_state<Rp>
        {
          typedef shared_state<Rp> base_type;
        protected:
#ifdef BOOST_THREAD_FUTURE_BLOCKING
          boost::thread thr_;
          void join()
          {
              if (this_thread::get_id() == thr_.get_id())
              {
                  thr_.detach();
                  return;
              }
              if (thr_.joinable()) thr_.join();
          }
#endif
        public:
          future_async_shared_state_base()
          {
            this->set_async();
          }

          ~future_async_shared_state_base()
          {
#ifdef BOOST_THREAD_FUTURE_BLOCKING
            join();
#elif defined BOOST_THREAD_ASYNC_FUTURE_WAITS
            unique_lock<boost::mutex> lk(this->mutex);
            this->waiters.wait(lk, boost::bind(&shared_state_base::is_done, boost::ref(*this)));
#endif
          }

          virtual void wait(boost::unique_lock<boost::mutex>& lk, bool rethrow)
          {
#ifdef BOOST_THREAD_FUTURE_BLOCKING
              {
                relocker rlk(lk);
                join();
              }
#endif
              this->base_type::wait(lk, rethrow);
          }
        };

        /////////////////////////
        /// future_async_shared_state
        /////////////////////////
        template<typename Rp, typename Fp>
        struct future_async_shared_state: future_async_shared_state_base<Rp>
        {
          future_async_shared_state()
          {
          }

          void init(BOOST_THREAD_FWD_REF(Fp) f)
          {
#ifdef BOOST_THREAD_FUTURE_BLOCKING
            this->thr_ = boost::thread(&future_async_shared_state::run, static_shared_from_this(this), boost::forward<Fp>(f));
#else
            boost::thread(&future_async_shared_state::run, static_shared_from_this(this), boost::forward<Fp>(f)).detach();
#endif
          }

          static void run(shared_ptr<future_async_shared_state> that, BOOST_THREAD_FWD_REF(Fp) f)
          {
            try
            {
              that->mark_finished_with_result(f());
            }
            catch(...)
            {
              that->mark_exceptional_finish();
            }
          }
        };

        template<typename Fp>
        struct future_async_shared_state<void, Fp>: public future_async_shared_state_base<void>
        {
          void init(BOOST_THREAD_FWD_REF(Fp) f)
          {
#ifdef BOOST_THREAD_FUTURE_BLOCKING
            this->thr_ = boost::thread(&future_async_shared_state::run, static_shared_from_this(this), boost::move(f));
#else
            boost::thread(&future_async_shared_state::run, static_shared_from_this(this), boost::move(f)).detach();
#endif
          }

          static void run(shared_ptr<future_async_shared_state> that, BOOST_THREAD_FWD_REF(Fp) f)
          {
            try
            {
              f();
              that->mark_finished_with_result();
            }
            catch(...)
            {
              that->mark_exceptional_finish();
            }
          }
        };

        template<typename Rp, typename Fp>
        struct future_async_shared_state<Rp&, Fp>: future_async_shared_state_base<Rp&>
        {
          void init(BOOST_THREAD_FWD_REF(Fp) f)
          {
#ifdef BOOST_THREAD_FUTURE_BLOCKING
            this->thr_ = boost::thread(&future_async_shared_state::run, static_shared_from_this(this), boost::move(f));
#else
            boost::thread(&future_async_shared_state::run, static_shared_from_this(this), boost::move(f)).detach();
#endif
          }

          static void run(shared_ptr<future_async_shared_state> that, BOOST_THREAD_FWD_REF(Fp) f)
          {
            try
            {
              that->mark_finished_with_result(f());
            }
            catch(...)
            {
              that->mark_exceptional_finish();
            }
          }
        };

        //////////////////////////
        /// future_deferred_shared_state
        //////////////////////////
        template<typename Rp, typename Fp>
        struct future_deferred_shared_state: shared_state<Rp>
        {
          Fp func_;

          explicit future_deferred_shared_state(BOOST_THREAD_FWD_REF(Fp) f)
          : func_(boost::move(f))
          {
            this->set_deferred();
          }

          virtual void execute(boost::unique_lock<boost::mutex>& lck) {
            try
            {
              Fp local_fuct=boost::move(func_);
              relocker relock(lck);
              Rp res = local_fuct();
              relock.lock();
              this->mark_finished_with_result_internal(boost::move(res), lck);
            }
            catch (...)
            {
              this->mark_exceptional_finish_internal(current_exception(), lck);
            }
          }
        };
        template<typename Rp, typename Fp>
        struct future_deferred_shared_state<Rp&,Fp>: shared_state<Rp&>
        {
          Fp func_;

          explicit future_deferred_shared_state(BOOST_THREAD_FWD_REF(Fp) f)
          : func_(boost::move(f))
          {
            this->set_deferred();
          }

          virtual void execute(boost::unique_lock<boost::mutex>& lck) {
            try
            {
              this->mark_finished_with_result_internal(func_(), lck);
            }
            catch (...)
            {
              this->mark_exceptional_finish_internal(current_exception(), lck);
            }
          }
        };

        template<typename Fp>
        struct future_deferred_shared_state<void,Fp>: shared_state<void>
        {
          Fp func_;

          explicit future_deferred_shared_state(BOOST_THREAD_FWD_REF(Fp) f)
          : func_(boost::move(f))
          {
            this->set_deferred();
          }

          virtual void execute(boost::unique_lock<boost::mutex>& lck) {
            try
            {
              Fp local_fuct=boost::move(func_);
              relocker relock(lck);
              local_fuct();
              relock.lock();
              this->mark_finished_with_result_internal(lck);
            }
            catch (...)
            {
              this->mark_exceptional_finish_internal(current_exception(), lck);
            }
          }
        };

        class future_waiter
        {
        public:
            typedef std::vector<int>::size_type count_type;
        private:
            struct registered_waiter
            {
                boost::shared_ptr<detail::shared_state_base> future_;
                detail::shared_state_base::notify_when_ready_handle handle;
                count_type index;

                registered_waiter(boost::shared_ptr<detail::shared_state_base> const& a_future,
                                  detail::shared_state_base::notify_when_ready_handle handle_,
                                  count_type index_):
                    future_(a_future),handle(handle_),index(index_)
                {}
            };

            struct all_futures_lock
            {
#ifdef _MANAGED
                   typedef std::ptrdiff_t count_type_portable;
#else
                   typedef count_type count_type_portable;
#endif
                   count_type_portable count;
                   boost::scoped_array<boost::unique_lock<boost::mutex> > locks;

                all_futures_lock(std::vector<registered_waiter>& futures):
                    count(futures.size()),locks(new boost::unique_lock<boost::mutex>[count])
                {
                    for(count_type_portable i=0;i<count;++i)
                    {
                        locks[i]=BOOST_THREAD_MAKE_RV_REF(boost::unique_lock<boost::mutex>(futures[i].future_->mutex));
                    }
                }

                void lock()
                {
                    boost::lock(locks.get(),locks.get()+count);
                }

                void unlock()
                {
                    for(count_type_portable i=0;i<count;++i)
                    {
                        locks[i].unlock();
                    }
                }
            };

            boost::condition_variable_any cv;
            std::vector<registered_waiter> futures_;
            count_type future_count;

        public:
            future_waiter():
                future_count(0)
            {}

            template<typename F>
            void add(F& f)
            {
                if(f.future_)
                {
                  registered_waiter waiter(f.future_,f.future_->notify_when_ready(cv),future_count);
                  try {
                    futures_.push_back(waiter);
                  } catch(...) {
                    f.future_->unnotify_when_ready(waiter.handle);
                    throw;
                  }
                }
                ++future_count;
            }

#ifndef BOOST_NO_CXX11_VARIADIC_TEMPLATES
            template<typename F1, typename... Fs>
            void add(F1& f1, Fs&... fs)
            {
              add(f1); add(fs...);
            }
#endif

            count_type wait()
            {
                all_futures_lock lk(futures_);
                for(;;)
                {
                    for(count_type i=0;i<futures_.size();++i)
                    {
                        if(futures_[i].future_->done)
                        {
                            return futures_[i].index;
                        }
                    }
                    cv.wait(lk);
                }
            }

            ~future_waiter()
            {
                for(count_type i=0;i<futures_.size();++i)
                {
                    futures_[i].future_->unnotify_when_ready(futures_[i].handle);
                }
            }
        };

    }

    template <typename R>
    class BOOST_THREAD_FUTURE;

    template <typename R>
    class shared_future;

    template<typename T>
    struct is_future_type<BOOST_THREAD_FUTURE<T> > : true_type
    {
    };

    template<typename T>
    struct is_future_type<shared_future<T> > : true_type
    {
    };

//    template<typename Iterator>
//    typename boost::disable_if<is_future_type<Iterator>,Iterator>::type wait_for_any(Iterator begin,Iterator end)
//    {
//        if(begin==end)
//            return end;
//
//        detail::future_waiter waiter;
//        for(Iterator current=begin;current!=end;++current)
//        {
//            waiter.add(*current);
//        }
//        return boost::next(begin,waiter.wait());
//    }

#ifdef BOOST_NO_CXX11_VARIADIC_TEMPLATES
    template<typename F1,typename F2>
    typename boost::enable_if<is_future_type<F1>,typename detail::future_waiter::count_type>::type wait_for_any(F1& f1,F2& f2)
    {
        detail::future_waiter waiter;
        waiter.add(f1);
        waiter.add(f2);
        return waiter.wait();
    }

    template<typename F1,typename F2,typename F3>
    typename detail::future_waiter::count_type wait_for_any(F1& f1,F2& f2,F3& f3)
    {
        detail::future_waiter waiter;
        waiter.add(f1);
        waiter.add(f2);
        waiter.add(f3);
        return waiter.wait();
    }

    template<typename F1,typename F2,typename F3,typename F4>
    typename detail::future_waiter::count_type wait_for_any(F1& f1,F2& f2,F3& f3,F4& f4)
    {
        detail::future_waiter waiter;
        waiter.add(f1);
        waiter.add(f2);
        waiter.add(f3);
        waiter.add(f4);
        return waiter.wait();
    }

    template<typename F1,typename F2,typename F3,typename F4,typename F5>
    typename detail::future_waiter::count_type wait_for_any(F1& f1,F2& f2,F3& f3,F4& f4,F5& f5)
    {
        detail::future_waiter waiter;
        waiter.add(f1);
        waiter.add(f2);
        waiter.add(f3);
        waiter.add(f4);
        waiter.add(f5);
        return waiter.wait();
    }
#else
    template<typename F1, typename... Fs>
    typename boost::enable_if<is_future_type<F1>, typename detail::future_waiter::count_type>::type
    wait_for_any(F1& f1, Fs&... fs)
    {
      detail::future_waiter waiter;
      waiter.add(f1, fs...);
      return waiter.wait();
    }
#endif // !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)

    template <typename R>
    class promise;

    template <typename R>
    class packaged_task;

    namespace detail
    {
      /// Common implementation for all the futures independently of the return type
      class base_future
      {
      public:
      };
      /// Common implementation for future and shared_future.
      template <typename R>
      class basic_future : public base_future
      {
      protected:
      public:

        typedef boost::shared_ptr<detail::shared_state<R> > future_ptr;
        typedef typename detail::shared_state<R>::move_dest_type move_dest_type;

        static //BOOST_CONSTEXPR
        future_ptr make_exceptional_future_ptr(exceptional_ptr const& ex) {
          return future_ptr(new detail::shared_state<R>(ex));
        }

        future_ptr future_;

        basic_future(future_ptr a_future):
          future_(a_future)
        {
        }

      public:
        typedef future_state::state state;

        BOOST_THREAD_MOVABLE_ONLY(basic_future)
        basic_future(): future_() {}


        //BOOST_CONSTEXPR
        basic_future(exceptional_ptr const& ex)
          : future_(make_exceptional_future_ptr(ex))
        {
        }

        ~basic_future() {
        }

        basic_future(BOOST_THREAD_RV_REF(basic_future) other) BOOST_NOEXCEPT:
        future_(BOOST_THREAD_RV(other).future_)
        {
            BOOST_THREAD_RV(other).future_.reset();
        }
        basic_future& operator=(BOOST_THREAD_RV_REF(basic_future) other) BOOST_NOEXCEPT
        {
            future_=BOOST_THREAD_RV(other).future_;
            BOOST_THREAD_RV(other).future_.reset();
            return *this;
        }
        void swap(basic_future& that) BOOST_NOEXCEPT
        {
          future_.swap(that.future_);
        }
        // functions to check state, and wait for ready
        state get_state(boost::unique_lock<boost::mutex>& lk) const
        {
            if(!future_)
            {
                return future_state::uninitialized;
            }
            return future_->get_state(lk);
        }
        state get_state() const
        {
            if(!future_)
            {
                return future_state::uninitialized;
            }
            return future_->get_state();
        }

        bool is_ready() const
        {
            return get_state()==future_state::ready;
        }

        bool is_ready(boost::unique_lock<boost::mutex>& lk) const
        {
            return get_state(lk)==future_state::ready;
        }
        bool has_exception() const
        {
            return future_ && future_->has_exception();
        }

        bool has_value() const
        {
            return future_ && future_->has_value();
        }

        launch launch_policy(boost::unique_lock<boost::mutex>& lk) const
        {
            if ( future_ ) return future_->launch_policy(lk);
            else return launch(launch::none);
        }

        launch launch_policy() const
        {
          if ( future_ ) {
            boost::unique_lock<boost::mutex> lk(this->future_->mutex);
            return future_->launch_policy(lk);
          }
          else return launch(launch::none);
        }

        exception_ptr get_exception_ptr()
        {
            return future_
                ? future_->get_exception_ptr()
                : exception_ptr();
        }

        bool valid() const BOOST_NOEXCEPT
        {
            return future_.get() != 0 && future_->valid();
        }

        void wait() const
        {
            if(!future_)
            {
                boost::throw_exception(future_uninitialized());
            }
            future_->wait(false);
        }

        typedef detail::shared_state_base::notify_when_ready_handle notify_when_ready_handle;

        boost::mutex& mutex() {
          if(!future_)
          {
              boost::throw_exception(future_uninitialized());
          }
          return future_->mutex;
        }

        notify_when_ready_handle notify_when_ready(boost::condition_variable_any& cv)
        {
          if(!future_)
          {
              boost::throw_exception(future_uninitialized());
          }
          return future_->notify_when_ready(cv);
        }

        void unnotify_when_ready(notify_when_ready_handle h)
        {
          if(!future_)
          {
              boost::throw_exception(future_uninitialized());
          }
          return future_->unnotify_when_ready(h);
        }

#if defined BOOST_THREAD_USES_DATETIME
        template<typename Duration>
        bool timed_wait(Duration const& rel_time) const
        {
            if(!future_)
            {
                boost::throw_exception(future_uninitialized());
            }
            return future_->timed_wait(rel_time);
        }

        bool timed_wait_until(boost::system_time const& abs_time) const
        {
            if(!future_)
            {
                boost::throw_exception(future_uninitialized());
            }
            return future_->timed_wait_until(abs_time);
        }
#endif
#ifdef BOOST_THREAD_USES_CHRONO
        template <class Rep, class Period>
        future_status
        wait_for(const chrono::duration<Rep, Period>& rel_time) const
        {
          return wait_until(chrono::steady_clock::now() + rel_time);

        }
        template <class Clock, class Duration>
        future_status
        wait_until(const chrono::time_point<Clock, Duration>& abs_time) const
        {
          if(!future_)
          {
              boost::throw_exception(future_uninitialized());
          }
          return future_->wait_until(abs_time);
        }
#endif

      };

    } // detail
    BOOST_THREAD_DCL_MOVABLE_BEG(R) detail::basic_future<R> BOOST_THREAD_DCL_MOVABLE_END

    namespace detail
    {
#if (!defined _MSC_VER || _MSC_VER >= 1400) // _MSC_VER == 1400 on MSVC 2005
        template <class Rp, class Fp>
        BOOST_THREAD_FUTURE<Rp>
        make_future_async_shared_state(BOOST_THREAD_FWD_REF(Fp) f);

        template <class Rp, class Fp>
        BOOST_THREAD_FUTURE<Rp>
        make_future_deferred_shared_state(BOOST_THREAD_FWD_REF(Fp) f);
#endif // #if (!defined _MSC_VER || _MSC_VER >= 1400)
#if defined BOOST_THREAD_PROVIDES_FUTURE_CONTINUATION
        template<typename F, typename Rp, typename Fp>
        struct future_deferred_continuation_shared_state;
        template<typename F, typename Rp, typename Fp>
        struct future_async_continuation_shared_state;

        template <class F, class Rp, class Fp>
        BOOST_THREAD_FUTURE<Rp>
        make_future_async_continuation_shared_state(boost::unique_lock<boost::mutex> &lock, BOOST_THREAD_RV_REF(F) f, BOOST_THREAD_FWD_REF(Fp) c);

        template <class F, class Rp, class Fp>
        BOOST_THREAD_FUTURE<Rp>
        make_future_sync_continuation_shared_state(boost::unique_lock<boost::mutex> &lock, BOOST_THREAD_RV_REF(F) f, BOOST_THREAD_FWD_REF(Fp) c);

        template <class F, class Rp, class Fp>
        BOOST_THREAD_FUTURE<Rp>
        make_future_deferred_continuation_shared_state(boost::unique_lock<boost::mutex> &lock, BOOST_THREAD_RV_REF(F) f, BOOST_THREAD_FWD_REF(Fp) c);

        template<typename F, typename Rp, typename Fp>
        BOOST_THREAD_FUTURE<Rp>
        make_shared_future_deferred_continuation_shared_state(boost::unique_lock<boost::mutex> &lock, F f, BOOST_THREAD_FWD_REF(Fp) c);

        template<typename F, typename Rp, typename Fp>
        BOOST_THREAD_FUTURE<Rp>
        make_shared_future_async_continuation_shared_state(boost::unique_lock<boost::mutex> &lock, F f, BOOST_THREAD_FWD_REF(Fp) c);

        template<typename F, typename Rp, typename Fp>
        BOOST_THREAD_FUTURE<Rp>
        make_shared_future_sync_continuation_shared_state(boost::unique_lock<boost::mutex> &lock, F f, BOOST_THREAD_FWD_REF(Fp) c);


  #ifdef BOOST_THREAD_PROVIDES_EXECUTORS
        template<typename Ex, typename F, typename Rp, typename Fp>
        BOOST_THREAD_FUTURE<Rp>
        make_future_executor_continuation_shared_state(Ex& ex, boost::unique_lock<boost::mutex> &lock, BOOST_THREAD_RV_REF(F) f, BOOST_THREAD_FWD_REF(Fp) c);

        template<typename Ex, typename F, typename Rp, typename Fp>
        BOOST_THREAD_FUTURE<Rp>
        make_shared_future_executor_continuation_shared_state(Ex& ex, boost::unique_lock<boost::mutex> &lock, F f, BOOST_THREAD_FWD_REF(Fp) c);

        template <class Rp, class Fp, class Executor>
        BOOST_THREAD_FUTURE<Rp>
        make_future_executor_shared_state(Executor& ex, BOOST_THREAD_FWD_REF(Fp) f);
  #endif
#endif
#if defined BOOST_THREAD_PROVIDES_FUTURE_UNWRAP
        template<typename F, typename Rp>
        struct future_unwrap_shared_state;
        template <class F, class Rp>
        inline BOOST_THREAD_FUTURE<Rp>
        make_future_unwrap_shared_state(boost::unique_lock<boost::mutex> &lock, BOOST_THREAD_RV_REF(F) f);
#endif
    }
#if defined(BOOST_THREAD_PROVIDES_FUTURE_WHEN_ALL_WHEN_ANY)
      template< typename InputIterator>
      typename boost::disable_if<is_future_type<InputIterator>,
        BOOST_THREAD_FUTURE<csbl::vector<typename InputIterator::value_type>  >
      >::type
      when_all(InputIterator first, InputIterator last);

      inline BOOST_THREAD_FUTURE<csbl::tuple<> > when_all();

    #if ! defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)
      template< typename T0, typename ...T>
      BOOST_THREAD_FUTURE<csbl::tuple<typename decay<T0>::type, typename decay<T>::type...> >
      when_all(BOOST_THREAD_FWD_REF(T0) f, BOOST_THREAD_FWD_REF(T) ... futures);
    #endif

      template< typename InputIterator>
      typename boost::disable_if<is_future_type<InputIterator>,
        BOOST_THREAD_FUTURE<csbl::vector<typename InputIterator::value_type>  >
      >::type
      when_any(InputIterator first, InputIterator last);

      inline BOOST_THREAD_FUTURE<csbl::tuple<> > when_any();

    #if ! defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)
      template< typename T0, typename ...T>
      BOOST_THREAD_FUTURE<csbl::tuple<typename decay<T0>::type, typename decay<T>::type...> >
      when_any(BOOST_THREAD_FWD_REF(T0) f, BOOST_THREAD_FWD_REF(T) ... futures);
    #endif
#endif // BOOST_THREAD_PROVIDES_FUTURE_WHEN_ALL_WHEN_ANY


    template <typename R>
    class BOOST_THREAD_FUTURE : public detail::basic_future<R>
    {
    private:
        typedef detail::basic_future<R> base_type;
        typedef typename base_type::future_ptr future_ptr;

        friend class shared_future<R>;
        friend class promise<R>;
#if defined BOOST_THREAD_PROVIDES_FUTURE_CONTINUATION
        template <typename, typename, typename>
        friend struct detail::future_async_continuation_shared_state;
        template <typename, typename, typename>
        friend struct detail::future_deferred_continuation_shared_state;

        template <class F, class Rp, class Fp>
        friend BOOST_THREAD_FUTURE<Rp>
        detail::make_future_async_continuation_shared_state(boost::unique_lock<boost::mutex> &lock, BOOST_THREAD_RV_REF(F) f, BOOST_THREAD_FWD_REF(Fp) c);

        template <class F, class Rp, class Fp>
        friend BOOST_THREAD_FUTURE<Rp>
        detail::make_future_sync_continuation_shared_state(boost::unique_lock<boost::mutex> &lock, BOOST_THREAD_RV_REF(F) f, BOOST_THREAD_FWD_REF(Fp) c);

        template <class F, class Rp, class Fp>
        friend BOOST_THREAD_FUTURE<Rp>
        detail::make_future_deferred_continuation_shared_state(boost::unique_lock<boost::mutex> &lock, BOOST_THREAD_RV_REF(F) f, BOOST_THREAD_FWD_REF(Fp) c);

        template<typename F, typename Rp, typename Fp>
        friend BOOST_THREAD_FUTURE<Rp>
        detail::make_shared_future_deferred_continuation_shared_state(boost::unique_lock<boost::mutex> &lock, F f, BOOST_THREAD_FWD_REF(Fp) c);

        template<typename F, typename Rp, typename Fp>
        friend BOOST_THREAD_FUTURE<Rp>
        detail::make_shared_future_async_continuation_shared_state(boost::unique_lock<boost::mutex> &lock, F f, BOOST_THREAD_FWD_REF(Fp) c);

        template<typename F, typename Rp, typename Fp>
        friend BOOST_THREAD_FUTURE<Rp>
        detail::make_shared_future_sync_continuation_shared_state(boost::unique_lock<boost::mutex> &lock, F f, BOOST_THREAD_FWD_REF(Fp) c);

  #ifdef BOOST_THREAD_PROVIDES_EXECUTORS
        template<typename Ex, typename F, typename Rp, typename Fp>
        friend BOOST_THREAD_FUTURE<Rp>
        detail::make_future_executor_continuation_shared_state(Ex& ex, boost::unique_lock<boost::mutex> &lock, BOOST_THREAD_RV_REF(F) f, BOOST_THREAD_FWD_REF(Fp) c);

        template<typename Ex, typename F, typename Rp, typename Fp>
        friend BOOST_THREAD_FUTURE<Rp>
        detail::make_shared_future_executor_continuation_shared_state(Ex& ex, boost::unique_lock<boost::mutex> &lock, F f, BOOST_THREAD_FWD_REF(Fp) c);

        template <class Rp, class Fp, class Executor>
        friend BOOST_THREAD_FUTURE<Rp>
        detail::make_future_executor_shared_state(Executor& ex, BOOST_THREAD_FWD_REF(Fp) f);
  #endif
#endif
#if defined BOOST_THREAD_PROVIDES_FUTURE_UNWRAP
        template<typename F, typename Rp>
        friend struct detail::future_unwrap_shared_state;
        template <class F, class Rp>
        friend BOOST_THREAD_FUTURE<Rp>
        detail::make_future_unwrap_shared_state(boost::unique_lock<boost::mutex> &lock, BOOST_THREAD_RV_REF(F) f);
#endif
#if defined(BOOST_THREAD_PROVIDES_FUTURE_WHEN_ALL_WHEN_ANY)
      template< typename InputIterator>
      friend typename boost::disable_if<is_future_type<InputIterator>,
        BOOST_THREAD_FUTURE<csbl::vector<typename InputIterator::value_type>  >
      >::type
      when_all(InputIterator first, InputIterator last);

      //friend inline BOOST_THREAD_FUTURE<csbl::tuple<> > when_all();

    #if ! defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)
      template< typename T0, typename ...T>
      friend BOOST_THREAD_FUTURE<csbl::tuple<typename decay<T0>::type, typename decay<T>::type...> >
      when_all(BOOST_THREAD_FWD_REF(T0) f, BOOST_THREAD_FWD_REF(T) ... futures);
    #endif

      template< typename InputIterator>
      friend typename boost::disable_if<is_future_type<InputIterator>,
        BOOST_THREAD_FUTURE<csbl::vector<typename InputIterator::value_type>  >
      >::type
      when_any(InputIterator first, InputIterator last);

      //friend inline BOOST_THREAD_FUTURE<csbl::tuple<> > when_any();

    #if ! defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)
      template< typename T0, typename ...T>
      friend BOOST_THREAD_FUTURE<csbl::tuple<typename decay<T0>::type, typename decay<T>::type...> >
      when_any(BOOST_THREAD_FWD_REF(T0) f, BOOST_THREAD_FWD_REF(T) ... futures);
    #endif
#endif // BOOST_THREAD_PROVIDES_FUTURE_WHEN_ALL_WHEN_ANY
#if defined BOOST_THREAD_PROVIDES_SIGNATURE_PACKAGED_TASK
        template <class> friend class packaged_task; // todo check if this works in windows
#else
        friend class packaged_task<R>;
#endif
        friend class detail::future_waiter;

        template <class Rp, class Fp>
        friend BOOST_THREAD_FUTURE<Rp>
        detail::make_future_async_shared_state(BOOST_THREAD_FWD_REF(Fp) f);

        template <class Rp, class Fp>
        friend BOOST_THREAD_FUTURE<Rp>
        detail::make_future_deferred_shared_state(BOOST_THREAD_FWD_REF(Fp) f);

        typedef typename base_type::move_dest_type move_dest_type;

        BOOST_THREAD_FUTURE(future_ptr a_future):
          base_type(a_future)
        {
        }

    public:
        BOOST_THREAD_MOVABLE_ONLY(BOOST_THREAD_FUTURE)
        typedef future_state::state state;
        typedef R value_type; // EXTENSION

        BOOST_CONSTEXPR BOOST_THREAD_FUTURE() {}
        //BOOST_CONSTEXPR
        BOOST_THREAD_FUTURE(exceptional_ptr const& ex):
            base_type(ex) {}

        ~BOOST_THREAD_FUTURE() {
        }

        BOOST_THREAD_FUTURE(BOOST_THREAD_RV_REF(BOOST_THREAD_FUTURE) other) BOOST_NOEXCEPT:
        base_type(boost::move(static_cast<base_type&>(BOOST_THREAD_RV(other))))
        {
        }
#if defined BOOST_THREAD_PROVIDES_FUTURE_UNWRAP
        inline explicit BOOST_THREAD_FUTURE(BOOST_THREAD_RV_REF(BOOST_THREAD_FUTURE<BOOST_THREAD_FUTURE<R> >) other); // EXTENSION
#endif

        explicit BOOST_THREAD_FUTURE(BOOST_THREAD_RV_REF(shared_future<R>) other) :
        base_type(boost::move(static_cast<base_type&>(BOOST_THREAD_RV(other))))
        {}

        BOOST_THREAD_FUTURE& operator=(BOOST_THREAD_RV_REF(BOOST_THREAD_FUTURE) other) BOOST_NOEXCEPT
        {
            this->base_type::operator=(boost::move(static_cast<base_type&>(BOOST_THREAD_RV(other))));
            return *this;
        }

        shared_future<R> share()
        {
          return shared_future<R>(::boost::move(*this));
        }

        void swap(BOOST_THREAD_FUTURE& other)
        {
            static_cast<base_type*>(this)->swap(other);
        }

        // todo this function must be private and friendship provided to the internal users.
        void set_async()
        {
          this->future_->set_async();
        }
        // todo this function must be private and friendship provided to the internal users.
        void set_deferred()
        {
          this->future_->set_deferred();
        }
        bool run_if_is_deferred() {
          return this->future_->run_if_is_deferred();
        }
        bool run_if_is_deferred_or_ready() {
          return this->future_->run_if_is_deferred_or_ready();
        }
        // retrieving the value
        move_dest_type get()
        {
            if (this->future_.get() == 0)
            {
                boost::throw_exception(future_uninitialized());
            }
            unique_lock<boost::mutex> lk(this->future_->mutex);
            if (! this->future_->valid(lk))
            {
                boost::throw_exception(future_uninitialized());
            }
#ifdef BOOST_THREAD_PROVIDES_FUTURE_INVALID_AFTER_GET
            this->future_->invalidate(lk);
#endif
            return this->future_->get(lk);
        }

        template <typename R2>
        typename boost::disable_if< is_void<R2>, move_dest_type>::type
        get_or(BOOST_THREAD_RV_REF(R2) v)
        {

            if (this->future_.get() == 0)
            {
                boost::throw_exception(future_uninitialized());
            }
            unique_lock<boost::mutex> lk(this->future_->mutex);
            if (! this->future_->valid(lk))
            {
                boost::throw_exception(future_uninitialized());
            }
            this->future_->wait(lk, false);
#ifdef BOOST_THREAD_PROVIDES_FUTURE_INVALID_AFTER_GET
            this->future_->invalidate(lk);
#endif

            if (this->future_->has_value(lk)) {
              return this->future_->get(lk);
            }
            else {
              return boost::move(v);
            }
        }

        template <typename R2>
        typename boost::disable_if< is_void<R2>, move_dest_type>::type
        get_or(R2 const& v)  // EXTENSION
        {
            if (this->future_.get() == 0)
            {
                boost::throw_exception(future_uninitialized());
            }
            unique_lock<boost::mutex> lk(this->future_->mutex);
            if (! this->future_->valid(lk))
            {
                boost::throw_exception(future_uninitialized());
            }
            this->future_->wait(lk, false);
#ifdef BOOST_THREAD_PROVIDES_FUTURE_INVALID_AFTER_GET
            this->future_->invalidate(lk);
#endif
            if (this->future_->has_value(lk)) {
              return this->future_->get(lk);
            }
            else {
              return v;
            }
        }

#if defined BOOST_THREAD_PROVIDES_FUTURE_CONTINUATION
        template<typename F>
        inline BOOST_THREAD_FUTURE<typename boost::result_of<F(BOOST_THREAD_FUTURE)>::type>
        then(BOOST_THREAD_FWD_REF(F) func);  // EXTENSION
        template<typename F>
        inline BOOST_THREAD_FUTURE<typename boost::result_of<F(BOOST_THREAD_FUTURE)>::type>
        then(launch policy, BOOST_THREAD_FWD_REF(F) func);  // EXTENSION
  #ifdef BOOST_THREAD_PROVIDES_EXECUTORS
        template<typename Ex, typename F>
        inline BOOST_THREAD_FUTURE<typename boost::result_of<F(BOOST_THREAD_FUTURE)>::type>
        then(Ex& ex, BOOST_THREAD_FWD_REF(F) func);  // EXTENSION
  #endif

        template <typename R2>
        inline typename boost::disable_if< is_void<R2>, BOOST_THREAD_FUTURE<R> >::type
        fallback_to(BOOST_THREAD_RV_REF(R2) v);  // EXTENSION
        template <typename R2>
        inline typename boost::disable_if< is_void<R2>, BOOST_THREAD_FUTURE<R> >::type
        fallback_to(R2 const& v);  // EXTENSION

#endif

    };

    BOOST_THREAD_DCL_MOVABLE_BEG(T) BOOST_THREAD_FUTURE<T> BOOST_THREAD_DCL_MOVABLE_END

        template <typename R2>
        class BOOST_THREAD_FUTURE<BOOST_THREAD_FUTURE<R2> > : public detail::basic_future<BOOST_THREAD_FUTURE<R2> >
        {
          typedef BOOST_THREAD_FUTURE<R2> R;

        private:
            typedef detail::basic_future<R> base_type;
            typedef typename base_type::future_ptr future_ptr;

            friend class shared_future<R>;
            friend class promise<R>;
#if defined BOOST_THREAD_PROVIDES_FUTURE_CONTINUATION
            template <typename, typename, typename>
            friend struct detail::future_async_continuation_shared_state;
            template <typename, typename, typename>
            friend struct detail::future_deferred_continuation_shared_state;

            template <class F, class Rp, class Fp>
            friend BOOST_THREAD_FUTURE<Rp>
            detail::make_future_async_continuation_shared_state(boost::unique_lock<boost::mutex> &lock, BOOST_THREAD_RV_REF(F) f, BOOST_THREAD_FWD_REF(Fp) c);

            template <class F, class Rp, class Fp>
            friend BOOST_THREAD_FUTURE<Rp>
            detail::make_future_sync_continuation_shared_state(boost::unique_lock<boost::mutex> &lock, BOOST_THREAD_RV_REF(F) f, BOOST_THREAD_FWD_REF(Fp) c);

            template <class F, class Rp, class Fp>
            friend BOOST_THREAD_FUTURE<Rp>
            detail::make_future_deferred_continuation_shared_state(boost::unique_lock<boost::mutex> &lock, BOOST_THREAD_RV_REF(F) f, BOOST_THREAD_FWD_REF(Fp) c);

            template<typename F, typename Rp, typename Fp>
            friend BOOST_THREAD_FUTURE<Rp>
            detail::make_shared_future_deferred_continuation_shared_state(boost::unique_lock<boost::mutex> &lock, F f, BOOST_THREAD_FWD_REF(Fp) c);

            template<typename F, typename Rp, typename Fp>
            friend BOOST_THREAD_FUTURE<Rp>
            detail::make_shared_future_async_continuation_shared_state(boost::unique_lock<boost::mutex> &lock, F f, BOOST_THREAD_FWD_REF(Fp) c);

            template<typename F, typename Rp, typename Fp>
            friend BOOST_THREAD_FUTURE<Rp>
            detail::make_shared_future_sync_continuation_shared_state(boost::unique_lock<boost::mutex> &lock, F f, BOOST_THREAD_FWD_REF(Fp) c);

      #ifdef BOOST_THREAD_PROVIDES_EXECUTORS
            template<typename Ex, typename F, typename Rp, typename Fp>
            friend BOOST_THREAD_FUTURE<Rp>
            detail::make_future_executor_continuation_shared_state(Ex& ex, boost::unique_lock<boost::mutex> &lock, BOOST_THREAD_RV_REF(F) f, BOOST_THREAD_FWD_REF(Fp) c);

            template<typename Ex, typename F, typename Rp, typename Fp>
            friend BOOST_THREAD_FUTURE<Rp>
            detail::make_shared_future_executor_continuation_shared_state(Ex& ex, boost::unique_lock<boost::mutex> &lock, F f, BOOST_THREAD_FWD_REF(Fp) c);

            template <class Rp, class Fp, class Executor>
            friend BOOST_THREAD_FUTURE<Rp>
            detail::make_future_executor_shared_state(Executor& ex, BOOST_THREAD_FWD_REF(Fp) f);
      #endif

#endif
#if defined BOOST_THREAD_PROVIDES_FUTURE_UNWRAP
            template<typename F, typename Rp>
            friend struct detail::future_unwrap_shared_state;
        template <class F, class Rp>
        friend BOOST_THREAD_FUTURE<Rp>
        detail::make_future_unwrap_shared_state(boost::unique_lock<boost::mutex> &lock, BOOST_THREAD_RV_REF(F) f);
#endif
#if defined(BOOST_THREAD_PROVIDES_FUTURE_WHEN_ALL_WHEN_ANY)
      template< typename InputIterator>
      friend typename boost::disable_if<is_future_type<InputIterator>,
        BOOST_THREAD_FUTURE<csbl::vector<typename InputIterator::value_type>  >
      >::type
      when_all(InputIterator first, InputIterator last);

      friend inline BOOST_THREAD_FUTURE<csbl::tuple<> > when_all();

    #if ! defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)
      template< typename T0, typename ...T>
      friend BOOST_THREAD_FUTURE<csbl::tuple<typename decay<T0>::type, typename decay<T>::type...> >
      when_all(BOOST_THREAD_FWD_REF(T0) f, BOOST_THREAD_FWD_REF(T) ... futures);
    #endif

      template< typename InputIterator>
      friend typename boost::disable_if<is_future_type<InputIterator>,
        BOOST_THREAD_FUTURE<csbl::vector<typename InputIterator::value_type>  >
      >::type
      when_any(InputIterator first, InputIterator last);

      friend inline BOOST_THREAD_FUTURE<csbl::tuple<> > when_any();

    #if ! defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)
      template< typename T0, typename ...T>
      friend BOOST_THREAD_FUTURE<csbl::tuple<typename decay<T0>::type, typename decay<T>::type...> >
      when_any(BOOST_THREAD_FWD_REF(T0) f, BOOST_THREAD_FWD_REF(T) ... futures);
    #endif
#endif // BOOST_THREAD_PROVIDES_FUTURE_WHEN_ALL_WHEN_ANY

    #if defined BOOST_THREAD_PROVIDES_SIGNATURE_PACKAGED_TASK
            template <class> friend class packaged_task; // todo check if this works in windows
    #else
            friend class packaged_task<R>;
    #endif
            friend class detail::future_waiter;

            template <class Rp, class Fp>
            friend BOOST_THREAD_FUTURE<Rp>
            detail::make_future_async_shared_state(BOOST_THREAD_FWD_REF(Fp) f);

            template <class Rp, class Fp>
            friend BOOST_THREAD_FUTURE<Rp>
            detail::make_future_deferred_shared_state(BOOST_THREAD_FWD_REF(Fp) f);

            typedef typename base_type::move_dest_type move_dest_type;

            BOOST_THREAD_FUTURE(future_ptr a_future):
              base_type(a_future)
            {
            }
        public:

            BOOST_THREAD_MOVABLE_ONLY(BOOST_THREAD_FUTURE)
            typedef future_state::state state;
            typedef R value_type; // EXTENSION

            BOOST_CONSTEXPR BOOST_THREAD_FUTURE() {}
            //BOOST_CONSTEXPR
            BOOST_THREAD_FUTURE(exceptional_ptr const& ex):
                base_type(ex) {}

            ~BOOST_THREAD_FUTURE() {
            }

            BOOST_THREAD_FUTURE(BOOST_THREAD_RV_REF(BOOST_THREAD_FUTURE) other) BOOST_NOEXCEPT:
            base_type(boost::move(static_cast<base_type&>(BOOST_THREAD_RV(other))))
            {
            }

            BOOST_THREAD_FUTURE& operator=(BOOST_THREAD_RV_REF(BOOST_THREAD_FUTURE) other) BOOST_NOEXCEPT
            {
                this->base_type::operator=(boost::move(static_cast<base_type&>(BOOST_THREAD_RV(other))));
                return *this;
            }

            shared_future<R> share()
            {
              return shared_future<R>(::boost::move(*this));
            }

            void swap(BOOST_THREAD_FUTURE& other)
            {
                static_cast<base_type*>(this)->swap(other);
            }

            // todo this function must be private and friendship provided to the internal users.
            void set_async()
            {
              this->future_->set_async();
            }
            // todo this function must be private and friendship provided to the internal users.
            void set_deferred()
            {
              this->future_->set_deferred();
            }
            bool run_if_is_deferred() {
              return this->future_->run_if_is_deferred();
            }
            bool run_if_is_deferred_or_ready() {
              return this->future_->run_if_is_deferred_or_ready();
            }
            // retrieving the value
            move_dest_type get()
            {
                if (this->future_.get() == 0)
                {
                    boost::throw_exception(future_uninitialized());
                }
                unique_lock<boost::mutex> lk(this->future_->mutex);
                if (! this->future_->valid(lk))
                {
                    boost::throw_exception(future_uninitialized());
                }
    #ifdef BOOST_THREAD_PROVIDES_FUTURE_INVALID_AFTER_GET
                this->future_->invalidate(lk);
    #endif
                return this->future_->get(lk);
            }
            move_dest_type get_or(BOOST_THREAD_RV_REF(R) v) // EXTENSION
            {
                if (this->future_.get() == 0)
                {
                    boost::throw_exception(future_uninitialized());
                }
                unique_lock<boost::mutex> lk(this->future_->mutex);
                if (! this->future_->valid(lk))
                {
                    boost::throw_exception(future_uninitialized());
                }
                this->future_->wait(lk, false);
    #ifdef BOOST_THREAD_PROVIDES_FUTURE_INVALID_AFTER_GET
                this->future_->invalidate(lk);
    #endif
                if (this->future_->has_value(lk)) return this->future_->get(lk);
                else return boost::move(v);
            }

            move_dest_type get_or(R const& v) // EXTENSION
            {
                if (this->future_.get() == 0)
                {
                    boost::throw_exception(future_uninitialized());
                }
                unique_lock<boost::mutex> lk(this->future_->mutex);
                if (! this->future_->valid(lk))
                {
                    boost::throw_exception(future_uninitialized());
                }
                this->future_->wait(lk, false);
    #ifdef BOOST_THREAD_PROVIDES_FUTURE_INVALID_AFTER_GET
                this->future_->invalidate(lk);
    #endif
                if (this->future_->has_value(lk)) return this->future_->get(lk);
                else return v;
            }


    #if defined BOOST_THREAD_PROVIDES_FUTURE_CONTINUATION
            template<typename F>
            inline BOOST_THREAD_FUTURE<typename boost::result_of<F(BOOST_THREAD_FUTURE)>::type>
            then(BOOST_THREAD_FWD_REF(F) func); // EXTENSION
            template<typename F>
            inline BOOST_THREAD_FUTURE<typename boost::result_of<F(BOOST_THREAD_FUTURE)>::type>
            then(launch policy, BOOST_THREAD_FWD_REF(F) func); // EXTENSION
      #ifdef BOOST_THREAD_PROVIDES_EXECUTORS
            template<typename Ex, typename F>
            inline BOOST_THREAD_FUTURE<typename boost::result_of<F(BOOST_THREAD_FUTURE)>::type>
            then(Ex &ex, BOOST_THREAD_FWD_REF(F) func); // EXTENSION
      #endif
    #endif

    #if defined BOOST_THREAD_PROVIDES_FUTURE_UNWRAP
            inline
            BOOST_THREAD_FUTURE<R2>
            unwrap(); // EXTENSION
    #endif

  };

    template <typename R>
    class shared_future : public detail::basic_future<R>
    {
        typedef detail::basic_future<R> base_type;
        typedef typename base_type::future_ptr future_ptr;

        friend class detail::future_waiter;
        friend class promise<R>;

#if defined BOOST_THREAD_PROVIDES_FUTURE_CONTINUATION
        template <typename, typename, typename>
        friend struct detail::future_async_continuation_shared_state;
        template <typename, typename, typename>
        friend struct detail::future_deferred_continuation_shared_state;

        template <class F, class Rp, class Fp>
        friend BOOST_THREAD_FUTURE<Rp>
        detail::make_future_async_continuation_shared_state(boost::unique_lock<boost::mutex> &lock, BOOST_THREAD_RV_REF(F) f, BOOST_THREAD_FWD_REF(Fp) c);

        template <class F, class Rp, class Fp>
        friend BOOST_THREAD_FUTURE<Rp>
        detail::make_future_sync_continuation_shared_state(boost::unique_lock<boost::mutex> &lock, BOOST_THREAD_RV_REF(F) f, BOOST_THREAD_FWD_REF(Fp) c);

        template <class F, class Rp, class Fp>
        friend BOOST_THREAD_FUTURE<Rp>
        detail::make_future_deferred_continuation_shared_state(boost::unique_lock<boost::mutex> &lock, BOOST_THREAD_RV_REF(F) f, BOOST_THREAD_FWD_REF(Fp) c);
#endif
#if defined BOOST_THREAD_PROVIDES_SIGNATURE_PACKAGED_TASK
        template <class> friend class packaged_task;// todo check if this works in windows
#else
        friend class packaged_task<R>;
#endif
        shared_future(future_ptr a_future):
          base_type(a_future)
        {}

    public:
        BOOST_THREAD_COPYABLE_AND_MOVABLE(shared_future)
        typedef R value_type; // EXTENSION

        shared_future(shared_future const& other):
        base_type(other.future_)
        {}

        typedef future_state::state state;

        BOOST_CONSTEXPR shared_future()
        {}
        //BOOST_CONSTEXPR
        shared_future(exceptional_ptr const& ex):
            base_type(ex) {}
        ~shared_future()
        {}

        shared_future& operator=(BOOST_THREAD_COPY_ASSIGN_REF(shared_future) other)
        {
            this->future_ = other.future_;
            return *this;
        }

        shared_future(BOOST_THREAD_RV_REF(shared_future) other) BOOST_NOEXCEPT :
        base_type(boost::move(static_cast<base_type&>(BOOST_THREAD_RV(other))))
        {
        }
        shared_future(BOOST_THREAD_RV_REF( BOOST_THREAD_FUTURE<R> ) other) BOOST_NOEXCEPT :
        base_type(boost::move(static_cast<base_type&>(BOOST_THREAD_RV(other))))
        {
        }

        shared_future& operator=(BOOST_THREAD_RV_REF(shared_future) other) BOOST_NOEXCEPT
        {
            base_type::operator=(boost::move(static_cast<base_type&>(BOOST_THREAD_RV(other))));
            return *this;
        }
        shared_future& operator=(BOOST_THREAD_RV_REF( BOOST_THREAD_FUTURE<R> ) other) BOOST_NOEXCEPT
        {
            base_type::operator=(boost::move(static_cast<base_type&>(BOOST_THREAD_RV(other))));
            return *this;
        }

        void swap(shared_future& other) BOOST_NOEXCEPT
        {
            static_cast<base_type*>(this)->swap(other);
        }
        bool run_if_is_deferred() {
          return this->future_->run_if_is_deferred();
        }
        bool run_if_is_deferred_or_ready() {
          return this->future_->run_if_is_deferred_or_ready();
        }
        // retrieving the value
        typename detail::shared_state<R>::shared_future_get_result_type get() const
        {
            if(!this->future_)
            {
                boost::throw_exception(future_uninitialized());
            }
            return this->future_->get_sh();
        }

        template <typename R2>
        typename boost::disable_if< is_void<R2>, typename detail::shared_state<R>::shared_future_get_result_type>::type
        get_or(BOOST_THREAD_RV_REF(R2) v)  const // EXTENSION
        {
            if(!this->future_)
            {
                boost::throw_exception(future_uninitialized());
            }
            this->future_->wait();
            if (this->future_->has_value()) return this->future_->get_sh();
            else return boost::move(v);
        }

#if defined BOOST_THREAD_PROVIDES_FUTURE_CONTINUATION
        template<typename F>
        inline BOOST_THREAD_FUTURE<typename boost::result_of<F(shared_future)>::type>
        then(BOOST_THREAD_FWD_REF(F) func) const; // EXTENSION
        template<typename F>
        inline BOOST_THREAD_FUTURE<typename boost::result_of<F(shared_future)>::type>
        then(launch policy, BOOST_THREAD_FWD_REF(F) func) const; // EXTENSION
  #ifdef BOOST_THREAD_PROVIDES_EXECUTORS
        template<typename Ex, typename F>
        inline BOOST_THREAD_FUTURE<typename boost::result_of<F(shared_future)>::type>
        then(Ex& ex, BOOST_THREAD_FWD_REF(F) func) const; // EXTENSION
  #endif
#endif

    };

    BOOST_THREAD_DCL_MOVABLE_BEG(T) shared_future<T> BOOST_THREAD_DCL_MOVABLE_END

    template <typename R>
    class promise
    {
        typedef boost::shared_ptr<detail::shared_state<R> > future_ptr;

        typedef typename detail::shared_state<R>::source_reference_type source_reference_type;
        typedef typename detail::shared_state<R>::rvalue_source_type rvalue_source_type;
        typedef typename detail::shared_state<R>::move_dest_type move_dest_type;
        typedef typename detail::shared_state<R>::shared_future_get_result_type shared_future_get_result_type;

        future_ptr future_;
        bool future_obtained;

        void lazy_init()
        {
#if defined BOOST_THREAD_PROVIDES_PROMISE_LAZY
#include <boost/thread/detail/atomic_undef_macros.hpp>
          if(!atomic_load(&future_))
            {
                future_ptr blank;
                atomic_compare_exchange(&future_,&blank,future_ptr(new detail::shared_state<R>));
            }
#include <boost/thread/detail/atomic_redef_macros.hpp>
#endif
        }

    public:
        BOOST_THREAD_MOVABLE_ONLY(promise)
#if defined BOOST_THREAD_PROVIDES_FUTURE_CTOR_ALLOCATORS
        template <class Allocator>
        promise(boost::allocator_arg_t, Allocator a)
        {
          typedef typename Allocator::template rebind<detail::shared_state<R> >::other A2;
          A2 a2(a);
          typedef thread_detail::allocator_destructor<A2> D;

          future_ = future_ptr(::new(a2.allocate(1)) detail::shared_state<R>(), D(a2, 1) );
          future_obtained = false;
        }
#endif
        promise():
#if defined BOOST_THREAD_PROVIDES_PROMISE_LAZY
            future_(),
#else
            future_(new detail::shared_state<R>()),
#endif
            future_obtained(false)
        {}

        ~promise()
        {
            if(future_)
            {
                boost::unique_lock<boost::mutex> lock(future_->mutex);

                if(!future_->done && !future_->is_constructed)
                {
                    future_->mark_exceptional_finish_internal(boost::copy_exception(broken_promise()), lock);
                }
            }
        }

        // Assignment
        promise(BOOST_THREAD_RV_REF(promise) rhs) BOOST_NOEXCEPT :
            future_(BOOST_THREAD_RV(rhs).future_),future_obtained(BOOST_THREAD_RV(rhs).future_obtained)
        {
            BOOST_THREAD_RV(rhs).future_.reset();
            BOOST_THREAD_RV(rhs).future_obtained=false;
        }
        promise & operator=(BOOST_THREAD_RV_REF(promise) rhs) BOOST_NOEXCEPT
        {
            future_=BOOST_THREAD_RV(rhs).future_;
            future_obtained=BOOST_THREAD_RV(rhs).future_obtained;
            BOOST_THREAD_RV(rhs).future_.reset();
            BOOST_THREAD_RV(rhs).future_obtained=false;
            return *this;
        }

        void swap(promise& other)
        {
            future_.swap(other.future_);
            std::swap(future_obtained,other.future_obtained);
        }

#ifdef BOOST_THREAD_PROVIDES_EXECUTORS
        void set_executor(executor_ptr_type aex)
        {
          lazy_init();
          if (future_.get()==0)
          {
              boost::throw_exception(promise_moved());
          }
          boost::lock_guard<boost::mutex> lk(future_->mutex);
          future_->set_executor_policy(aex, lk);
        }
#endif
        // Result retrieval
        BOOST_THREAD_FUTURE<R> get_future()
        {
            lazy_init();
            if (future_.get()==0)
            {
                boost::throw_exception(promise_moved());
            }
            if (future_obtained)
            {
                boost::throw_exception(future_already_retrieved());
            }
            future_obtained=true;
            return BOOST_THREAD_FUTURE<R>(future_);
        }

#if defined  BOOST_NO_CXX11_RVALUE_REFERENCES
        template <class TR>
        typename boost::enable_if_c<is_copy_constructible<TR>::value && is_same<R, TR>::value, void>::type
            set_value(TR const &  r)
        {
            lazy_init();
            boost::unique_lock<boost::mutex> lock(future_->mutex);
            if(future_->done)
            {
                boost::throw_exception(promise_already_satisfied());
            }
            future_->mark_finished_with_result_internal(r, lock);
        }
#else
        void set_value(source_reference_type r)
        {
            lazy_init();
            boost::unique_lock<boost::mutex> lock(future_->mutex);
            if(future_->done)
            {
                boost::throw_exception(promise_already_satisfied());
            }
            future_->mark_finished_with_result_internal(r, lock);
        }
#endif

        void set_value(rvalue_source_type r)
        {
            lazy_init();
            boost::unique_lock<boost::mutex> lock(future_->mutex);
            if(future_->done)
            {
                boost::throw_exception(promise_already_satisfied());
            }
#if ! defined  BOOST_NO_CXX11_RVALUE_REFERENCES
            future_->mark_finished_with_result_internal(boost::move(r), lock);
#else
            future_->mark_finished_with_result_internal(static_cast<rvalue_source_type>(r), lock);
#endif
        }

#if defined  BOOST_NO_CXX11_RVALUE_REFERENCES
        template <class TR>
        typename boost::enable_if_c<is_copy_constructible<TR>::value && is_same<R, TR>::value, void>::type
            set_value_deferred(TR const &  r)
        {
            lazy_init();
            if (future_.get()==0)
            {
                boost::throw_exception(promise_moved());
            }
            future_->set_value_deferred(r);
        }
#else
        void set_value_deferred(source_reference_type r)
        {
            lazy_init();
            if (future_.get()==0)
            {
                boost::throw_exception(promise_moved());
            }
            future_->set_value_deferred(r);
        }
#endif

        void set_value_deferred(rvalue_source_type r)
        {
            lazy_init();
            if (future_.get()==0)
            {
                boost::throw_exception(promise_moved());
            }
#if ! defined  BOOST_NO_CXX11_RVALUE_REFERENCES
            future_->set_value_deferred(boost::move(r));
#else
            future_->set_value_deferred(static_cast<rvalue_source_type>(r));
#endif
        }

#if ! defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)
        template <class ...Args>
        void emplace(BOOST_THREAD_FWD_REF(Args) ...args)
        {
            lazy_init();
            boost::unique_lock<boost::mutex> lock(future_->mutex);
            if(future_->done)
            {
                boost::throw_exception(promise_already_satisfied());
            }
            future_->mark_finished_with_result_internal(lock, boost::forward<Args>(args)...);
        }

#endif

        void set_exception(boost::exception_ptr p)
        {
            lazy_init();
            boost::unique_lock<boost::mutex> lock(future_->mutex);
            if(future_->done)
            {
                boost::throw_exception(promise_already_satisfied());
            }
            future_->mark_exceptional_finish_internal(p, lock);
        }
        template <typename E>
        void set_exception(E ex)
        {
          set_exception(boost::copy_exception(ex));
        }
        void set_exception_deferred(boost::exception_ptr p)
        {
            lazy_init();
            if (future_.get()==0)
            {
                boost::throw_exception(promise_moved());
            }
            future_->set_exception_deferred(p);
        }
        template <typename E>
        void set_exception_deferred(E ex)
        {
          set_exception_deferred(boost::copy_exception(ex));
        }

        // setting the result with deferred notification
#if defined  BOOST_NO_CXX11_RVALUE_REFERENCES
        template <class TR>
        typename boost::enable_if_c<is_copy_constructible<TR>::value && is_same<R, TR>::value, void>::type set_value_at_thread_exit(TR const& r)
        {
          if (future_.get()==0)
          {
              boost::throw_exception(promise_moved());
          }
          future_->set_value_at_thread_exit(r);
        }
#else
        void set_value_at_thread_exit(source_reference_type r)
        {
          if (future_.get()==0)
          {
              boost::throw_exception(promise_moved());
          }
          future_->set_value_at_thread_exit(r);
        }
#endif
        void set_value_at_thread_exit(BOOST_THREAD_RV_REF(R) r)
        {
          if (future_.get()==0)
          {
              boost::throw_exception(promise_moved());
          }
          future_->set_value_at_thread_exit(boost::move(r));
        }
        void set_exception_at_thread_exit(exception_ptr e)
        {
          if (future_.get()==0)
          {
              boost::throw_exception(promise_moved());
          }
          future_->set_exception_at_thread_exit(e);
        }
        template <typename E>
        void set_exception_at_thread_exit(E ex)
        {
          set_exception_at_thread_exit(boost::copy_exception(ex));
        }

        template<typename F>
        void set_wait_callback(F f)
        {
            lazy_init();
            future_->set_wait_callback(f,this);
        }
        void notify_deferred()
        {
            if (future_.get()==0)
            {
                boost::throw_exception(promise_moved());
            }
            future_->notify_deferred();
        }

    };

    template <typename R>
    class promise<R&>
    {
        typedef boost::shared_ptr<detail::shared_state<R&> > future_ptr;

        future_ptr future_;
        bool future_obtained;

        void lazy_init()
        {
#if defined BOOST_THREAD_PROVIDES_PROMISE_LAZY
#include <boost/thread/detail/atomic_undef_macros.hpp>
            if(!atomic_load(&future_))
            {
                future_ptr blank;
                atomic_compare_exchange(&future_,&blank,future_ptr(new detail::shared_state<R&>));
            }
#include <boost/thread/detail/atomic_redef_macros.hpp>
#endif
        }

    public:
        BOOST_THREAD_MOVABLE_ONLY(promise)
#if defined BOOST_THREAD_PROVIDES_FUTURE_CTOR_ALLOCATORS
        template <class Allocator>
        promise(boost::allocator_arg_t, Allocator a)
        {
          typedef typename Allocator::template rebind<detail::shared_state<R&> >::other A2;
          A2 a2(a);
          typedef thread_detail::allocator_destructor<A2> D;

          future_ = future_ptr(::new(a2.allocate(1)) detail::shared_state<R&>(), D(a2, 1) );
          future_obtained = false;
        }
#endif
        promise():
#if defined BOOST_THREAD_PROVIDES_PROMISE_LAZY
            future_(),
#else
            future_(new detail::shared_state<R&>()),
#endif
            future_obtained(false)
        {}

        ~promise()
        {
            if(future_)
            {
                boost::unique_lock<boost::mutex> lock(future_->mutex);

                if(!future_->done && !future_->is_constructed)
                {
                    future_->mark_exceptional_finish_internal(boost::copy_exception(broken_promise()), lock);
                }
            }
        }

        // Assignment
        promise(BOOST_THREAD_RV_REF(promise) rhs) BOOST_NOEXCEPT :
            future_(BOOST_THREAD_RV(rhs).future_),future_obtained(BOOST_THREAD_RV(rhs).future_obtained)
        {
            BOOST_THREAD_RV(rhs).future_.reset();
            BOOST_THREAD_RV(rhs).future_obtained=false;
        }
        promise & operator=(BOOST_THREAD_RV_REF(promise) rhs) BOOST_NOEXCEPT
        {
            future_=BOOST_THREAD_RV(rhs).future_;
            future_obtained=BOOST_THREAD_RV(rhs).future_obtained;
            BOOST_THREAD_RV(rhs).future_.reset();
            BOOST_THREAD_RV(rhs).future_obtained=false;
            return *this;
        }

        void swap(promise& other)
        {
            future_.swap(other.future_);
            std::swap(future_obtained,other.future_obtained);
        }

        // Result retrieval
        BOOST_THREAD_FUTURE<R&> get_future()
        {
            lazy_init();
            if (future_.get()==0)
            {
                boost::throw_exception(promise_moved());
            }
            if (future_obtained)
            {
                boost::throw_exception(future_already_retrieved());
            }
            future_obtained=true;
            return BOOST_THREAD_FUTURE<R&>(future_);
        }

        void set_value(R& r)
        {
            lazy_init();
            boost::unique_lock<boost::mutex> lock(future_->mutex);
            if(future_->done)
            {
                boost::throw_exception(promise_already_satisfied());
            }
            future_->mark_finished_with_result_internal(r, lock);
        }
        void set_value_deferred(R& r)
        {
            lazy_init();
            if (future_.get()==0)
            {
                boost::throw_exception(promise_already_satisfied());
            }
            future_->set_value_deferred(r);
        }
        void set_exception(boost::exception_ptr p)
        {
            lazy_init();
            boost::unique_lock<boost::mutex> lock(future_->mutex);
            if(future_->done)
            {
                boost::throw_exception(promise_already_satisfied());
            }
            future_->mark_exceptional_finish_internal(p, lock);
        }
        template <typename E>
        void set_exception(E ex)
        {
          set_exception(boost::copy_exception(ex));
        }
        void set_exception_deferred(boost::exception_ptr p)
        {
            lazy_init();
            if (future_.get()==0)
            {
                boost::throw_exception(promise_moved());
            }
            future_->set_exception_deferred(p);
        }
        template <typename E>
        void set_exception_deferred(E ex)
        {
          set_exception_deferred(boost::copy_exception(ex));
        }
        // setting the result with deferred notification
        void set_value_at_thread_exit(R& r)
        {
          if (future_.get()==0)
          {
              boost::throw_exception(promise_moved());
          }
          future_->set_value_at_thread_exit(r);
        }

        void set_exception_at_thread_exit(exception_ptr e)
        {
          if (future_.get()==0)
          {
              boost::throw_exception(promise_moved());
          }
          future_->set_exception_at_thread_exit(e);
        }
        template <typename E>
        void set_exception_at_thread_exit(E ex)
        {
          set_exception_at_thread_exit(boost::copy_exception(ex));
        }

        template<typename F>
        void set_wait_callback(F f)
        {
            lazy_init();
            future_->set_wait_callback(f,this);
        }
        void notify_deferred()
        {
            if (future_.get()==0)
            {
                boost::throw_exception(promise_moved());
            }
            future_->notify_deferred();
        }
    };

    template <>
    class promise<void>
    {
        typedef boost::shared_ptr<detail::shared_state<void> > future_ptr;

        future_ptr future_;
        bool future_obtained;

        void lazy_init()
        {
#if defined BOOST_THREAD_PROVIDES_PROMISE_LAZY
            if(!atomic_load(&future_))
            {
                future_ptr blank;
                atomic_compare_exchange(&future_,&blank,future_ptr(new detail::shared_state<void>));
            }
#endif
        }
    public:
        BOOST_THREAD_MOVABLE_ONLY(promise)

#if defined BOOST_THREAD_PROVIDES_FUTURE_CTOR_ALLOCATORS
        template <class Allocator>
        promise(boost::allocator_arg_t, Allocator a)
        {
          typedef typename Allocator::template rebind<detail::shared_state<void> >::other A2;
          A2 a2(a);
          typedef thread_detail::allocator_destructor<A2> D;

          future_ = future_ptr(::new(a2.allocate(1)) detail::shared_state<void>(), D(a2, 1) );
          future_obtained = false;
        }
#endif
        promise():
#if defined BOOST_THREAD_PROVIDES_PROMISE_LAZY
            future_(),
#else
            future_(new detail::shared_state<void>),
#endif
            future_obtained(false)
        {}

        ~promise()
        {
            if(future_)
            {
                boost::unique_lock<boost::mutex> lock(future_->mutex);

                if(!future_->done && !future_->is_constructed)
                {
                    future_->mark_exceptional_finish_internal(boost::copy_exception(broken_promise()), lock);
                }
            }
        }

        // Assignment
        promise(BOOST_THREAD_RV_REF(promise) rhs) BOOST_NOEXCEPT :
            future_(BOOST_THREAD_RV(rhs).future_),future_obtained(BOOST_THREAD_RV(rhs).future_obtained)
        {
          // we need to release the future as shared_ptr doesn't implements move semantics
            BOOST_THREAD_RV(rhs).future_.reset();
            BOOST_THREAD_RV(rhs).future_obtained=false;
        }

        promise & operator=(BOOST_THREAD_RV_REF(promise) rhs) BOOST_NOEXCEPT
        {
            future_=BOOST_THREAD_RV(rhs).future_;
            future_obtained=BOOST_THREAD_RV(rhs).future_obtained;
            BOOST_THREAD_RV(rhs).future_.reset();
            BOOST_THREAD_RV(rhs).future_obtained=false;
            return *this;
        }

        void swap(promise& other)
        {
            future_.swap(other.future_);
            std::swap(future_obtained,other.future_obtained);
        }

        // Result retrieval
        BOOST_THREAD_FUTURE<void> get_future()
        {
            lazy_init();

            if (future_.get()==0)
            {
                boost::throw_exception(promise_moved());
            }
            if(future_obtained)
            {
                boost::throw_exception(future_already_retrieved());
            }
            future_obtained=true;
            //return BOOST_THREAD_MAKE_RV_REF(BOOST_THREAD_FUTURE<void>(future_));
            return BOOST_THREAD_FUTURE<void>(future_);
        }

        void set_value()
        {
            lazy_init();
            boost::unique_lock<boost::mutex> lock(future_->mutex);
            if(future_->done)
            {
                boost::throw_exception(promise_already_satisfied());
            }
            future_->mark_finished_with_result_internal(lock);
        }
        void set_value_deferred()
        {
            lazy_init();
            if (future_.get()==0)
            {
                boost::throw_exception(promise_moved());
            }
            future_->set_value_deferred();
        }

        void set_exception(boost::exception_ptr p)
        {
            lazy_init();
            boost::unique_lock<boost::mutex> lock(future_->mutex);
            if(future_->done)
            {
                boost::throw_exception(promise_already_satisfied());
            }
            future_->mark_exceptional_finish_internal(p,lock);
        }
        template <typename E>
        void set_exception(E ex)
        {
          set_exception(boost::copy_exception(ex));
        }
        void set_exception_deferred(boost::exception_ptr p)
        {
            lazy_init();
            if (future_.get()==0)
            {
                boost::throw_exception(promise_moved());
            }
            future_->set_exception_deferred(p);
        }
        template <typename E>
        void set_exception_deferred(E ex)
        {
          set_exception_deferred(boost::copy_exception(ex));
        }
        // setting the result with deferred notification
        void set_value_at_thread_exit()
        {
          if (future_.get()==0)
          {
              boost::throw_exception(promise_moved());
          }
          future_->set_value_at_thread_exit();
        }

        void set_exception_at_thread_exit(exception_ptr e)
        {
          if (future_.get()==0)
          {
              boost::throw_exception(promise_moved());
          }
          future_->set_exception_at_thread_exit(e);
        }
        template <typename E>
        void set_exception_at_thread_exit(E ex)
        {
          set_exception_at_thread_exit(boost::copy_exception(ex));
        }

        template<typename F>
        void set_wait_callback(F f)
        {
            lazy_init();
            future_->set_wait_callback(f,this);
        }
        void notify_deferred()
        {
            if (future_.get()==0)
            {
                boost::throw_exception(promise_moved());
            }
            future_->notify_deferred();
        }
    };
}
#if defined BOOST_THREAD_PROVIDES_FUTURE_CTOR_ALLOCATORS
namespace boost { namespace container {
    template <class R, class Alloc>
    struct uses_allocator< ::boost::promise<R> , Alloc> : true_type
    {
    };
}}
#if ! defined  BOOST_NO_CXX11_ALLOCATOR
namespace std {
    template <class R, class Alloc>
    struct uses_allocator< ::boost::promise<R> , Alloc> : true_type
    {
    };
}
#endif
#endif

namespace boost
{

    BOOST_THREAD_DCL_MOVABLE_BEG(T) promise<T> BOOST_THREAD_DCL_MOVABLE_END

    namespace detail
    {
#if defined BOOST_THREAD_PROVIDES_SIGNATURE_PACKAGED_TASK
      template<typename R>
      struct task_base_shared_state;
#if defined(BOOST_THREAD_PROVIDES_VARIADIC_THREAD)
      template<typename R, typename ...ArgTypes>
      struct task_base_shared_state<R(ArgTypes...)>:
#else
      template<typename R>
      struct task_base_shared_state<R()>:
#endif
#else
      template<typename R>
      struct task_base_shared_state:
#endif
            detail::shared_state<R>
        {
            bool started;

            task_base_shared_state():
                started(false)
            {}

            void reset()
            {
              // todo The packaged_task::reset must be as if an assignemnt froma new packaged_task with the same function
              // the reset function is an optimization that avoids reallocating a new task.
              started=false;
              this->validate();
            }
#if defined BOOST_THREAD_PROVIDES_SIGNATURE_PACKAGED_TASK && defined(BOOST_THREAD_PROVIDES_VARIADIC_THREAD)
            virtual void do_run(BOOST_THREAD_RV_REF(ArgTypes) ... args)=0;
            void run(BOOST_THREAD_RV_REF(ArgTypes) ... args)
#else
            virtual void do_run()=0;
            void run()
#endif
            {
                {
                    boost::lock_guard<boost::mutex> lk(this->mutex);
                    if(started)
                    {
                        boost::throw_exception(task_already_started());
                    }
                    started=true;
                }
#if defined BOOST_THREAD_PROVIDES_SIGNATURE_PACKAGED_TASK && defined(BOOST_THREAD_PROVIDES_VARIADIC_THREAD)
                do_run(boost::move(args)...);
#else
                do_run();
#endif
            }

#if defined BOOST_THREAD_PROVIDES_SIGNATURE_PACKAGED_TASK && defined(BOOST_THREAD_PROVIDES_VARIADIC_THREAD)
            virtual void do_apply(BOOST_THREAD_RV_REF(ArgTypes) ... args)=0;
            void apply(BOOST_THREAD_RV_REF(ArgTypes) ... args)
#else
            virtual void do_apply()=0;
            void apply()
#endif
            {
                {
                    boost::lock_guard<boost::mutex> lk(this->mutex);
                    if(started)
                    {
                        boost::throw_exception(task_already_started());
                    }
                    started=true;
                }
#if defined BOOST_THREAD_PROVIDES_SIGNATURE_PACKAGED_TASK && defined(BOOST_THREAD_PROVIDES_VARIADIC_THREAD)
                do_apply(boost::move(args)...);
#else
                do_apply();
#endif
            }

            void owner_destroyed()
            {
                boost::unique_lock<boost::mutex> lk(this->mutex);
                if(!started)
                {
                    started=true;
                    this->mark_exceptional_finish_internal(boost::copy_exception(boost::broken_promise()), lk);
                }
            }
        };

#if defined BOOST_THREAD_PROVIDES_SIGNATURE_PACKAGED_TASK
        template<typename F, typename R>
        struct task_shared_state;
#if defined(BOOST_THREAD_PROVIDES_VARIADIC_THREAD)
        template<typename F, typename R, typename ...ArgTypes>
        struct task_shared_state<F, R(ArgTypes...)>:
          task_base_shared_state<R(ArgTypes...)>
#else
        template<typename F, typename R>
        struct task_shared_state<F, R()>:
          task_base_shared_state<R()>
#endif
#else
        template<typename F, typename R>
        struct task_shared_state:
            task_base_shared_state<R>
#endif
        {
        private:
          task_shared_state(task_shared_state&);
        public:
            F f;
            task_shared_state(F const& f_):
                f(f_)
            {}
            task_shared_state(BOOST_THREAD_RV_REF(F) f_):
              f(boost::move(f_))
            {}

            F callable()
            {
              return boost::move(f);
            }

#if defined BOOST_THREAD_PROVIDES_SIGNATURE_PACKAGED_TASK && defined(BOOST_THREAD_PROVIDES_VARIADIC_THREAD)
            void do_apply(BOOST_THREAD_RV_REF(ArgTypes) ... args)
            {
                try
                {
                    this->set_value_at_thread_exit(f(boost::move(args)...));
                }
#else
            void do_apply()
            {
                try
                {
                    this->set_value_at_thread_exit(f());
                }
#endif
                catch(...)
                {
                    this->set_exception_at_thread_exit(current_exception());
                }
            }

#if defined BOOST_THREAD_PROVIDES_SIGNATURE_PACKAGED_TASK && defined(BOOST_THREAD_PROVIDES_VARIADIC_THREAD)
            void do_run(BOOST_THREAD_RV_REF(ArgTypes) ... args)
            {
                try
                {
                    this->mark_finished_with_result(f(boost::move(args)...));
                }
#else
            void do_run()
            {
                try
                {
#if ! defined  BOOST_NO_CXX11_RVALUE_REFERENCES
                  R res((f()));
                  this->mark_finished_with_result(boost::move(res));
#else
                  this->mark_finished_with_result(f());
#endif
                  }
#endif
                catch(...)
                {
                    this->mark_exceptional_finish();
                }
            }
        };

#if defined BOOST_THREAD_PROVIDES_SIGNATURE_PACKAGED_TASK
#if defined(BOOST_THREAD_PROVIDES_VARIADIC_THREAD)
        template<typename F, typename R, typename ...ArgTypes>
        struct task_shared_state<F, R&(ArgTypes...)>:
          task_base_shared_state<R&(ArgTypes...)>
#else
        template<typename F, typename R>
        struct task_shared_state<F, R&()>:
          task_base_shared_state<R&()>
#endif
#else
        template<typename F, typename R>
        struct task_shared_state<F,R&>:
            task_base_shared_state<R&>
#endif
        {
        private:
          task_shared_state(task_shared_state&);
        public:
            F f;
            task_shared_state(F const& f_):
                f(f_)
            {}
            task_shared_state(BOOST_THREAD_RV_REF(F) f_):
                f(boost::move(f_))
            {}

            F callable()
            {
              return f;
            }

#if defined BOOST_THREAD_PROVIDES_SIGNATURE_PACKAGED_TASK && defined(BOOST_THREAD_PROVIDES_VARIADIC_THREAD)
            void do_apply(BOOST_THREAD_RV_REF(ArgTypes) ... args)
            {
                try
                {
                    this->set_value_at_thread_exit(f(boost::move(args)...));
                }
#else
            void do_apply()
            {
                try
                {
                    this->set_value_at_thread_exit(f());
                }
#endif
                catch(...)
                {
                    this->set_exception_at_thread_exit(current_exception());
                }
            }

#if defined BOOST_THREAD_PROVIDES_SIGNATURE_PACKAGED_TASK && defined(BOOST_THREAD_PROVIDES_VARIADIC_THREAD)
            void do_run(BOOST_THREAD_RV_REF(ArgTypes) ... args)
            {
                try
                {
                    this->mark_finished_with_result(f(boost::move(args)...));
                }
#else
            void do_run()
            {
                try
                {
                  R& res((f()));
                  this->mark_finished_with_result(res);
                }
#endif
                catch(...)
                {
                    this->mark_exceptional_finish();
                }
            }
        };

#if defined(BOOST_THREAD_RVALUE_REFERENCES_DONT_MATCH_FUNCTION_PTR)

#if defined BOOST_THREAD_PROVIDES_SIGNATURE_PACKAGED_TASK
#if defined(BOOST_THREAD_PROVIDES_VARIADIC_THREAD)
        template<typename R, typename ...ArgTypes>
        struct task_shared_state<R (*)(ArgTypes...), R(ArgTypes...)>:
          task_base_shared_state<R(ArgTypes...)>
#else
        template<typename R>
        struct task_shared_state<R (*)(), R()>:
          task_base_shared_state<R()>
#endif
#else
        template<typename R>
        struct task_shared_state<R (*)(), R> :
           task_base_shared_state<R>
#endif
            {
            private:
              task_shared_state(task_shared_state&);
#if defined BOOST_THREAD_PROVIDES_SIGNATURE_PACKAGED_TASK && defined(BOOST_THREAD_PROVIDES_VARIADIC_THREAD)
              typedef R (*CallableType)(ArgTypes ... );
#else
              typedef R (*CallableType)();
#endif
            public:
                CallableType f;
                task_shared_state(CallableType f_):
                    f(f_)
                {}

                CallableType callable()
                {
                  return f;
                }

#if defined BOOST_THREAD_PROVIDES_SIGNATURE_PACKAGED_TASK && defined(BOOST_THREAD_PROVIDES_VARIADIC_THREAD)
                void do_apply(BOOST_THREAD_RV_REF(ArgTypes) ... args)
                {
                    try
                    {
                        this->set_value_at_thread_exit(f(boost::move(args)...));
                    }
#else
                void do_apply()
                {
                    try
                    {
                        R r((f()));
                        this->set_value_at_thread_exit(boost::move(r));
                    }
#endif
                    catch(...)
                    {
                        this->set_exception_at_thread_exit(current_exception());
                    }
                }


#if defined BOOST_THREAD_PROVIDES_SIGNATURE_PACKAGED_TASK && defined(BOOST_THREAD_PROVIDES_VARIADIC_THREAD)
                void do_run(BOOST_THREAD_RV_REF(ArgTypes) ... args)
                {
                    try
                    {
                        this->mark_finished_with_result(f(boost::move(args)...));
                    }
#else
                void do_run()
                {
                    try
                    {
                        R res((f()));
                        this->mark_finished_with_result(boost::move(res));
                    }
#endif
                    catch(...)
                    {
                        this->mark_exceptional_finish();
                    }
                }
            };
#if defined BOOST_THREAD_PROVIDES_SIGNATURE_PACKAGED_TASK
#if defined(BOOST_THREAD_PROVIDES_VARIADIC_THREAD)
        template<typename R, typename ...ArgTypes>
        struct task_shared_state<R& (*)(ArgTypes...), R&(ArgTypes...)>:
          task_base_shared_state<R&(ArgTypes...)>
#else
        template<typename R>
        struct task_shared_state<R& (*)(), R&()>:
          task_base_shared_state<R&()>
#endif
#else
        template<typename R>
        struct task_shared_state<R& (*)(), R&> :
           task_base_shared_state<R&>
#endif
            {
            private:
              task_shared_state(task_shared_state&);
            public:
#if defined BOOST_THREAD_PROVIDES_SIGNATURE_PACKAGED_TASK && defined(BOOST_THREAD_PROVIDES_VARIADIC_THREAD)
                typedef R& (*CallableType)(BOOST_THREAD_RV_REF(ArgTypes) ... );
#else
                typedef R& (*CallableType)();
#endif
                CallableType f;
                task_shared_state(CallableType f_):
                    f(f_)
                {}

                CallableType callable()
                {
                  return boost::move(f);
                }

#if defined BOOST_THREAD_PROVIDES_SIGNATURE_PACKAGED_TASK && defined(BOOST_THREAD_PROVIDES_VARIADIC_THREAD)
                void do_apply(BOOST_THREAD_RV_REF(ArgTypes) ... args)
                {
                    try
                    {
                        this->set_value_at_thread_exit(f(boost::move(args)...));
                    }
#else
                void do_apply()
                {
                    try
                    {
                      this->set_value_at_thread_exit(f());
                    }
#endif
                    catch(...)
                    {
                        this->set_exception_at_thread_exit(current_exception());
                    }
                }


#if defined BOOST_THREAD_PROVIDES_SIGNATURE_PACKAGED_TASK && defined(BOOST_THREAD_PROVIDES_VARIADIC_THREAD)
                void do_run(BOOST_THREAD_RV_REF(ArgTypes) ... args)
                {
                    try
                    {
                        this->mark_finished_with_result(f(boost::move(args)...));
                    }
#else
                void do_run()
                {
                    try
                    {
                        this->mark_finished_with_result(f());
                    }
#endif
                    catch(...)
                    {
                        this->mark_exceptional_finish();
                    }
                }
            };
#endif
#if defined BOOST_THREAD_PROVIDES_SIGNATURE_PACKAGED_TASK
#if defined(BOOST_THREAD_PROVIDES_VARIADIC_THREAD)
        template<typename F, typename ...ArgTypes>
        struct task_shared_state<F, void(ArgTypes...)>:
          task_base_shared_state<void(ArgTypes...)>
#else
        template<typename F>
        struct task_shared_state<F, void()>:
          task_base_shared_state<void()>
#endif
#else
        template<typename F>
        struct task_shared_state<F,void>:
          task_base_shared_state<void>
#endif
        {
        private:
          task_shared_state(task_shared_state&);
        public:
            typedef F CallableType;
            F f;
            task_shared_state(F const& f_):
                f(f_)
            {}
            task_shared_state(BOOST_THREAD_RV_REF(F) f_):
                f(boost::move(f_))
            {}
            F callable()
            {
              return boost::move(f);
            }
#if defined BOOST_THREAD_PROVIDES_SIGNATURE_PACKAGED_TASK && defined(BOOST_THREAD_PROVIDES_VARIADIC_THREAD)
            void do_apply(BOOST_THREAD_RV_REF(ArgTypes) ... args)
            {
              try
              {
                f(boost::move(args)...);
#else
            void do_apply()
            {
                try
                {
                    f();
#endif
                  this->set_value_at_thread_exit();
                }
                catch(...)
                {
                    this->set_exception_at_thread_exit(current_exception());
                }
            }

#if defined BOOST_THREAD_PROVIDES_SIGNATURE_PACKAGED_TASK && defined(BOOST_THREAD_PROVIDES_VARIADIC_THREAD)
            void do_run(BOOST_THREAD_RV_REF(ArgTypes) ... args)
            {
                try
                {
                    f(boost::move(args)...);
#else
            void do_run()
            {
                try
                {
                    f();
#endif
                    this->mark_finished_with_result();
                }
                catch(...)
                {
                    this->mark_exceptional_finish();
                }
            }
        };

#if defined BOOST_THREAD_PROVIDES_SIGNATURE_PACKAGED_TASK
#if defined(BOOST_THREAD_PROVIDES_VARIADIC_THREAD)
        template<typename ...ArgTypes>
        struct task_shared_state<void (*)(ArgTypes...), void(ArgTypes...)>:
        task_base_shared_state<void(ArgTypes...)>
#else
        template<>
        struct task_shared_state<void (*)(), void()>:
        task_base_shared_state<void()>
#endif
#else
        template<>
        struct task_shared_state<void (*)(),void>:
          task_base_shared_state<void>
#endif
        {
        private:
          task_shared_state(task_shared_state&);
#if defined BOOST_THREAD_PROVIDES_SIGNATURE_PACKAGED_TASK && defined(BOOST_THREAD_PROVIDES_VARIADIC_THREAD)
            typedef void (*CallableType)(ArgTypes...);
#else
            typedef void (*CallableType)();
#endif
        public:
            CallableType f;
            task_shared_state(CallableType f_):
                f(f_)
            {}
            CallableType callable()
            {
              return f;
            }
#if defined BOOST_THREAD_PROVIDES_SIGNATURE_PACKAGED_TASK && defined(BOOST_THREAD_PROVIDES_VARIADIC_THREAD)
            void do_apply(BOOST_THREAD_RV_REF(ArgTypes) ... args)
            {
                try
                {
                    f(boost::move(args)...);
#else
            void do_apply()
            {
                try
                {
                    f();
#endif
                    this->set_value_at_thread_exit();
                }
                catch(...)
                {
                    this->set_exception_at_thread_exit(current_exception());
                }
            }

#if defined BOOST_THREAD_PROVIDES_SIGNATURE_PACKAGED_TASK && defined(BOOST_THREAD_PROVIDES_VARIADIC_THREAD)
            void do_run(BOOST_THREAD_RV_REF(ArgTypes) ... args)
            {
                try
                {
                    f(boost::move(args)...);
#else
            void do_run()
            {
                try
                {
                  f();
#endif
                  this->mark_finished_with_result();
                }
                catch(...)
                {
                    this->mark_exceptional_finish();
                }
            }
        };
    }

#if defined BOOST_THREAD_PROVIDES_SIGNATURE_PACKAGED_TASK
  #if defined(BOOST_THREAD_PROVIDES_VARIADIC_THREAD)
    template<typename R, typename ...ArgTypes>
    class packaged_task<R(ArgTypes...)>
    {
      typedef boost::shared_ptr<detail::task_base_shared_state<R(ArgTypes...)> > task_ptr;
      boost::shared_ptr<detail::task_base_shared_state<R(ArgTypes...)> > task;
  #else
    template<typename R>
    class packaged_task<R()>
    {
      typedef boost::shared_ptr<detail::task_base_shared_state<R()> > task_ptr;
      boost::shared_ptr<detail::task_base_shared_state<R()> > task;
  #endif
#else
    template<typename R>
    class packaged_task
    {
      typedef boost::shared_ptr<detail::task_base_shared_state<R> > task_ptr;
      boost::shared_ptr<detail::task_base_shared_state<R> > task;
#endif
        bool future_obtained;
        struct dummy;

    public:
        typedef R result_type;
        BOOST_THREAD_MOVABLE_ONLY(packaged_task)

        packaged_task():
            future_obtained(false)
        {}

        // construction and destruction
#if defined(BOOST_THREAD_RVALUE_REFERENCES_DONT_MATCH_FUNCTION_PTR)

#if defined BOOST_THREAD_PROVIDES_SIGNATURE_PACKAGED_TASK
  #if defined(BOOST_THREAD_PROVIDES_VARIADIC_THREAD)
        explicit packaged_task(R(*f)(), BOOST_THREAD_FWD_REF(ArgTypes)... args)
        {
            typedef R(*FR)(BOOST_THREAD_FWD_REF(ArgTypes)...);
            typedef detail::task_shared_state<FR,R(ArgTypes...)> task_shared_state_type;
            task= task_ptr(new task_shared_state_type(f, boost::move(args)...));
            future_obtained=false;
        }
  #else
        explicit packaged_task(R(*f)())
        {
            typedef R(*FR)();
            typedef detail::task_shared_state<FR,R()> task_shared_state_type;
            task= task_ptr(new task_shared_state_type(f));
            future_obtained=false;
        }
  #endif
#else
        explicit packaged_task(R(*f)())
        {
              typedef R(*FR)();
            typedef detail::task_shared_state<FR,R> task_shared_state_type;
            task= task_ptr(new task_shared_state_type(f));
            future_obtained=false;
        }
#endif
#endif
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
        template <class F>
        explicit packaged_task(BOOST_THREAD_FWD_REF(F) f
            , typename boost::disable_if<is_same<typename decay<F>::type, packaged_task>, dummy* >::type=0
            )
        {
          typedef typename decay<F>::type FR;
#if defined BOOST_THREAD_PROVIDES_SIGNATURE_PACKAGED_TASK
  #if defined(BOOST_THREAD_PROVIDES_VARIADIC_THREAD)
            typedef detail::task_shared_state<FR,R(ArgTypes...)> task_shared_state_type;
  #else
            typedef detail::task_shared_state<FR,R()> task_shared_state_type;
  #endif
#else
            typedef detail::task_shared_state<FR,R> task_shared_state_type;
#endif
            task = task_ptr(new task_shared_state_type(boost::forward<F>(f)));
            future_obtained = false;

        }

#else
        template <class F>
        explicit packaged_task(F const& f
            , typename boost::disable_if<is_same<typename decay<F>::type, packaged_task>, dummy* >::type=0
            )
        {
#if defined BOOST_THREAD_PROVIDES_SIGNATURE_PACKAGED_TASK
  #if defined(BOOST_THREAD_PROVIDES_VARIADIC_THREAD)
            typedef detail::task_shared_state<F,R(ArgTypes...)> task_shared_state_type;
  #else
            typedef detail::task_shared_state<F,R()> task_shared_state_type;
  #endif
#else
            typedef detail::task_shared_state<F,R> task_shared_state_type;
#endif
            task = task_ptr(new task_shared_state_type(f));
            future_obtained=false;
        }
        template <class F>
        explicit packaged_task(BOOST_THREAD_RV_REF(F) f)
        {
#if defined BOOST_THREAD_PROVIDES_SIGNATURE_PACKAGED_TASK
#if defined(BOOST_THREAD_PROVIDES_VARIADIC_THREAD)
            typedef detail::task_shared_state<F,R(ArgTypes...)> task_shared_state_type;
            task = task_ptr(new task_shared_state_type(boost::move(f)));
#else
            typedef detail::task_shared_state<F,R()> task_shared_state_type;
            task = task_ptr(new task_shared_state_type(boost::move(f)));
#endif
#else
            typedef detail::task_shared_state<F,R> task_shared_state_type;
            task = task_ptr(new task_shared_state_type(boost::move(f)));
#endif
            future_obtained=false;

        }
#endif

#if defined BOOST_THREAD_PROVIDES_FUTURE_CTOR_ALLOCATORS
#if defined(BOOST_THREAD_RVALUE_REFERENCES_DONT_MATCH_FUNCTION_PTR)
        template <class Allocator>
        packaged_task(boost::allocator_arg_t, Allocator a, R(*f)())
        {
          typedef R(*FR)();
#if defined BOOST_THREAD_PROVIDES_SIGNATURE_PACKAGED_TASK
  #if defined(BOOST_THREAD_PROVIDES_VARIADIC_THREAD)
          typedef detail::task_shared_state<FR,R(ArgTypes...)> task_shared_state_type;
  #else
          typedef detail::task_shared_state<FR,R()> task_shared_state_type;
  #endif
#else
          typedef detail::task_shared_state<FR,R> task_shared_state_type;
#endif
          typedef typename Allocator::template rebind<task_shared_state_type>::other A2;
          A2 a2(a);
          typedef thread_detail::allocator_destructor<A2> D;

          task = task_ptr(::new(a2.allocate(1)) task_shared_state_type(f), D(a2, 1) );
          future_obtained = false;
        }
#endif // BOOST_THREAD_RVALUE_REFERENCES_DONT_MATCH_FUNCTION_PTR

#if ! defined BOOST_NO_CXX11_RVALUE_REFERENCES
        template <class F, class Allocator>
        packaged_task(boost::allocator_arg_t, Allocator a, BOOST_THREAD_FWD_REF(F) f)
        {
          typedef typename decay<F>::type FR;

#if defined BOOST_THREAD_PROVIDES_SIGNATURE_PACKAGED_TASK
  #if defined(BOOST_THREAD_PROVIDES_VARIADIC_THREAD)
          typedef detail::task_shared_state<FR,R(ArgTypes...)> task_shared_state_type;
  #else
          typedef detail::task_shared_state<FR,R()> task_shared_state_type;
  #endif
#else
          typedef detail::task_shared_state<FR,R> task_shared_state_type;
#endif
          typedef typename Allocator::template rebind<task_shared_state_type>::other A2;
          A2 a2(a);
          typedef thread_detail::allocator_destructor<A2> D;

          task = task_ptr(::new(a2.allocate(1)) task_shared_state_type(boost::forward<F>(f)), D(a2, 1) );
          future_obtained = false;
        }
#else // ! defined BOOST_NO_CXX11_RVALUE_REFERENCES
        template <class F, class Allocator>
        packaged_task(boost::allocator_arg_t, Allocator a, const F& f)
        {
#if defined BOOST_THREAD_PROVIDES_SIGNATURE_PACKAGED_TASK
  #if defined(BOOST_THREAD_PROVIDES_VARIADIC_THREAD)
          typedef detail::task_shared_state<F,R(ArgTypes...)> task_shared_state_type;
  #else
          typedef detail::task_shared_state<F,R()> task_shared_state_type;
  #endif
#else
          typedef detail::task_shared_state<F,R> task_shared_state_type;
#endif
          typedef typename Allocator::template rebind<task_shared_state_type>::other A2;
          A2 a2(a);
          typedef thread_detail::allocator_destructor<A2> D;

          task = task_ptr(::new(a2.allocate(1)) task_shared_state_type(f), D(a2, 1) );
          future_obtained = false;
        }
        template <class F, class Allocator>
        packaged_task(boost::allocator_arg_t, Allocator a, BOOST_THREAD_RV_REF(F) f)
        {
#if defined BOOST_THREAD_PROVIDES_SIGNATURE_PACKAGED_TASK
  #if defined(BOOST_THREAD_PROVIDES_VARIADIC_THREAD)
          typedef detail::task_shared_state<F,R(ArgTypes...)> task_shared_state_type;
  #else
          typedef detail::task_shared_state<F,R()> task_shared_state_type;
  #endif
#else
          typedef detail::task_shared_state<F,R> task_shared_state_type;
#endif
          typedef typename Allocator::template rebind<task_shared_state_type>::other A2;
          A2 a2(a);
          typedef thread_detail::allocator_destructor<A2> D;

          task = task_ptr(::new(a2.allocate(1)) task_shared_state_type(boost::move(f)), D(a2, 1) );
          future_obtained = false;
        }

#endif //BOOST_NO_CXX11_RVALUE_REFERENCES
#endif // BOOST_THREAD_PROVIDES_FUTURE_CTOR_ALLOCATORS

        ~packaged_task() {
            if(task) {
                task->owner_destroyed();
            }
        }

        // assignment
        packaged_task(BOOST_THREAD_RV_REF(packaged_task) other) BOOST_NOEXCEPT
        : future_obtained(BOOST_THREAD_RV(other).future_obtained) {
            task.swap(BOOST_THREAD_RV(other).task);
            BOOST_THREAD_RV(other).future_obtained=false;
        }
        packaged_task& operator=(BOOST_THREAD_RV_REF(packaged_task) other) BOOST_NOEXCEPT {

#if ! defined  BOOST_NO_CXX11_RVALUE_REFERENCES
            packaged_task temp(boost::move(other));
#else
            packaged_task temp(static_cast<BOOST_THREAD_RV_REF(packaged_task)>(other));
#endif
            swap(temp);
            return *this;
        }

#ifdef BOOST_THREAD_PROVIDES_EXECUTORS
        void set_executor(executor_ptr_type aex)
        {
          if (!valid())
              boost::throw_exception(task_moved());
          boost::lock_guard<boost::mutex> lk(task->mutex);
          task->set_executor_policy(aex, lk);
        }
#endif
        void reset() {
            if (!valid())
              boost::throw_exception(future_error(system::make_error_code(future_errc::no_state)));

            // As if *this = packaged_task(task->callable());

            task->reset();
            future_obtained=false;
        }

        void swap(packaged_task& other) BOOST_NOEXCEPT {
            task.swap(other.task);
            std::swap(future_obtained,other.future_obtained);
        }
        bool valid() const BOOST_NOEXCEPT {
          return task.get()!=0;
        }

        // result retrieval
        BOOST_THREAD_FUTURE<R> get_future() {
            if(!task) {
                boost::throw_exception(task_moved());
            } else if(!future_obtained) {
                future_obtained=true;
                return BOOST_THREAD_FUTURE<R>(task);
            } else {
                boost::throw_exception(future_already_retrieved());
            }
        }

        // execution
#if defined BOOST_THREAD_PROVIDES_SIGNATURE_PACKAGED_TASK && defined(BOOST_THREAD_PROVIDES_VARIADIC_THREAD)
        void operator()(ArgTypes... args) {
            if(!task) {
                boost::throw_exception(task_moved());
            }
            task->run(boost::move(args)...);
        }
        void make_ready_at_thread_exit(ArgTypes... args) {
          if(!task) {
              boost::throw_exception(task_moved());
          }
          if (task->has_value()) {
                boost::throw_exception(promise_already_satisfied());
          }
          task->apply(boost::move(args)...);
        }
#else
        void operator()() {
            if(!task) {
                boost::throw_exception(task_moved());
            }
            task->run();
        }
        void make_ready_at_thread_exit() {
          if(!task) {
              boost::throw_exception(task_moved());
          }
          if (task->has_value()) boost::throw_exception(promise_already_satisfied());
          task->apply();
        }
#endif
        template<typename F>
        void set_wait_callback(F f) {
            task->set_wait_callback(f,this);
        }
    };
}
#if defined BOOST_THREAD_PROVIDES_FUTURE_CTOR_ALLOCATORS
namespace boost { namespace container {
    template <class R, class Alloc>
    struct uses_allocator< ::boost::packaged_task<R> , Alloc> : true_type
    {};
}}
#if ! defined  BOOST_NO_CXX11_ALLOCATOR
namespace std {
    template <class R, class Alloc>
    struct uses_allocator< ::boost::packaged_task<R> , Alloc> : true_type
    {};
}
#endif
#endif

namespace boost
{
  BOOST_THREAD_DCL_MOVABLE_BEG(T) packaged_task<T> BOOST_THREAD_DCL_MOVABLE_END

namespace detail
{
  ////////////////////////////////
  // make_future_deferred_shared_state
  ////////////////////////////////
  template <class Rp, class Fp>
  BOOST_THREAD_FUTURE<Rp>
  make_future_deferred_shared_state(BOOST_THREAD_FWD_REF(Fp) f) {
    shared_ptr<future_deferred_shared_state<Rp, Fp> >
        h(new future_deferred_shared_state<Rp, Fp>(boost::forward<Fp>(f)));
    return BOOST_THREAD_FUTURE<Rp>(h);
  }

  ////////////////////////////////
  // make_future_async_shared_state
  ////////////////////////////////
  template <class Rp, class Fp>
  BOOST_THREAD_FUTURE<Rp>
  make_future_async_shared_state(BOOST_THREAD_FWD_REF(Fp) f) {
    shared_ptr<future_async_shared_state<Rp, Fp> >
        h(new future_async_shared_state<Rp, Fp>());
    h->init(boost::forward<Fp>(f));
    return BOOST_THREAD_FUTURE<Rp>(h);
  }
}

    ////////////////////////////////
    // template <class F, class... ArgTypes>
    // future<R> async(launch policy, F&&, ArgTypes&&...);
    ////////////////////////////////

#if defined BOOST_THREAD_RVALUE_REFERENCES_DONT_MATCH_FUNCTION_PTR

#if defined(BOOST_THREAD_PROVIDES_VARIADIC_THREAD)
  template <class R, class... ArgTypes>
  BOOST_THREAD_FUTURE<R>
  async(launch policy, R(*f)(BOOST_THREAD_FWD_REF(ArgTypes)...), BOOST_THREAD_FWD_REF(ArgTypes)... args) {
    typedef R(*F)(BOOST_THREAD_FWD_REF(ArgTypes)...);
    typedef detail::invoker<typename decay<F>::type, typename decay<ArgTypes>::type...> BF;
    typedef typename BF::result_type Rp;

    if (underlying_cast<int>(policy) & int(launch::async)) {
      return BOOST_THREAD_MAKE_RV_REF(boost::detail::make_future_async_shared_state<Rp>(
              BF(
                  f
                  , thread_detail::decay_copy(boost::forward<ArgTypes>(args))...
              )
          ));
    } else if (underlying_cast<int>(policy) & int(launch::deferred)) {
      return BOOST_THREAD_MAKE_RV_REF(boost::detail::make_future_deferred_shared_state<Rp>(
              BF(
                  f
                  , thread_detail::decay_copy(boost::forward<ArgTypes>(args))...
              )
          ));
    } else {
      std::terminate();
      //BOOST_THREAD_FUTURE<R> ret;
      //return ::boost::move(ret);
    }
  }

#else // defined(BOOST_THREAD_PROVIDES_VARIADIC_THREAD)

  template <class R>
  BOOST_THREAD_FUTURE<R>
  async(launch policy, R(*f)()) {
  #if defined BOOST_THREAD_PROVIDES_SIGNATURE_PACKAGED_TASK
    typedef packaged_task<R()> packaged_task_type;
  #else
    typedef packaged_task<R> packaged_task_type;
  #endif

    if (underlying_cast<int>(policy) & int(launch::async)) {
      packaged_task_type pt( f );
      BOOST_THREAD_FUTURE<R> ret = BOOST_THREAD_MAKE_RV_REF(pt.get_future());
      ret.set_async();
      boost::thread( boost::move(pt) ).detach();
      return ::boost::move(ret);
    } else if (underlying_cast<int>(policy) & int(launch::deferred)) {
      std::terminate();
      //BOOST_THREAD_FUTURE<R> ret;
      //return ::boost::move(ret);
    } else {
      std::terminate();
      //BOOST_THREAD_FUTURE<R> ret;
      //return ::boost::move(ret);
    }
  }
#endif
#endif // defined(BOOST_THREAD_RVALUE_REFERENCES_DONT_MATCH_FUNCTION_PTR)

#if defined(BOOST_THREAD_PROVIDES_VARIADIC_THREAD)

  template <class F, class ...ArgTypes>
  BOOST_THREAD_FUTURE<typename boost::result_of<typename decay<F>::type(
      typename decay<ArgTypes>::type...
  )>::type>
  async(launch policy, BOOST_THREAD_FWD_REF(F) f, BOOST_THREAD_FWD_REF(ArgTypes)... args) {
    typedef detail::invoker<typename decay<F>::type, typename decay<ArgTypes>::type...> BF;
    typedef typename BF::result_type Rp;

    if (underlying_cast<int>(policy) & int(launch::async)) {
      return BOOST_THREAD_MAKE_RV_REF(boost::detail::make_future_async_shared_state<Rp>(
              BF(
                  thread_detail::decay_copy(boost::forward<F>(f))
                , thread_detail::decay_copy(boost::forward<ArgTypes>(args))...
              )
          ));
    } else if (underlying_cast<int>(policy) & int(launch::deferred)) {
      return BOOST_THREAD_MAKE_RV_REF(boost::detail::make_future_deferred_shared_state<Rp>(
              BF(
                  thread_detail::decay_copy(boost::forward<F>(f))
                , thread_detail::decay_copy(boost::forward<ArgTypes>(args))...
              )
          ));
    } else {
      std::terminate();
      //BOOST_THREAD_FUTURE<R> ret;
      //return ::boost::move(ret);
    }
  }

#else // defined(BOOST_THREAD_PROVIDES_VARIADIC_THREAD)

  template <class F>
  BOOST_THREAD_FUTURE<typename boost::result_of<typename decay<F>::type()>::type>
  async(launch policy, BOOST_THREAD_FWD_REF(F) f) {
    typedef typename boost::result_of<typename decay<F>::type()>::type R;
#if defined BOOST_THREAD_PROVIDES_SIGNATURE_PACKAGED_TASK
    typedef packaged_task<R()> packaged_task_type;
#else // defined BOOST_THREAD_PROVIDES_SIGNATURE_PACKAGED_TASK
    typedef packaged_task<R> packaged_task_type;
#endif // defined BOOST_THREAD_PROVIDES_SIGNATURE_PACKAGED_TASK

    if (underlying_cast<int>(policy) & int(launch::async)) {
      packaged_task_type pt( boost::forward<F>(f) );
      BOOST_THREAD_FUTURE<R> ret = pt.get_future();
      ret.set_async();
      boost::thread( boost::move(pt) ).detach();
      return ::boost::move(ret);
    } else if (underlying_cast<int>(policy) & int(launch::deferred)) {
      std::terminate();
      //BOOST_THREAD_FUTURE<R> ret;
      //return ::boost::move(ret);
      //          return boost::detail::make_future_deferred_shared_state<Rp>(
      //              BF(
      //                  thread_detail::decay_copy(boost::forward<F>(f))
      //              )
      //          );
    } else {
      std::terminate();
      //BOOST_THREAD_FUTURE<R> ret;
      //return ::boost::move(ret);
    }
  }
#endif // defined(BOOST_THREAD_PROVIDES_VARIADIC_THREAD)

#ifdef BOOST_THREAD_PROVIDES_EXECUTORS
namespace detail {

    /////////////////////////
    /// shared_state_nullary_task
    /////////////////////////
    template<typename Rp, typename Fp>
    struct shared_state_nullary_task
    {

      typedef shared_ptr<shared_state_base > storage_type;
      storage_type that;
      Fp f_;
    public:

      shared_state_nullary_task(storage_type st, BOOST_THREAD_FWD_REF(Fp) f)
      : that(st), f_(boost::move(f))
      {};

#if ! defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
      BOOST_THREAD_COPYABLE_AND_MOVABLE(shared_state_nullary_task)
      shared_state_nullary_task(shared_state_nullary_task const& x) //BOOST_NOEXCEPT
      : that(x.that), f_(x.f_)
      {}
      shared_state_nullary_task& operator=(BOOST_THREAD_COPY_ASSIGN_REF(shared_state_nullary_task) x) //BOOST_NOEXCEPT
      {
        if (this != &x) {
          that=x.that;
          f_=x.f_;
        }
        return *this;
      }
      // move
      shared_state_nullary_task(BOOST_THREAD_RV_REF(shared_state_nullary_task) x) //BOOST_NOEXCEPT
      : that(x.that), f_(boost::move(x.f_))
      {
        x.that.reset();
      }
      shared_state_nullary_task& operator=(BOOST_THREAD_RV_REF(shared_state_nullary_task) x) //BOOST_NOEXCEPT
      {
        if (this != &x) {
          that=x.that;
          f_=boost::move(x.f_);
          x.that.reset();
        }
        return *this;
      }
#endif
      void operator()() {
        shared_ptr<shared_state<Rp> > that_ = static_pointer_cast<shared_state<Rp> >(that);
        try {
          that_->mark_finished_with_result(f_());
        } catch(...) {
          that_->mark_exceptional_finish();
        }
      }
      ~shared_state_nullary_task()
      {
      }
    };

    template<typename Fp>
    struct shared_state_nullary_task<void, Fp>
    {
      typedef shared_ptr<shared_state_base > storage_type;
      storage_type that;
      Fp f_;
    public:
      shared_state_nullary_task(storage_type st, BOOST_THREAD_FWD_REF(Fp) f)
      : that(st), f_(boost::move(f))
      {};

#if ! defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
      BOOST_THREAD_COPYABLE_AND_MOVABLE(shared_state_nullary_task)
      shared_state_nullary_task(shared_state_nullary_task const& x) //BOOST_NOEXCEPT
      : that(x.that), f_(x.f_)
      {}
      shared_state_nullary_task& operator=(BOOST_THREAD_COPY_ASSIGN_REF(shared_state_nullary_task) x) //BOOST_NOEXCEPT
      {
        if (this != &x) {
          that=x.that;
          f_=x.f_;
        }
        return *this;
      }
      // move
      shared_state_nullary_task(BOOST_THREAD_RV_REF(shared_state_nullary_task) x) BOOST_NOEXCEPT
      : that(x.that), f_(boost::move(x.f_))
      {
        x.that.reset();
      }
      shared_state_nullary_task& operator=(BOOST_THREAD_RV_REF(shared_state_nullary_task) x) BOOST_NOEXCEPT {
        if (this != &x) {
          that=x.that;
          f_=boost::move(x.f_);
          x.that.reset();
        }
        return *this;
      }
#endif
      void operator()() {
        shared_ptr<shared_state<void> > that_ = static_pointer_cast<shared_state<void> >(that);
        try {
          f_();
          that_->mark_finished_with_result();
        } catch(...) {
          that_->mark_exceptional_finish();
        }
      }
    };

}
    BOOST_THREAD_DCL_MOVABLE_BEG2(R,F) detail::shared_state_nullary_task<R,F> BOOST_THREAD_DCL_MOVABLE_END
namespace detail {

    /////////////////////////
    /// future_executor_shared_state_base
    /////////////////////////
    template<typename Rp>
    struct future_executor_shared_state: shared_state<Rp>
    {
      typedef shared_state<Rp> base_type;
    protected:
    public:
      future_executor_shared_state() {
      }

      template <class Fp, class Executor>
      void init(Executor& ex, BOOST_THREAD_FWD_REF(Fp) f)
      {
        typedef typename decay<Fp>::type Cont;
        this->set_executor_policy(executor_ptr_type(new executor_ref<Executor>(ex)));
        shared_state_nullary_task<Rp,Cont> t(this->shared_from_this(), boost::forward<Fp>(f));
        ex.submit(boost::move(t));
      }

      ~future_executor_shared_state() {}
    };

    ////////////////////////////////
    // make_future_executor_shared_state
    ////////////////////////////////
    template <class Rp, class Fp, class Executor>
    BOOST_THREAD_FUTURE<Rp>
    make_future_executor_shared_state(Executor& ex, BOOST_THREAD_FWD_REF(Fp) f) {
      shared_ptr<future_executor_shared_state<Rp> >
          h(new future_executor_shared_state<Rp>());
      h->init(ex, boost::forward<Fp>(f));
      return BOOST_THREAD_FUTURE<Rp>(h);
    }

} // detail

    ////////////////////////////////
    // template <class Executor, class F, class... ArgTypes>
    // future<R> async(Executor& ex, F&&, ArgTypes&&...);
    ////////////////////////////////

//#if ! defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)
#if defined(BOOST_THREAD_PROVIDES_INVOKE) && ! defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES) && ! defined(BOOST_NO_CXX11_HDR_TUPLE)

#if defined BOOST_THREAD_RVALUE_REFERENCES_DONT_MATCH_FUNCTION_PTR

  template <class Executor, class R, class... ArgTypes>
  BOOST_THREAD_FUTURE<R>
  async(Executor& ex, R(*f)(BOOST_THREAD_FWD_REF(ArgTypes)...), BOOST_THREAD_FWD_REF(ArgTypes)... args) {
    typedef R(*F)(BOOST_THREAD_FWD_REF(ArgTypes)...);
    typedef detail::invoker<typename decay<F>::type, typename decay<ArgTypes>::type...> BF;
    typedef typename BF::result_type Rp;

    return BOOST_THREAD_MAKE_RV_REF(boost::detail::make_future_executor_shared_state<Rp>(ex,
        BF(
            f
            , thread_detail::decay_copy(boost::forward<ArgTypes>(args))...
        )
    ));
  }
#endif // defined BOOST_THREAD_RVALUE_REFERENCES_DONT_MATCH_FUNCTION_PTR

  template <class Executor, class F, class ...ArgTypes>
  BOOST_THREAD_FUTURE<typename boost::result_of<typename decay<F>::type(
      typename decay<ArgTypes>::type...
  )>::type>
  async(Executor& ex, BOOST_THREAD_FWD_REF(F) f, BOOST_THREAD_FWD_REF(ArgTypes)... args) {
    typedef detail::invoker<typename decay<F>::type, typename decay<ArgTypes>::type...> BF;
    typedef typename BF::result_type Rp;

    return BOOST_THREAD_MAKE_RV_REF(boost::detail::make_future_executor_shared_state<Rp>(ex,
        BF(
            thread_detail::decay_copy(boost::forward<F>(f))
            , thread_detail::decay_copy(boost::forward<ArgTypes>(args))...
        )
    ));
  }

#else // ! defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)
#if defined BOOST_THREAD_RVALUE_REFERENCES_DONT_MATCH_FUNCTION_PTR

  template <class Executor, class R>
  BOOST_THREAD_FUTURE<R>
  async(Executor& ex, R(*f)()) {
    typedef R(*F)();
    typedef detail::invoker<F> BF;
    typedef typename BF::result_type Rp;

    return BOOST_THREAD_MAKE_RV_REF(boost::detail::make_future_executor_shared_state<Rp>(ex,
        BF(
            f
        )
    ));
  }

  template <class Executor, class R, class A1>
  BOOST_THREAD_FUTURE<R>
  async(Executor& ex, R(*f)(BOOST_THREAD_FWD_REF(A1)), BOOST_THREAD_FWD_REF(A1) a1) {
    typedef R(*F)(BOOST_THREAD_FWD_REF(A1));
    typedef detail::invoker<F, typename decay<A1>::type> BF;
    typedef typename BF::result_type Rp;

    return BOOST_THREAD_MAKE_RV_REF(boost::detail::make_future_executor_shared_state<Rp>(ex,
        BF(
            f
            , thread_detail::decay_copy(boost::forward<A1>(a1))
        )
    ));
  }
#endif // defined BOOST_THREAD_RVALUE_REFERENCES_DONT_MATCH_FUNCTION_PTR

  template <class Executor, class F>
  BOOST_THREAD_FUTURE<typename boost::result_of<typename decay<F>::type()>::type>
  async(Executor& ex, BOOST_THREAD_FWD_REF(F) f)  {
    typedef detail::invoker<typename decay<F>::type> BF;
    typedef typename BF::result_type Rp;

    return boost::detail::make_future_executor_shared_state<Rp>(ex,
        BF(
            thread_detail::decay_copy(boost::forward<F>(f))
        )
    );
  }

  template <class Executor, class F, class A1>
  BOOST_THREAD_FUTURE<typename boost::result_of<typename decay<F>::type(
      typename decay<A1>::type
  )>::type>
  async(Executor& ex, BOOST_THREAD_FWD_REF(F) f, BOOST_THREAD_FWD_REF(A1) a1) {
    typedef detail::invoker<typename decay<F>::type, typename decay<A1>::type> BF;
    typedef typename BF::result_type Rp;

    return BOOST_THREAD_MAKE_RV_REF(boost::detail::make_future_executor_shared_state<Rp>(ex,
        BF(
            thread_detail::decay_copy(boost::forward<F>(f))
          , thread_detail::decay_copy(boost::forward<A1>(a1))
        )
    ));
  }

  template <class Executor, class F, class A1, class A2>
  BOOST_THREAD_FUTURE<typename boost::result_of<typename decay<F>::type(
      typename decay<A1>::type, typename decay<A2>::type
  )>::type>
  async(Executor& ex, BOOST_THREAD_FWD_REF(F) f, BOOST_THREAD_FWD_REF(A1) a1, BOOST_THREAD_FWD_REF(A2) a2) {
    typedef detail::invoker<typename decay<F>::type, typename decay<A1>::type, typename decay<A2>::type> BF;
    typedef typename BF::result_type Rp;

    return BOOST_THREAD_MAKE_RV_REF(boost::detail::make_future_executor_shared_state<Rp>(ex,
        BF(
            thread_detail::decay_copy(boost::forward<F>(f))
          , thread_detail::decay_copy(boost::forward<A1>(a1))
          , thread_detail::decay_copy(boost::forward<A2>(a2))
        )
    ));
  }

#endif //! defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)
#endif

  ////////////////////////////////
  // template <class F, class... ArgTypes>
  // future<R> async(F&&, ArgTypes&&...);
  ////////////////////////////////

#if defined BOOST_THREAD_RVALUE_REFERENCES_DONT_MATCH_FUNCTION_PTR
  #if defined(BOOST_THREAD_PROVIDES_VARIADIC_THREAD)
  template <class R, class... ArgTypes>
  BOOST_THREAD_FUTURE<R>
  async(R(*f)(BOOST_THREAD_FWD_REF(ArgTypes)...), BOOST_THREAD_FWD_REF(ArgTypes)... args) {
    return BOOST_THREAD_MAKE_RV_REF(async(launch(launch::any), f, boost::forward<ArgTypes>(args)...));
  }
  #else
  template <class R>
  BOOST_THREAD_FUTURE<R>
  async(R(*f)()) {
    return BOOST_THREAD_MAKE_RV_REF(async(launch(launch::any), f));
  }
  #endif
#endif

#if defined(BOOST_THREAD_PROVIDES_VARIADIC_THREAD)
  template <class F, class ...ArgTypes>
  BOOST_THREAD_FUTURE<typename boost::result_of<typename decay<F>::type(
      typename decay<ArgTypes>::type...
  )>::type>
  async(BOOST_THREAD_FWD_REF(F) f, BOOST_THREAD_FWD_REF(ArgTypes)... args) {
      return BOOST_THREAD_MAKE_RV_REF(async(launch(launch::any), boost::forward<F>(f), boost::forward<ArgTypes>(args)...));
  }
#else
  template <class F>
  BOOST_THREAD_FUTURE<typename boost::result_of<F()>::type>
  async(BOOST_THREAD_FWD_REF(F) f) {
      return BOOST_THREAD_MAKE_RV_REF(async(launch(launch::any), boost::forward<F>(f)));
  }
#endif

  ////////////////////////////////
  // make_future deprecated
  ////////////////////////////////
  template <typename T>
  BOOST_THREAD_FUTURE<typename decay<T>::type> make_future(BOOST_THREAD_FWD_REF(T) value) {
    typedef typename decay<T>::type future_value_type;
    promise<future_value_type> p;
    p.set_value(boost::forward<future_value_type>(value));
    return BOOST_THREAD_MAKE_RV_REF(p.get_future());
  }

#if defined BOOST_THREAD_USES_MOVE
  inline BOOST_THREAD_FUTURE<void> make_future() {
    promise<void> p;
    p.set_value();
    return BOOST_THREAD_MAKE_RV_REF(p.get_future());
  }
#endif

  ////////////////////////////////
  // make_ready_future
  ////////////////////////////////
  namespace detail {
    template <class T>
    struct deduced_type_impl
    {
        typedef T type;
    };

    template <class T>
    struct deduced_type_impl<reference_wrapper<T> const>
    {
        typedef T& type;
    };
    template <class T>
    struct deduced_type_impl<reference_wrapper<T> >
    {
        typedef T& type;
    };
#if __cplusplus > 201103L
    template <class T>
    struct deduced_type_impl<std::reference_wrapper<T> >
    {
        typedef T& type;
    };
#endif
    template <class T>
    struct deduced_type
    {
        typedef typename detail::deduced_type_impl<typename decay<T>::type>::type type;
    };

  }


#if ! defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)
  template <int = 0, int..., class T>
#else
  template <class T>
#endif
  BOOST_THREAD_FUTURE<typename detail::deduced_type<T>::type> make_ready_future(BOOST_THREAD_FWD_REF(T) value) {
    typedef typename detail::deduced_type<T>::type future_value_type;
    promise<future_value_type> p;
    p.set_value(boost::forward<T>(value));
    return BOOST_THREAD_MAKE_RV_REF(p.get_future());
  }

  // explicit overloads
  template <class T>
  BOOST_THREAD_FUTURE<T> make_ready_future(typename remove_reference<T>::type & x)
  {
    promise<T> p;
    p.set_value(x);
    return p.get_future();
  }

  template <class T>
  BOOST_THREAD_FUTURE<T> make_ready_future(BOOST_THREAD_FWD_REF(typename remove_reference<T>::type) x)
  {
    promise<T> p;
    p.set_value(forward<typename remove_reference<T>::type>(x));
    return p.get_future();
  }

  // variadic overload
#if ! defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)
  template <class T, class ...Args>
  BOOST_THREAD_FUTURE<T> make_ready_future(Args&&... args)
  {
    promise<T> p;
    p.emplace(forward<Args>(args)...);
    return p.get_future();

  }
#endif

  template <typename T, typename T1>
  BOOST_THREAD_FUTURE<T> make_ready_no_decay_future(T1 value) {
    typedef T future_value_type;
    promise<future_value_type> p;
    p.set_value(value);
    return BOOST_THREAD_MAKE_RV_REF(p.get_future());
  }

#if ! defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES) || defined BOOST_THREAD_USES_MOVE
  inline BOOST_THREAD_FUTURE<void> make_ready_future() {
    promise<void> p;
    p.set_value();
    return p.get_future();
  }
#endif


  template <typename T>
  BOOST_THREAD_FUTURE<T> make_exceptional_future(exception_ptr ex) {
    promise<T> p;
    p.set_exception(ex);
    return BOOST_THREAD_MAKE_RV_REF(p.get_future());
  }

  template <typename T, typename E>
  BOOST_THREAD_FUTURE<T> make_exceptional_future(E ex) {
    promise<T> p;
    p.set_exception(boost::copy_exception(ex));
    return BOOST_THREAD_MAKE_RV_REF(p.get_future());
  }

  template <typename T>
  BOOST_THREAD_FUTURE<T> make_exceptional_future() {
    promise<T> p;
    p.set_exception(boost::current_exception());
    return BOOST_THREAD_MAKE_RV_REF(p.get_future());
  }
  template <typename T>
  BOOST_THREAD_FUTURE<T> make_ready_future(exception_ptr ex)  {
    return make_exceptional_future<T>(ex);
  }

#if 0
  template<typename CLOSURE>
  make_future(CLOSURE closure) -> BOOST_THREAD_FUTURE<decltype(closure())> {
      typedef decltype(closure()) T;
      promise<T> p;
      try {
        p.set_value(closure());
      } catch(...) {
        p.set_exception(std::current_exception());
      }
      return BOOST_THREAD_MAKE_RV_REF(p.get_future());
  }
#endif

  ////////////////////////////////
  // make_shared_future deprecated
  ////////////////////////////////
  template <typename T>
  shared_future<typename decay<T>::type> make_shared_future(BOOST_THREAD_FWD_REF(T) value) {
    typedef typename decay<T>::type future_type;
    promise<future_type> p;
    p.set_value(boost::forward<T>(value));
    return BOOST_THREAD_MAKE_RV_REF(p.get_future().share());
  }

  inline shared_future<void> make_shared_future()  {
    promise<void> p;
    return BOOST_THREAD_MAKE_RV_REF(p.get_future().share());
  }

  ////////////////////////////////
  // detail::future_async_continuation_shared_state
  ////////////////////////////////
#if defined BOOST_THREAD_PROVIDES_FUTURE_CONTINUATION

namespace detail
{
  //////////////////////
  // detail::continuation_shared_state
  //////////////////////
  template<typename F, typename Rp, typename Fp, class ShSt=shared_state<Rp> >
  struct continuation_shared_state: ShSt
  {
    F parent;
    Fp continuation;

  public:
    continuation_shared_state(BOOST_THREAD_RV_REF(F) f, BOOST_THREAD_FWD_REF(Fp) c)
    : parent(boost::move(f)),
      continuation(boost::move(c))
    {
    }

    void init(boost::unique_lock<boost::mutex> &lock)
    {
      parent.future_->set_continuation_ptr(this->shared_from_this(), lock);
    }

    void call() {
      try {
        this->mark_finished_with_result(this->continuation(boost::move(this->parent)));
      } catch(...) {
        this->mark_exceptional_finish();
      }
      // make sure parent is really cleared to prevent memory "leaks"
      this->parent = F();
    }

    void call(boost::unique_lock<boost::mutex>& lck) {
      try {
        relocker relock(lck);

        // neither continuation nor parent are protected by the lock - call() must only
        // be called once, and no one else must modify it.
        Rp res = this->continuation(boost::move(this->parent));

        // make sure parent is really cleared to prevent memory "leaks"
        this->parent = F();

        relock.lock();

        this->mark_finished_with_result_internal(boost::move(res), lck);
      } catch (...) {
        this->mark_exceptional_finish_internal(current_exception(), lck);

        // make sure parent is really cleared to prevent memory "leaks"
        relocker relock(lck);
        this->parent = F();
      }
    }

    static void run(shared_ptr<boost::detail::shared_state_base> that_)
    {
      continuation_shared_state* that = static_cast<continuation_shared_state*>(that_.get());
      that->call();
    }

    ~continuation_shared_state() {}
  };

  template<typename F, typename Fp, class ShSt>
  struct continuation_shared_state<F, void, Fp, ShSt>: ShSt
  {
    F parent;
    Fp continuation;

  public:
    continuation_shared_state(BOOST_THREAD_RV_REF(F) f, BOOST_THREAD_FWD_REF(Fp) c)
    : parent(boost::move(f)),
      continuation(boost::move(c))
    {
    }

    void init(boost::unique_lock<boost::mutex> &lock)
    {
      parent.future_->set_continuation_ptr(this->shared_from_this(), lock);
    }

    void call()
    {
      try {
        this->continuation(boost::move(this->parent));
        this->mark_finished_with_result();
      } catch(...) {
        this->mark_exceptional_finish();
      }
      // make sure parent is really cleared to prevent memory "leaks"
      this->parent = F();
    }

    void call(boost::unique_lock<boost::mutex>& lck) {
      try {
        {
          relocker relock(lck);
          // neither continuation nor parent are protected by the lock - call() must only
          // be called once, and no one else must modify it.
          this->continuation(boost::move(this->parent));

          // make sure parent is really cleared to prevent memory "leaks"
          this->parent = F();
        }
        this->mark_finished_with_result_internal(lck);
      } catch (...) {
        this->mark_exceptional_finish_internal(current_exception(), lck);

        // make sure parent is really cleared to prevent memory "leaks"
        relocker relock(lck);
        this->parent = F();
      }
    }

    static void run(shared_ptr<boost::detail::shared_state_base> that_)
    {
      continuation_shared_state* that = static_cast<continuation_shared_state*>(that_.get());
      that->call();
    }

    ~continuation_shared_state() {}
  };
  /////////////////////////
  /// future_async_continuation_shared_state
  /////////////////////////

  template<typename F, typename Rp, typename Fp>
  struct future_async_continuation_shared_state: continuation_shared_state<F,Rp,Fp,future_async_shared_state_base<Rp> >
  {
    typedef continuation_shared_state<F,Rp,Fp,future_async_shared_state_base<Rp> > base_type;
  public:
    future_async_continuation_shared_state(BOOST_THREAD_RV_REF(F) f, BOOST_THREAD_FWD_REF(Fp) c)
    : base_type(boost::move(f), boost::forward<Fp>(c))
    {    }

    void launch_continuation() {
#if defined BOOST_THREAD_FUTURE_BLOCKING
      boost::lock_guard<boost::mutex> lk(this->mutex);
      this->thr_ = boost::thread(&future_async_continuation_shared_state::run, static_shared_from_this(this));
#else
      boost::thread(&base_type::run, static_shared_from_this(this)).detach();
#endif
    }
  };

  /////////////////////////
  /// future_sync_continuation_shared_state
  /////////////////////////

  template<typename F, typename Rp, typename Fp>
  struct future_sync_continuation_shared_state: continuation_shared_state<F,Rp,Fp,shared_state<Rp> >
  {
    typedef continuation_shared_state<F,Rp,Fp,shared_state<Rp> > base_type;
  public:
    future_sync_continuation_shared_state(BOOST_THREAD_RV_REF(F) f, BOOST_THREAD_FWD_REF(Fp) c)
    : base_type(boost::move(f), boost::forward<Fp>(c))
    {    }

    void launch_continuation() {
      this->call();
    }
  };


  /////////////////////////
  /// future_executor_continuation_shared_state
  /////////////////////////
#ifdef BOOST_THREAD_PROVIDES_EXECUTORS

  template <typename FutureExecutorContinuationSharedState>
  struct run_it {
    shared_ptr<FutureExecutorContinuationSharedState> that_;

#if ! defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
      BOOST_THREAD_COPYABLE_AND_MOVABLE(run_it)
      run_it(run_it const& x) //BOOST_NOEXCEPT
      : that_(x.that_)
      {}
      run_it& operator=(BOOST_THREAD_COPY_ASSIGN_REF(run_it) x) //BOOST_NOEXCEPT
      {
        if (this != &x) {
          that_=x.that_;
        }
        return *this;
      }
      // move
      run_it(BOOST_THREAD_RV_REF(run_it) x) BOOST_NOEXCEPT
      : that_(x.that_)
      {
        x.that_.reset();
      }
      run_it& operator=(BOOST_THREAD_RV_REF(run_it) x) BOOST_NOEXCEPT {
        if (this != &x) {
          that_=x.that;
          x.that_.reset();
        }
        return *this;
      }
#endif
    run_it(shared_ptr<FutureExecutorContinuationSharedState> that) : that_ (that) {}

    void operator()()
    {
      that_->run(that_);
    }
  };

}
  BOOST_THREAD_DCL_MOVABLE_BEG(F) detail::run_it<F> BOOST_THREAD_DCL_MOVABLE_END

namespace detail {

  template<typename F, typename Rp, typename Fp>
  struct future_executor_continuation_shared_state: continuation_shared_state<F,Rp,Fp>
  {
    typedef continuation_shared_state<F,Rp,Fp> base_type;

  public:
    future_executor_continuation_shared_state(BOOST_THREAD_RV_REF(F) f, BOOST_THREAD_FWD_REF(Fp) c)
    : base_type(boost::move(f), boost::forward<Fp>(c))
    {
    }

    template <class Ex>
    void init(boost::unique_lock<boost::mutex> &lk, Ex& ex)
    {
      this->set_executor_policy(executor_ptr_type(new executor_ref<Ex>(ex)), lk);
      this->base_type::init(lk);
    }

    void launch_continuation() {
      run_it<base_type> fct(static_shared_from_this(this));
      this->get_executor()->submit(boost::move(fct));
    }

    ~future_executor_continuation_shared_state() {}
  };
#endif

  /////////////////////////
  /// shared_future_async_continuation_shared_state
  /////////////////////////

  template<typename F, typename Rp, typename Fp>
  struct shared_future_async_continuation_shared_state: continuation_shared_state<F,Rp,Fp,future_async_shared_state_base<Rp> >
  {
    typedef continuation_shared_state<F,Rp,Fp,future_async_shared_state_base<Rp> > base_type;

  public:
    shared_future_async_continuation_shared_state(F f, BOOST_THREAD_FWD_REF(Fp) c)
    : base_type(boost::move(f), boost::forward<Fp>(c))
    {
    }

    void launch_continuation() {
#if defined BOOST_THREAD_FUTURE_BLOCKING
      boost::lock_guard<boost::mutex> lk(this->mutex);
      this->thr_ = boost::thread(&base_type::run, static_shared_from_this(this));
#else
      boost::thread(&base_type::run, static_shared_from_this(this)).detach();
#endif
    }
  };

  /////////////////////////
  /// shared_future_async_continuation_shared_state
  /////////////////////////

  template<typename F, typename Rp, typename Fp>
  struct shared_future_sync_continuation_shared_state: continuation_shared_state<F,Rp,Fp,shared_state<Rp> >
  {
    typedef continuation_shared_state<F,Rp,Fp,shared_state<Rp> > base_type;

  public:
    shared_future_sync_continuation_shared_state(F f, BOOST_THREAD_FWD_REF(Fp) c)
    : base_type(boost::move(f), boost::forward<Fp>(c))
    {
    }

    void launch_continuation() {
      this->call();
    }
  };


  /////////////////////////
  /// shared_future_executor_continuation_shared_state
  /////////////////////////
#ifdef BOOST_THREAD_PROVIDES_EXECUTORS

  template<typename F, typename Rp, typename Fp>
  struct shared_future_executor_continuation_shared_state: continuation_shared_state<F,Rp,Fp>
  {
    typedef continuation_shared_state<F,Rp,Fp> base_type;

  public:

    shared_future_executor_continuation_shared_state(F f, BOOST_THREAD_FWD_REF(Fp) c)
    : base_type(boost::move(f), boost::forward<Fp>(c))
    {
    }

    template <class Ex>
    void init(boost::unique_lock<boost::mutex> &lk, Ex& ex)
    {
      this->set_executor_policy(executor_ptr_type(new executor_ref<Ex>(ex)), lk);
      this->base_type::init(lk);
    }

    void launch_continuation() {
      run_it<base_type> fct(static_shared_from_this(this));
      this->get_executor()->submit(boost::move(fct));
    }

    ~shared_future_executor_continuation_shared_state() {}
  };

#endif
  //////////////////////////
  /// future_deferred_continuation_shared_state
  //////////////////////////
  template<typename F, typename Rp, typename Fp>
  struct future_deferred_continuation_shared_state: continuation_shared_state<F,Rp,Fp>
  {
    typedef continuation_shared_state<F,Rp,Fp> base_type;
  public:
    future_deferred_continuation_shared_state(BOOST_THREAD_RV_REF(F) f, BOOST_THREAD_FWD_REF(Fp) c)
    : base_type(boost::move(f), boost::forward<Fp>(c))
    {
      this->set_deferred();
    }

    virtual void execute(boost::unique_lock<boost::mutex>& lk) {
      this->parent.wait();
      this->call(lk);
    }

    virtual void launch_continuation() {    }
  };

  //////////////////////////
  /// shared_future_deferred_continuation_shared_state
  //////////////////////////
  template<typename F, typename Rp, typename Fp>
  struct shared_future_deferred_continuation_shared_state: continuation_shared_state<F,Rp,Fp>
  {
    typedef continuation_shared_state<F,Rp,Fp> base_type;

  public:
    shared_future_deferred_continuation_shared_state(F f, BOOST_THREAD_FWD_REF(Fp) c)
    : base_type(boost::move(f), boost::forward<Fp>(c))
    {
      this->set_deferred();
    }

    virtual void execute(boost::unique_lock<boost::mutex>& lk) {
      this->parent.wait();
      this->call(lk);
    }

    virtual void launch_continuation() {    }
  };

  ////////////////////////////////
  // make_future_deferred_continuation_shared_state
  ////////////////////////////////
  template<typename F, typename Rp, typename Fp>
  BOOST_THREAD_FUTURE<Rp>
  make_future_deferred_continuation_shared_state(
      boost::unique_lock<boost::mutex> &lock,
      BOOST_THREAD_RV_REF(F) f, BOOST_THREAD_FWD_REF(Fp) c) {
    typedef typename decay<Fp>::type Cont;
    shared_ptr<future_deferred_continuation_shared_state<F, Rp, Cont> >
        h(new future_deferred_continuation_shared_state<F, Rp, Cont>(boost::move(f), boost::forward<Fp>(c)));
    h->init(lock);
    return BOOST_THREAD_FUTURE<Rp>(h);
  }

  ////////////////////////////////
  // make_future_async_continuation_shared_state
  ////////////////////////////////
  template<typename F, typename Rp, typename Fp>
  BOOST_THREAD_FUTURE<Rp>
  make_future_async_continuation_shared_state(
      boost::unique_lock<boost::mutex> &lock, BOOST_THREAD_RV_REF(F) f,
      BOOST_THREAD_FWD_REF(Fp) c) {
    typedef typename decay<Fp>::type Cont;
    shared_ptr<future_async_continuation_shared_state<F,Rp, Cont> >
        h(new future_async_continuation_shared_state<F,Rp, Cont>(boost::move(f), boost::forward<Fp>(c)));
    h->init(lock);

    return BOOST_THREAD_FUTURE<Rp>(h);
  }
  ////////////////////////////////
  // make_future_sync_continuation_shared_state
  ////////////////////////////////
  template<typename F, typename Rp, typename Fp>
  BOOST_THREAD_FUTURE<Rp>
  make_future_sync_continuation_shared_state(
      boost::unique_lock<boost::mutex> &lock, BOOST_THREAD_RV_REF(F) f,
      BOOST_THREAD_FWD_REF(Fp) c) {
    typedef typename decay<Fp>::type Cont;
    shared_ptr<future_sync_continuation_shared_state<F,Rp, Cont> >
        h(new future_sync_continuation_shared_state<F,Rp, Cont>(boost::move(f), boost::forward<Fp>(c)));
    h->init(lock);

    return BOOST_THREAD_FUTURE<Rp>(h);
  }

  ////////////////////////////////
  // make_future_executor_continuation_shared_state
  ////////////////////////////////
#ifdef BOOST_THREAD_PROVIDES_EXECUTORS

  template<typename Ex, typename F, typename Rp, typename Fp>
  BOOST_THREAD_FUTURE<Rp>
  make_future_executor_continuation_shared_state(Ex& ex,
      boost::unique_lock<boost::mutex> &lock, BOOST_THREAD_RV_REF(F) f,
      BOOST_THREAD_FWD_REF(Fp) c) {
    typedef typename decay<Fp>::type Cont;
    shared_ptr<future_executor_continuation_shared_state<F,Rp, Cont> >
        h(new future_executor_continuation_shared_state<F,Rp, Cont>(boost::move(f), boost::forward<Fp>(c)));
    h->init(lock, ex);

    return BOOST_THREAD_FUTURE<Rp>(h);
  }
#endif

  ////////////////////////////////
  // make_shared_future_deferred_continuation_shared_state
  ////////////////////////////////
  template<typename F, typename Rp, typename Fp>
  BOOST_THREAD_FUTURE<Rp>
  make_shared_future_deferred_continuation_shared_state(
      boost::unique_lock<boost::mutex> &lock,
      F f, BOOST_THREAD_FWD_REF(Fp) c) {
    typedef typename decay<Fp>::type Cont;
    shared_ptr<shared_future_deferred_continuation_shared_state<F, Rp, Cont> >
        h(new shared_future_deferred_continuation_shared_state<F, Rp, Cont>(f, boost::forward<Fp>(c)));
    h->init(lock);

    return BOOST_THREAD_FUTURE<Rp>(h);
  }
  ////////////////////////////////
  // make_shared_future_async_continuation_shared_state
  ////////////////////////////////
  template<typename F, typename Rp, typename Fp>
  BOOST_THREAD_FUTURE<Rp>
  make_shared_future_async_continuation_shared_state(
      boost::unique_lock<boost::mutex> &lock, F f,
      BOOST_THREAD_FWD_REF(Fp) c) {
    typedef typename decay<Fp>::type Cont;
    shared_ptr<shared_future_async_continuation_shared_state<F,Rp, Cont> >
        h(new shared_future_async_continuation_shared_state<F,Rp, Cont>(f, boost::forward<Fp>(c)));
    h->init(lock);

    return BOOST_THREAD_FUTURE<Rp>(h);
  }
  ////////////////////////////////
  // make_shared_future_sync_continuation_shared_state
  ////////////////////////////////
  template<typename F, typename Rp, typename Fp>
  BOOST_THREAD_FUTURE<Rp>
  make_shared_future_sync_continuation_shared_state(
      boost::unique_lock<boost::mutex> &lock, F f,
      BOOST_THREAD_FWD_REF(Fp) c) {
    typedef typename decay<Fp>::type Cont;
    shared_ptr<shared_future_sync_continuation_shared_state<F,Rp, Cont> >
        h(new shared_future_sync_continuation_shared_state<F,Rp, Cont>(f, boost::forward<Fp>(c)));
    h->init(lock);

    return BOOST_THREAD_FUTURE<Rp>(h);
  }
  ////////////////////////////////
  // make_shared_future_executor_continuation_shared_state
  ////////////////////////////////
#ifdef BOOST_THREAD_PROVIDES_EXECUTORS
  template<typename Ex, typename F, typename Rp, typename Fp>
  BOOST_THREAD_FUTURE<Rp>
  make_shared_future_executor_continuation_shared_state(Ex& ex,
      boost::unique_lock<boost::mutex> &lock, F f,
      BOOST_THREAD_FWD_REF(Fp) c) {
    typedef typename decay<Fp>::type Cont;
    shared_ptr<shared_future_executor_continuation_shared_state<F, Rp, Cont> >
        h(new shared_future_executor_continuation_shared_state<F, Rp, Cont>(f, boost::forward<Fp>(c)));
    h->init(lock, ex);

    return BOOST_THREAD_FUTURE<Rp>(h);
  }
#endif
}

  ////////////////////////////////
  // template<typename F>
  // auto future<R>::then(launch policy, F&& func) -> BOOST_THREAD_FUTURE<decltype(func(*this))>;
  ////////////////////////////////
  template <typename R>
  template <typename F>
  inline BOOST_THREAD_FUTURE<typename boost::result_of<F(BOOST_THREAD_FUTURE<R>)>::type>
  BOOST_THREAD_FUTURE<R>::then(launch policy, BOOST_THREAD_FWD_REF(F) func) {
    typedef typename boost::result_of<F(BOOST_THREAD_FUTURE<R>)>::type future_type;
    BOOST_THREAD_ASSERT_PRECONDITION(this->future_.get()!=0, future_uninitialized());

    // keep state alive as we move ourself but hold the lock
    shared_ptr<detail::shared_state_base> sentinel(this->future_);
    boost::unique_lock<boost::mutex> lock(sentinel->mutex);

    if (underlying_cast<int>(policy) & int(launch::async)) {
      return BOOST_THREAD_MAKE_RV_REF((boost::detail::make_future_async_continuation_shared_state<BOOST_THREAD_FUTURE<R>, future_type>(
                  lock, boost::move(*this), boost::forward<F>(func)
              )));
    } else if (underlying_cast<int>(policy) & int(launch::deferred)) {
      return BOOST_THREAD_MAKE_RV_REF((boost::detail::make_future_deferred_continuation_shared_state<BOOST_THREAD_FUTURE<R>, future_type>(
                  lock, boost::move(*this), boost::forward<F>(func)
              )));
    } else if (underlying_cast<int>(policy) & int(launch::sync)) {
      return BOOST_THREAD_MAKE_RV_REF((boost::detail::make_future_sync_continuation_shared_state<BOOST_THREAD_FUTURE<R>, future_type>(
                  lock, boost::move(*this), boost::forward<F>(func)
              )));
#ifdef BOOST_THREAD_PROVIDES_EXECUTORS
    } else if (underlying_cast<int>(policy) & int(launch::executor)) {
      assert(this->future_->get_executor());
      typedef executor Ex;
      Ex& ex = *(this->future_->get_executor());
      return BOOST_THREAD_MAKE_RV_REF((boost::detail::make_future_executor_continuation_shared_state<Ex, BOOST_THREAD_FUTURE<R>, future_type>(ex,
                    lock, boost::move(*this), boost::forward<F>(func)
                )));
#endif
    } else if (underlying_cast<int>(policy) & int(launch::inherit)) {

        launch policy_ = this->launch_policy(lock);
        if (underlying_cast<int>(policy_) & int(launch::async)) {
          return BOOST_THREAD_MAKE_RV_REF((boost::detail::make_future_async_continuation_shared_state<BOOST_THREAD_FUTURE<R>, future_type>(
                      lock, boost::move(*this), boost::forward<F>(func)
                  )));
        } else if (underlying_cast<int>(policy_) & int(launch::deferred)) {
          return BOOST_THREAD_MAKE_RV_REF((boost::detail::make_future_deferred_continuation_shared_state<BOOST_THREAD_FUTURE<R>, future_type>(
                      lock, boost::move(*this), boost::forward<F>(func)
                  )));
        } else if (underlying_cast<int>(policy_) & int(launch::sync)) {
          return BOOST_THREAD_MAKE_RV_REF((boost::detail::make_future_sync_continuation_shared_state<BOOST_THREAD_FUTURE<R>, future_type>(
                      lock, boost::move(*this), boost::forward<F>(func)
                  )));
#ifdef BOOST_THREAD_PROVIDES_EXECUTORS
        } else if (underlying_cast<int>(policy_) & int(launch::executor)) {
          assert(this->future_->get_executor());
          typedef executor Ex;
          Ex& ex = *(this->future_->get_executor());
          return BOOST_THREAD_MAKE_RV_REF((boost::detail::make_future_executor_continuation_shared_state<Ex, BOOST_THREAD_FUTURE<R>, future_type>(ex,
                        lock, boost::move(*this), boost::forward<F>(func)
                    )));
#endif
        } else {
          return BOOST_THREAD_MAKE_RV_REF((boost::detail::make_future_async_continuation_shared_state<BOOST_THREAD_FUTURE<R>, future_type>(
                      lock, boost::move(*this), boost::forward<F>(func)
                  )));
        }
    } else {
      return BOOST_THREAD_MAKE_RV_REF((boost::detail::make_future_async_continuation_shared_state<BOOST_THREAD_FUTURE<R>, future_type>(
                  lock, boost::move(*this), boost::forward<F>(func)
              )));
    }
  }
#ifdef BOOST_THREAD_PROVIDES_EXECUTORS
  ////////////////////////////////
  // template<typename Ex, typename F>
  // auto future<future<R2> >::then(Ex&, F&& func) -> BOOST_THREAD_FUTURE<decltype(func(*this))>;
  ////////////////////////////////
  template <typename R>
  template <typename Ex, typename F>
  inline BOOST_THREAD_FUTURE<typename boost::result_of<F(BOOST_THREAD_FUTURE<R>)>::type>
  BOOST_THREAD_FUTURE<R>::then(Ex& ex, BOOST_THREAD_FWD_REF(F) func) {
    typedef typename boost::result_of<F(BOOST_THREAD_FUTURE<R>)>::type future_type;
    BOOST_THREAD_ASSERT_PRECONDITION(this->future_.get()!=0, future_uninitialized());

    // keep state alive as we move ourself but hold the lock
    shared_ptr<detail::shared_state_base> sentinel(this->future_);
    boost::unique_lock<boost::mutex> lock(sentinel->mutex);

    return BOOST_THREAD_MAKE_RV_REF((boost::detail::make_future_executor_continuation_shared_state<Ex, BOOST_THREAD_FUTURE<R>, future_type>(ex,
                  lock, boost::move(*this), boost::forward<F>(func)
              )));
  }
#endif
  ////////////////////////////////
  // template<typename F>
  // auto future<future<R2> >::then(F&& func) -> BOOST_THREAD_FUTURE<decltype(func(*this))>;
  ////////////////////////////////
  template <typename R>
  template <typename F>
  inline BOOST_THREAD_FUTURE<typename boost::result_of<F(BOOST_THREAD_FUTURE<R>)>::type>
  BOOST_THREAD_FUTURE<R>::then(BOOST_THREAD_FWD_REF(F) func)  {

#ifndef BOOST_THREAD_CONTINUATION_SYNC
    return this->then(this->launch_policy(), boost::forward<F>(func));
#else
    typedef typename boost::result_of<F(BOOST_THREAD_FUTURE<R>)>::type future_type;
    BOOST_THREAD_ASSERT_PRECONDITION(this->future_.get()!=0, future_uninitialized());

    // keep state alive as we move ourself but hold the lock
    shared_ptr<detail::shared_state_base> sentinel(this->future_);
    boost::unique_lock<boost::mutex> lock(sentinel->mutex);

    launch policy = this->launch_policy(lock);
    if (underlying_cast<int>(policy) & int(launch::deferred)) {
      return BOOST_THREAD_MAKE_RV_REF((boost::detail::make_future_deferred_continuation_shared_state<BOOST_THREAD_FUTURE<R>, future_type>(
                  lock, boost::move(*this), boost::forward<F>(func)
              )));
    } else {
      return BOOST_THREAD_MAKE_RV_REF((boost::detail::make_future_async_continuation_shared_state<BOOST_THREAD_FUTURE<R>, future_type>(
                  lock, boost::move(*this), boost::forward<F>(func)
              )));
    }
#endif

  }

  ////////////////////////////////
  // template<typename F>
  // auto future<future<R2> >::then(launch, F&& func) -> BOOST_THREAD_FUTURE<decltype(func(*this))>;
  ////////////////////////////////
  template <typename R2>
  template <typename F>
  inline BOOST_THREAD_FUTURE<typename boost::result_of<F(BOOST_THREAD_FUTURE<BOOST_THREAD_FUTURE<R2> >)>::type>
  BOOST_THREAD_FUTURE<BOOST_THREAD_FUTURE<R2> >::then(launch policy, BOOST_THREAD_FWD_REF(F) func) {
    typedef BOOST_THREAD_FUTURE<R2> R;
    typedef typename boost::result_of<F(BOOST_THREAD_FUTURE<R>)>::type future_type;
    BOOST_THREAD_ASSERT_PRECONDITION(this->future_.get()!=0, future_uninitialized());

    // keep state alive as we move ourself but hold the lock
    shared_ptr<detail::shared_state_base> sentinel(this->future_);
    boost::unique_lock<boost::mutex> lock(sentinel->mutex);

    if (underlying_cast<int>(policy) & int(launch::async)) {
      return BOOST_THREAD_MAKE_RV_REF((boost::detail::make_future_async_continuation_shared_state<BOOST_THREAD_FUTURE<R>, future_type>(
                  lock, boost::move(*this), boost::forward<F>(func)
              )));
    } else if (underlying_cast<int>(policy) & int(launch::deferred)) {
      return BOOST_THREAD_MAKE_RV_REF((boost::detail::make_future_deferred_continuation_shared_state<BOOST_THREAD_FUTURE<R>, future_type>(
                  lock, boost::move(*this), boost::forward<F>(func)
              )));
    } else if (underlying_cast<int>(policy) & int(launch::sync)) {
      return BOOST_THREAD_MAKE_RV_REF((boost::detail::make_future_sync_continuation_shared_state<BOOST_THREAD_FUTURE<R>, future_type>(
                  lock, boost::move(*this), boost::forward<F>(func)
              )));
#ifdef BOOST_THREAD_PROVIDES_EXECUTORS
    } else if (underlying_cast<int>(policy) & int(launch::executor)) {
      assert(this->future_->get_executor());
      typedef executor Ex;
      Ex& ex = *(this->future_->get_executor());
      return BOOST_THREAD_MAKE_RV_REF((boost::detail::make_future_executor_continuation_shared_state<Ex, BOOST_THREAD_FUTURE<R>, future_type>(ex,
                    lock, boost::move(*this), boost::forward<F>(func)
                )));
#endif
    } else if (underlying_cast<int>(policy) & int(launch::inherit)) {
        launch policy_ = this->launch_policy(lock);

        if (underlying_cast<int>(policy_) & int(launch::async)) {
          return BOOST_THREAD_MAKE_RV_REF((boost::detail::make_future_async_continuation_shared_state<BOOST_THREAD_FUTURE<R>, future_type>(
                      lock, boost::move(*this), boost::forward<F>(func)
                  )));
        } else if (underlying_cast<int>(policy_) & int(launch::deferred)) {
          return BOOST_THREAD_MAKE_RV_REF((boost::detail::make_future_deferred_continuation_shared_state<BOOST_THREAD_FUTURE<R>, future_type>(
                      lock, boost::move(*this), boost::forward<F>(func)
                  )));
        } else if (underlying_cast<int>(policy_) & int(launch::sync)) {
          return BOOST_THREAD_MAKE_RV_REF((boost::detail::make_future_sync_continuation_shared_state<BOOST_THREAD_FUTURE<R>, future_type>(
                      lock, boost::move(*this), boost::forward<F>(func)
                  )));
#ifdef BOOST_THREAD_PROVIDES_EXECUTORS
        } else if (underlying_cast<int>(policy_) & int(launch::executor)) {
          assert(this->future_->get_executor());
          typedef executor Ex;
          Ex& ex = *(this->future_->get_executor());
          return BOOST_THREAD_MAKE_RV_REF((boost::detail::make_future_executor_continuation_shared_state<Ex, BOOST_THREAD_FUTURE<R>, future_type>(ex,
                        lock, boost::move(*this), boost::forward<F>(func)
                    )));
#endif
        } else {
          return BOOST_THREAD_MAKE_RV_REF((boost::detail::make_future_async_continuation_shared_state<BOOST_THREAD_FUTURE<R>, future_type>(
                      lock, boost::move(*this), boost::forward<F>(func)
                  )));
        }
    } else {
      return BOOST_THREAD_MAKE_RV_REF((boost::detail::make_future_async_continuation_shared_state<BOOST_THREAD_FUTURE<R>, future_type>(
                  lock, boost::move(*this), boost::forward<F>(func)
              )));
    }
  }

#ifdef BOOST_THREAD_PROVIDES_EXECUTORS
  ////////////////////////////////
  // template<typename Ex, typename F>
  // auto future<future<R2> >::then(Ex&, F&& func) -> BOOST_THREAD_FUTURE<decltype(func(*this))>;
  ////////////////////////////////
  template <typename R2>
  template <typename Ex, typename F>
  inline BOOST_THREAD_FUTURE<typename boost::result_of<F(BOOST_THREAD_FUTURE<BOOST_THREAD_FUTURE<R2> >)>::type>
  BOOST_THREAD_FUTURE<BOOST_THREAD_FUTURE<R2> >::then(Ex& ex, BOOST_THREAD_FWD_REF(F) func) {
    typedef BOOST_THREAD_FUTURE<R2> R;
    typedef typename boost::result_of<F(BOOST_THREAD_FUTURE<R>)>::type future_type;
    BOOST_THREAD_ASSERT_PRECONDITION(this->future_.get()!=0, future_uninitialized());

    // keep state alive as we move ourself but hold the lock
    shared_ptr<detail::shared_state_base> sentinel(this->future_);
    boost::unique_lock<boost::mutex> lock(sentinel->mutex);

    return BOOST_THREAD_MAKE_RV_REF((boost::detail::make_future_executor_continuation_shared_state<Ex, BOOST_THREAD_FUTURE<R>, future_type>(ex,
                  lock, boost::move(*this), boost::forward<F>(func)
              )));
  }
#endif

  ////////////////////////////////
  // template<typename F>
  // auto future<future<R2> >::then(F&& func) -> BOOST_THREAD_FUTURE<decltype(func(*this))>;
  ////////////////////////////////
  template <typename R2>
  template <typename F>
  inline BOOST_THREAD_FUTURE<typename boost::result_of<F(BOOST_THREAD_FUTURE<BOOST_THREAD_FUTURE<R2> >)>::type>
  BOOST_THREAD_FUTURE<BOOST_THREAD_FUTURE<R2> >::then(BOOST_THREAD_FWD_REF(F) func)  {

#ifndef BOOST_THREAD_CONTINUATION_SYNC
    return this->then(this->launch_policy(), boost::forward<F>(func));
#else
    typedef BOOST_THREAD_FUTURE<R2> R;
    typedef typename boost::result_of<F(BOOST_THREAD_FUTURE<R>)>::type future_type;
    BOOST_THREAD_ASSERT_PRECONDITION(this->future_.get()!=0, future_uninitialized());

    // keep state alive as we move ourself but hold the lock
    shared_ptr<detail::shared_state_base> sentinel(this->future_);
    boost::unique_lock<boost::mutex> lock(sentinel->mutex);

    launch policy = this->launch_policy(lock);

    if  (underlying_cast<int>(policy) & int(launch::deferred)) {
      return BOOST_THREAD_MAKE_RV_REF((boost::detail::make_future_deferred_continuation_shared_state<BOOST_THREAD_FUTURE<R>, future_type>(
                  lock, boost::move(*this), boost::forward<F>(func)
              )));
    } else {
      return BOOST_THREAD_MAKE_RV_REF((boost::detail::make_future_sync_continuation_shared_state<BOOST_THREAD_FUTURE<R>, future_type>(
                  lock, boost::move(*this), boost::forward<F>(func)
              )));
    }
#endif
  }

  ////////////////////////////////
  // template<typename F>
  // auto shared_future<R>::then(launch policy, F&& func) -> BOOST_THREAD_FUTURE<decltype(func(*this))>;
  ////////////////////////////////
  template <typename R>
  template <typename F>
  inline BOOST_THREAD_FUTURE<typename boost::result_of<F(shared_future<R>)>::type>
  shared_future<R>::then(launch policy, BOOST_THREAD_FWD_REF(F) func)  const
  {
    typedef typename boost::result_of<F(shared_future<R>)>::type future_type;
    BOOST_THREAD_ASSERT_PRECONDITION(this->future_.get()!=0, future_uninitialized());

    boost::unique_lock<boost::mutex> lock(this->future_->mutex);
    if (underlying_cast<int>(policy) & int(launch::async)) {
      return BOOST_THREAD_MAKE_RV_REF((boost::detail::make_shared_future_async_continuation_shared_state<shared_future<R>, future_type>(
                  lock, *this, boost::forward<F>(func)
              )));
    } else if (underlying_cast<int>(policy) & int(launch::deferred)) {
      return BOOST_THREAD_MAKE_RV_REF((boost::detail::make_shared_future_deferred_continuation_shared_state<shared_future<R>, future_type>(
                  lock, *this, boost::forward<F>(func)
              )));
    } else if (underlying_cast<int>(policy) & int(launch::sync)) {
      return BOOST_THREAD_MAKE_RV_REF((boost::detail::make_shared_future_sync_continuation_shared_state<shared_future<R>, future_type>(
                  lock, *this, boost::forward<F>(func)
              )));
#ifdef BOOST_THREAD_PROVIDES_EXECUTORS
    } else if (underlying_cast<int>(policy) & int(launch::executor)) {
      typedef executor Ex;
      Ex& ex = *(this->future_->get_executor());
      return BOOST_THREAD_MAKE_RV_REF((boost::detail::make_shared_future_executor_continuation_shared_state<Ex, shared_future<R>, future_type>(ex,
                    lock, *this, boost::forward<F>(func)
                )));
#endif
    } else if (underlying_cast<int>(policy) & int(launch::inherit)) {

        launch policy_ = this->launch_policy(lock);
        if (underlying_cast<int>(policy_) & int(launch::async)) {
          return BOOST_THREAD_MAKE_RV_REF((boost::detail::make_shared_future_async_continuation_shared_state<shared_future<R>, future_type>(
                      lock, *this, boost::forward<F>(func)
                  )));
        } else if (underlying_cast<int>(policy_) & int(launch::deferred)) {
          return BOOST_THREAD_MAKE_RV_REF((boost::detail::make_shared_future_deferred_continuation_shared_state<shared_future<R>, future_type>(
                      lock, *this, boost::forward<F>(func)
                  )));
        } else if (underlying_cast<int>(policy_) & int(launch::sync)) {
          return BOOST_THREAD_MAKE_RV_REF((boost::detail::make_shared_future_sync_continuation_shared_state<shared_future<R>, future_type>(
                      lock, *this, boost::forward<F>(func)
                  )));
#ifdef BOOST_THREAD_PROVIDES_EXECUTORS
        } else if (underlying_cast<int>(policy_) & int(launch::executor)) {
          typedef executor Ex;
          Ex& ex = *(this->future_->get_executor());
          return BOOST_THREAD_MAKE_RV_REF((boost::detail::make_shared_future_executor_continuation_shared_state<Ex, shared_future<R>, future_type>(ex,
                        lock, *this, boost::forward<F>(func)
                    )));
#endif
        } else {
          return BOOST_THREAD_MAKE_RV_REF((boost::detail::make_shared_future_async_continuation_shared_state<shared_future<R>, future_type>(
                      lock, *this, boost::forward<F>(func)
                  )));
        }

    } else {
      return BOOST_THREAD_MAKE_RV_REF((boost::detail::make_shared_future_async_continuation_shared_state<shared_future<R>, future_type>(
                  lock, *this, boost::forward<F>(func)
              )));
    }
  }
#ifdef BOOST_THREAD_PROVIDES_EXECUTORS
  ////////////////////////////////
  // template<typename Ex, typename F>
  // auto shared_future<R>::then(Ex&, F&& func) -> BOOST_THREAD_FUTURE<decltype(func(*this))>;
  ////////////////////////////////
  template <typename R>
  template <typename Ex, typename F>
  inline BOOST_THREAD_FUTURE<typename boost::result_of<F(shared_future<R>)>::type>
  shared_future<R>::then(Ex& ex, BOOST_THREAD_FWD_REF(F) func)  const
  {
    typedef typename boost::result_of<F(shared_future<R>)>::type future_type;
    BOOST_THREAD_ASSERT_PRECONDITION(this->future_.get()!=0, future_uninitialized());

    boost::unique_lock<boost::mutex> lock(this->future_->mutex);
    return BOOST_THREAD_MAKE_RV_REF((boost::detail::make_shared_future_executor_continuation_shared_state<Ex, shared_future<R>, future_type>(ex,
                  lock, *this, boost::forward<F>(func)
              )));
  }
#endif

  ////////////////////////////////
  // template<typename F>
  // auto shared_future<R>::then(F&& func) -> BOOST_THREAD_FUTURE<decltype(func(*this))>;
  ////////////////////////////////
  template <typename R>
  template <typename F>
  inline BOOST_THREAD_FUTURE<typename boost::result_of<F(shared_future<R>)>::type>
  shared_future<R>::then(BOOST_THREAD_FWD_REF(F) func)  const {
#ifndef BOOST_THREAD_CONTINUATION_SYNC
    return this->then(this->launch_policy(), boost::forward<F>(func));
#else
    typedef typename boost::result_of<F(shared_future<R>)>::type future_type;
    BOOST_THREAD_ASSERT_PRECONDITION(this->future_.get()!=0, future_uninitialized());

    boost::unique_lock<boost::mutex> lock(this->future_->mutex);
    launch policy = this->launch_policy(lock);
    if (underlying_cast<int>(policy) & int(launch::deferred)) {
      return BOOST_THREAD_MAKE_RV_REF((boost::detail::make_shared_future_deferred_continuation_shared_state<shared_future<R>, future_type>(
                  lock, *this, boost::forward<F>(func)
              )));
    } else {
      return BOOST_THREAD_MAKE_RV_REF((boost::detail::make_shared_future_sync_continuation_shared_state<shared_future<R>, future_type>(
                  lock, *this, boost::forward<F>(func)
              )));
    }
#endif
  }

namespace detail
{
  template <typename T>
  struct mfallbacker_to
  {
    T value_;
    typedef T result_type;
    mfallbacker_to(BOOST_THREAD_RV_REF(T) v)
    : value_(boost::move(v))
    {}

    T operator()(BOOST_THREAD_FUTURE<T> fut) {
      return fut.get_or(boost::move(value_));
    }
  };
  template <typename T>
  struct cfallbacker_to
  {
    T value_;
    typedef T result_type;
    cfallbacker_to(T const& v)
    : value_(v)
    {}

    T operator()(BOOST_THREAD_FUTURE<T> fut) const {
      return fut.get_or(value_);

    }
  };
}
  ////////////////////////////////
  // future<R> future<R>::fallback_to(R&& v);
  ////////////////////////////////

  template <typename R>
  template <typename R2>
  inline typename boost::disable_if< is_void<R2>, BOOST_THREAD_FUTURE<R> >::type
  BOOST_THREAD_FUTURE<R>::fallback_to(BOOST_THREAD_RV_REF(R2) v) {
    return then(detail::mfallbacker_to<R>(boost::move(v)));
  }

  template <typename R>
  template <typename R2>
  inline typename boost::disable_if< is_void<R2>, BOOST_THREAD_FUTURE<R> >::type
  BOOST_THREAD_FUTURE<R>::fallback_to(R2 const& v) {
    return then(detail::cfallbacker_to<R>(v));
  }

#endif

#if defined BOOST_THREAD_PROVIDES_FUTURE_UNWRAP
namespace detail
{
  /////////////////////////
  /// future_unwrap_shared_state
  /////////////////////////

  template<typename F, typename Rp>
  struct future_unwrap_shared_state: shared_state<Rp>
  {
    F wrapped;
    typename F::value_type unwrapped;
  public:
    explicit future_unwrap_shared_state(BOOST_THREAD_RV_REF(F) f)
    : wrapped(boost::move(f)) {
    }

    void launch_continuation()
    {
      boost::unique_lock<boost::mutex> lk(this->mutex);
      // assert(wrapped.is_ready());
      if (! unwrapped.valid() )
      {
        if (wrapped.has_exception()) {
          this->mark_exceptional_finish_internal(wrapped.get_exception_ptr(), lk);
        } else {
          unwrapped = wrapped.get();
          if (unwrapped.valid())
          {
            lk.unlock();
            boost::unique_lock<boost::mutex> lk2(unwrapped.future_->mutex);
            unwrapped.future_->set_continuation_ptr(this->shared_from_this(), lk2);
          } else {
            this->mark_exceptional_finish_internal(boost::copy_exception(future_uninitialized()), lk);
          }
        }
      } else {
        // assert(unwrapped.is_ready());
        if (unwrapped.has_exception()) {
          this->mark_exceptional_finish_internal(unwrapped.get_exception_ptr(), lk);
        } else {
          this->mark_finished_with_result_internal(unwrapped.get(), lk);
        }
      }
    }
  };

  template<typename F>
  struct future_unwrap_shared_state<F,void>: shared_state<void>
  {
    F wrapped;
    typename F::value_type unwrapped;
  public:
    explicit future_unwrap_shared_state(BOOST_THREAD_RV_REF(F) f)
    : wrapped(boost::move(f)) {
    }

    void launch_continuation()
    {
      boost::unique_lock<boost::mutex> lk(this->mutex);
      // assert(wrapped.is_ready());
      if (! unwrapped.valid() )
      {
        if (wrapped.has_exception()) {
          this->mark_exceptional_finish_internal(wrapped.get_exception_ptr(), lk);
        } else {
          unwrapped = wrapped.get();
          if (unwrapped.valid())
          {
            lk.unlock();
            boost::unique_lock<boost::mutex> lk2(unwrapped.future_->mutex);
            unwrapped.future_->set_continuation_ptr(this->shared_from_this(), lk2);
          } else {
            this->mark_exceptional_finish_internal(boost::copy_exception(future_uninitialized()), lk);
          }
        }
      } else {
        // assert(unwrapped.is_ready());
        if (unwrapped.has_exception()) {
          this->mark_exceptional_finish_internal(unwrapped.get_exception_ptr(), lk);
        } else {
          this->mark_finished_with_result_internal(lk);
        }
      }
    }
  };

  template <class F, class Rp>
  BOOST_THREAD_FUTURE<Rp>
  make_future_unwrap_shared_state(boost::unique_lock<boost::mutex> &lock, BOOST_THREAD_RV_REF(F) f) {
    shared_ptr<future_unwrap_shared_state<F, Rp> >
        h(new future_unwrap_shared_state<F, Rp>(boost::move(f)));
    h->wrapped.future_->set_continuation_ptr(h, lock);

    return BOOST_THREAD_FUTURE<Rp>(h);
  }
}

  template <typename R>
  inline BOOST_THREAD_FUTURE<R>::BOOST_THREAD_FUTURE(BOOST_THREAD_RV_REF(BOOST_THREAD_FUTURE<BOOST_THREAD_FUTURE<R> >) other)
  : base_type(other.unwrap()) {}

  template <typename R2>
  BOOST_THREAD_FUTURE<R2>
  BOOST_THREAD_FUTURE<BOOST_THREAD_FUTURE<R2> >::unwrap()
  {
    BOOST_THREAD_ASSERT_PRECONDITION(this->future_.get()!=0, future_uninitialized());

    // keep state alive as we move ourself but hold the lock
    shared_ptr<detail::shared_state_base> sentinel(this->future_);
    boost::unique_lock<boost::mutex> lock(sentinel->mutex);

    return boost::detail::make_future_unwrap_shared_state<BOOST_THREAD_FUTURE<BOOST_THREAD_FUTURE<R2> >, R2>(lock, boost::move(*this));
  }
#endif

#if defined BOOST_THREAD_PROVIDES_FUTURE_WHEN_ALL_WHEN_ANY
namespace detail
{
  struct input_iterator_tag {};
  struct vector_tag {};
  struct values_tag {};
  template <typename T>
  struct alias_t { typedef T type; };

  BOOST_CONSTEXPR_OR_CONST input_iterator_tag input_iterator_tag_value = {};
  BOOST_CONSTEXPR_OR_CONST vector_tag vector_tag_value = {};
  BOOST_CONSTEXPR_OR_CONST values_tag values_tag_value = {};
  ////////////////////////////////
  // detail::future_async_when_all_shared_state
  ////////////////////////////////
  template<typename F>
  struct future_when_all_vector_shared_state: future_async_shared_state_base<csbl::vector<F> >
  {
    typedef csbl::vector<F> vector_type;
    typedef typename F::value_type value_type;
    vector_type vec_;

    static void run(shared_ptr<boost::detail::shared_state_base> that_) {
      future_when_all_vector_shared_state* that = static_cast<future_when_all_vector_shared_state*>(that_.get());
      try {
        boost::wait_for_all(that->vec_.begin(), that->vec_.end());
        that->mark_finished_with_result(boost::move(that->vec_));
      } catch(...) {
        that->mark_exceptional_finish();
      }
    }
    bool run_deferred() {

      bool res = false;
      for (typename csbl::vector<F>::iterator it = vec_.begin(); it != vec_.end(); ++it) {
        if (! it->run_if_is_deferred())
        {
          res = true;
        }
      }
      return res;
    }
    void init() {
      if (! run_deferred())
      {
        future_when_all_vector_shared_state::run(this->shared_from_this());
        return;
      }
#ifdef BOOST_THREAD_FUTURE_BLOCKING
      this->thr_ = boost::thread(&future_when_all_vector_shared_state::run, this->shared_from_this());
#else
      boost::thread(&future_when_all_vector_shared_state::run, this->shared_from_this()).detach();
#endif
    }

  public:
    template< typename InputIterator>
    future_when_all_vector_shared_state(input_iterator_tag, InputIterator first, InputIterator last)
    : vec_(std::make_move_iterator(first), std::make_move_iterator(last))
    {
    }

    future_when_all_vector_shared_state(vector_tag, BOOST_THREAD_RV_REF(csbl::vector<F>) v)
    : vec_(boost::move(v))
    {
    }

#if ! defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)
    template< typename T0, typename ...T>
    future_when_all_vector_shared_state(values_tag, BOOST_THREAD_FWD_REF(T0) f, BOOST_THREAD_FWD_REF(T) ... futures) {
      vec_.push_back(boost::forward<T0>(f));
      typename alias_t<char[]>::type{
          ( //first part of magic unpacker
          vec_.push_back(boost::forward<T>(futures)),'0'
          )..., '0'
      }; //second part of magic unpacker
    }
#endif

    ~future_when_all_vector_shared_state() {}
  };

  ////////////////////////////////
  // detail::future_async_when_any_shared_state
  ////////////////////////////////
  template<typename F>
  struct future_when_any_vector_shared_state: future_async_shared_state_base<csbl::vector<F> >
  {
    typedef csbl::vector<F> vector_type;
    typedef typename F::value_type value_type;
    vector_type vec_;

    static void run(shared_ptr<boost::detail::shared_state_base> that_)
    {
      future_when_any_vector_shared_state* that = static_cast<future_when_any_vector_shared_state*>(that_.get());
      try {
        boost::wait_for_any(that->vec_.begin(), that->vec_.end());
        that->mark_finished_with_result(boost::move(that->vec_));
      } catch(...) {
        that->mark_exceptional_finish();
      }
    }
    bool run_deferred() {

      for (typename csbl::vector<F>::iterator it = vec_.begin(); it != vec_.end(); ++it) {
        if (it->run_if_is_deferred_or_ready())
        {
          return true;
        }
      }
      return false;
    }
    void init() {
      if (run_deferred())
      {
        future_when_any_vector_shared_state::run(this->shared_from_this());
        return;
      }

#ifdef BOOST_THREAD_FUTURE_BLOCKING
      this->thr_ = boost::thread(&future_when_any_vector_shared_state::run, this->shared_from_this());
#else
      boost::thread(&future_when_any_vector_shared_state::run, this->shared_from_this()).detach();
#endif
    }

  public:
    template< typename InputIterator>
    future_when_any_vector_shared_state(input_iterator_tag, InputIterator first, InputIterator last)
    : vec_(std::make_move_iterator(first), std::make_move_iterator(last))
    {
    }

    future_when_any_vector_shared_state(vector_tag, BOOST_THREAD_RV_REF(csbl::vector<F>) v)
    : vec_(boost::move(v))
    {
    }

#if ! defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)
    template< typename T0, typename ...T>
    future_when_any_vector_shared_state(values_tag,
        BOOST_THREAD_FWD_REF(T0) f, BOOST_THREAD_FWD_REF(T) ... futures
    ) {
      vec_.push_back(boost::forward<T0>(f));
      typename alias_t<char[]>::type{
          ( //first part of magic unpacker
          vec_.push_back(boost::forward<T>(futures))
          ,'0'
          )...,
          '0'
      }; //second part of magic unpacker
    }
#endif

    ~future_when_any_vector_shared_state() {}
  };

#if ! defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)
  struct wait_for_all_fctr {
    template <class ...T>
    void operator()(T&&... v) {
      boost::wait_for_all(boost::forward<T>(v)...);
    }
  };

  struct wait_for_any_fctr {
    template <class ...T>
    void operator()(T&&... v) {
      boost::wait_for_any(boost::forward<T>(v)...);
    }
  };


  template <class Tuple, std::size_t i=csbl::tuple_size<Tuple>::value>
  struct accumulate_run_if_is_deferred {
    bool operator ()(Tuple& t)
    {
      return (! csbl::get<i-1>(t).run_if_is_deferred()) || accumulate_run_if_is_deferred<Tuple,i-1>()(t);
    }
  };
  template <class Tuple>
  struct accumulate_run_if_is_deferred<Tuple, 0> {
    bool operator ()(Tuple& )
    {
      return false;
    }
  };


  template< typename Tuple, typename T0, typename ...T>
  struct future_when_all_tuple_shared_state: future_async_shared_state_base<Tuple>
  {
    Tuple tup_;
    typedef typename make_tuple_indices<1+sizeof...(T)>::type Index;

    static void run(shared_ptr<boost::detail::shared_state_base> that_) {
      future_when_all_tuple_shared_state* that = static_cast<future_when_all_tuple_shared_state*>(that_.get());
      try {
        // TODO make use of apply(that->tup_, boost::detail::wait_for_all_fctor());
        that->wait_for_all(Index());

        that->mark_finished_with_result(boost::move(that->tup_));
      } catch(...) {
        that->mark_exceptional_finish();
      }
    }

    template <size_t ...Indices>
    void wait_for_all(tuple_indices<Indices...>) {
#if defined BOOST_THREAD_PROVIDES_INVOKE
      return invoke<void>(wait_for_all_fctr(), csbl::get<Indices>(tup_)...);
#else
      return wait_for_all_fctr()(csbl::get<Indices>(tup_)...);
#endif
    }

    bool run_deferred() {

      return accumulate_run_if_is_deferred<Tuple>()(tup_);
    }
    void init() {
      if (! run_deferred())
      {
        future_when_all_tuple_shared_state::run(this->shared_from_this());
        return;
      }
#ifdef BOOST_THREAD_FUTURE_BLOCKING
      this->thr_ = boost::thread(&future_when_all_tuple_shared_state::run, this->shared_from_this());
#else
      boost::thread(&future_when_all_tuple_shared_state::run, this->shared_from_this()).detach();
#endif

    }
  public:
    template< typename F, typename ...Fs>
    future_when_all_tuple_shared_state(values_tag, BOOST_THREAD_FWD_REF(F) f, BOOST_THREAD_FWD_REF(Fs) ... futures) :
      tup_(boost::csbl::make_tuple(boost::forward<F>(f), boost::forward<Fs>(futures)...))
    {
    }

    ~future_when_all_tuple_shared_state() {}

  };


  template <class Tuple, std::size_t i=csbl::tuple_size<Tuple>::value>
  struct apply_any_run_if_is_deferred_or_ready {
    bool operator ()(Tuple& t)
    {
      if (csbl::get<i-1>(t).run_if_is_deferred_or_ready()) return true;
      return apply_any_run_if_is_deferred_or_ready<Tuple,i-1>()(t);
    }
  };
  template <class Tuple>
  struct apply_any_run_if_is_deferred_or_ready<Tuple, 0> {
    bool operator ()(Tuple& )
    {
      return false;
    }
  };

  template< typename Tuple, typename T0, typename ...T >
  struct future_when_any_tuple_shared_state: future_async_shared_state_base<Tuple>
  {
    Tuple tup_;
    typedef typename make_tuple_indices<1+sizeof...(T)>::type Index;

    static void run(shared_ptr<boost::detail::shared_state_base> that_)
    {
      future_when_any_tuple_shared_state* that = static_cast<future_when_any_tuple_shared_state*>(that_.get());
      try {
        // TODO make use of apply(that->tup_, wait_for_any_fctr);
        that->wait_for_any(Index());

        that->mark_finished_with_result(boost::move(that->tup_));
      } catch(...) {
        that->mark_exceptional_finish();
      }
    }
    template <size_t ...Indices>
    void wait_for_any(tuple_indices<Indices...>) {
#if defined BOOST_THREAD_PROVIDES_INVOKE
      return invoke<void>(wait_for_any_fctr(), csbl::get<Indices>(tup_)...);
#else
      return wait_for_any_fctr()(csbl::get<Indices>(tup_)...);
#endif
    }
    bool run_deferred() {
      return apply_any_run_if_is_deferred_or_ready<Tuple>()(tup_);
    }
    void init() {
      if (run_deferred())
      {
        future_when_any_tuple_shared_state::run(this->shared_from_this());
        return;
      }

#ifdef BOOST_THREAD_FUTURE_BLOCKING
      this->thr_ = boost::thread(&future_when_any_tuple_shared_state::run, this->shared_from_this());
#else
      boost::thread(&future_when_any_tuple_shared_state::run, this->shared_from_this()).detach();
#endif
    }

  public:
    template< typename F, typename ...Fs>
    future_when_any_tuple_shared_state(values_tag,
        BOOST_THREAD_FWD_REF(F) f, BOOST_THREAD_FWD_REF(Fs) ... futures
    ) :
      tup_(boost::csbl::make_tuple(boost::forward<F>(f), boost::forward<Fs>(futures)...))
    {
    }

    ~future_when_any_tuple_shared_state() {}
  };
#endif

}

  template< typename InputIterator>
  typename boost::disable_if<is_future_type<InputIterator>,
    BOOST_THREAD_FUTURE<csbl::vector<typename InputIterator::value_type>  >
  >::type
  when_all(InputIterator first, InputIterator last) {
    typedef  typename InputIterator::value_type value_type;
    typedef  csbl::vector<value_type> container_type;
    typedef  detail::future_when_all_vector_shared_state<value_type> factory_type;

    if (first==last) return make_ready_future(container_type());
    shared_ptr<factory_type >
        h(new factory_type(detail::input_iterator_tag_value, first,last));
    h->init();
    return BOOST_THREAD_FUTURE<container_type>(h);
  }

  inline BOOST_THREAD_FUTURE<csbl::tuple<> > when_all() {
    return make_ready_future(csbl::tuple<>());
  }

#if ! defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)
  template< typename T0, typename ...T>
  BOOST_THREAD_FUTURE<csbl::tuple<typename decay<T0>::type, typename decay<T>::type...> >
  when_all(BOOST_THREAD_FWD_REF(T0) f, BOOST_THREAD_FWD_REF(T) ... futures) {
    typedef csbl::tuple<typename decay<T0>::type, typename decay<T>::type...> container_type;
    typedef detail::future_when_all_tuple_shared_state<container_type, typename decay<T0>::type, typename decay<T>::type...> factory_type;

    shared_ptr<factory_type>
        h(new factory_type(detail::values_tag_value, boost::forward<T0>(f), boost::forward<T>(futures)...));
    h->init();
    return BOOST_THREAD_FUTURE<container_type>(h);
  }
#endif

  template< typename InputIterator>
  typename boost::disable_if<is_future_type<InputIterator>,
    BOOST_THREAD_FUTURE<csbl::vector<typename InputIterator::value_type>  >
  >::type
  when_any(InputIterator first, InputIterator last) {
    typedef  typename InputIterator::value_type value_type;
    typedef  csbl::vector<value_type> container_type;
    typedef  detail::future_when_any_vector_shared_state<value_type> factory_type;

    if (first==last) return make_ready_future(container_type());
    shared_ptr<factory_type >
        h(new factory_type(detail::input_iterator_tag_value, first,last));
    h->init();
    return BOOST_THREAD_FUTURE<container_type>(h);
  }

  inline BOOST_THREAD_FUTURE<csbl::tuple<> > when_any() {
    return make_ready_future(csbl::tuple<>());
  }

#if ! defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)
  template< typename T0, typename ...T>
  BOOST_THREAD_FUTURE<csbl::tuple<typename decay<T0>::type, typename decay<T>::type...> >
  when_any(BOOST_THREAD_FWD_REF(T0) f, BOOST_THREAD_FWD_REF(T) ... futures) {
    typedef csbl::tuple<typename decay<T0>::type, typename decay<T>::type...> container_type;
    typedef detail::future_when_any_tuple_shared_state<container_type, typename decay<T0>::type, typename decay<T>::type...> factory_type;

    shared_ptr<factory_type>
        h(new factory_type(detail::values_tag_value, boost::forward<T0>(f), boost::forward<T>(futures)...));
    h->init();
    return BOOST_THREAD_FUTURE<container_type>(h);
  }
#endif
#endif // BOOST_THREAD_PROVIDES_FUTURE_WHEN_ALL_WHEN_ANY
}

#endif // BOOST_NO_EXCEPTIONS
#endif // header

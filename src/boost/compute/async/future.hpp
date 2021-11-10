//---------------------------------------------------------------------------//
// Copyright (c) 2013 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_ASYNC_FUTURE_HPP
#define BOOST_COMPUTE_ASYNC_FUTURE_HPP

#include <boost/compute/event.hpp>

namespace boost {
namespace compute {

/// \class future
/// \brief Holds the result of an asynchronous computation.
///
/// \see event, wait_list
template<class T>
class future
{
public:
    future()
        : m_event(0)
    {
    }

    future(const T &result, const event &event)
        : m_result(result),
          m_event(event)
    {
    }

    future(const future<T> &other)
        : m_result(other.m_result),
          m_event(other.m_event)
    {
    }

    future& operator=(const future<T> &other)
    {
        if(this != &other){
            m_result = other.m_result;
            m_event = other.m_event;
        }

        return *this;
    }

    ~future()
    {
    }

    /// Returns the result of the computation. This will block until
    /// the result is ready.
    T get()
    {
        wait();

        return m_result;
    }

    /// Returns \c true if the future is valid.
    bool valid() const
    {
        return m_event != 0;
    }

    /// Blocks until the computation is complete.
    void wait() const
    {
        m_event.wait();
    }

    /// Returns the underlying event object.
    event get_event() const
    {
        return m_event;
    }

    #if defined(BOOST_COMPUTE_CL_VERSION_1_1) || defined(BOOST_COMPUTE_DOXYGEN_INVOKED)
    /// Invokes a generic callback function once the future is ready.
    ///
    /// The function specified by callback must be invokable with zero arguments.
    ///
    /// \see_opencl_ref{clSetEventCallback}
    /// \opencl_version_warning{1,1}
    template<class Function>
    future& then(Function callback)
    {
        m_event.set_callback(callback);
        return *this;
    }
    #endif // BOOST_COMPUTE_CL_VERSION_1_1

private:
    T m_result;
    event m_event;
};

/// \internal_
template<>
class future<void>
{
public:
    future()
        : m_event(0)
    {
    }

    template<class T>
    future(const future<T> &other)
        : m_event(other.get_event())
    {
    }

    explicit future(const event &event)
        : m_event(event)
    {
    }

    template<class T>
    future<void> &operator=(const future<T> &other)
    {
        m_event = other.get_event();

        return *this;
    }

    future<void> &operator=(const future<void> &other)
    {
        if(this != &other){
            m_event = other.m_event;
        }

        return *this;
    }

    ~future()
    {
    }

    void get()
    {
        wait();
    }

    bool valid() const
    {
        return m_event != 0;
    }

    void wait() const
    {
        m_event.wait();
    }

    event get_event() const
    {
        return m_event;
    }

    #if defined(BOOST_COMPUTE_CL_VERSION_1_1) || defined(BOOST_COMPUTE_DOXYGEN_INVOKED)
    /// Invokes a generic callback function once the future is ready.
    ///
    /// The function specified by callback must be invokable with zero arguments.
    ///
    /// \see_opencl_ref{clSetEventCallback}
    /// \opencl_version_warning{1,1}
    template<class Function>
    future<void> &then(Function callback)
    {
        m_event.set_callback(callback);
        return *this;
    }
    #endif // BOOST_COMPUTE_CL_VERSION_1_1

private:
    event m_event;
};

/// \internal_
template<class Result>
inline future<Result> make_future(const Result &result, const event &event)
{
    return future<Result>(result, event);
}

} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_ASYNC_FUTURE_HPP

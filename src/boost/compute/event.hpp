//---------------------------------------------------------------------------//
// Copyright (c) 2013 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_EVENT_HPP
#define BOOST_COMPUTE_EVENT_HPP

#include <boost/function.hpp>

#include <boost/compute/config.hpp>
#include <boost/compute/exception.hpp>
#include <boost/compute/detail/duration.hpp>
#include <boost/compute/detail/get_object_info.hpp>
#include <boost/compute/detail/assert_cl_success.hpp>
#include <boost/compute/types/fundamental.hpp>

namespace boost {
namespace compute {

/// \class event
/// \brief An event corresponding to an operation on a compute device
///
/// Event objects are used to track operations running on the device (such as
/// kernel executions and memory transfers). Event objects are returned by the
/// various \c enqueue_* methods of the command_queue class.
///
/// Events can be used to synchronize operations between the host and the
/// device. The \c wait() method will block execution on the host until the
/// operation corresponding to the event on the device has completed. The
/// status of the operation can also be polled with the \c status() method.
///
/// Event objects can also be used for performance profiling. In order to use
/// events for profiling, the command queue must be constructed with the
/// \c CL_QUEUE_PROFILING_ENABLE flag. Then the \c duration() method can be
/// used to retrieve the total duration of the operation on the device:
/// \code
/// std::cout << "time = " << e.duration<std::chrono::milliseconds>().count() << "ms\n";
/// \endcode
///
/// \see \ref future "future<T>", wait_list
class event
{
public:
    /// \internal_
    enum execution_status {
        complete = CL_COMPLETE,
        running = CL_RUNNING,
        submitted = CL_SUBMITTED,
        queued = CL_QUEUED
    };

    /// \internal_
    enum command_type {
        ndrange_kernel = CL_COMMAND_NDRANGE_KERNEL,
        task = CL_COMMAND_TASK,
        native_kernel = CL_COMMAND_NATIVE_KERNEL,
        read_buffer = CL_COMMAND_READ_BUFFER,
        write_buffer = CL_COMMAND_WRITE_BUFFER,
        copy_buffer = CL_COMMAND_COPY_BUFFER,
        read_image = CL_COMMAND_READ_IMAGE,
        write_image = CL_COMMAND_WRITE_IMAGE,
        copy_image = CL_COMMAND_COPY_IMAGE,
        copy_image_to_buffer = CL_COMMAND_COPY_IMAGE_TO_BUFFER,
        copy_buffer_to_image = CL_COMMAND_COPY_BUFFER_TO_IMAGE,
        map_buffer = CL_COMMAND_MAP_BUFFER,
        map_image = CL_COMMAND_MAP_IMAGE,
        unmap_mem_object = CL_COMMAND_UNMAP_MEM_OBJECT,
        marker = CL_COMMAND_MARKER,
        aquire_gl_objects = CL_COMMAND_ACQUIRE_GL_OBJECTS,
        release_gl_object = CL_COMMAND_RELEASE_GL_OBJECTS
        #if defined(BOOST_COMPUTE_CL_VERSION_1_1)
        ,
        read_buffer_rect = CL_COMMAND_READ_BUFFER_RECT,
        write_buffer_rect = CL_COMMAND_WRITE_BUFFER_RECT,
        copy_buffer_rect = CL_COMMAND_COPY_BUFFER_RECT
        #endif
    };

    /// \internal_
    enum profiling_info {
        profiling_command_queued = CL_PROFILING_COMMAND_QUEUED,
        profiling_command_submit = CL_PROFILING_COMMAND_SUBMIT,
        profiling_command_start = CL_PROFILING_COMMAND_START,
        profiling_command_end = CL_PROFILING_COMMAND_END
    };

    /// Creates a null event object.
    event()
        : m_event(0)
    {
    }

    explicit event(cl_event event, bool retain = true)
        : m_event(event)
    {
        if(m_event && retain){
            clRetainEvent(event);
        }
    }

    /// Makes a new event as a copy of \p other.
    event(const event &other)
        : m_event(other.m_event)
    {
        if(m_event){
            clRetainEvent(m_event);
        }
    }

    /// Copies the event object from \p other to \c *this.
    event& operator=(const event &other)
    {
        if(this != &other){
            if(m_event){
                clReleaseEvent(m_event);
            }

            m_event = other.m_event;

            if(m_event){
                clRetainEvent(m_event);
            }
        }

        return *this;
    }

    #ifndef BOOST_COMPUTE_NO_RVALUE_REFERENCES
    /// Move-constructs a new event object from \p other.
    event(event&& other) BOOST_NOEXCEPT
        : m_event(other.m_event)
    {
        other.m_event = 0;
    }

    /// Move-assigns the event from \p other to \c *this.
    event& operator=(event&& other) BOOST_NOEXCEPT
    {
        if(m_event){
            clReleaseEvent(m_event);
        }

        m_event = other.m_event;
        other.m_event = 0;

        return *this;
    }
    #endif // BOOST_COMPUTE_NO_RVALUE_REFERENCES

    /// Destroys the event object.
    ~event()
    {
        if(m_event){
            BOOST_COMPUTE_ASSERT_CL_SUCCESS(
                clReleaseEvent(m_event)
            );
        }
    }

    /// Returns a reference to the underlying OpenCL event object.
    cl_event& get() const
    {
        return const_cast<cl_event &>(m_event);
    }

    /// Returns the status of the event.
    cl_int status() const
    {
        return get_info<cl_int>(CL_EVENT_COMMAND_EXECUTION_STATUS);
    }

    /// Returns the command type for the event.
    cl_command_type get_command_type() const
    {
        return get_info<cl_command_type>(CL_EVENT_COMMAND_TYPE);
    }

    /// Returns information about the event.
    ///
    /// \see_opencl_ref{clGetEventInfo}
    template<class T>
    T get_info(cl_event_info info) const
    {
        return detail::get_object_info<T>(clGetEventInfo, m_event, info);
    }

    /// \overload
    template<int Enum>
    typename detail::get_object_info_type<event, Enum>::type
    get_info() const;

    /// Returns profiling information for the event.
    ///
    /// \see event::duration()
    ///
    /// \see_opencl_ref{clGetEventProfilingInfo}
    template<class T>
    T get_profiling_info(cl_profiling_info info) const
    {
        return detail::get_object_info<T>(clGetEventProfilingInfo,
                                          m_event,
                                          info);
    }

    /// Blocks until the actions corresponding to the event have
    /// completed.
    void wait() const
    {
        cl_int ret = clWaitForEvents(1, &m_event);
        if(ret != CL_SUCCESS){
            BOOST_THROW_EXCEPTION(opencl_error(ret));
        }
    }

    #if defined(BOOST_COMPUTE_CL_VERSION_1_1) || defined(BOOST_COMPUTE_DOXYGEN_INVOKED)
    /// Registers a function to be called when the event status changes to
    /// \p status (by default CL_COMPLETE). The callback is passed the OpenCL
    /// event object, the event status, and a pointer to arbitrary user data.
    ///
    /// \see_opencl_ref{clSetEventCallback}
    ///
    /// \opencl_version_warning{1,1}
    void set_callback(void (BOOST_COMPUTE_CL_CALLBACK *callback)(
                          cl_event event, cl_int status, void *user_data
                      ),
                      cl_int status = CL_COMPLETE,
                      void *user_data = 0)
    {
        cl_int ret = clSetEventCallback(m_event, status, callback, user_data);
        if(ret != CL_SUCCESS){
            BOOST_THROW_EXCEPTION(opencl_error(ret));
        }
    }

    /// Registers a generic function to be called when the event status
    /// changes to \p status (by default \c CL_COMPLETE).
    ///
    /// The function specified by \p callback must be invokable with zero
    /// arguments (e.g. \c callback()).
    ///
    /// \opencl_version_warning{1,1}
    template<class Function>
    void set_callback(Function callback, cl_int status = CL_COMPLETE)
    {
        set_callback(
            event_callback_invoker,
            status,
            new boost::function<void()>(callback)
        );
    }
    #endif // BOOST_COMPUTE_CL_VERSION_1_1

    /// Returns the total duration of the event from \p start to \p end.
    ///
    /// For example, to print the number of milliseconds the event took to
    /// execute:
    /// \code
    /// std::cout << event.duration<std::chrono::milliseconds>().count() << " ms" << std::endl;
    /// \endcode
    ///
    /// \see event::get_profiling_info()
    template<class Duration>
    Duration duration(cl_profiling_info start = CL_PROFILING_COMMAND_START,
                      cl_profiling_info end = CL_PROFILING_COMMAND_END) const
    {
        const ulong_ nanoseconds =
            get_profiling_info<ulong_>(end) - get_profiling_info<ulong_>(start);

        return detail::make_duration_from_nanoseconds(Duration(), nanoseconds);
    }

    /// Returns \c true if the event is the same as \p other.
    bool operator==(const event &other) const
    {
        return m_event == other.m_event;
    }

    /// Returns \c true if the event is different from \p other.
    bool operator!=(const event &other) const
    {
        return m_event != other.m_event;
    }

    /// \internal_
    operator cl_event() const
    {
        return m_event;
    }

    /// \internal_ (deprecated)
    cl_int get_status() const
    {
        return status();
    }

private:
    #ifdef BOOST_COMPUTE_CL_VERSION_1_1
    /// \internal_
    static void BOOST_COMPUTE_CL_CALLBACK
    event_callback_invoker(cl_event, cl_int, void *user_data)
    {
        boost::function<void()> *callback =
            static_cast<boost::function<void()> *>(user_data);

        (*callback)();

        delete callback;
    }
    #endif // BOOST_COMPUTE_CL_VERSION_1_1

protected:
    cl_event m_event;
};

/// \internal_ define get_info() specializations for event
BOOST_COMPUTE_DETAIL_DEFINE_GET_INFO_SPECIALIZATIONS(event,
    ((cl_command_queue, CL_EVENT_COMMAND_QUEUE))
    ((cl_command_type, CL_EVENT_COMMAND_TYPE))
    ((cl_int, CL_EVENT_COMMAND_EXECUTION_STATUS))
    ((cl_uint, CL_EVENT_REFERENCE_COUNT))
)

#ifdef BOOST_COMPUTE_CL_VERSION_1_1
BOOST_COMPUTE_DETAIL_DEFINE_GET_INFO_SPECIALIZATIONS(event,
    ((cl_context, CL_EVENT_CONTEXT))
)
#endif

} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_EVENT_HPP

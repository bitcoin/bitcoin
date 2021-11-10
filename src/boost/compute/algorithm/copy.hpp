//---------------------------------------------------------------------------//
// Copyright (c) 2013 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_ALGORITHM_COPY_HPP
#define BOOST_COMPUTE_ALGORITHM_COPY_HPP

#include <algorithm>
#include <iterator>

#include <boost/utility/enable_if.hpp>

#include <boost/mpl/and.hpp>
#include <boost/mpl/not.hpp>
#include <boost/mpl/or.hpp>

#include <boost/compute/buffer.hpp>
#include <boost/compute/system.hpp>
#include <boost/compute/command_queue.hpp>
#include <boost/compute/algorithm/detail/copy_on_device.hpp>
#include <boost/compute/algorithm/detail/copy_to_device.hpp>
#include <boost/compute/algorithm/detail/copy_to_host.hpp>
#include <boost/compute/async/future.hpp>
#include <boost/compute/container/mapped_view.hpp>
#include <boost/compute/detail/device_ptr.hpp>
#include <boost/compute/detail/is_contiguous_iterator.hpp>
#include <boost/compute/detail/iterator_range_size.hpp>
#include <boost/compute/detail/parameter_cache.hpp>
#include <boost/compute/iterator/buffer_iterator.hpp>
#include <boost/compute/type_traits/type_name.hpp>
#include <boost/compute/type_traits/is_device_iterator.hpp>

namespace boost {
namespace compute {
namespace detail {

namespace mpl = boost::mpl;

// meta-function returning true if copy() between InputIterator and
// OutputIterator can be implemented with clEnqueueCopyBuffer().
template<class InputIterator, class OutputIterator>
struct can_copy_with_copy_buffer :
    mpl::and_<
        mpl::or_<
            boost::is_same<
                InputIterator,
                buffer_iterator<typename InputIterator::value_type>
            >,
            boost::is_same<
                InputIterator,
                detail::device_ptr<typename InputIterator::value_type>
            >
        >,
        mpl::or_<
            boost::is_same<
                OutputIterator,
                buffer_iterator<typename OutputIterator::value_type>
            >,
            boost::is_same<
                OutputIterator,
                detail::device_ptr<typename OutputIterator::value_type>
            >
        >,
        boost::is_same<
            typename InputIterator::value_type,
            typename OutputIterator::value_type
        >
    >::type {};

// meta-function returning true if value_types of HostIterator and
// DeviceIterator are same
template<class HostIterator, class DeviceIterator>
struct is_same_value_type :
    boost::is_same<
        typename boost::remove_cv<
            typename std::iterator_traits<HostIterator>::value_type
        >::type,
        typename boost::remove_cv<
            typename DeviceIterator::value_type
        >::type
    >::type {};

// meta-function returning true if value_type of HostIterator is bool
template<class HostIterator>
struct is_bool_value_type :
    boost::is_same<
        typename boost::remove_cv<
            typename std::iterator_traits<HostIterator>::value_type
        >::type,
        bool
    >::type {};

// host -> device (async)
template<class InputIterator, class OutputIterator>
inline future<OutputIterator>
dispatch_copy_async(InputIterator first,
                    InputIterator last,
                    OutputIterator result,
                    command_queue &queue,
                    const wait_list &events,
                    typename boost::enable_if<
                        mpl::and_<
                            mpl::not_<
                                is_device_iterator<InputIterator>
                            >,
                            is_device_iterator<OutputIterator>,
                            is_same_value_type<InputIterator, OutputIterator>
                        >
                    >::type* = 0)
{
    BOOST_STATIC_ASSERT_MSG(
        is_contiguous_iterator<InputIterator>::value,
        "copy_async() is only supported for contiguous host iterators"
    );

    return copy_to_device_async(first, last, result, queue, events);
}

// host -> device (async)
// Type mismatch between InputIterator and OutputIterator value_types
template<class InputIterator, class OutputIterator>
inline future<OutputIterator>
dispatch_copy_async(InputIterator first,
                    InputIterator last,
                    OutputIterator result,
                    command_queue &queue,
                    const wait_list &events,
                    typename boost::enable_if<
                        mpl::and_<
                            mpl::not_<
                                is_device_iterator<InputIterator>
                            >,
                            is_device_iterator<OutputIterator>,
                            mpl::not_<
                                is_same_value_type<InputIterator, OutputIterator>
                            >
                        >
                    >::type* = 0)
{
    BOOST_STATIC_ASSERT_MSG(
        is_contiguous_iterator<InputIterator>::value,
        "copy_async() is only supported for contiguous host iterators"
    );

    typedef typename std::iterator_traits<InputIterator>::value_type input_type;

    const context &context = queue.get_context();
    size_t count = iterator_range_size(first, last);

    if(count < size_t(1)) {
        return future<OutputIterator>();
    }

    // map [first; last) to device and run copy kernel
    // on device for copying & casting
    ::boost::compute::mapped_view<input_type> mapped_host(
        // make sure it's a pointer to constant data
        // to force read only mapping
        const_cast<const input_type*>(
            ::boost::addressof(*first)
        ),
        count,
        context
    );
    return copy_on_device_async(
        mapped_host.begin(), mapped_host.end(), result, queue, events
    );
}

// host -> device
// InputIterator is a contiguous iterator
template<class InputIterator, class OutputIterator>
inline OutputIterator
dispatch_copy(InputIterator first,
              InputIterator last,
              OutputIterator result,
              command_queue &queue,
              const wait_list &events,
              typename boost::enable_if<
                  mpl::and_<
                      mpl::not_<
                          is_device_iterator<InputIterator>
                      >,
                      is_device_iterator<OutputIterator>,
                      is_same_value_type<InputIterator, OutputIterator>,
                      is_contiguous_iterator<InputIterator>
                  >
              >::type* = 0)
{
    return copy_to_device(first, last, result, queue, events);
}

// host -> device
// Type mismatch between InputIterator and OutputIterator value_types
// InputIterator is a contiguous iterator
template<class InputIterator, class OutputIterator>
inline OutputIterator
dispatch_copy(InputIterator first,
              InputIterator last,
              OutputIterator result,
              command_queue &queue,
              const wait_list &events,
              typename boost::enable_if<
                  mpl::and_<
                      mpl::not_<
                          is_device_iterator<InputIterator>
                      >,
                      is_device_iterator<OutputIterator>,
                      mpl::not_<
                          is_same_value_type<InputIterator, OutputIterator>
                      >,
                      is_contiguous_iterator<InputIterator>
                  >
              >::type* = 0)
{
    typedef typename OutputIterator::value_type output_type;
    typedef typename std::iterator_traits<InputIterator>::value_type input_type;

    const device &device = queue.get_device();

    // loading parameters
    std::string cache_key =
        std::string("__boost_compute_copy_to_device_")
            + type_name<input_type>() + "_" + type_name<output_type>();
    boost::shared_ptr<parameter_cache> parameters =
        detail::parameter_cache::get_global_cache(device);

    uint_ map_copy_threshold;
    uint_ direct_copy_threshold;

    // calculate default values of thresholds
    if (device.type() & device::gpu) {
        // GPUs
        map_copy_threshold = 524288;  // 0.5 MB
        direct_copy_threshold = 52428800; // 50 MB
    }
    else {
        // CPUs and other devices
        map_copy_threshold = 134217728; // 128 MB
        direct_copy_threshold = 0; // it's never efficient for CPUs
    }

    // load thresholds
    map_copy_threshold =
        parameters->get(
            cache_key, "map_copy_threshold", map_copy_threshold
        );
    direct_copy_threshold =
        parameters->get(
            cache_key, "direct_copy_threshold", direct_copy_threshold
        );

    // select copy method based on thresholds & input_size_bytes
    size_t count = iterator_range_size(first, last);
    size_t input_size_bytes = count * sizeof(input_type);

    // [0; map_copy_threshold) -> copy_to_device_map()
    if(input_size_bytes < map_copy_threshold) {
        return copy_to_device_map(first, last, result, queue, events);
    }
    // [map_copy_threshold; direct_copy_threshold) -> convert [first; last)
    //     on host and then perform copy_to_device()
    else if(input_size_bytes < direct_copy_threshold) {
        std::vector<output_type> vector(first, last);
        return copy_to_device(
            vector.begin(), vector.end(), result, queue, events
        );
    }

    // [direct_copy_threshold; inf) -> map [first; last) to device and
    //     run copy kernel on device for copying & casting
    // At this point we are sure that count > 1 (first != last).

    // Perform async copy to device, wait for it to be finished and
    // return the result.
    // At this point we are sure that count > 1 (first != last), so event
    // returned by dispatch_copy_async() must be valid.
    return dispatch_copy_async(first, last, result, queue, events).get();
}

// host -> device
// InputIterator is NOT a contiguous iterator
template<class InputIterator, class OutputIterator>
inline OutputIterator
dispatch_copy(InputIterator first,
              InputIterator last,
              OutputIterator result,
              command_queue &queue,
              const wait_list &events,
              typename boost::enable_if<
                  mpl::and_<
                      mpl::not_<
                          is_device_iterator<InputIterator>
                      >,
                      is_device_iterator<OutputIterator>,
                      mpl::not_<
                          is_contiguous_iterator<InputIterator>
                      >
                  >
              >::type* = 0)
{
    typedef typename OutputIterator::value_type output_type;
    typedef typename std::iterator_traits<InputIterator>::value_type input_type;

    const device &device = queue.get_device();

    // loading parameters
    std::string cache_key =
        std::string("__boost_compute_copy_to_device_")
            + type_name<input_type>() + "_" + type_name<output_type>();
    boost::shared_ptr<parameter_cache> parameters =
        detail::parameter_cache::get_global_cache(device);

    uint_ map_copy_threshold;
    uint_ direct_copy_threshold;

    // calculate default values of thresholds
    if (device.type() & device::gpu) {
        // GPUs
        map_copy_threshold = 524288;  // 0.5 MB
        direct_copy_threshold = 52428800; // 50 MB
    }
    else {
        // CPUs and other devices
        map_copy_threshold = 134217728; // 128 MB
        direct_copy_threshold = 0; // it's never efficient for CPUs
    }

    // load thresholds
    map_copy_threshold =
        parameters->get(
            cache_key, "map_copy_threshold", map_copy_threshold
        );
    direct_copy_threshold =
        parameters->get(
            cache_key, "direct_copy_threshold", direct_copy_threshold
        );

    // select copy method based on thresholds & input_size_bytes
    size_t input_size = iterator_range_size(first, last);
    size_t input_size_bytes = input_size * sizeof(input_type);

    // [0; map_copy_threshold) -> copy_to_device_map()
    //
    // if direct_copy_threshold is less than map_copy_threshold
    // copy_to_device_map() is used for every input
    if(input_size_bytes < map_copy_threshold
        || direct_copy_threshold <= map_copy_threshold) {
        return copy_to_device_map(first, last, result, queue, events);
    }
    // [map_copy_threshold; inf) -> convert [first; last)
    //     on host and then perform copy_to_device()
    std::vector<output_type> vector(first, last);
    return copy_to_device(vector.begin(), vector.end(), result, queue, events);
}

// device -> host (async)
template<class InputIterator, class OutputIterator>
inline future<OutputIterator>
dispatch_copy_async(InputIterator first,
                    InputIterator last,
                    OutputIterator result,
                    command_queue &queue,
                    const wait_list &events,
                    typename boost::enable_if<
                        mpl::and_<
                            is_device_iterator<InputIterator>,
                            mpl::not_<
                                is_device_iterator<OutputIterator>
                            >,
                            is_same_value_type<OutputIterator, InputIterator>
                        >
                    >::type* = 0)
{
    BOOST_STATIC_ASSERT_MSG(
        is_contiguous_iterator<OutputIterator>::value,
        "copy_async() is only supported for contiguous host iterators"
    );

    return copy_to_host_async(first, last, result, queue, events);
}

// device -> host (async)
// Type mismatch between InputIterator and OutputIterator value_types
template<class InputIterator, class OutputIterator>
inline future<OutputIterator>
dispatch_copy_async(InputIterator first,
                    InputIterator last,
                    OutputIterator result,
                    command_queue &queue,
                    const wait_list &events,
                    typename boost::enable_if<
                        mpl::and_<
                            is_device_iterator<InputIterator>,
                            mpl::not_<
                                is_device_iterator<OutputIterator>
                            >,
                            mpl::not_<
                                is_same_value_type<OutputIterator, InputIterator>
                            >
                        >
                    >::type* = 0)
{
    BOOST_STATIC_ASSERT_MSG(
        is_contiguous_iterator<OutputIterator>::value,
        "copy_async() is only supported for contiguous host iterators"
    );

    typedef typename std::iterator_traits<OutputIterator>::value_type output_type;
    const context &context = queue.get_context();
    size_t count = iterator_range_size(first, last);

    if(count < size_t(1)) {
        return future<OutputIterator>();
    }

    // map host memory to device
    buffer mapped_host(
        context,
        count * sizeof(output_type),
        buffer::write_only | buffer::use_host_ptr,
        static_cast<void*>(
            ::boost::addressof(*result)
        )
    );
    // copy async on device
    ::boost::compute::future<buffer_iterator<output_type> > future =
        copy_on_device_async(
            first,
            last,
            make_buffer_iterator<output_type>(mapped_host),
            queue,
            events
        );
    // update host memory asynchronously by maping and unmaping memory
    event map_event;
    void* ptr = queue.enqueue_map_buffer_async(
        mapped_host,
        CL_MAP_READ,
        0,
        count * sizeof(output_type),
        map_event,
        future.get_event()
    );
    event unmap_event =
        queue.enqueue_unmap_buffer(mapped_host, ptr, map_event);
    return make_future(result + count, unmap_event);
}

// device -> host
// OutputIterator is a contiguous iterator
template<class InputIterator, class OutputIterator>
inline OutputIterator
dispatch_copy(InputIterator first,
              InputIterator last,
              OutputIterator result,
              command_queue &queue,
              const wait_list &events,
              typename boost::enable_if<
                  mpl::and_<
                      is_device_iterator<InputIterator>,
                      mpl::not_<
                          is_device_iterator<OutputIterator>
                      >,
                      is_same_value_type<OutputIterator, InputIterator>,
                      is_contiguous_iterator<OutputIterator>,
                      mpl::not_<
                          is_bool_value_type<OutputIterator>
                      >
                  >
              >::type* = 0)
{
    return copy_to_host(first, last, result, queue, events);
}

// device -> host
// Type mismatch between InputIterator and OutputIterator value_types
// OutputIterator is NOT a contiguous iterator or value_type of OutputIterator
// is a boolean type.
template<class InputIterator, class OutputIterator>
inline OutputIterator
dispatch_copy(InputIterator first,
              InputIterator last,
              OutputIterator result,
              command_queue &queue,
              const wait_list &events,
              typename boost::enable_if<
                  mpl::and_<
                      is_device_iterator<InputIterator>,
                      mpl::not_<
                          is_device_iterator<OutputIterator>
                      >,
                      mpl::or_<
                          mpl::not_<
                              is_contiguous_iterator<OutputIterator>
                          >,
                          is_bool_value_type<OutputIterator>
                      >
                  >
              >::type* = 0)
{
    typedef typename std::iterator_traits<OutputIterator>::value_type output_type;
    typedef typename InputIterator::value_type input_type;

    const device &device = queue.get_device();

    // loading parameters
    std::string cache_key =
        std::string("__boost_compute_copy_to_host_")
            + type_name<input_type>() + "_" + type_name<output_type>();
    boost::shared_ptr<parameter_cache> parameters =
        detail::parameter_cache::get_global_cache(device);

    uint_ map_copy_threshold;
    uint_ direct_copy_threshold;

    // calculate default values of thresholds
    if (device.type() & device::gpu) {
        // GPUs
        map_copy_threshold = 33554432;  // 30 MB
        direct_copy_threshold = 0; // it's never efficient for GPUs
    }
    else {
        // CPUs and other devices
        map_copy_threshold = 134217728; // 128 MB
        direct_copy_threshold = 0; // it's never efficient for CPUs
    }

    // load thresholds
    map_copy_threshold =
        parameters->get(
            cache_key, "map_copy_threshold", map_copy_threshold
        );
    direct_copy_threshold =
        parameters->get(
            cache_key, "direct_copy_threshold", direct_copy_threshold
        );

    // select copy method based on thresholds & input_size_bytes
    size_t count = iterator_range_size(first, last);
    size_t input_size_bytes = count * sizeof(input_type);

    // [0; map_copy_threshold) -> copy_to_host_map()
    //
    // if direct_copy_threshold is less than map_copy_threshold
    // copy_to_host_map() is used for every input
    if(input_size_bytes < map_copy_threshold
        || direct_copy_threshold <= map_copy_threshold) {
        return copy_to_host_map(first, last, result, queue, events);
    }
    // [map_copy_threshold; inf) -> copy [first;last) to temporary vector
    //     then copy (and convert) to result using std::copy()
    std::vector<input_type> vector(count);
    copy_to_host(first, last, vector.begin(), queue, events);
    return std::copy(vector.begin(), vector.end(), result);
}

// device -> host
// Type mismatch between InputIterator and OutputIterator value_types
// OutputIterator is a contiguous iterator
// value_type of OutputIterator is NOT a boolean type
template<class InputIterator, class OutputIterator>
inline OutputIterator
dispatch_copy(InputIterator first,
              InputIterator last,
              OutputIterator result,
              command_queue &queue,
              const wait_list &events,
              typename boost::enable_if<
                  mpl::and_<
                      is_device_iterator<InputIterator>,
                      mpl::not_<
                          is_device_iterator<OutputIterator>
                      >,
                      mpl::not_<
                          is_same_value_type<OutputIterator, InputIterator>
                      >,
                      is_contiguous_iterator<OutputIterator>,
                      mpl::not_<
                          is_bool_value_type<OutputIterator>
                      >
                  >
              >::type* = 0)
{
    typedef typename std::iterator_traits<OutputIterator>::value_type output_type;
    typedef typename InputIterator::value_type input_type;

    const device &device = queue.get_device();

    // loading parameters
    std::string cache_key =
        std::string("__boost_compute_copy_to_host_")
            + type_name<input_type>() + "_" + type_name<output_type>();
    boost::shared_ptr<parameter_cache> parameters =
        detail::parameter_cache::get_global_cache(device);

    uint_ map_copy_threshold;
    uint_ direct_copy_threshold;

    // calculate default values of thresholds
    if (device.type() & device::gpu) {
        // GPUs
        map_copy_threshold = 524288;  // 0.5 MB
        direct_copy_threshold = 52428800; // 50 MB
    }
    else {
        // CPUs and other devices
        map_copy_threshold = 134217728; // 128 MB
        direct_copy_threshold = 0; // it's never efficient for CPUs
    }

    // load thresholds
    map_copy_threshold =
        parameters->get(
            cache_key, "map_copy_threshold", map_copy_threshold
        );
    direct_copy_threshold =
        parameters->get(
            cache_key, "direct_copy_threshold", direct_copy_threshold
        );

    // select copy method based on thresholds & input_size_bytes
    size_t count = iterator_range_size(first, last);
    size_t input_size_bytes = count * sizeof(input_type);

    // [0; map_copy_threshold) -> copy_to_host_map()
    if(input_size_bytes < map_copy_threshold) {
        return copy_to_host_map(first, last, result, queue, events);
    }
    // [map_copy_threshold; direct_copy_threshold) -> copy [first;last) to
    //     temporary vector then copy (and convert) to result using std::copy()
    else if(input_size_bytes < direct_copy_threshold) {
        std::vector<input_type> vector(count);
        copy_to_host(first, last, vector.begin(), queue, events);
        return std::copy(vector.begin(), vector.end(), result);
    }

    // [direct_copy_threshold; inf) -> map [result; result + input_size) to
    //     device and run copy kernel on device for copying & casting
    // map host memory to device.

    // Perform async copy to host, wait for it to be finished and
    // return the result.
    // At this point we are sure that count > 1 (first != last), so event
    // returned by dispatch_copy_async() must be valid.
    return dispatch_copy_async(first, last, result, queue, events).get();
}

// device -> device
template<class InputIterator, class OutputIterator>
inline OutputIterator
dispatch_copy(InputIterator first,
              InputIterator last,
              OutputIterator result,
              command_queue &queue,
              const wait_list &events,
              typename boost::enable_if<
                  mpl::and_<
                      is_device_iterator<InputIterator>,
                      is_device_iterator<OutputIterator>,
                      mpl::not_<
                          can_copy_with_copy_buffer<
                              InputIterator, OutputIterator
                          >
                      >
                  >
              >::type* = 0)
{
    return copy_on_device(first, last, result, queue, events);
}

// device -> device (specialization for buffer iterators)
template<class InputIterator, class OutputIterator>
inline OutputIterator
dispatch_copy(InputIterator first,
              InputIterator last,
              OutputIterator result,
              command_queue &queue,
              const wait_list &events,
              typename boost::enable_if<
                  mpl::and_<
                      is_device_iterator<InputIterator>,
                      is_device_iterator<OutputIterator>,
                      can_copy_with_copy_buffer<
                          InputIterator, OutputIterator
                      >
                  >
              >::type* = 0)
{
    typedef typename std::iterator_traits<InputIterator>::value_type value_type;
    typedef typename std::iterator_traits<InputIterator>::difference_type difference_type;

    difference_type n = std::distance(first, last);
    if(n < 1){
        // nothing to copy
        return result;
    }

    queue.enqueue_copy_buffer(first.get_buffer(),
                              result.get_buffer(),
                              first.get_index() * sizeof(value_type),
                              result.get_index() * sizeof(value_type),
                              static_cast<size_t>(n) * sizeof(value_type),
                              events);
    return result + n;
}

// device -> device (async)
template<class InputIterator, class OutputIterator>
inline future<OutputIterator>
dispatch_copy_async(InputIterator first,
                    InputIterator last,
                    OutputIterator result,
                    command_queue &queue,
                    const wait_list &events,
                    typename boost::enable_if<
                        mpl::and_<
                            is_device_iterator<InputIterator>,
                            is_device_iterator<OutputIterator>,
                            mpl::not_<
                                can_copy_with_copy_buffer<
                                    InputIterator, OutputIterator
                                >
                            >
                        >
                    >::type* = 0)
{
    return copy_on_device_async(first, last, result, queue, events);
}

// device -> device (async, specialization for buffer iterators)
template<class InputIterator, class OutputIterator>
inline future<OutputIterator>
dispatch_copy_async(InputIterator first,
                    InputIterator last,
                    OutputIterator result,
                    command_queue &queue,
                    const wait_list &events,
                    typename boost::enable_if<
                        mpl::and_<
                            is_device_iterator<InputIterator>,
                            is_device_iterator<OutputIterator>,
                            can_copy_with_copy_buffer<
                                InputIterator, OutputIterator
                            >
                        >
                    >::type* = 0)
{
    typedef typename std::iterator_traits<InputIterator>::value_type value_type;
    typedef typename std::iterator_traits<InputIterator>::difference_type difference_type;

    difference_type n = std::distance(first, last);
    if(n < 1){
        // nothing to copy
        return make_future(result, event());
    }

    event event_ =
        queue.enqueue_copy_buffer(
            first.get_buffer(),
            result.get_buffer(),
            first.get_index() * sizeof(value_type),
            result.get_index() * sizeof(value_type),
            static_cast<size_t>(n) * sizeof(value_type),
            events
        );

    return make_future(result + n, event_);
}

// host -> host
template<class InputIterator, class OutputIterator>
inline OutputIterator
dispatch_copy(InputIterator first,
              InputIterator last,
              OutputIterator result,
              command_queue &queue,
              const wait_list &events,
              typename boost::enable_if_c<
                  !is_device_iterator<InputIterator>::value &&
                  !is_device_iterator<OutputIterator>::value
              >::type* = 0)
{
    (void) queue;
    (void) events;

    return std::copy(first, last, result);
}

} // end detail namespace

/// Copies the values in the range [\p first, \p last) to the range
/// beginning at \p result.
///
/// The generic copy() function can be used for a variety of data
/// transfer tasks and provides a standard interface to the following
/// OpenCL functions:
///
/// \li \c clEnqueueReadBuffer()
/// \li \c clEnqueueWriteBuffer()
/// \li \c clEnqueueCopyBuffer()
///
/// Unlike the aforementioned OpenCL functions, copy() will also work
/// with non-contiguous data-structures (e.g. \c std::list<T>) as
/// well as with "fancy" iterators (e.g. transform_iterator).
///
/// \param first first element in the range to copy
/// \param last last element in the range to copy
/// \param result first element in the result range
/// \param queue command queue to perform the operation
///
/// \return \c OutputIterator to the end of the result range
///
/// For example, to copy an array of \c int values on the host to a vector on
/// the device:
/// \code
/// // array on the host
/// int data[] = { 1, 2, 3, 4 };
///
/// // vector on the device
/// boost::compute::vector<int> vec(4, context);
///
/// // copy values to the device vector
/// boost::compute::copy(data, data + 4, vec.begin(), queue);
/// \endcode
///
/// The copy algorithm can also be used with standard containers such as
/// \c std::vector<T>:
/// \code
/// std::vector<int> host_vector = ...
/// boost::compute::vector<int> device_vector = ...
///
/// // copy from the host to the device
/// boost::compute::copy(
///     host_vector.begin(), host_vector.end(), device_vector.begin(), queue
/// );
///
/// // copy from the device to the host
/// boost::compute::copy(
///     device_vector.begin(), device_vector.end(), host_vector.begin(), queue
/// );
/// \endcode
///
/// Space complexity: \Omega(1)
///
/// \see copy_n(), copy_if(), copy_async()
template<class InputIterator, class OutputIterator>
inline OutputIterator copy(InputIterator first,
                           InputIterator last,
                           OutputIterator result,
                           command_queue &queue = system::default_queue(),
                           const wait_list &events = wait_list())
{
    return detail::dispatch_copy(first, last, result, queue, events);
}

/// Copies the values in the range [\p first, \p last) to the range
/// beginning at \p result. The copy is performed asynchronously.
///
/// \see copy()
template<class InputIterator, class OutputIterator>
inline future<OutputIterator>
copy_async(InputIterator first,
           InputIterator last,
           OutputIterator result,
           command_queue &queue = system::default_queue(),
           const wait_list &events = wait_list())
{
    return detail::dispatch_copy_async(first, last, result, queue, events);
}

} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_ALGORITHM_COPY_HPP

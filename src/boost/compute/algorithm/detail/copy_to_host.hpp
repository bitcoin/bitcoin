//---------------------------------------------------------------------------//
// Copyright (c) 2013 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_ALGORITHM_DETAIL_COPY_TO_HOST_HPP
#define BOOST_COMPUTE_ALGORITHM_DETAIL_COPY_TO_HOST_HPP

#include <iterator>

#include <boost/utility/addressof.hpp>

#include <boost/compute/command_queue.hpp>
#include <boost/compute/async/future.hpp>
#include <boost/compute/iterator/buffer_iterator.hpp>
#include <boost/compute/memory/svm_ptr.hpp>
#include <boost/compute/detail/iterator_plus_distance.hpp>

namespace boost {
namespace compute {
namespace detail {

template<class DeviceIterator, class HostIterator>
inline HostIterator copy_to_host(DeviceIterator first,
                                 DeviceIterator last,
                                 HostIterator result,
                                 command_queue &queue,
                                 const wait_list &events)
{
    typedef typename
        std::iterator_traits<DeviceIterator>::value_type
        value_type;

    size_t count = iterator_range_size(first, last);
    if(count == 0){
        return result;
    }

    const buffer &buffer = first.get_buffer();
    size_t offset = first.get_index();

    queue.enqueue_read_buffer(buffer,
                              offset * sizeof(value_type),
                              count * sizeof(value_type),
                              ::boost::addressof(*result),
                              events);

    return iterator_plus_distance(result, count);
}

template<class DeviceIterator, class HostIterator>
inline HostIterator copy_to_host_map(DeviceIterator first,
                                     DeviceIterator last,
                                     HostIterator result,
                                     command_queue &queue,
                                     const wait_list &events)
{
    typedef typename
        std::iterator_traits<DeviceIterator>::value_type
        value_type;
    typedef typename
        std::iterator_traits<DeviceIterator>::difference_type
        difference_type;

    size_t count = iterator_range_size(first, last);
    if(count == 0){
        return result;
    }

    size_t offset = first.get_index();

    // map [first; last) buffer to host
    value_type *pointer = static_cast<value_type*>(
        queue.enqueue_map_buffer(
            first.get_buffer(),
            CL_MAP_READ,
            offset * sizeof(value_type),
            count * sizeof(value_type),
            events
        )
    );

    // copy [first; last) to result buffer
    std::copy(
        pointer,
        pointer + static_cast<difference_type>(count),
        result
    );

    // unmap [first; last)
    boost::compute::event unmap_event = queue.enqueue_unmap_buffer(
        first.get_buffer(),
        static_cast<void*>(pointer)
    );
    unmap_event.wait();

    return iterator_plus_distance(result, count);
}

template<class DeviceIterator, class HostIterator>
inline future<HostIterator> copy_to_host_async(DeviceIterator first,
                                               DeviceIterator last,
                                               HostIterator result,
                                               command_queue &queue,
                                               const wait_list &events)
{
    typedef typename
        std::iterator_traits<DeviceIterator>::value_type
        value_type;

    size_t count = iterator_range_size(first, last);
    if(count == 0){
        return future<HostIterator>();
    }

    const buffer &buffer = first.get_buffer();
    size_t offset = first.get_index();

    event event_ =
        queue.enqueue_read_buffer_async(buffer,
                                        offset * sizeof(value_type),
                                        count * sizeof(value_type),
                                        ::boost::addressof(*result),
                                        events);

    return make_future(iterator_plus_distance(result, count), event_);
}

#ifdef BOOST_COMPUTE_CL_VERSION_2_0
// copy_to_host() specialization for svm_ptr
template<class T, class HostIterator>
inline HostIterator copy_to_host(svm_ptr<T> first,
                                 svm_ptr<T> last,
                                 HostIterator result,
                                 command_queue &queue,
                                 const wait_list &events)
{
    size_t count = iterator_range_size(first, last);
    if(count == 0){
        return result;
    }

    queue.enqueue_svm_memcpy(
        ::boost::addressof(*result), first.get(), count * sizeof(T), events
    );

    return result + count;
}

template<class T, class HostIterator>
inline future<HostIterator> copy_to_host_async(svm_ptr<T> first,
                                               svm_ptr<T> last,
                                               HostIterator result,
                                               command_queue &queue,
                                               const wait_list &events)
{
    size_t count = iterator_range_size(first, last);
    if(count == 0){
        return future<HostIterator>();
    }

    event event_ = queue.enqueue_svm_memcpy_async(
        ::boost::addressof(*result), first.get(), count * sizeof(T), events
    );

    return make_future(iterator_plus_distance(result, count), event_);
}

template<class T, class HostIterator>
inline HostIterator copy_to_host_map(svm_ptr<T> first,
                                     svm_ptr<T> last,
                                     HostIterator result,
                                     command_queue &queue,
                                     const wait_list &events)
{
    size_t count = iterator_range_size(first, last);
    if(count == 0){
        return result;
    }

    // map
    queue.enqueue_svm_map(first.get(), count * sizeof(T), CL_MAP_READ, events);

    // copy [first; last) to result
    std::copy(
        static_cast<T*>(first.get()),
        static_cast<T*>(last.get()),
        result
    );

    // unmap [first; last)
    queue.enqueue_svm_unmap(first.get()).wait();

    return iterator_plus_distance(result, count);
}
#endif // BOOST_COMPUTE_CL_VERSION_2_0

} // end detail namespace
} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_ALGORITHM_DETAIL_COPY_TO_HOST_HPP

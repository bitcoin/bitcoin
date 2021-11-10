//---------------------------------------------------------------------------//
// Copyright (c) 2013 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_ALGORITHM_DETAIL_INSERTION_SORT_HPP
#define BOOST_COMPUTE_ALGORITHM_DETAIL_INSERTION_SORT_HPP

#include <boost/compute/kernel.hpp>
#include <boost/compute/program.hpp>
#include <boost/compute/command_queue.hpp>
#include <boost/compute/detail/meta_kernel.hpp>
#include <boost/compute/detail/iterator_range_size.hpp>
#include <boost/compute/memory/local_buffer.hpp>

namespace boost {
namespace compute {
namespace detail {

template<class Iterator, class Compare>
inline void serial_insertion_sort(Iterator first,
                                  Iterator last,
                                  Compare compare,
                                  command_queue &queue)
{
    typedef typename std::iterator_traits<Iterator>::value_type T;

    size_t count = iterator_range_size(first, last);
    if(count < 2){
        return;
    }

    meta_kernel k("serial_insertion_sort");
    size_t local_data_arg = k.add_arg<T *>(memory_object::local_memory, "data");
    size_t count_arg = k.add_arg<uint_>("n");

    k <<
        // copy data to local memory
        "for(uint i = 0; i < n; i++){\n" <<
        "    data[i] = " << first[k.var<uint_>("i")] << ";\n"
        "}\n"

        // sort data in local memory
        "for(uint i = 1; i < n; i++){\n" <<
        "    " << k.decl<const T>("value") << " = data[i];\n" <<
        "    uint pos = i;\n" <<
        "    while(pos > 0 && " <<
                   compare(k.var<const T>("value"),
                           k.var<const T>("data[pos-1]")) << "){\n" <<
        "        data[pos] = data[pos-1];\n" <<
        "        pos--;\n" <<
        "    }\n" <<
        "    data[pos] = value;\n" <<
        "}\n" <<

        // copy sorted data to output
        "for(uint i = 0; i < n; i++){\n" <<
        "    " << first[k.var<uint_>("i")] << " = data[i];\n"
        "}\n";

    const context &context = queue.get_context();
    ::boost::compute::kernel kernel = k.compile(context);
    kernel.set_arg(local_data_arg, local_buffer<T>(count));
    kernel.set_arg(count_arg, static_cast<uint_>(count));

    queue.enqueue_task(kernel);
}

template<class Iterator>
inline void serial_insertion_sort(Iterator first,
                                  Iterator last,
                                  command_queue &queue)
{
    typedef typename std::iterator_traits<Iterator>::value_type T;

    ::boost::compute::less<T> less;

    return serial_insertion_sort(first, last, less, queue);
}

template<class KeyIterator, class ValueIterator, class Compare>
inline void serial_insertion_sort_by_key(KeyIterator keys_first,
                                         KeyIterator keys_last,
                                         ValueIterator values_first,
                                         Compare compare,
                                         command_queue &queue)
{
    typedef typename std::iterator_traits<KeyIterator>::value_type key_type;
    typedef typename std::iterator_traits<ValueIterator>::value_type value_type;

    size_t count = iterator_range_size(keys_first, keys_last);
    if(count < 2){
        return;
    }

    meta_kernel k("serial_insertion_sort_by_key");
    size_t local_keys_arg = k.add_arg<key_type *>(memory_object::local_memory, "keys");
    size_t local_data_arg = k.add_arg<value_type *>(memory_object::local_memory, "data");
    size_t count_arg = k.add_arg<uint_>("n");

    k <<
        // copy data to local memory
        "for(uint i = 0; i < n; i++){\n" <<
        "    keys[i] = " << keys_first[k.var<uint_>("i")] << ";\n"
        "    data[i] = " << values_first[k.var<uint_>("i")] << ";\n"
        "}\n"

        // sort data in local memory
        "for(uint i = 1; i < n; i++){\n" <<
        "    " << k.decl<const key_type>("key") << " = keys[i];\n" <<
        "    " << k.decl<const value_type>("value") << " = data[i];\n" <<
        "    uint pos = i;\n" <<
        "    while(pos > 0 && " <<
                   compare(k.var<const key_type>("key"),
                           k.var<const key_type>("keys[pos-1]")) << "){\n" <<
        "        keys[pos] = keys[pos-1];\n" <<
        "        data[pos] = data[pos-1];\n" <<
        "        pos--;\n" <<
        "    }\n" <<
        "    keys[pos] = key;\n" <<
        "    data[pos] = value;\n" <<
        "}\n" <<

        // copy sorted data to output
        "for(uint i = 0; i < n; i++){\n" <<
        "    " << keys_first[k.var<uint_>("i")] << " = keys[i];\n"
        "    " << values_first[k.var<uint_>("i")] << " = data[i];\n"
        "}\n";

    const context &context = queue.get_context();
    ::boost::compute::kernel kernel = k.compile(context);
    kernel.set_arg(local_keys_arg, static_cast<uint_>(count * sizeof(key_type)), 0);
    kernel.set_arg(local_data_arg, static_cast<uint_>(count * sizeof(value_type)), 0);
    kernel.set_arg(count_arg, static_cast<uint_>(count));

    queue.enqueue_task(kernel);
}

template<class KeyIterator, class ValueIterator>
inline void serial_insertion_sort_by_key(KeyIterator keys_first,
                                         KeyIterator keys_last,
                                         ValueIterator values_first,
                                         command_queue &queue)
{
    typedef typename std::iterator_traits<KeyIterator>::value_type key_type;

    serial_insertion_sort_by_key(
        keys_first,
        keys_last,
        values_first,
        boost::compute::less<key_type>(),
        queue
    );
}

} // end detail namespace
} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_ALGORITHM_DETAIL_INSERTION_SORT_HPP

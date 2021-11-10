//---------------------------------------------------------------------------//
// Copyright (c) 2013 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_ALGORITHM_DETAIL_SERIAL_FIND_EXTREMA_HPP
#define BOOST_COMPUTE_ALGORITHM_DETAIL_SERIAL_FIND_EXTREMA_HPP

#include <boost/compute/command_queue.hpp>
#include <boost/compute/types/fundamental.hpp>
#include <boost/compute/detail/meta_kernel.hpp>
#include <boost/compute/detail/iterator_range_size.hpp>
#include <boost/compute/container/detail/scalar.hpp>

namespace boost {
namespace compute {
namespace detail {

template<class InputIterator, class Compare>
inline InputIterator serial_find_extrema(InputIterator first,
                                         InputIterator last,
                                         Compare compare,
                                         const bool find_minimum,
                                         command_queue &queue)
{
    typedef typename std::iterator_traits<InputIterator>::value_type value_type;
    typedef typename std::iterator_traits<InputIterator>::difference_type difference_type;

    const context &context = queue.get_context();

    meta_kernel k("serial_find_extrema");

    k <<
        k.decl<value_type>("value") << " = " << first[k.expr<uint_>("0")] << ";\n" <<
        k.decl<uint_>("value_index") << " = 0;\n" <<
        "for(uint i = 1; i < size; i++){\n" <<
        "  " << k.decl<value_type>("candidate") << "="
             << first[k.expr<uint_>("i")] << ";\n" <<

        "#ifndef BOOST_COMPUTE_FIND_MAXIMUM\n" <<
        "  if(" << compare(k.var<value_type>("candidate"),
                           k.var<value_type>("value")) << "){\n" <<
        "#else\n" <<
        "  if(" << compare(k.var<value_type>("value"),
                           k.var<value_type>("candidate")) << "){\n" <<
        "#endif\n" <<

        "    value = candidate;\n" <<
        "    value_index = i;\n" <<
        "  }\n" <<
        "}\n" <<
        "*index = value_index;\n";

    size_t index_arg_index = k.add_arg<uint_ *>(memory_object::global_memory, "index");
    size_t size_arg_index = k.add_arg<uint_>("size");

    std::string options;
    if(!find_minimum){
        options = "-DBOOST_COMPUTE_FIND_MAXIMUM";
    }
    kernel kernel = k.compile(context, options);

    // setup index buffer
    scalar<uint_> index(context);
    kernel.set_arg(index_arg_index, index.get_buffer());

    // setup count
    size_t count = iterator_range_size(first, last);
    kernel.set_arg(size_arg_index, static_cast<uint_>(count));

    // run kernel
    queue.enqueue_task(kernel);

    // read index and return iterator
    return first + static_cast<difference_type>(index.read(queue));
}

} // end detail namespace
} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_ALGORITHM_DETAIL_SERIAL_FIND_EXTREMA_HPP

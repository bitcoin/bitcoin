//---------------------------------------------------------------------------//
// Copyright (c) 2013 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_ALGORITHM_DETAIL_SERIAL_ACCUMULATE_HPP
#define BOOST_COMPUTE_ALGORITHM_DETAIL_SERIAL_ACCUMULATE_HPP

#include <boost/compute/command_queue.hpp>
#include <boost/compute/detail/meta_kernel.hpp>
#include <boost/compute/detail/iterator_range_size.hpp>

namespace boost {
namespace compute {
namespace detail {

template<class InputIterator, class OutputIterator, class T, class BinaryFunction>
inline void serial_accumulate(InputIterator first,
                              InputIterator last,
                              OutputIterator result,
                              T init,
                              BinaryFunction function,
                              command_queue &queue)
{
    const context &context = queue.get_context();
    size_t count = detail::iterator_range_size(first, last);

    meta_kernel k("serial_accumulate");
    size_t init_arg = k.add_arg<T>("init");
    size_t count_arg = k.add_arg<cl_uint>("count");

    k <<
        k.decl<T>("result") << " = init;\n" <<
        "for(uint i = 0; i < count; i++)\n" <<
        "    result = " << function(k.var<T>("result"),
                                    first[k.var<cl_uint>("i")]) << ";\n" <<
        result[0] << " = result;\n";

    kernel kernel = k.compile(context);

    kernel.set_arg(init_arg, init);
    kernel.set_arg(count_arg, static_cast<cl_uint>(count));

    queue.enqueue_task(kernel);
}

} // end detail namespace
} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_ALGORITHM_DETAIL_SERIAL_ACCUMULATE_HPP

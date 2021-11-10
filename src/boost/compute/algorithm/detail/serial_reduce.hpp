//---------------------------------------------------------------------------//
// Copyright (c) 2013 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_ALGORITHM_DETAIL_SERIAL_REDUCE_HPP
#define BOOST_COMPUTE_ALGORITHM_DETAIL_SERIAL_REDUCE_HPP

#include <boost/compute/command_queue.hpp>
#include <boost/compute/detail/meta_kernel.hpp>
#include <boost/compute/detail/iterator_range_size.hpp>
#include <boost/compute/type_traits/result_of.hpp>

namespace boost {
namespace compute {
namespace detail {

// Space complexity: O(1)
template<class InputIterator, class OutputIterator, class BinaryFunction>
inline void serial_reduce(InputIterator first,
                          InputIterator last,
                          OutputIterator result,
                          BinaryFunction function,
                          command_queue &queue)
{
    typedef typename
        std::iterator_traits<InputIterator>::value_type T;
    typedef typename
        ::boost::compute::result_of<BinaryFunction(T, T)>::type result_type;

    const context &context = queue.get_context();
    size_t count = detail::iterator_range_size(first, last);
    if(count == 0){
        return;
    }

    meta_kernel k("serial_reduce");
    size_t count_arg = k.add_arg<cl_uint>("count");

    k <<
        k.decl<result_type>("result") << " = " << first[0] << ";\n" <<
        "for(uint i = 1; i < count; i++)\n" <<
        "    result = " << function(k.var<T>("result"),
                                    first[k.var<uint_>("i")]) << ";\n" <<
        result[0] << " = result;\n";

    kernel kernel = k.compile(context);

    kernel.set_arg(count_arg, static_cast<uint_>(count));

    queue.enqueue_task(kernel);
}

} // end detail namespace
} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_ALGORITHM_DETAIL_SERIAL_REDUCE_HPP

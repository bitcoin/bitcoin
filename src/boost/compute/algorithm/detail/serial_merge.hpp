//---------------------------------------------------------------------------//
// Copyright (c) 2013 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_ALGORITHM_SERIAL_MERGE_HPP
#define BOOST_COMPUTE_ALGORITHM_SERIAL_MERGE_HPP

#include <iterator>

#include <boost/compute/command_queue.hpp>
#include <boost/compute/detail/meta_kernel.hpp>
#include <boost/compute/detail/iterator_range_size.hpp>

namespace boost {
namespace compute {
namespace detail {

template<class InputIterator1,
         class InputIterator2,
         class OutputIterator,
         class Compare>
inline OutputIterator serial_merge(InputIterator1 first1,
                                   InputIterator1 last1,
                                   InputIterator2 first2,
                                   InputIterator2 last2,
                                   OutputIterator result,
                                   Compare comp,
                                   command_queue &queue)
{
    typedef typename
        std::iterator_traits<InputIterator1>::value_type
        input_type1;
    typedef typename
        std::iterator_traits<InputIterator2>::value_type
        input_type2;
    typedef typename
        std::iterator_traits<OutputIterator>::difference_type
        result_difference_type;

    std::ptrdiff_t size1 = std::distance(first1, last1);
    std::ptrdiff_t size2 = std::distance(first2, last2);

    meta_kernel k("serial_merge");
    k.add_set_arg<uint_>("size1", static_cast<uint_>(size1));
    k.add_set_arg<uint_>("size2", static_cast<uint_>(size2));

    k <<
        "uint i = 0;\n" << // index in result range
        "uint j = 0;\n" << // index in first input range
        "uint k = 0;\n" << // index in second input range

        // fetch initial values from each range
        k.decl<input_type1>("j_value") << " = " << first1[0] << ";\n" <<
        k.decl<input_type2>("k_value") << " = " << first2[0] << ";\n" <<

        // merge values from both input ranges to the result range
        "while(j < size1 && k < size2){\n" <<
        "    if(" << comp(k.var<input_type1>("j_value"),
                          k.var<input_type2>("k_value")) << "){\n" <<
        "        " << result[k.var<uint_>("i++")] << " = j_value;\n" <<
        "        j_value = " << first1[k.var<uint_>("++j")] << ";\n" <<
        "    }\n" <<
        "    else{\n"
        "        " << result[k.var<uint_>("i++")] << " = k_value;\n"
        "        k_value = " << first2[k.var<uint_>("++k")] << ";\n" <<
        "    }\n"
        "}\n"

        // copy any remaining values from first range
        "while(j < size1){\n" <<
            result[k.var<uint_>("i++")] << " = " <<
               first1[k.var<uint_>("j++")] << ";\n" <<
        "}\n"

        // copy any remaining values from second range
        "while(k < size2){\n" <<
            result[k.var<uint_>("i++")] << " = " <<
               first2[k.var<uint_>("k++")] << ";\n" <<
        "}\n";

    // run kernel
    k.exec(queue);

    return result + static_cast<result_difference_type>(size1 + size2);
}

} // end detail namespace
} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_ALGORITHM_SERIAL_MERGE_HPP

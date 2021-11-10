//---------------------------------------------------------------------------//
// Copyright (c) 2014 Mageswaran.D <mageswaran1989@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#include <boost/static_assert.hpp>

#include <boost/compute/system.hpp>
#include <boost/compute/context.hpp>
#include <boost/compute/command_queue.hpp>
#include <boost/compute/algorithm/any_of.hpp>
#include <boost/compute/container/vector.hpp>
#include <boost/compute/utility/program_cache.hpp>
#include <boost/compute/type_traits/is_device_iterator.hpp>

namespace boost {
namespace compute {

namespace detail {

const char lexicographical_compare_source[] =
"__kernel void lexicographical_compare(const uint size1,\n"
"                                      const uint size2,\n"
"                                      __global const T1 *range1,\n"
"                                      __global const T2 *range2,\n"
"                                      __global bool *result_buf)\n"
"{\n"
"   const uint i = get_global_id(0);\n"
"   if((i != size1) && (i != size2)){\n"
        //Individual elements are compared and results are stored in parallel.
        //0 is true
"       if(range1[i] < range2[i])\n"
"           result_buf[i] = 0;\n"
"       else\n"
"           result_buf[i] = 1;\n"
"   }\n"
"   else\n"
"       result_buf[i] = !((i == size1) && (i != size2));\n"
"}\n";

template<class InputIterator1, class InputIterator2>
inline bool dispatch_lexicographical_compare(InputIterator1 first1,
                                             InputIterator1 last1,
                                             InputIterator2 first2,
                                             InputIterator2 last2,
                                             command_queue &queue)
{
    const boost::compute::context &context = queue.get_context();

    boost::shared_ptr<program_cache> cache =
        program_cache::get_global_cache(context);

    size_t iterator_size1 = iterator_range_size(first1, last1);
    size_t iterator_size2 = iterator_range_size(first2, last2);
    size_t max_size = (std::max)(iterator_size1, iterator_size2);

    if(max_size == 0){
        return false;
    }

    boost::compute::vector<bool> result_vector(max_size, context);


    typedef typename std::iterator_traits<InputIterator1>::value_type value_type1;
    typedef typename std::iterator_traits<InputIterator2>::value_type value_type2;

    // load (or create) lexicographical compare program
    std::string cache_key =
            std::string("__boost_lexicographical_compare")
            + type_name<value_type1>() + type_name<value_type2>();

    std::stringstream options;
    options << " -DT1=" << type_name<value_type1>();
    options << " -DT2=" << type_name<value_type2>();

    program lexicographical_compare_program = cache->get_or_build(
        cache_key, options.str(), lexicographical_compare_source, context
    );

    kernel lexicographical_compare_kernel(lexicographical_compare_program,
                                          "lexicographical_compare");

    lexicographical_compare_kernel.set_arg<uint_>(0, iterator_size1);
    lexicographical_compare_kernel.set_arg<uint_>(1, iterator_size2);
    lexicographical_compare_kernel.set_arg(2, first1.get_buffer());
    lexicographical_compare_kernel.set_arg(3, first2.get_buffer());
    lexicographical_compare_kernel.set_arg(4, result_vector.get_buffer());

    queue.enqueue_1d_range_kernel(lexicographical_compare_kernel,
                                  0,
                                  max_size,
                                  0);

    return boost::compute::any_of(result_vector.begin(),
                                  result_vector.end(),
                                  _1 == 0,
                                  queue);
}

} // end detail namespace

/// Checks if the first range [first1, last1) is lexicographically
/// less than the second range [first2, last2).
///
/// Space complexity:
/// \Omega(max(distance(\p first1, \p last1), distance(\p first2, \p last2)))
template<class InputIterator1, class InputIterator2>
inline bool lexicographical_compare(InputIterator1 first1,
                                    InputIterator1 last1,
                                    InputIterator2 first2,
                                    InputIterator2 last2,
                                    command_queue &queue = system::default_queue())
{
    BOOST_STATIC_ASSERT(is_device_iterator<InputIterator1>::value);
    BOOST_STATIC_ASSERT(is_device_iterator<InputIterator2>::value);

    return detail::dispatch_lexicographical_compare(first1, last1, first2, last2, queue);
}

} // end compute namespace
} // end boost namespac

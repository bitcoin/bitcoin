//---------------------------------------------------------------------------//
// Copyright (c) 2014 Roshan <thisisroshansmail@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_RANDOM_LINEAR_CONGRUENTIAL_ENGINE_HPP
#define BOOST_COMPUTE_RANDOM_LINEAR_CONGRUENTIAL_ENGINE_HPP

#include <algorithm>

#include <boost/compute/types.hpp>
#include <boost/compute/buffer.hpp>
#include <boost/compute/kernel.hpp>
#include <boost/compute/context.hpp>
#include <boost/compute/program.hpp>
#include <boost/compute/command_queue.hpp>
#include <boost/compute/algorithm/transform.hpp>
#include <boost/compute/container/vector.hpp>
#include <boost/compute/detail/iterator_range_size.hpp>
#include <boost/compute/iterator/discard_iterator.hpp>
#include <boost/compute/utility/program_cache.hpp>

namespace boost {
namespace compute {

///
/// \class linear_congruential_engine
/// \brief 'Quick and Dirty' linear congruential engine
///
/// Quick and dirty linear congruential engine to generate low quality
/// random numbers very quickly. For uses in which good quality of random
/// numbers is required(Monte-Carlo Simulations), use other engines like
/// Mersenne Twister instead.
///
template<class T = uint_>
class linear_congruential_engine
{
public:
    typedef T result_type;
    static const T default_seed = 1;
    static const T a = 1099087573;
    static const size_t threads = 1024;

    /// Creates a new linear_congruential_engine and seeds it with \p value.
    explicit linear_congruential_engine(command_queue &queue,
                                        result_type value = default_seed)
        : m_context(queue.get_context()),
          m_multiplicands(m_context, threads * sizeof(result_type))
    {
        // setup program
        load_program();

        // seed state
        seed(value, queue);

        // generate multiplicands
        generate_multiplicands(queue);
    }

    /// Creates a new linear_congruential_engine object as a copy of \p other.
    linear_congruential_engine(const linear_congruential_engine<T> &other)
        : m_context(other.m_context),
          m_program(other.m_program),
          m_seed(other.m_seed),
          m_multiplicands(other.m_multiplicands)
    {
    }

    /// Copies \p other to \c *this.
    linear_congruential_engine<T>&
    operator=(const linear_congruential_engine<T> &other)
    {
        if(this != &other){
            m_context = other.m_context;
            m_program = other.m_program;
            m_seed = other.m_seed;
            m_multiplicands = other.m_multiplicands;
        }

        return *this;
    }

    /// Destroys the linear_congruential_engine object.
    ~linear_congruential_engine()
    {
    }

    /// Seeds the random number generator with \p value.
    ///
    /// \param value seed value for the random-number generator
    /// \param queue command queue to perform the operation
    ///
    /// If no seed value is provided, \c default_seed is used.
    void seed(result_type value, command_queue &queue)
    {
        (void) queue;

        m_seed = value;
    }

    /// \overload
    void seed(command_queue &queue)
    {
        seed(default_seed, queue);
    }

    /// Generates random numbers and stores them to the range [\p first, \p last).
    template<class OutputIterator>
    void generate(OutputIterator first, OutputIterator last, command_queue &queue)
    {
        size_t size = detail::iterator_range_size(first, last);

        kernel fill_kernel(m_program, "fill");
        fill_kernel.set_arg(1, m_multiplicands);
        fill_kernel.set_arg(2, first.get_buffer());

        size_t offset = 0;

        for(;;){
            size_t count = 0;
            if(size > threads){
                count = (std::min)(static_cast<size_t>(threads), size - offset);
            }
            else {
                count = size;
            }
            fill_kernel.set_arg(0, static_cast<const uint_>(m_seed));
            fill_kernel.set_arg(3, static_cast<const uint_>(offset));
            queue.enqueue_1d_range_kernel(fill_kernel, 0, count, 0);

            offset += count;

            if(offset >= size){
                break;
            }

            update_seed(queue);
        }
    }

    /// \internal_
    void generate(discard_iterator first, discard_iterator last, command_queue &queue)
    {
        (void) queue;

        size_t size = detail::iterator_range_size(first, last);
        uint_ max_mult =
            detail::read_single_value<T>(m_multiplicands, threads-1, queue);
        while(size >= threads) {
            m_seed *= max_mult;
            size -= threads;
        }
        m_seed *=
            detail::read_single_value<T>(m_multiplicands, size-1, queue);
    }

    /// Generates random numbers, transforms them with \p op, and then stores
    /// them to the range [\p first, \p last).
    template<class OutputIterator, class Function>
    void generate(OutputIterator first, OutputIterator last, Function op, command_queue &queue)
    {
        vector<T> tmp(std::distance(first, last), queue.get_context());
        generate(tmp.begin(), tmp.end(), queue);
        transform(tmp.begin(), tmp.end(), first, op, queue);
    }

    /// Generates \p z random numbers and discards them.
    void discard(size_t z, command_queue &queue)
    {
        generate(discard_iterator(0), discard_iterator(z), queue);
    }

private:
    /// \internal_
    /// Generates the multiplicands for each thread
    void generate_multiplicands(command_queue &queue)
    {
        kernel multiplicand_kernel =
            m_program.create_kernel("multiplicand");
        multiplicand_kernel.set_arg(0, m_multiplicands);

        queue.enqueue_task(multiplicand_kernel);
    }

    /// \internal_
    void update_seed(command_queue &queue)
    {
        m_seed *=
            detail::read_single_value<T>(m_multiplicands, threads-1, queue);
    }

    /// \internal_
    void load_program()
    {
        boost::shared_ptr<program_cache> cache =
            program_cache::get_global_cache(m_context);

        std::string cache_key =
            std::string("__boost_linear_congruential_engine_") + type_name<T>();

        const char source[] =
            "__kernel void multiplicand(__global uint *multiplicands)\n"
            "{\n"
            "    uint a = 1099087573;\n"
            "    multiplicands[0] = a;\n"
            "    for(uint i = 1; i < 1024; i++){\n"
            "        multiplicands[i] = a * multiplicands[i-1];\n"
            "    }\n"
            "}\n"

            "__kernel void fill(const uint seed,\n"
            "                   __global uint *multiplicands,\n"
            "                   __global uint *result,"
            "                   const uint offset)\n"
            "{\n"
            "    const uint i = get_global_id(0);\n"
            "    result[offset+i] = seed * multiplicands[i];\n"
            "}\n";

        m_program = cache->get_or_build(cache_key, std::string(), source, m_context);
    }

private:
    context m_context;
    program m_program;
    T m_seed;
    buffer m_multiplicands;
};

} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_RANDOM_LINEAR_CONGRUENTIAL_ENGINE_HPP

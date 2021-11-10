//---------------------------------------------------------------------------//
// Copyright (c) 2013 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_RANDOM_UNIFORM_REAL_DISTRIBUTION_HPP
#define BOOST_COMPUTE_RANDOM_UNIFORM_REAL_DISTRIBUTION_HPP

#include <boost/assert.hpp>
#include <boost/type_traits.hpp>

#include <boost/compute/command_queue.hpp>
#include <boost/compute/function.hpp>
#include <boost/compute/detail/literal.hpp>
#include <boost/compute/types/fundamental.hpp>

namespace boost {
namespace compute {

/// \class uniform_real_distribution
/// \brief Produces uniformly distributed random floating-point numbers.
///
/// The following example shows how to setup a uniform real distribution to
/// produce random \c float values between \c 1 and \c 100.
///
/// \snippet test/test_uniform_real_distribution.cpp generate
///
/// \see default_random_engine, normal_distribution
template<class RealType = float>
class uniform_real_distribution
{
public:
    typedef RealType result_type;

    /// Creates a new uniform distribution producing numbers in the range
    /// [\p a, \p b).
    /// Requires a < b
    uniform_real_distribution(RealType a = 0.f, RealType b = 1.f)
        : m_a(a),
          m_b(b)
    {
        BOOST_ASSERT(a < b);
    }

    /// Destroys the uniform_real_distribution object.
    ~uniform_real_distribution()
    {
    }

    /// Returns the minimum value of the distribution.
    result_type a() const
    {
        return m_a;
    }

    /// Returns the maximum value of the distribution.
    result_type b() const
    {
        return m_b;
    }

    /// Generates uniformly distributed floating-point numbers and stores
    /// them to the range [\p first, \p last).
    template<class OutputIterator, class Generator>
    void generate(OutputIterator first,
                  OutputIterator last,
                  Generator &generator,
                  command_queue &queue)
    {
        BOOST_COMPUTE_FUNCTION(RealType, scale_random, (const uint_ x),
        {
            return nextafter(LO + (convert_RealType(x) / MAX_RANDOM) * (HI - LO), (RealType) LO);
        });

        scale_random.define("LO", detail::make_literal(m_a));
        scale_random.define("HI", detail::make_literal(m_b));
        scale_random.define("MAX_RANDOM", "UINT_MAX");
        scale_random.define(
            "convert_RealType", std::string("convert_") + type_name<RealType>()
        );
        scale_random.define("RealType", type_name<RealType>());

        generator.generate(
            first, last, scale_random, queue
        );
    }

    /// \internal_ (deprecated)
    template<class OutputIterator, class Generator>
    void fill(OutputIterator first,
              OutputIterator last,
              Generator &g,
              command_queue &queue)
    {
        generate(first, last, g, queue);
    }

private:
    RealType m_a;
    RealType m_b;

    BOOST_STATIC_ASSERT_MSG(
        boost::is_floating_point<RealType>::value,
        "Template argument must be a floating point type"
    );
};

} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_RANDOM_UNIFORM_REAL_DISTRIBUTION_HPP

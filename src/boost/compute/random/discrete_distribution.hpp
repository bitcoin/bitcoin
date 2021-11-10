//---------------------------------------------------------------------------//
// Copyright (c) 2014 Roshan <thisisroshansmail@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_RANDOM_DISCRETE_DISTRIBUTION_HPP
#define BOOST_COMPUTE_RANDOM_DISCRETE_DISTRIBUTION_HPP

#include <numeric>

#include <boost/config.hpp>
#include <boost/type_traits.hpp>
#include <boost/static_assert.hpp>

#include <boost/compute/command_queue.hpp>
#include <boost/compute/function.hpp>
#include <boost/compute/algorithm/accumulate.hpp>
#include <boost/compute/algorithm/copy.hpp>
#include <boost/compute/algorithm/transform.hpp>
#include <boost/compute/detail/literal.hpp>
#include <boost/compute/types/fundamental.hpp>

namespace boost {
namespace compute {

/// \class discrete_distribution
/// \brief Produces random integers on the interval [0, n), where
/// probability of each integer is given by the weight of the ith
/// integer divided by the sum of all weights.
///
/// The following example shows how to setup a discrete distribution to
/// produce 0 and 1 with equal probability
///
/// \snippet test/test_discrete_distribution.cpp generate
///
template<class IntType = uint_>
class discrete_distribution
{
public:
    typedef IntType result_type;

    /// Creates a new discrete distribution with a single weight p = { 1 }.
    /// This distribution produces only zeroes.
    discrete_distribution()
        : m_probabilities(1, double(1)),
          m_scanned_probabilities(1, double(1))
    {

    }

    /// Creates a new discrete distribution with weights given by
    /// the range [\p first, \p last).
    template<class InputIterator>
    discrete_distribution(InputIterator first, InputIterator last)
        : m_probabilities(first, last),
          m_scanned_probabilities(std::distance(first, last))
    {
        if(first != last) {
            // after this m_scanned_probabilities.back() is a sum of all
            // weights from the range [first, last)
            std::partial_sum(first, last, m_scanned_probabilities.begin());

            std::vector<double>::iterator i = m_probabilities.begin();
            std::vector<double>::iterator j = m_scanned_probabilities.begin();
            for(; i != m_probabilities.end(); ++i, ++j)
            {
                // dividing each weight by sum of all weights to
                // get probabilities
                *i = *i / m_scanned_probabilities.back();
                // dividing each partial sum of weights by sum of
                // all weights to get partial sums of probabilities
                *j = *j / m_scanned_probabilities.back();
            }
        }
        else {
            m_probabilities.push_back(double(1));
            m_scanned_probabilities.push_back(double(1));
        }
    }

    /// Destroys the discrete_distribution object.
    ~discrete_distribution()
    {
    }

    /// Returns the probabilities
    ::std::vector<double> probabilities() const
    {
        return m_probabilities;
    }

    /// Returns the minimum potentially generated value.
    result_type min BOOST_PREVENT_MACRO_SUBSTITUTION () const
    {
        return result_type(0);
    }

    /// Returns the maximum potentially generated value.
    result_type max BOOST_PREVENT_MACRO_SUBSTITUTION () const
    {
        size_t type_max = static_cast<size_t>(
            (std::numeric_limits<result_type>::max)()
        );
        if(m_probabilities.size() - 1 > type_max) {
            return (std::numeric_limits<result_type>::max)();
        }
        return static_cast<result_type>(m_probabilities.size() - 1);
    }

    /// Generates uniformly distributed integers and stores
    /// them to the range [\p first, \p last).
    template<class OutputIterator, class Generator>
    void generate(OutputIterator first,
                  OutputIterator last,
                  Generator &generator,
                  command_queue &queue)
    {
        std::string source = "inline IntType scale_random(uint x)\n";

        source = source +
            "{\n" +
            "float rno = convert_float(x) / UINT_MAX;\n";
        for(size_t i = 0; i < m_scanned_probabilities.size() - 1; i++)
        {
            source = source +
                "if(rno <= " + detail::make_literal<float>(m_scanned_probabilities[i]) + ")\n" +
                "   return " + detail::make_literal(i) + ";\n";
        }

        source = source +
            "return " + detail::make_literal(m_scanned_probabilities.size() - 1) + ";\n" +
            "}\n";

        BOOST_COMPUTE_FUNCTION(IntType, scale_random, (const uint_ x), {});

        scale_random.set_source(source);
        scale_random.define("IntType", type_name<IntType>());

        generator.generate(first, last, scale_random, queue);
    }

private:
    ::std::vector<double> m_probabilities;
    ::std::vector<double> m_scanned_probabilities;

    BOOST_STATIC_ASSERT_MSG(
        boost::is_integral<IntType>::value,
        "Template argument must be integral"
    );
};

} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_RANDOM_UNIFORM_INT_DISTRIBUTION_HPP

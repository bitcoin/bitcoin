//---------------------------------------------------------------------------//
// Copyright (c) 2014 Roshan <thisisroshansmail@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_RANDOM_BERNOULLI_DISTRIBUTION_HPP
#define BOOST_COMPUTE_RANDOM_BERNOULLI_DISTRIBUTION_HPP

#include <boost/assert.hpp>
#include <boost/type_traits.hpp>

#include <boost/compute/command_queue.hpp>
#include <boost/compute/function.hpp>
#include <boost/compute/types/fundamental.hpp>
#include <boost/compute/detail/iterator_range_size.hpp>
#include <boost/compute/detail/literal.hpp>

namespace boost {
namespace compute {

///
/// \class bernoulli_distribution
/// \brief Produces random boolean values according to the following
/// discrete probability function with parameter p :
/// P(true/p) = p and P(false/p) = (1 - p)
///
/// The following example shows how to setup a bernoulli distribution to
/// produce random boolean values with parameter p = 0.25
///
/// \snippet test/test_bernoulli_distribution.cpp generate
///
template<class RealType = float>
class bernoulli_distribution
{
public:

    /// Creates a new bernoulli distribution
    bernoulli_distribution(RealType p = 0.5f)
        : m_p(p)
    {
    }

    /// Destroys the bernoulli_distribution object
    ~bernoulli_distribution()
    {
    }

    /// Returns the value of the parameter p
    RealType p() const
    {
        return m_p;
    }

    /// Generates bernoulli distributed booleans and stores
    /// them in the range [\p first, \p last).
    template<class OutputIterator, class Generator>
    void generate(OutputIterator first,
                  OutputIterator last,
                  Generator &generator,
                  command_queue &queue)
    {
        size_t count = detail::iterator_range_size(first, last);

        vector<uint_> tmp(count, queue.get_context());
        generator.generate(tmp.begin(), tmp.end(), queue);

        BOOST_COMPUTE_FUNCTION(bool, scale_random, (const uint_ x),
        {
            return (convert_RealType(x) / MAX_RANDOM) < PARAM;
        });

        scale_random.define("PARAM", detail::make_literal(m_p));
        scale_random.define("MAX_RANDOM", "UINT_MAX");
        scale_random.define(
            "convert_RealType", std::string("convert_") + type_name<RealType>()
        );

        transform(
            tmp.begin(), tmp.end(), first, scale_random, queue
        );
    }

private:
    RealType m_p;

    BOOST_STATIC_ASSERT_MSG(
        boost::is_floating_point<RealType>::value,
        "Template argument must be a floating point type"
    );
};

} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_RANDOM_BERNOULLI_DISTRIBUTION_HPP

/* boost random/variate_generator.hpp header file
 *
 * Copyright Jens Maurer 2002
 * Copyright Steven Watanabe 2011
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org for most recent version including documentation.
 *
 * $Id$
 *
 */

#ifndef BOOST_RANDOM_RANDOM_GENERATOR_HPP
#define BOOST_RANDOM_RANDOM_GENERATOR_HPP

#include <boost/random/detail/ptr_helper.hpp>

#include <boost/random/detail/disable_warnings.hpp>

namespace boost {

/// \cond hide_private_members

namespace random {

///\endcond

/**
 * A random variate generator is used to join a random number
 * generator together with a random number distribution.
 * Boost.Random provides a vast choice of \generators as well
 * as \distributions.
 *
 * The argument for the template parameter Engine shall be of
 * the form U, U&, or U*, where U models a
 * \uniform_random_number_generator.  Then, the member
 * engine_value_type names U (not the pointer or reference to U).
 *
 * Specializations of @c variate_generator satisfy the
 * requirements of CopyConstructible. They also satisfy the
 * requirements of Assignable unless the template parameter
 * Engine is of the form U&.
 *
 * The complexity of all functions specified in this section
 * is constant. No function described in this section except
 * the constructor throws an exception.
 */
template<class Engine, class Distribution>
class variate_generator
{
private:
    typedef boost::random::detail::ptr_helper<Engine> helper_type;
public:
    typedef typename helper_type::value_type engine_value_type;
    typedef Engine engine_type;
    typedef Distribution distribution_type;
    typedef typename Distribution::result_type result_type;

    /**
     * Constructs a @c variate_generator object with the associated
     * \uniform_random_number_generator eng and the associated
     * \random_distribution d.
     *
     * Throws: If and what the copy constructor of Engine or
     * Distribution throws.
     */
    variate_generator(Engine e, Distribution d)
      : _eng(e), _dist(d) { }

    /** Returns: distribution()(engine()) */
    result_type operator()() { return _dist(engine()); }
    /**
     * Returns: distribution()(engine(), value).
     */
    template<class T>
    result_type operator()(const T& value) { return _dist(engine(), value); }

    /**
     * Returns: A reference to the associated uniform random number generator.
     */
    engine_value_type& engine() { return helper_type::ref(_eng); }
    /**
     * Returns: A reference to the associated uniform random number generator.
     */
    const engine_value_type& engine() const { return helper_type::ref(_eng); }

    /** Returns: A reference to the associated \random_distribution. */
    distribution_type& distribution() { return _dist; }
    /**
     * Returns: A reference to the associated random distribution.
     */
    const distribution_type& distribution() const { return _dist; }

    /**
     * Precondition: distribution().min() is well-formed
     *
     * Returns: distribution().min()
     */
    result_type min BOOST_PREVENT_MACRO_SUBSTITUTION () const { return (distribution().min)(); }
    /**
     * Precondition: distribution().max() is well-formed
     *
     * Returns: distribution().max()
     */
    result_type max BOOST_PREVENT_MACRO_SUBSTITUTION () const { return (distribution().max)(); }

private:
    Engine _eng;
    distribution_type _dist;
};

} // namespace random

using random::variate_generator;

} // namespace boost

#include <boost/random/detail/enable_warnings.hpp>

#endif // BOOST_RANDOM_RANDOM_GENERATOR_HPP

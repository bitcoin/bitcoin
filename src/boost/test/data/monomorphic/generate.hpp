//  (C) Copyright Gennadiy Rozental 2001.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
/// @file
/// Defines generic interface for monomorphic dataset based on generator
// ***************************************************************************

#ifndef BOOST_TEST_DATA_MONOMORPHIC_GENERATE_HPP_112011GER
#define BOOST_TEST_DATA_MONOMORPHIC_GENERATE_HPP_112011GER

// Boost.Test
#include <boost/test/data/config.hpp>
#include <boost/test/data/monomorphic/fwd.hpp>

#include <boost/core/ref.hpp>

#include <boost/test/detail/suppress_warnings.hpp>

//____________________________________________________________________________//

namespace boost {
namespace unit_test {
namespace data {
namespace monomorphic {

// ************************************************************************** //
// **************                  generated_by                ************** //
// ************************************************************************** //

/*!@brief Generators interface
 *
 * This class implements the dataset concept over a generator. Examples of generators are:
 * - xrange_t
 * - random_t
 *
 * The generator concept is the following:
 * - the type of the generated samples is given by field @c sample
 * - the member function @c capacity should return the size of the collection being generated (potentially infinite)
 * - the member function @c next should change the state of the generator to the next generated value
 * - the member function @c reset should put the state of the object in the same state as right after its instanciation
 */
template<typename Generator>
class generated_by {
public:
    typedef typename Generator::sample sample;

    enum { arity = 1 };

    struct iterator {
        // Constructor
        explicit    iterator( Generator& gen )
        : m_gen( &gen )
        {
            if(m_gen->capacity() > 0) {
                m_gen->reset();
                ++*this;
            }
        }

        // forward iterator interface
        sample const&   operator*() const   { return m_curr_sample; }
        void            operator++()        { m_curr_sample = m_gen->next(); }

    private:
        // Data members
        Generator*  m_gen;
        sample      m_curr_sample;
    };

    typedef Generator generator_type;

    // Constructor
    explicit        generated_by( Generator&& G )
    : m_generator( std::forward<Generator>(G) )
    {}

    // Move constructor
    generated_by( generated_by&& rhs )
    : m_generator( std::forward<Generator>(rhs.m_generator) )
    {}

    //! Size of the underlying dataset
    data::size_t    size() const            { return m_generator.capacity(); }

    //! Iterator on the beginning of the dataset
    iterator        begin() const           { return iterator( boost::ref(const_cast<Generator&>(m_generator)) ); }

private:
    // Data members
    Generator       m_generator;
};

//____________________________________________________________________________//

//! A generated dataset is a dataset.
template<typename Generator>
struct is_dataset<generated_by<Generator>> : mpl::true_ {};

} // namespace monomorphic
} // namespace data
} // namespace unit_test
} // namespace boost

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_TEST_DATA_MONOMORPHIC_GENERATE_HPP_112011GER


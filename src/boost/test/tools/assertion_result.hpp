//  (C) Copyright Gennadiy Rozental 2001.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
/// @file
/// Enhanced result for test predicate that include message explaining failure
// ***************************************************************************

#ifndef BOOST_TEST_PREDICATE_RESULT_HPP_012705GER
#define BOOST_TEST_PREDICATE_RESULT_HPP_012705GER

// Boost.Test
#include <boost/test/utils/class_properties.hpp>
#include <boost/test/utils/wrap_stringstream.hpp>
#include <boost/test/utils/basic_cstring/basic_cstring.hpp>

// Boost
#include <boost/shared_ptr.hpp>
#include <boost/detail/workaround.hpp>

// STL
#include <cstddef>          // for std::size_t

#include <boost/test/detail/suppress_warnings.hpp>

//____________________________________________________________________________//

namespace boost {
namespace test_tools {

// ************************************************************************** //
// **************                assertion_result              ************** //
// ************************************************************************** //

//!@brief Type used for storing the result of an assertion.
class BOOST_TEST_DECL assertion_result {

    //!@internal
    typedef unit_test::const_string      const_string;

    //!@internal
    struct dummy { void nonnull() {} };

    //!@internal
    typedef void (dummy::*safe_bool)();

public:
    // Constructor
    assertion_result( bool pv_ )
    : p_predicate_value( pv_ )
    {}

    template<typename BoolConvertable>
    assertion_result( BoolConvertable const& pv_ ) : p_predicate_value( !!pv_ ) {}

    // Access methods
    bool                operator!() const           { return !p_predicate_value; }
    void                operator=( bool pv_ )       { p_predicate_value.value = pv_; }
    operator            safe_bool() const           { return !!p_predicate_value ? &dummy::nonnull : 0; }

    // Public properties
    BOOST_READONLY_PROPERTY( bool, (assertion_result) ) p_predicate_value;

    // Access methods
    bool                has_empty_message() const   { return !m_message; }
    wrap_stringstream&  message()
    {
        if( !m_message )
            m_message.reset( new wrap_stringstream );

        return *m_message;
    }
    const_string        message() const                   { return !m_message ? const_string() : const_string( m_message->str() ); }

private:
    // Data members
    shared_ptr<wrap_stringstream> m_message;
};

typedef assertion_result predicate_result;

} // namespace test_tools
} // namespace boost

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_TEST_PREDICATE_RESULT_HPP_012705GER

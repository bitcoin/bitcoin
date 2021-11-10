//  (C) Copyright Gennadiy Rozental 2001.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
//!@file
//!@brief some trivial global typedefs
// ***************************************************************************

#ifndef BOOST_TEST_GLOBAL_TYPEDEF_HPP_021005GER
#define BOOST_TEST_GLOBAL_TYPEDEF_HPP_021005GER

#include <boost/test/utils/basic_cstring/basic_cstring.hpp>

#define BOOST_TEST_L( s )         ::boost::unit_test::const_string( s, sizeof( s ) - 1 )
#define BOOST_TEST_STRINGIZE( s ) BOOST_TEST_L( BOOST_STRINGIZE( s ) )
#define BOOST_TEST_EMPTY_STRING   BOOST_TEST_L( "" )

#include <boost/test/detail/suppress_warnings.hpp>

//____________________________________________________________________________//

namespace boost {
namespace unit_test {

typedef unsigned long   counter_t;

//____________________________________________________________________________//

enum report_level  { INV_REPORT_LEVEL, CONFIRMATION_REPORT, SHORT_REPORT, DETAILED_REPORT, NO_REPORT };

//____________________________________________________________________________//

//! Indicates the output format for the loggers or the test tree printing
enum output_format { OF_INVALID,
                     OF_CLF,      ///< compiler log format
                     OF_XML,      ///< XML format for report and log,
                     OF_JUNIT,    ///< JUNIT format for report and log,
                     OF_CUSTOM_LOGGER, ///< User specified logger.
                     OF_DOT       ///< dot format for output content
};

//____________________________________________________________________________//

enum test_unit_type { TUT_CASE = 0x01, TUT_SUITE = 0x10, TUT_ANY = 0x11 };

//____________________________________________________________________________//

enum assertion_result { AR_FAILED, AR_PASSED, AR_TRIGGERED };

//____________________________________________________________________________//

typedef unsigned long   test_unit_id;

const test_unit_id INV_TEST_UNIT_ID  = 0xFFFFFFFF;
const test_unit_id MAX_TEST_CASE_ID  = 0xFFFFFFFE;
const test_unit_id MIN_TEST_CASE_ID  = 0x00010000;
const test_unit_id MAX_TEST_SUITE_ID = 0x0000FF00;
const test_unit_id MIN_TEST_SUITE_ID = 0x00000001;

//____________________________________________________________________________//

namespace ut_detail {

inline test_unit_type
test_id_2_unit_type( test_unit_id id )
{
    return (id & 0xFFFF0000) != 0 ? TUT_CASE : TUT_SUITE;
}

//! Helper class for restoring the current test unit ID in a RAII manner
struct test_unit_id_restore {
    test_unit_id_restore(test_unit_id& to_restore_, test_unit_id new_value)
    : to_restore(to_restore_)
    , bkup(to_restore_) {
        to_restore = new_value;
    }
    ~test_unit_id_restore() {
        to_restore = bkup;
    }
private:
    test_unit_id& to_restore;
    test_unit_id bkup;
};

//____________________________________________________________________________//

} // namespace ut_detail

// helper templates to prevent ODR violations
template<class T>
struct static_constant {
    static T value;
};

template<class T>
T static_constant<T>::value;

//____________________________________________________________________________//

// helper defines for singletons.
// BOOST_TEST_SINGLETON_CONS should appear in the class body,
// BOOST_TEST_SINGLETON_CONS_IMPL should be in only one translation unit. The
// global instance should be declared by BOOST_TEST_SINGLETON_INST.

#define BOOST_TEST_SINGLETON_CONS_NO_CTOR( type )       \
public:                                                 \
  static type& instance();                              \
private:                                                \
  BOOST_DELETED_FUNCTION(type(type const&))             \
  BOOST_DELETED_FUNCTION(type& operator=(type const&))  \
  BOOST_DEFAULTED_FUNCTION(~type(), {})                 \
/**/

#define BOOST_TEST_SINGLETON_CONS( type )               \
  BOOST_TEST_SINGLETON_CONS_NO_CTOR(type)               \
private:                                                \
  BOOST_DEFAULTED_FUNCTION(type(), {})                  \
/**/

#define BOOST_TEST_SINGLETON_CONS_IMPL( type )          \
  type& type::instance() {                              \
    static type the_inst; return the_inst;              \
  }                                                     \
/**/

//____________________________________________________________________________//

#if defined(__APPLE_CC__) && defined(__GNUC__) && __GNUC__ < 4
#define BOOST_TEST_SINGLETON_INST( inst ) \
static BOOST_JOIN( inst, _t)& inst BOOST_ATTRIBUTE_UNUSED = BOOST_JOIN (inst, _t)::instance();

#else

#define BOOST_TEST_SINGLETON_INST( inst ) \
namespace { BOOST_JOIN( inst, _t)& inst BOOST_ATTRIBUTE_UNUSED = BOOST_JOIN( inst, _t)::instance(); }

#endif

} // namespace unit_test
} // namespace boost

//____________________________________________________________________________//

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_TEST_GLOBAL_TYPEDEF_HPP_021005GER

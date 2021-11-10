//
// Copyright 2005-2007 Adobe Systems Incorporated
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
#ifndef BOOST_GIL_CONCEPTS_CONCEPTS_CHECK_HPP
#define BOOST_GIL_CONCEPTS_CONCEPTS_CHECK_HPP

#include <boost/config.hpp>

#if defined(BOOST_CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunknown-pragmas"
#pragma clang diagnostic ignored "-Wconversion"
#pragma clang diagnostic ignored "-Wfloat-equal"
#pragma clang diagnostic ignored "-Wuninitialized"
#endif

#if defined(BOOST_GCC) && (BOOST_GCC >= 40900)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wfloat-equal"
#pragma GCC diagnostic ignored "-Wuninitialized"
#endif

#include <boost/concept_check.hpp>

#if defined(BOOST_CLANG)
#pragma clang diagnostic pop
#endif

#if defined(BOOST_GCC) && (BOOST_GCC >= 40900)
#pragma GCC diagnostic pop
#endif

// TODO: Document BOOST_GIL_USE_CONCEPT_CHECK here

namespace boost { namespace gil {

// TODO: What is BOOST_GIL_CLASS_REQUIRE for; Why not use BOOST_CLASS_REQUIRE?
// TODO: What is gil_function_requires for; Why not function_requires?

#ifdef BOOST_GIL_USE_CONCEPT_CHECK
    #define BOOST_GIL_CLASS_REQUIRE(type_var, ns, concept) \
        BOOST_CLASS_REQUIRE(type_var, ns, concept);

    template <typename Concept>
    void gil_function_requires() { function_requires<Concept>(); }
#else
    #define BOOST_GIL_CLASS_REQUIRE(type_var, ns, concept)

    template <typename C>
    void gil_function_requires() {}
#endif

}} // namespace boost::gil:

#endif

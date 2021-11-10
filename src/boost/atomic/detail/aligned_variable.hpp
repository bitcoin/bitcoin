/*
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * Copyright (c) 2020 Andrey Semashev
 */
/*!
 * \file   atomic/detail/aligned_variable.hpp
 *
 * This header defines a convenience macro for declaring aligned variables
 */

#ifndef BOOST_ATOMIC_DETAIL_ALIGNED_VARIABLE_HPP_INCLUDED_
#define BOOST_ATOMIC_DETAIL_ALIGNED_VARIABLE_HPP_INCLUDED_

#include <boost/atomic/detail/config.hpp>
#if defined(BOOST_ATOMIC_DETAIL_NO_CXX11_ALIGNAS)
#include <boost/config/helper_macros.hpp>
#include <boost/type_traits/type_with_alignment.hpp>
#endif
#include <boost/atomic/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

#if !defined(BOOST_ATOMIC_DETAIL_NO_CXX11_ALIGNAS)

#define BOOST_ATOMIC_DETAIL_ALIGNED_VAR(var_alignment, var_type, var_name) \
    alignas(var_alignment) var_type var_name

#define BOOST_ATOMIC_DETAIL_ALIGNED_VAR_TPL(var_alignment, var_type, var_name) \
    alignas(var_alignment) var_type var_name

#else // !defined(BOOST_ATOMIC_DETAIL_NO_CXX11_ALIGNAS)

// Note: Some compilers cannot use constant expressions in alignment attributes or alignas, so we have to use the union trick
#define BOOST_ATOMIC_DETAIL_ALIGNED_VAR(var_alignment, var_type, var_name) \
    union \
    { \
        var_type var_name; \
        boost::type_with_alignment< var_alignment >::type BOOST_JOIN(var_name, _aligner); \
    }

#define BOOST_ATOMIC_DETAIL_ALIGNED_VAR_TPL(var_alignment, var_type, var_name) \
    union \
    { \
        var_type var_name; \
        typename boost::type_with_alignment< var_alignment >::type BOOST_JOIN(var_name, _aligner); \
    }

#endif // !defined(BOOST_ATOMIC_DETAIL_NO_CXX11_ALIGNAS)

#include <boost/atomic/detail/footer.hpp>

#endif // BOOST_ATOMIC_DETAIL_ALIGNED_VARIABLE_HPP_INCLUDED_

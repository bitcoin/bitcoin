//---------------------------------------------------------------------------//
// Copyright (c) 2013-2014 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_UTILITY_SOURCE_HPP
#define BOOST_COMPUTE_UTILITY_SOURCE_HPP

/// Stringizes OpenCL source code.
///
/// For example, to create a simple kernel which squares each input value:
/// \code
/// const char source[] = BOOST_COMPUTE_STRINGIZE_SOURCE(
///     __kernel void square(const float *input, float *output)
///     {
///         const uint i = get_global_id(0);
///         const float x = input[i];
///         output[i] = x * x;
///     }
/// );
///
/// // create and build square program
/// program square_program = program::build_with_source(source, context);
///
/// // create square kernel
/// kernel square_kernel(square_program, "square");
/// \endcode
#ifdef BOOST_COMPUTE_DOXYGEN_INVOKED
#define BOOST_COMPUTE_STRINGIZE_SOURCE(source)
#else
#define BOOST_COMPUTE_STRINGIZE_SOURCE(...) #__VA_ARGS__
#endif

#endif // BOOST_COMPUTE_UTILITY_SOURCE_HPP

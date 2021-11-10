//---------------------------------------------------------------------------//
// Copyright (c) 2013-2014 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_FUNCTIONAL_HASH_HPP
#define BOOST_COMPUTE_FUNCTIONAL_HASH_HPP

#include <boost/compute/function.hpp>
#include <boost/compute/types/fundamental.hpp>

namespace boost {
namespace compute {
namespace detail {

template<class Key>
std::string make_hash_function_name()
{
    return std::string("boost_hash_") + type_name<Key>();
}

template<class Key>
inline std::string make_hash_function_source()
{
  std::stringstream source;
  source << "inline ulong " << make_hash_function_name<Key>()
         << "(const " << type_name<Key>() << " x)\n"
         << "{\n"
         // note we reinterpret the argument as a 32-bit uint and
         // then promote it to a 64-bit ulong for the result type
         << "    ulong a = as_uint(x);\n"
         << "    a = (a ^ 61) ^ (a >> 16);\n"
         << "    a = a + (a << 3);\n"
         << "    a = a ^ (a >> 4);\n"
         << "    a = a * 0x27d4eb2d;\n"
         << "    a = a ^ (a >> 15);\n"
         << "    return a;\n"
         << "}\n";
    return source.str();
}

template<class Key>
struct hash_impl
{
    typedef Key argument_type;
    typedef ulong_ result_type;

    hash_impl()
        : m_function("")
    {
        m_function = make_function_from_source<result_type(argument_type)>(
            make_hash_function_name<argument_type>(),
            make_hash_function_source<argument_type>()
        );
    }

    template<class Arg>
    invoked_function<result_type, boost::tuple<Arg> >
    operator()(const Arg &arg) const
    {
        return m_function(arg);
    }

    function<result_type(argument_type)> m_function;
};

} // end detail namespace

/// The hash function returns a hash value for the input value.
///
/// The return type is \c ulong_ (the OpenCL unsigned long type).
template<class Key> struct hash;

/// \internal_
template<> struct hash<int_> : detail::hash_impl<int_> { };

/// \internal_
template<> struct hash<uint_> : detail::hash_impl<uint_> { };

/// \internal_
template<> struct hash<float_> : detail::hash_impl<float_> { };

} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_FUNCTIONAL_HASH_HPP

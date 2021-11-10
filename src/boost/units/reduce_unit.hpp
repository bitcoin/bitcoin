// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2003-2008 Matthias Christian Schabel
// Copyright (C) 2007-2008 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UNITS_REDUCE_UNIT_HPP_INCLUDED
#define BOOST_UNITS_REDUCE_UNIT_HPP_INCLUDED

/// \file
/// \brief Returns a unique type for every unit.

namespace boost {
namespace units {

#ifdef BOOST_UNITS_DOXYGEN

/// Returns a unique type for every unit.
template<class Unit>
struct reduce_unit {
    typedef detail::unspecified type;
};

#else

// default implementation: return Unit unchanged.
template<class Unit>
struct reduce_unit {
    typedef Unit type;
};

#endif

}
}

#endif

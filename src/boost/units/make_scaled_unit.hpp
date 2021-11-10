// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2003-2008 Matthias Christian Schabel
// Copyright (C) 2007-2008 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UNITS_MAKE_SCALED_UNIT_HPP_INCLUDED
#define BOOST_UNITS_MAKE_SCALED_UNIT_HPP_INCLUDED

#include <boost/units/units_fwd.hpp>
#include <boost/units/heterogeneous_system.hpp>
#include <boost/units/unit.hpp>

namespace boost {
namespace units {

template<class Unit, class Scale>
struct make_scaled_unit {
    typedef typename make_scaled_unit<typename reduce_unit<Unit>::type, Scale>::type type;
};

template<class Dimension, class UnitList, class OldScale, class Scale>
struct make_scaled_unit<unit<Dimension, heterogeneous_system<heterogeneous_system_impl<UnitList, Dimension, OldScale> > >, Scale> {
    typedef unit<
        Dimension,
        heterogeneous_system<
            heterogeneous_system_impl<
                UnitList,
                Dimension,
                typename mpl::times<
                    OldScale,
                    list<scale_list_dim<Scale>, dimensionless_type>
                >::type
            >
        >
    > type;
};

template<class Dimension, class UnitList, class OldScale, long Base>
struct make_scaled_unit<unit<Dimension, heterogeneous_system<heterogeneous_system_impl<UnitList, Dimension, OldScale> > >, scale<Base, static_rational<0> > > {
    typedef unit<
        Dimension,
        heterogeneous_system<
            heterogeneous_system_impl<
                UnitList,
                Dimension,
                OldScale
            >
        >
    > type;
};

}
}

#endif

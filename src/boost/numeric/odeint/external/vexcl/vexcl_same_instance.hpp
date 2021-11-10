/*
 [auto_generated]
 boost/numeric/odeint/external/vexcl/vexcl_same_instance.hpp

 [begin_description]
 Check if two VexCL containers are the same instance.
 [end_description]

 Copyright 2009-2011 Karsten Ahnert
 Copyright 2009-2011 Mario Mulansky

 Distributed under the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or
 copy at http://www.boost.org/LICENSE_1_0.txt)
 */


#ifndef BOOST_NUMERIC_ODEINT_EXTERNAL_VEXCL_VEXCL_SAME_INSTANCE_HPP_INCLUDED
#define BOOST_NUMERIC_ODEINT_EXTERNAL_VEXCL_VEXCL_SAME_INSTANCE_HPP_INCLUDED

#include <vexcl/vector.hpp>
#include <vexcl/multivector.hpp>

#include <boost/numeric/odeint/util/same_instance.hpp>

namespace boost {
namespace numeric {
namespace odeint {

template <typename T>
struct same_instance_impl< vex::vector<T> , vex::vector<T> >
{
    static bool same_instance( const vex::vector<T> &x1 , const vex::vector<T> &x2 )
    {
        return
            static_cast<const vex::vector<T>*>(&x1) ==
            static_cast<const vex::vector<T>*>(&x2);
    }
};

template <typename T, size_t N>
struct same_instance_impl< vex::multivector<T, N> , vex::multivector<T, N> >
{
    static bool same_instance( const vex::multivector<T, N> &x1 , const vex::multivector<T, N> &x2 )
    {
        return
            static_cast<const vex::multivector<T, N>*>(&x1) ==
            static_cast<const vex::multivector<T, N>*>(&x2);
    }
};

} // namespace odeint
} // namespace numeric
} // namespace boost



#endif // BOOST_NUMERIC_ODEINT_EXTERNAL_VEXCL_VEXCL_SAME_INSTANCE_HPP_INCLUDED

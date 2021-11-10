// Copyright David Abrahams 2006. Distributed under the Boost
// Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_CONCEPT_CHECK_HAS_CONSTRAINTS_DWA2006429_HPP
# define BOOST_CONCEPT_CHECK_HAS_CONSTRAINTS_DWA2006429_HPP

namespace boost { namespace concept_checking { 

// Here we implement the "metafunction" that detects whether a
// constraints metafunction exists
typedef char yes;
typedef char (&no)[2];

template <class Model, void (Model::*)()>
struct wrap_constraints {};
    
template <class Model>
inline yes has_constraints_(Model*, wrap_constraints<Model,&Model::constraints>* = 0);
inline no has_constraints_(...);

template <class Model>
struct has_constraints
{
    BOOST_STATIC_CONSTANT(
        bool
      , value = sizeof( concept_checking::has_constraints_((Model*)0) ) == 1 );
};

}} // namespace boost::concept_checking

#endif // BOOST_CONCEPT_CHECK_HAS_CONSTRAINTS_DWA2006429_HPP

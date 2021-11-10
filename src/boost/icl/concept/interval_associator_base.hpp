/*-----------------------------------------------------------------------------+    
Copyright (c) 2011-2011: Joachim Faulhaber
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/
#ifndef BOOST_ICL_CONCEPT_INTERVAL_ASSOCIATOR_BASE_HPP_JOFA_110301
#define BOOST_ICL_CONCEPT_INTERVAL_ASSOCIATOR_BASE_HPP_JOFA_110301

#include <boost/icl/type_traits/domain_type_of.hpp>
#include <boost/icl/type_traits/interval_type_of.hpp>
#include <boost/icl/type_traits/is_combinable.hpp>
#include <boost/icl/concept/set_value.hpp>
#include <boost/icl/concept/map_value.hpp>
#include <boost/icl/concept/interval.hpp>

namespace boost{ namespace icl
{

//==============================================================================
//= Selection<IntervalSet|IntervalMap>
//==============================================================================
template<class Type> inline
typename enable_if<mpl::and_< is_interval_container<Type>
                            , is_discrete<typename domain_type_of<Type>::type> 
                            > 
                   , typename Type::const_iterator>::type
find(const Type& object, const typename domain_type_of<Type>::type& key_val)
{
    //CL typedef typename Type::const_iterator const_iterator;
    typedef typename Type::interval_type  interval_type;
    return object.find(icl::detail::unit_trail<interval_type>(key_val));
}

template<class Type> inline
typename enable_if<mpl::and_< is_interval_container<Type>
                            , is_continuous<typename domain_type_of<Type>::type> 
                            , has_dynamic_bounds<typename interval_type_of<Type>::type> 
                            > 
                   , typename Type::const_iterator>::type
find(const Type& object, const typename domain_type_of<Type>::type& key_val)
{
    //CL typedef typename Type::const_iterator const_iterator;
    typedef typename Type::interval_type  interval_type;
    return object.find(icl::singleton<interval_type>(key_val));
}

template<class Type> inline
typename enable_if<mpl::and_< is_interval_container<Type>
                            , is_continuous<typename domain_type_of<Type>::type> 
                            , is_static_right_open<typename interval_type_of<Type>::type> 
                            , boost::detail::is_incrementable<typename domain_type_of<Type>::type>
                            > 
                   , typename Type::const_iterator>::type
find(const Type& object, const typename domain_type_of<Type>::type& key_val)
{
    typedef typename Type::const_iterator const_iterator;
    typedef typename Type::interval_type  interval_type;
    const_iterator first_collision = object.lower_bound(icl::detail::unit_trail<interval_type>(key_val));
    // A part of the unit_trail(key_value)-interval may be found in the container, that
    // does not contain key_value. Therefore we have to check for its existence:
    return (  first_collision == object.end() 
           || icl::contains(key_value<Type>(first_collision), key_val) ) 
            ? first_collision 
            : object.end();
}

template<class Type> inline
typename enable_if<mpl::and_< is_interval_container<Type>
                            , is_continuous<typename domain_type_of<Type>::type> 
                            , is_static_left_open<typename interval_type_of<Type>::type> 
                            , boost::detail::is_incrementable<typename domain_type_of<Type>::type>
                            > 
                   , typename Type::const_iterator>::type
find(const Type& object, const typename domain_type_of<Type>::type& key_val)
{
    typedef typename Type::const_iterator const_iterator;
    typedef typename Type::interval_type  interval_type;
    const_iterator last_collision = object.upper_bound(icl::detail::unit_trail<interval_type>(key_val));
    if(last_collision != object.begin())
        --last_collision;
    // A part of the unit_trail(key_value)-interval may be found in the container, that
    // does not contain key_value. Therefore we have to check for its existence:
    return (  last_collision == object.end() 
           || icl::contains(key_value<Type>(last_collision), key_val) ) 
            ? last_collision 
            : object.end();
}

// NOTE: find(object, key) won't compile if key is of continuous type that does
// not implement in(de)crementation (e.g. std::string).

template<class Type> inline
typename enable_if< is_interval_container<Type>
                  , typename Type::const_iterator>::type
find(const Type& object, const typename interval_type_of<Type>::type& inter_val)
{
    return object.find(inter_val);
}

//==============================================================================
//= Morphisms
//==============================================================================
template<class Type>
typename enable_if<is_interval_container<Type>, Type>::type&
join(Type& object)
{
    typedef typename Type::interval_type interval_type;
    typedef typename Type::iterator      iterator;

    iterator it_ = object.begin();
    if(it_ == object.end()) 
        return object;

    iterator next_ = it_; next_++;

    while(next_ != object.end())
    {
        if( segmental::is_joinable<Type>(it_, next_) )
        {
            iterator fst_mem = it_;  // hold the first member
            
            // Go on while touching members are found
            it_++; next_++;
            while(     next_ != object.end()
                    && segmental::is_joinable<Type>(it_, next_) )
            { it_++; next_++; }

            // finally we arrive at the end of a sequence of joinable intervals
            // and it points to the last member of that sequence
            const_cast<interval_type&>(key_value<Type>(it_)) 
                = hull(key_value<Type>(it_), key_value<Type>(fst_mem));
            object.erase(fst_mem, it_);

            it_++; next_=it_; 
            if(next_!=object.end())
                next_++;
        }
        else { it_++; next_++; }
    }
    return object;
}

}} // namespace boost icl

#endif



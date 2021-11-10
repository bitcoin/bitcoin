/*-----------------------------------------------------------------------------+    
Copyright (c) 2010-2010: Joachim Faulhaber
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/
#ifndef BOOST_ICL_CONCEPT_ELEMENT_MAP_HPP_JOFA_100921
#define BOOST_ICL_CONCEPT_ELEMENT_MAP_HPP_JOFA_100921

#include <boost/mpl/and.hpp>
#include <boost/mpl/not.hpp>
#include <boost/icl/detail/on_absorbtion.hpp>
#include <boost/icl/type_traits/unit_element.hpp>
#include <boost/icl/type_traits/is_total.hpp>
#include <boost/icl/type_traits/absorbs_identities.hpp>
#include <boost/icl/type_traits/is_associative_element_container.hpp>
#include <boost/icl/type_traits/is_combinable.hpp>

#include <boost/icl/concept/map_value.hpp>
#include <boost/icl/detail/map_algo.hpp>


namespace boost{ namespace icl
{

//NOTE: Some forward declarations are needed by some compilers.
template<class Type, class Predicate>
typename enable_if<is_associative_element_container<Type>, Type>::type&
erase_if(const Predicate& pred, Type& object);


//==============================================================================
//= Containedness<ElementMap>
//==============================================================================
//------------------------------------------------------------------------------
//- bool within(c P&, c T&) T:{m} P:{b} fragment_types
//------------------------------------------------------------------------------
/** Checks if a key-value pair is in the map */
template<class Type>
typename enable_if<is_element_map<Type>, bool>::type
within(const typename Type::element_type& value_pair, const Type& super)
{ 
    typedef typename Type::const_iterator const_iterator;
    const_iterator found_ = super.find(value_pair.first);
    return found_ != super.end() && (*found_).second == value_pair.second;
}

//------------------------------------------------------------------------------
//- bool contains(c T&, c P&) T:{m} P:{b} fragment_types
//------------------------------------------------------------------------------
template<class Type>
typename enable_if<is_element_map<Type>, bool>::type
contains(const Type& super, const typename Type::element_type& value_pair)
{
    return icl::within(value_pair, super);
}

//==============================================================================
//= Equivalences and Orderings<ElementMap>
//==============================================================================

/** Protonic equality is equality on all elements that do not carry an identity element as content. */
template<class Type>
inline typename enable_if<is_element_map<Type>, bool>::type
is_distinct_equal(const Type& lhs, const Type& rhs)
{
    return Map::lexicographical_distinct_equal(lhs, rhs);
}

//==============================================================================
//= Addition<ElementMap>
//==============================================================================
/** \c add inserts \c value_pair into the map if it's key does 
    not exist in the map.    
    If \c value_pairs's key value exists in the map, it's data
    value is added to the data value already found in the map. */
template <class Type>
typename enable_if<is_element_map<Type>, Type>::type&
add(Type& object, const typename Type::value_type& value_pair)
{
    return object.add(value_pair); 
}

/** \c add add \c value_pair into the map using \c prior as a hint to
    insert \c value_pair after the position \c prior is pointing to. */
template <class Type>
typename enable_if<is_element_map<Type>, typename Type::iterator>::type
add(Type& object, typename Type::iterator prior, 
    const typename Type::value_type& value_pair) 
{ 
    return object.add(prior, value_pair); 
}

//==============================================================================
//= Erasure
//==============================================================================
//------------------------------------------------------------------------------
//- T& erase(T&, c P&) T:{m} P:{b} fragment_type
//------------------------------------------------------------------------------
template <class Type>
typename enable_if<is_element_map<Type>, typename Type::size_type>::type
erase(Type& object, const typename Type::element_type& value_pair)
{
    typedef typename Type::size_type             size_type;
    typedef typename Type::iterator              iterator;
    typedef typename Type::on_identity_absorbtion on_identity_absorbtion;

    if(on_identity_absorbtion::is_absorbable(value_pair.second)) 
        return identity_element<size_type>::value();

    iterator it_ = object.find(value_pair.first);
    if(it_ != object.end() && value_pair.second == (*it_).second)
    {
        object.erase(it_);
        return unit_element<size_type>::value();
    }

    return identity_element<size_type>::value();
}

template<class Type>
typename enable_if<is_element_map<Type>, Type>::type&
erase(Type& object, const typename Type::set_type& erasure)
{
    typedef typename Type::set_type set_type;
    ICL_const_FORALL(typename set_type, elem_, erasure) 
        icl::erase(object, *elem_); 

    return object; 
}

//==============================================================================
//= Subtraction
//==============================================================================
//------------------------------------------------------------------------------
//- T& subtract(T&, c P&) T:{m} P:{b} fragment_type
//------------------------------------------------------------------------------
template <class Type>
inline typename enable_if<is_element_map<Type>, Type>::type&
subtract(Type& object, const typename Type::element_type& operand) 
{ 
    return object.subtract(operand); 
}

//------------------------------------------------------------------------------
//- T& subtract(T&, c P&) T:{m} P:{e} key_type
//------------------------------------------------------------------------------
template <class Type>
typename enable_if<is_element_map<Type>, Type>::type&
subtract(Type& object, const typename Type::domain_type& key_value)
{
    return icl::erase(object, key_value);
}

//------------------------------------------------------------------------------
//- T& subtract(T&, c P&) T:{m} P:{s} set key_type 
//------------------------------------------------------------------------------
template <class Type>
inline typename enable_if<is_element_map<Type>, Type>::type&
operator -= (Type& object, const typename Type::set_type& operand) 
{ 
    typedef typename Type::set_type           set_type;
    typedef typename set_type::const_iterator co_iterator;
    typedef typename Type::iterator           iterator;

    co_iterator common_lwb_, common_upb_;
    if(!Set::common_range(common_lwb_, common_upb_, operand, object))
        return object;

    co_iterator it_ = common_lwb_;
    iterator    common_;

    while(it_ != common_upb_)
        object.erase(*it_++);

    return object;
}

template <class Type>
inline typename enable_if<is_element_map<Type>, Type>::type
operator - (Type object, const typename Type::set_type& subtrahend) 
{ 
    return object -= subtrahend; 
}

//==============================================================================
//= Selective Update<ElementMap>
//==============================================================================
//------------------------------------------------------------------------------
//- T& set_at(T&, c P&) T:{m} P:{b}
//------------------------------------------------------------------------------
template<class Type>
inline typename enable_if<is_element_map<Type>, Type>::type&
set_at(Type& object, const typename Type::element_type& operand)
{
    typedef typename Type::iterator         iterator;
    typedef typename Type::codomain_combine codomain_combine;
    typedef on_absorbtion<Type,codomain_combine,absorbs_identities<Type>::value>
                                            on_identity_absorbtion;

    if(!on_identity_absorbtion::is_absorbable(operand.second))
    {
        std::pair<iterator,bool> insertion = object.insert(operand);
        if(!insertion.second)
            insertion->second = operand.second;
    }
    return object;
}


//==============================================================================
//= Intersection
//==============================================================================
template<class Type>
inline typename enable_if<is_element_map<Type>, void>::type
add_intersection(Type& section, const Type&               object, 
                       const typename Type::element_type& operand)
{
    object.add_intersection(section, operand);
}

template<class Type>
inline typename enable_if<is_element_map<Type>, void>::type
add_intersection(Type& section, const Type& object, const Type& operand)
{
    ICL_const_FORALL(typename Type, it_, operand) 
        icl::add_intersection(section, object, *it_); 
}

//------------------------------------------------------------------------------
//- T& op &=(T&, c P&) T:{m} P:{b m} fragment_types
//------------------------------------------------------------------------------

template<class Type>
inline typename enable_if<mpl::and_<is_element_map<Type>, is_total<Type> >, Type>::type&
operator &=(Type& object, const typename Type::element_type& operand)
{
    object.add(operand);
    return object;
}

template<class Type>
inline typename enable_if<mpl::and_<is_element_map<Type>, mpl::not_<is_total<Type> > >, Type>::type&
operator &=(Type& object, const typename Type::element_type& operand)
{
    Type section;
    icl::add_intersection(section, object, operand);
    object.swap(section);
    return object; 
}

template<class Type>
inline typename enable_if<is_element_map<Type>, Type>::type
operator & (Type object, const typename Type::element_type& operand)
{
    return object &= operand;
}

template<class Type>
inline typename enable_if<is_element_map<Type>, Type>::type
operator & (const typename Type::element_type& operand, Type object)
{
    return object &= operand;
}


template<class Type>
inline typename enable_if<mpl::and_<is_element_map<Type>, is_total<Type> >, Type>::type&
operator &=(Type& object, const Type& operand)
{
    object += operand; 
    return object;
}

template<class Type>
inline typename enable_if<mpl::and_<is_element_map<Type>, mpl::not_<is_total<Type> > >, Type>::type&
operator &=(Type& object, const Type& operand)
{
    Type section;
    icl::add_intersection(section, object, operand);
    object.swap(section);
    return object; 
}

template<class Type>
inline typename enable_if<is_element_map<Type>, Type>::type
operator & (Type object, const typename Type::key_object_type& operand)
{
    return object &= operand;
}

template<class Type>
inline typename enable_if<is_element_map<Type>, Type>::type
operator & (const typename Type::key_object_type& operand, Type object)
{
    return object &= operand;
}

//==============================================================================
//= Intersection<ElementMap> bool intersects(x,y)
//==============================================================================
template<class Type, class CoType>
inline typename enable_if< mpl::and_< is_element_map<Type>
                                    , is_total<Type>      > 
                         , bool>::type
intersects(const Type&, const CoType&)
{
    return true;
}

template<class Type>
inline typename enable_if< mpl::and_< is_element_map<Type>
                                    , mpl::not_<is_total<Type> > > 
                         , bool>::type
intersects(const Type& object, const typename Type::domain_type& operand)
{
    return icl::contains(object, operand);
}

template<class Type>
inline typename enable_if< mpl::and_< is_element_map<Type>
                                    , mpl::not_<is_total<Type> > > 
                         , bool>::type
intersects(const Type& object, const typename Type::set_type& operand)
{
    if(object.iterative_size() < operand.iterative_size())
        return Map::intersects(object, operand);
    else
        return Map::intersects(operand, object);
}

template<class Type>
inline typename enable_if< mpl::and_< is_element_map<Type>
                                    , mpl::not_<is_total<Type> > > 
                         , bool>::type
intersects(const Type& object, const typename Type::element_type& operand)
{
    Type intersection;
    icl::add_intersection(intersection, object, operand);
    return !intersection.empty();
}

template<class Type>
inline typename enable_if< mpl::and_< is_element_map<Type>
                                    , mpl::not_<is_total<Type> > > 
                         , bool>::type
intersects(const Type& object, const Type& operand)
{
    if(object.iterative_size() < operand.iterative_size())
        return Map::intersects(object, operand);
    else
        return Map::intersects(operand, object);
}

//==============================================================================
//= Symmetric difference
//==============================================================================
template<class Type>
inline typename enable_if<is_element_map<Type>, Type>::type&
flip(Type& object, const typename Type::element_type& operand)
{
    return object.flip(operand);
}

template<class Type, class CoType>
inline typename enable_if< mpl::and_< is_element_map<Type>
                                    , is_total<Type>       
                                    , absorbs_identities<Type> > 
                         , Type>::type&
operator ^= (Type& object, const CoType&)
{
    icl::clear(object);
    return object;
}

template<class Type>
inline typename enable_if< mpl::and_< is_element_map<Type>
                                    , is_total<Type>       
                                    , mpl::not_<absorbs_identities<Type> > > 
                         , Type>::type&
operator ^= (Type& object, const typename Type::element_type& operand)
{
    return object.flip(operand);
}

template<class Type>
inline typename enable_if< mpl::and_< is_element_map<Type>
                                    , is_total<Type>       
                                    , mpl::not_<absorbs_identities<Type> > > 
                         , Type>::type&
operator ^= (Type& object, const Type& operand)
{
    ICL_const_FORALL(typename Type, it_, operand)
        icl::flip(object, *it_);

    ICL_FORALL(typename Type, it2_, object)
        (*it2_).second = identity_element<typename Type::codomain_type>::value();

    return object;
}


template<class Type>
inline typename enable_if< mpl::and_< is_element_map<Type>
                                    , mpl::not_<is_total<Type> > > 
                         , Type>::type&
operator ^= (Type& object, const typename Type::element_type& operand)
{
    return icl::flip(object, operand);
}

template<class Type>
inline typename enable_if< mpl::and_< is_element_map<Type>
                                    , mpl::not_<is_total<Type> > > 
                         , Type>::type&
operator ^= (Type& object, const Type& operand)
{
    typedef typename Type::const_iterator const_iterator;
    const_iterator it_ = operand.begin();
    while(it_ != operand.end())
        icl::flip(object, *it_++);

    return object;
}


//==============================================================================
//= Set selection
//==============================================================================
template<class Type>
inline typename enable_if<is_element_map<Type>, 
                          typename Type::set_type>::type&
domain(typename Type::set_type& domain_set, const Type& object)
{
    typename Type::set_type::iterator prior_ = domain_set.end();
    typename Type::const_iterator it_ = object.begin();
    while(it_ != object.end())
        prior_ = domain_set.insert(prior_, (*it_++).first);

    return domain_set;
}

//==============================================================================
//= Neutron absorbtion
//==============================================================================
template<class Type>
inline typename enable_if<mpl::and_< is_element_map<Type>
                                   , absorbs_identities<Type> >, Type>::type&
absorb_identities(Type& object)
{
    typedef typename Type::element_type element_type;
    return icl::erase_if(content_is_identity_element<element_type>(), object);
}

template<class Type>
inline typename enable_if<mpl::and_< is_element_map<Type>
                                   , mpl::not_<absorbs_identities<Type> > >
                         , Type>::type&
absorb_identities(Type&){}

//==============================================================================
//= Streaming<ElementMap>
//==============================================================================
template<class CharType, class CharTraits, class Type>
inline typename enable_if<is_element_map<Type>, std::basic_ostream<CharType, CharTraits> >::type&
operator << (std::basic_ostream<CharType, CharTraits>& stream, const Type& object)
{
    stream << "{";
    ICL_const_FORALL(typename Type, it, object)
        stream << "(" << it->first << "->" << it->second << ")";

    return stream << "}";
}


}} // namespace boost icl

#endif



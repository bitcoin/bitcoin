/*-----------------------------------------------------------------------------+    
Copyright (c) 2010-2010: Joachim Faulhaber
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/
#ifndef BOOST_ICL_CONCEPT_INTERVAL_ASSOCIATOR_HPP_JOFA_100920
#define BOOST_ICL_CONCEPT_INTERVAL_ASSOCIATOR_HPP_JOFA_100920

#include <boost/range/iterator_range.hpp>
#include <boost/icl/type_traits/domain_type_of.hpp>
#include <boost/icl/type_traits/interval_type_of.hpp>
#include <boost/icl/type_traits/is_combinable.hpp>
#include <boost/icl/detail/set_algo.hpp>
#include <boost/icl/detail/map_algo.hpp>
#include <boost/icl/detail/interval_set_algo.hpp>
#include <boost/icl/detail/interval_map_algo.hpp>
#include <boost/icl/concept/interval.hpp>

namespace boost{ namespace icl
{

//==============================================================================
//= Containedness<IntervalSet|IntervalMap>
//==============================================================================
//------------------------------------------------------------------------------
//- bool within(c T&, c P&) T={Set,Map} P={e i b p S M}
//------------------------------------------------------------------------------
template<class SubT, class SuperT>
typename enable_if<is_interval_container<SuperT>, bool>::type 
within(const SubT& sub, const SuperT& super)
{
    return icl::contains(super, sub); 
}

//==============================================================================
//= Equivalences and Orderings<IntervalSet|IntervalMap>
//==============================================================================
template<class Type>
inline typename enable_if<is_interval_container<Type>, bool>::type
operator == (const Type& left, const Type& right)
{
    return Set::lexicographical_equal(left, right);
}

template<class Type>
inline typename enable_if<is_interval_container<Type>, bool>::type
operator < (const Type& left, const Type& right)
{
    typedef typename Type::segment_compare segment_compare;
    return std::lexicographical_compare(
        left.begin(), left.end(), right.begin(), right.end(), 
        segment_compare()
        );
}

/** Returns true, if \c left and \c right contain the same elements. 
    Complexity: linear. */
template<class LeftT, class RightT>
typename enable_if<is_intra_combinable<LeftT, RightT>, bool>::type
is_element_equal(const LeftT& left, const RightT& right)
{
    return Interval_Set::is_element_equal(left, right);
}

/** Returns true, if \c left is lexicographically less than \c right. 
    Intervals are interpreted as sequence of elements.
    Complexity: linear. */
template<class LeftT, class RightT>
typename enable_if<is_intra_combinable<LeftT, RightT>, bool>::type
is_element_less(const LeftT& left, const RightT& right)
{
    return Interval_Set::is_element_less(left, right);
}

/** Returns true, if \c left is lexicographically greater than \c right. 
    Intervals are interpreted as sequence of elements.
    Complexity: linear. */
template<class LeftT, class RightT>
typename enable_if<is_intra_combinable<LeftT, RightT>, bool>::type
is_element_greater(const LeftT& left, const RightT& right)
{
    return Interval_Set::is_element_greater(left, right);
}

//------------------------------------------------------------------------------
template<class LeftT, class RightT>
typename enable_if<is_inter_combinable<LeftT, RightT>, int>::type
inclusion_compare(const LeftT& left, const RightT& right)
{
    return Interval_Set::subset_compare(left, right, 
                                        left.begin(), left.end(),
                                        right.begin(), right.end());
}

//------------------------------------------------------------------------------
template<class LeftT, class RightT>
typename enable_if< is_concept_compatible<is_interval_map, LeftT, RightT>,
                    bool >::type
is_distinct_equal(const LeftT& left, const RightT& right)
{
    return Map::lexicographical_distinct_equal(left, right);
}

//==============================================================================
//= Size<IntervalSet|IntervalMap>
//==============================================================================
template<class Type> 
typename enable_if<is_interval_container<Type>, std::size_t>::type
iterative_size(const Type& object)
{ 
    return object.iterative_size(); 
}

template<class Type>
typename enable_if
< mpl::and_< is_interval_container<Type>
           , is_discrete<typename Type::domain_type> >
, typename Type::size_type
>::type
cardinality(const Type& object)
{
    typedef typename Type::size_type size_type;
    //CL typedef typename Type::interval_type interval_type;

    size_type size = identity_element<size_type>::value();
    ICL_const_FORALL(typename Type, it, object)
        size += icl::cardinality(key_value<Type>(it));
    return size;

}

template<class Type>
typename enable_if
< mpl::and_< is_interval_container<Type>
           , mpl::not_<is_discrete<typename Type::domain_type> > >
, typename Type::size_type
>::type
cardinality(const Type& object)
{
    typedef typename Type::size_type size_type;
    //CL typedef typename Type::interval_type interval_type;

    size_type size = identity_element<size_type>::value();
    size_type interval_size;
    ICL_const_FORALL(typename Type, it, object)
    {
        interval_size = icl::cardinality(key_value<Type>(it));
        if(interval_size == icl::infinity<size_type>::value())
            return interval_size;
        else
            size += interval_size;
    }
    return size;
}

template<class Type>
inline typename enable_if<is_interval_container<Type>, typename Type::size_type>::type
size(const Type& object)
{
    return icl::cardinality(object);
}

template<class Type>
typename enable_if<is_interval_container<Type>, typename Type::difference_type>::type
length(const Type& object)
{
    typedef typename Type::difference_type difference_type;
    typedef typename Type::const_iterator  const_iterator;
    difference_type length = identity_element<difference_type>::value();
    const_iterator it_ = object.begin();

    while(it_ != object.end())
        length += icl::length(key_value<Type>(it_++));
    return length;
}

template<class Type>
typename enable_if<is_interval_container<Type>, std::size_t>::type
interval_count(const Type& object)
{
    return icl::iterative_size(object);
}


template<class Type>
typename enable_if< is_interval_container<Type> 
                  , typename Type::difference_type >::type
distance(const Type& object)
{
    typedef typename Type::difference_type DiffT;
    typedef typename Type::const_iterator const_iterator;
    const_iterator it_ = object.begin(), pred_;
    DiffT dist = identity_element<DiffT>::value();

    if(it_ != object.end())
        pred_ = it_++;

    while(it_ != object.end())
        dist += icl::distance(key_value<Type>(pred_++), key_value<Type>(it_++));
    
    return dist;
}

//==============================================================================
//= Range<IntervalSet|IntervalMap>
//==============================================================================
template<class Type>
typename enable_if<is_interval_container<Type>, 
                   typename Type::interval_type>::type
hull(const Type& object)
{
    return 
        icl::is_empty(object) 
            ? identity_element<typename Type::interval_type>::value()
            : icl::hull( key_value<Type>(object.begin()), 
                         key_value<Type>(object.rbegin()) );
}

template<class Type>
typename enable_if<is_interval_container<Type>, 
                   typename domain_type_of<Type>::type>::type
lower(const Type& object)
{
    typedef typename domain_type_of<Type>::type DomainT;
    return 
        icl::is_empty(object) 
            ? unit_element<DomainT>::value()
            : icl::lower( key_value<Type>(object.begin()) );
}

template<class Type>
typename enable_if<is_interval_container<Type>, 
                   typename domain_type_of<Type>::type>::type
upper(const Type& object)
{
    typedef typename domain_type_of<Type>::type DomainT;
    return 
        icl::is_empty(object) 
            ? identity_element<DomainT>::value()
            : icl::upper( key_value<Type>(object.rbegin()) );
}

//------------------------------------------------------------------------------
template<class Type>
typename enable_if
< mpl::and_< is_interval_container<Type>
           , is_discrete<typename domain_type_of<Type>::type> > 
, typename domain_type_of<Type>::type>::type
first(const Type& object)
{
    typedef typename domain_type_of<Type>::type DomainT;
    return 
        icl::is_empty(object) 
            ? unit_element<DomainT>::value()
            : icl::first( key_value<Type>(object.begin()) );
}

template<class Type>
typename enable_if
< mpl::and_< is_interval_container<Type>
           , is_discrete<typename domain_type_of<Type>::type> >
, typename domain_type_of<Type>::type>::type
last(const Type& object)
{
    typedef typename domain_type_of<Type>::type DomainT;
    return 
        icl::is_empty(object) 
            ? identity_element<DomainT>::value()
            : icl::last( key_value<Type>(object.rbegin()) );
}


//==============================================================================
//= Addition<IntervalSet|IntervalMap>
//==============================================================================
//------------------------------------------------------------------------------
//- T& op +=(T&, c P&) T:{S}|{M} P:{e i}|{b p}
//------------------------------------------------------------------------------
/* \par \b Requires: \c OperandT is an addable derivative type of \c Type. 
    \b Effects: \c operand is added to \c object.
    \par \b Returns: A reference to \c object.
    \b Complexity:
\code
                  \ OperandT:                    
                   \         element     segment 
Type:
       interval container    O(log n)     O(n)   

             interval_set               amortized
    spearate_interval_set                O(log n) 

n = object.interval_count()
\endcode

For the addition of \b elements or \b segments
complexity is \b logarithmic or \b linear respectively.
For \c interval_sets and \c separate_interval_sets addition of segments
is \b amortized \b logarithmic.
*/
template<class Type, class OperandT>
typename enable_if<is_intra_derivative<Type, OperandT>, Type>::type&
operator += (Type& object, const OperandT& operand)
{ 
    return icl::add(object, operand); 
}


//------------------------------------------------------------------------------
//- T& op +=(T&, c P&) T:{S}|{M} P:{S'}|{M'}
//------------------------------------------------------------------------------
/** \par \b Requires: \c OperandT is an interval container addable to \c Type. 
    \b Effects: \c operand is added to \c object.
    \par \b Returns: A reference to \c object.
    \b Complexity: loglinear */
template<class Type, class OperandT>
typename enable_if<is_intra_combinable<Type, OperandT>, Type>::type&
operator += (Type& object, const OperandT& operand)
{
    typename Type::iterator prior_ = object.end();
    ICL_const_FORALL(typename OperandT, elem_, operand) 
        prior_ = icl::add(object, prior_, *elem_); 

    return object; 
}


#ifdef BOOST_ICL_NO_CXX11_RVALUE_REFERENCES
//------------------------------------------------------------------------------
//- T op + (T, c P&) T:{S}|{M} P:{e i S}|{b p M}
//------------------------------------------------------------------------------
/** \par \b Requires: \c object and \c operand are addable.
    \b Effects: \c operand is added to \c object.
    \par \b Efficieny: There is one additional copy of 
    \c Type \c object compared to inplace \c operator \c += */
template<class Type, class OperandT>
typename enable_if<is_binary_intra_combinable<Type, OperandT>, Type>::type
operator + (Type object, const OperandT& operand)
{
    return object += operand; 
}

#else //BOOST_ICL_NO_CXX11_RVALUE_REFERENCES

template<class Type, class OperandT>
typename enable_if<is_binary_intra_combinable<Type, OperandT>, Type>::type
operator + (const Type& object, const OperandT& operand)
{
    Type temp = object;
    return boost::move(temp += operand); 
}

template<class Type, class OperandT>
typename enable_if<is_binary_intra_combinable<Type, OperandT>, Type>::type
operator + (Type&& object, const OperandT& operand)
{
    return boost::move(object += operand); 
}

#endif //BOOST_ICL_NO_CXX11_RVALUE_REFERENCES

#ifdef BOOST_ICL_NO_CXX11_RVALUE_REFERENCES
//------------------------------------------------------------------------------
//- T op + (c P&, T) T:{S}|{M} P:{e i S'}|{b p M'}
//------------------------------------------------------------------------------
/** \par \b Requires: \c object and \c operand are addable.
    \b Effects: \c operand is added to \c object.
    \par \b Efficieny: There is one additional copy of 
    \c Type \c object compared to inplace \c operator \c += */
template<class Type, class OperandT>
typename enable_if<is_binary_intra_combinable<Type, OperandT>, Type>::type
operator + (const OperandT& operand, Type object)
{
    return object += operand; 
}

#else //BOOST_ICL_NO_CXX11_RVALUE_REFERENCES

template<class Type, class OperandT>
typename enable_if<is_binary_intra_combinable<Type, OperandT>, Type>::type
operator + (const OperandT& operand, const Type& object)
{
    Type temp = object;
    return boost::move(temp += operand);
}

template<class Type, class OperandT>
typename enable_if<is_binary_intra_combinable<Type, OperandT>, Type>::type
operator + (const OperandT& operand, Type&& object)
{
    return boost::move(object += operand); 
}

#endif //BOOST_ICL_NO_CXX11_RVALUE_REFERENCES

#ifdef BOOST_ICL_NO_CXX11_RVALUE_REFERENCES
//------------------------------------------------------------------------------
//- T op + (T, c P&) T:{S}|{M} P:{S}|{M}
//------------------------------------------------------------------------------
/** \par \b Requires: \c object and \c operand are addable.
    \b Effects: \c operand is added to \c object.
    \par \b Efficieny: There is one additional copy of 
    \c Type \c object compared to inplace \c operator \c += */
template<class Type>
typename enable_if<is_interval_container<Type>, Type>::type
operator + (Type object, const Type& operand)
{
    return object += operand; 
}

#else //BOOST_ICL_NO_CXX11_RVALUE_REFERENCES

template<class Type>
typename enable_if<is_interval_container<Type>, Type>::type
operator + (const Type& object, const Type& operand)
{
    Type temp = object;
    return boost::move(temp += operand); 
}

template<class Type>
typename enable_if<is_interval_container<Type>, Type>::type
operator + (Type&& object, const Type& operand)
{
    return boost::move(object += operand); 
}

template<class Type>
typename enable_if<is_interval_container<Type>, Type>::type
operator + (const Type& operand, Type&& object)
{
    return boost::move(object += operand); 
}

template<class Type>
typename enable_if<is_interval_container<Type>, Type>::type
operator + (Type&& object, Type&& operand)
{
    return boost::move(object += operand); 
}

#endif //BOOST_ICL_NO_CXX11_RVALUE_REFERENCES

//------------------------------------------------------------------------------
//- Addition |=, | 
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//- T& op |=(c P&) T:{S}|{M} P:{e i}|{b p}
//------------------------------------------------------------------------------
/** \par \b Requires: Types \c Type and \c OperandT are addable.
    \par \b Effects: \c operand is added to \c object.
    \par \b Returns: A reference to \c object.
    \b Complexity:
\code
                  \ OperandT:                      interval
                   \         element     segment   container
Type:
       interval container    O(log n)     O(n)     O(m log(n+m))

             interval_set               amortized
    spearate_interval_set                O(log n) 

n = object.interval_count()
m = operand.interval_count()
\endcode

For the addition of \b elements, \b segments and \b interval \b containers
complexity is \b logarithmic, \b linear and \b loglinear respectively.
For \c interval_sets and \c separate_interval_sets addition of segments
is \b amortized \b logarithmic.
*/
template<class Type, class OperandT>
typename enable_if<is_right_intra_combinable<Type, OperandT>, Type>::type&
operator |= (Type& object, const OperandT& operand)
{ 
    return object += operand; 
}

#ifdef BOOST_ICL_NO_CXX11_RVALUE_REFERENCES
//------------------------------------------------------------------------------
//- T op | (T, c P&) T:{S}|{M} P:{e i S}|{b p M}
//------------------------------------------------------------------------------
/** \par \b Requires: \c object and \c operand are addable.
    \b Effects: \c operand is added to \c object.
    \par \b Efficieny: There is one additional copy of 
    \c Type \c object compared to inplace \c operator \c |= */
template<class Type, class OperandT>
typename enable_if<is_binary_intra_combinable<Type, OperandT>, Type>::type
operator | (Type object, const OperandT& operand)
{
    return object += operand; 
}

#else //BOOST_ICL_NO_CXX11_RVALUE_REFERENCES

template<class Type, class OperandT>
typename enable_if<is_binary_intra_combinable<Type, OperandT>, Type>::type
operator | (const Type& object, const OperandT& operand)
{
    Type temp = object;
    return boost::move(temp += operand); 
}

template<class Type, class OperandT>
typename enable_if<is_binary_intra_combinable<Type, OperandT>, Type>::type
operator | (Type&& object, const OperandT& operand)
{
    return boost::move(object += operand); 
}

#endif //BOOST_ICL_NO_CXX11_RVALUE_REFERENCES

#ifdef BOOST_ICL_NO_CXX11_RVALUE_REFERENCES
//------------------------------------------------------------------------------
//- T op | (T, c P&) T:{S}|{M} P:{S}|{M}
//------------------------------------------------------------------------------
/** \par \b Requires: \c object and \c operand are addable.
    \b Effects: \c operand is added to \c object.
    \par \b Efficieny: There is one additional copy of 
    \c Type \c object compared to inplace \c operator \c |= */
template<class Type, class OperandT>
typename enable_if<is_binary_intra_combinable<Type, OperandT>, Type>::type
operator | (const OperandT& operand, Type object)
{
    return object += operand; 
}

#else //BOOST_ICL_NO_CXX11_RVALUE_REFERENCES

template<class Type, class OperandT>
typename enable_if<is_binary_intra_combinable<Type, OperandT>, Type>::type
operator | (const OperandT& operand, const Type& object)
{
    Type temp = object;
    return boost::move(temp += operand);
}

template<class Type, class OperandT>
typename enable_if<is_binary_intra_combinable<Type, OperandT>, Type>::type
operator | (const OperandT& operand, Type&& object)
{
    return boost::move(object += operand); 
}

#endif //BOOST_ICL_NO_CXX11_RVALUE_REFERENCES

#ifdef BOOST_ICL_NO_CXX11_RVALUE_REFERENCES
//------------------------------------------------------------------------------
//- T op | (T, c P&) T:{S}|{M} P:{S}|{M}
//------------------------------------------------------------------------------
/** \par \b Requires: \c object and \c operand are addable.
    \b Effects: \c operand is added to \c object.
    \par \b Efficieny: There is one additional copy of 
    \c Type \c object compared to inplace \c operator \c |= */
template<class Type>
typename enable_if<is_interval_container<Type>, Type>::type
operator | (Type object, const Type& operand)
{
    return object += operand; 
}
#else //BOOST_ICL_NO_CXX11_RVALUE_REFERENCES

template<class Type>
typename enable_if<is_interval_container<Type>, Type>::type
operator | (const Type& object, const Type& operand)
{
    Type temp = object;
    return boost::move(temp += operand); 
}

template<class Type>
typename enable_if<is_interval_container<Type>, Type>::type
operator | (Type&& object, const Type& operand)
{
    return boost::move(object += operand); 
}

template<class Type>
typename enable_if<is_interval_container<Type>, Type>::type
operator | (const Type& operand, Type&& object)
{
    return boost::move(object += operand); 
}

template<class Type>
typename enable_if<is_interval_container<Type>, Type>::type
operator | (Type&& object, Type&& operand)
{
    return boost::move(object += operand); 
}

#endif //BOOST_ICL_NO_CXX11_RVALUE_REFERENCES


//==============================================================================
//= Insertion<IntervalSet|IntervalSet>
//==============================================================================
//------------------------------------------------------------------------------
//- T& insert(T&, c P&) T:{S}|{M} P:{S'}|{M'}
//------------------------------------------------------------------------------
template<class Type, class OperandT>
typename enable_if<is_intra_combinable<Type, OperandT>, Type>::type&
insert(Type& object, const OperandT& operand)
{
    typename Type::iterator prior_ = object.end();
    ICL_const_FORALL(typename OperandT, elem_, operand) 
        insert(object, prior_, *elem_); 

    return object; 
}

//==============================================================================
//= Erasure<IntervalSet|IntervalSet>
//==============================================================================
//------------------------------------------------------------------------------
//- T& erase(T&, c P&) T:{S}|{M} P:{S'}|{S' M'}
//------------------------------------------------------------------------------
template<class Type, class OperandT>
typename enable_if<combines_right_to_interval_container<Type, OperandT>,
                   Type>::type&
erase(Type& object, const OperandT& operand)
{
    typedef typename OperandT::const_iterator const_iterator;

    if(icl::is_empty(operand))
        return object;

    const_iterator common_lwb, common_upb;
    if(!Set::common_range(common_lwb, common_upb, operand, object))
        return object;

    const_iterator it_ = common_lwb;
    while(it_ != common_upb)
        icl::erase(object, *it_++);

    return object; 
}

//==============================================================================
//= Subtraction<IntervalSet|IntervalSet>
//==============================================================================
//------------------------------------------------------------------------------
//- T& op -= (c P&) T:{M} P:{M'} 
//------------------------------------------------------------------------------
/** \par \b Requires: Types \c Type and \c OperandT are subtractable.
    \par \b Effects: \c operand is subtracted from \c object.
    \par \b Returns: A reference to \c object.
    \b Complexity:
\code
                  \ OperandT:                      interval
                   \         element    segment    container
Type:
       interval container    O(log n)     O(n)     O(m log(n+m))

                                       amortized
            interval_sets               O(log n) 

n = object.interval_count()
m = operand.interval_count()
\endcode

For the subtraction of \em elements, \b segments and \b interval \b containers
complexity is \b logarithmic, \b linear and \b loglinear respectively.
For interval sets subtraction of segments
is \b amortized \b logarithmic.
*/
template<class Type, class OperandT>
typename enable_if<is_concept_compatible<is_interval_map, Type, OperandT>, 
                   Type>::type& 
operator -=(Type& object, const OperandT& operand)
{
    ICL_const_FORALL(typename OperandT, elem_, operand) 
        icl::subtract(object, *elem_);

    return object; 
}

//------------------------------------------------------------------------------
//- T& op -= (c P&) T:{S}|{M} P:{e i}|{b p} 
//------------------------------------------------------------------------------
template<class Type, class OperandT>
typename enable_if<is_intra_derivative<Type, OperandT>, Type>::type&
operator -= (Type& object, const OperandT& operand)
{ 
    return icl::subtract(object, operand); 
}

//------------------------------------------------------------------------------
//- T& op -= (c P&) T:{M} P:{e i} 
//------------------------------------------------------------------------------
template<class Type, class OperandT>
typename enable_if<is_cross_derivative<Type, OperandT>, Type>::type&
operator -= (Type& object, const OperandT& operand)
{ 
    return icl::erase(object, operand); 
}

//------------------------------------------------------------------------------
//- T& op -= (c P&) T:{S M} P:{S'}
//------------------------------------------------------------------------------
template<class Type, class IntervalSetT>
typename enable_if<combines_right_to_interval_set<Type, IntervalSetT>,
                   Type>::type&
operator -= (Type& object, const IntervalSetT& operand)
{
    return erase(object, operand);
}

#ifdef BOOST_ICL_NO_CXX11_RVALUE_REFERENCES
//------------------------------------------------------------------------------
//- T op - (T, c P&) T:{S}|{M} P:{e i S'}|{e i b p S' M'} 
//------------------------------------------------------------------------------
template<class Type, class OperandT>
typename enable_if<is_right_inter_combinable<Type, OperandT>, Type>::type
operator - (Type object, const OperandT& operand)
{
    return object -= operand; 
}

#else //BOOST_ICL_NO_CXX11_RVALUE_REFERENCES

template<class Type, class OperandT>
typename enable_if<is_right_inter_combinable<Type, OperandT>, Type>::type
operator - (const Type& object, const OperandT& operand)
{
    Type temp = object;
    return boost::move(temp -= operand); 
}

template<class Type, class OperandT>
typename enable_if<is_right_inter_combinable<Type, OperandT>, Type>::type
operator - (Type&& object, const OperandT& operand)
{
    return boost::move(object -= operand); 
}

#endif //BOOST_ICL_NO_CXX11_RVALUE_REFERENCES

//==============================================================================
//= Intersection<IntervalSet|IntervalSet>
//==============================================================================
//------------------------------------------------------------------------------
//- void add_intersection(T&, c T&, c P&) T:{S M} P:{S'}
//------------------------------------------------------------------------------
template<class Type, class OperandT>
typename enable_if<mpl::and_<is_interval_set<Type>, 
                             combines_right_to_interval_set<Type, OperandT> >,
                   void>::type
add_intersection(Type& section, const Type& object, const OperandT& operand)
{
    typedef typename OperandT::const_iterator const_iterator;

    if(operand.empty())
        return;

    const_iterator common_lwb, common_upb;
    if(!Set::common_range(common_lwb, common_upb, operand, object))
        return;

    const_iterator it_ = common_lwb;
    while(it_ != common_upb)
        icl::add_intersection(section, object, key_value<OperandT>(it_++));
}

//------------------------------------------------------------------------------
//- T& op &=(T&, c P&) T:{S}|{M} P:{e i S'}|{e i b p S' M'}
//------------------------------------------------------------------------------
template<class Type, class OperandT>
typename enable_if<is_right_inter_combinable<Type, OperandT>, Type>::type&
operator &= (Type& object, const OperandT& operand)
{
    Type intersection;
    add_intersection(intersection, object, operand);
    object.swap(intersection);
    return object;
}

#ifdef BOOST_ICL_NO_CXX11_RVALUE_REFERENCES
//------------------------------------------------------------------------------
//- T op & (T, c P&) T:{S}|{M} P:{e i S'}|{e i b p S' M'} S<S' M<M' <:coarser
//------------------------------------------------------------------------------
template<class Type, class OperandT>
typename enable_if<is_binary_inter_combinable<Type, OperandT>, Type>::type
operator & (Type object, const OperandT& operand)
{
    return object &= operand; 
}

#else //BOOST_ICL_NO_CXX11_RVALUE_REFERENCES

template<class Type, class OperandT>
typename enable_if<is_binary_inter_combinable<Type, OperandT>, Type>::type
operator & (const Type& object, const OperandT& operand)
{
    Type temp = object;
    return boost::move(temp &= operand); 
}

template<class Type, class OperandT>
typename enable_if<is_binary_inter_combinable<Type, OperandT>, Type>::type
operator & (Type&& object, const OperandT& operand)
{
    return boost::move(object &= operand); 
}

#endif //BOOST_ICL_NO_CXX11_RVALUE_REFERENCES

#ifdef BOOST_ICL_NO_CXX11_RVALUE_REFERENCES
//------------------------------------------------------------------------------
//- T op & (c P&, T) T:{S}|{M} P:{e i S'}|{e i b p S' M'} S<S' M<M' <:coarser
//------------------------------------------------------------------------------
template<class Type, class OperandT>
typename enable_if<is_binary_inter_combinable<Type, OperandT>, Type>::type
operator & (const OperandT& operand, Type object)
{
    return object &= operand; 
}

#else //BOOST_ICL_NO_CXX11_RVALUE_REFERENCES

template<class Type, class OperandT>
typename enable_if<is_binary_inter_combinable<Type, OperandT>, Type>::type
operator & (const OperandT& operand, const Type& object)
{
    Type temp = object;
    return boost::move(temp &= operand);
}

template<class Type, class OperandT>
typename enable_if<is_binary_inter_combinable<Type, OperandT>, Type>::type
operator & (const OperandT& operand, Type&& object)
{
    return boost::move(object &= operand); 
}

#endif //BOOST_ICL_NO_CXX11_RVALUE_REFERENCES

#ifdef BOOST_ICL_NO_CXX11_RVALUE_REFERENCES
//------------------------------------------------------------------------------
//- T op & (T, c T&) T:{S M}
//------------------------------------------------------------------------------
template<class Type>
typename enable_if<is_interval_container<Type>, Type>::type
operator & (Type object, const Type& operand)
{
    return object &= operand; 
}

#else //BOOST_ICL_NO_CXX11_RVALUE_REFERENCES

template<class Type>
typename enable_if<is_interval_container<Type>, Type>::type
operator & (const Type& object, const Type& operand)
{
    Type temp = object;
    return boost::move(temp &= operand); 
}

template<class Type>
typename enable_if<is_interval_container<Type>, Type>::type
operator & (Type&& object, const Type& operand)
{
    return boost::move(object &= operand); 
}

template<class Type>
typename enable_if<is_interval_container<Type>, Type>::type
operator & (const Type& operand, Type&& object)
{
    return boost::move(object &= operand); 
}

template<class Type>
typename enable_if<is_interval_container<Type>, Type>::type
operator & (Type&& object, Type&& operand)
{
    return boost::move(object &= operand); 
}

#endif //BOOST_ICL_NO_CXX11_RVALUE_REFERENCES

//------------------------------------------------------------------------------
//- intersects<IntervalSet|IntervalMap>
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//- bool intersects(c T&, c P&) T:{S}|{M} P:{e i}
//------------------------------------------------------------------------------
template<class Type, class CoType>
typename enable_if<mpl::and_< is_interval_container<Type>
                            , boost::is_same<CoType, typename domain_type_of<Type>::type> >, 
                   bool>::type
intersects(const Type& left, const CoType& right)
{
    return icl::contains(left, right); 
}

template<class Type, class CoType>
typename enable_if<mpl::and_< is_interval_container<Type>
                            , boost::is_same<CoType, typename interval_type_of<Type>::type> >, 
                   bool>::type
intersects(const Type& left, const CoType& right)
{
    return icl::find(left, right) != left.end();
}


template<class LeftT, class RightT>
typename enable_if< mpl::and_< is_intra_combinable<LeftT, RightT> 
                             , mpl::or_<is_total<LeftT>, is_total<RightT> > >
                  , bool>::type
intersects(const LeftT&, const RightT&)
{
    return true;
}

template<class LeftT, class RightT>
typename enable_if< mpl::and_< is_intra_combinable<LeftT, RightT> 
                             , mpl::not_<mpl::or_< is_total<LeftT>
                                                 , is_total<RightT> > > >
                  , bool>::type
intersects(const LeftT& left, const RightT& right)
{
    typedef typename RightT::const_iterator const_iterator;
    LeftT intersection;

    const_iterator right_common_lower_, right_common_upper_;
    if(!Set::common_range(right_common_lower_, right_common_upper_, right, left))
        return false;

    const_iterator it_ = right_common_lower_;
    while(it_ != right_common_upper_)
    {
        icl::add_intersection(intersection, left, *it_++);
        if(!icl::is_empty(intersection))
            return true;
    }
    return false; 
}

template<class LeftT, class RightT>
typename enable_if<is_cross_combinable<LeftT, RightT>, bool>::type
intersects(const LeftT& left, const RightT& right)
{
    typedef typename RightT::const_iterator const_iterator;
    LeftT intersection;

    if(icl::is_empty(left) || icl::is_empty(right))
        return false;

    const_iterator right_common_lower_, right_common_upper_;
    if(!Set::common_range(right_common_lower_, right_common_upper_, right, left))
        return false;

    typename RightT::const_iterator it_ = right_common_lower_;
    while(it_ != right_common_upper_)
    {
        icl::add_intersection(intersection, left, key_value<RightT>(it_++));
        if(!icl::is_empty(intersection))
            return true;
    }

    return false; 
}

/** \b Returns true, if \c left and \c right have no common elements.
    Intervals are interpreted as sequence of elements.
    \b Complexity: loglinear, if \c left and \c right are interval containers. */
template<class LeftT, class RightT>
typename enable_if<is_inter_combinable<LeftT, RightT>, bool>::type
disjoint(const LeftT& left, const RightT& right)
{
    return !intersects(left, right);
}

/** \b Returns true, if \c left and \c right have no common elements.
    Intervals are interpreted as sequence of elements.
    \b Complexity: logarithmic, if \c AssociateT is an element type \c Type::element_type. 
    linear, if \c AssociateT is a segment type \c Type::segment_type. */
template<class Type, class AssociateT>
typename enable_if<is_inter_derivative<Type, AssociateT>, bool>::type
disjoint(const Type& left, const AssociateT& right)
{
    return !intersects(left,right);
}


//==============================================================================
//= Symmetric difference<IntervalSet|IntervalSet>
//==============================================================================
//------------------------------------------------------------------------------
//- Symmetric difference ^=, ^
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//- T& op ^=(T&, c P&) T:{S}|{M} P:{S'}|{M'}
//------------------------------------------------------------------------------
template<class Type, class OperandT>
typename enable_if<is_intra_combinable<Type, OperandT>, Type>::type&
operator ^= (Type& object, const OperandT& operand)
{ 
    return icl::flip(object, operand); 
}

//------------------------------------------------------------------------------
//- T& op ^=(T&, c P&) T:{S}|{M} P:{e i}|{b p}
//------------------------------------------------------------------------------
template<class Type, class OperandT>
typename enable_if<is_intra_derivative<Type, OperandT>, Type>::type&
operator ^= (Type& object, const OperandT& operand)
{ 
    return icl::flip(object, operand); 
}

#ifdef BOOST_ICL_NO_CXX11_RVALUE_REFERENCES
//------------------------------------------------------------------------------
//- T op ^ (T, c P&) T:{S}|{M} P:{e i S'}|{b p M'} S<S' M<M' <:coarser
//------------------------------------------------------------------------------
template<class Type, class OperandT>
typename enable_if<is_binary_intra_combinable<Type, OperandT>, Type>::type
operator ^ (Type object, const OperandT& operand)
{
    return object ^= operand; 
}

#else //BOOST_ICL_NO_CXX11_RVALUE_REFERENCES

template<class Type, class OperandT>
typename enable_if<is_binary_intra_combinable<Type, OperandT>, Type>::type
operator ^ (const Type& object, const OperandT& operand)
{
    Type temp = object;
    return boost::move(temp ^= operand); 
}

template<class Type, class OperandT>
typename enable_if<is_binary_intra_combinable<Type, OperandT>, Type>::type
operator ^ (Type&& object, const OperandT& operand)
{
    return boost::move(object ^= operand); 
}

#endif //BOOST_ICL_NO_CXX11_RVALUE_REFERENCES

#ifdef BOOST_ICL_NO_CXX11_RVALUE_REFERENCES
//------------------------------------------------------------------------------
//- T op ^ (c P&, T) T:{S}|{M} P:{e i S'}|{b p M'} S<S' M<M' <:coarser
//------------------------------------------------------------------------------
template<class Type, class OperandT>
typename enable_if<is_binary_intra_combinable<Type, OperandT>, Type>::type
operator ^ (const OperandT& operand, Type object)
{
    return object ^= operand; 
}

#else //BOOST_ICL_NO_CXX11_RVALUE_REFERENCES

template<class Type, class OperandT>
typename enable_if<is_binary_intra_combinable<Type, OperandT>, Type>::type
operator ^ (const OperandT& operand, const Type& object)
{
    Type temp = object;
    return boost::move(temp ^= operand);
}

template<class Type, class OperandT>
typename enable_if<is_binary_intra_combinable<Type, OperandT>, Type>::type
operator ^ (const OperandT& operand, Type&& object)
{
    return boost::move(object ^= operand); 
}

#endif //BOOST_ICL_NO_CXX11_RVALUE_REFERENCES

#ifdef BOOST_ICL_NO_CXX11_RVALUE_REFERENCES
//------------------------------------------------------------------------------
//- T op ^ (T, c T&) T:{S M}
//------------------------------------------------------------------------------
template<class Type>
typename enable_if<is_interval_container<Type>, Type>::type
operator ^ (typename Type::overloadable_type object, const Type& operand)
{
    return object ^= operand; 
}

#else //BOOST_ICL_NO_CXX11_RVALUE_REFERENCES

template<class Type>
typename enable_if<is_interval_container<Type>, Type>::type
operator ^ (const Type& object, const Type& operand)
{
    Type temp = object;
    return boost::move(temp ^= operand); 
}

template<class Type>
typename enable_if<is_interval_container<Type>, Type>::type
operator ^ (Type&& object, const Type& operand)
{
    return boost::move(object ^= operand); 
}

template<class Type>
typename enable_if<is_interval_container<Type>, Type>::type
operator ^ (const Type& operand, Type&& object)
{
    return boost::move(object ^= operand); 
}

template<class Type>
typename enable_if<is_interval_container<Type>, Type>::type
operator ^ (Type&& object, Type&& operand)
{
    return boost::move(object ^= operand); 
}

#endif //BOOST_ICL_NO_CXX11_RVALUE_REFERENCES

//==========================================================================
//= Element Iteration <IntervalSet|IntervalMap>
//==========================================================================
//--------------------------------------------------------------------------
//- Forward
//--------------------------------------------------------------------------
template<class Type>
typename enable_if
<mpl::and_< is_interval_container<Type> 
          , mpl::not_<is_continuous_interval<typename Type::interval_type> > >,
typename Type::element_iterator>::type
elements_begin(Type& object)
{
    return typename Type::element_iterator(object.begin());
}

template<class Type>
typename enable_if
<mpl::and_< is_interval_container<Type> 
          , mpl::not_<is_continuous_interval<typename Type::interval_type> > >,
typename Type::element_iterator>::type
elements_end(Type& object)
{ 
    return typename Type::element_iterator(object.end()); 
}

template<class Type>
typename enable_if
<mpl::and_< is_interval_container<Type> 
          , mpl::not_<is_continuous_interval<typename Type::interval_type> > >,
typename Type::element_const_iterator>::type
elements_begin(const Type& object)
{ 
    return typename Type::element_const_iterator(object.begin());
}

template<class Type>
typename enable_if
<mpl::and_< is_interval_container<Type> 
          , mpl::not_<is_continuous_interval<typename Type::interval_type> > >,
typename Type::element_const_iterator>::type
elements_end(const Type& object)
{ 
    return typename Type::element_const_iterator(object.end());
}

template<class Type>
typename enable_if
<mpl::and_< is_interval_container<Type>
          , mpl::not_<is_continuous_interval<typename Type::interval_type> > >,
iterator_range<typename Type::element_iterator> >::type
elements(Type& object)
{
    return
    make_iterator_range( typename Type::element_iterator(object.begin())
                       , typename Type::element_iterator(object.end())  );
}

template<class Type>
typename enable_if
<mpl::and_< is_interval_container<Type>
          , mpl::not_<is_continuous_interval<typename Type::interval_type> > >,
iterator_range<typename Type::element_const_iterator> >::type
elements(Type const& object)
{
    return
    make_iterator_range( typename Type::element_const_iterator(object.begin())
                       , typename Type::element_const_iterator(object.end())  );
}

//--------------------------------------------------------------------------
//- Reverse
//--------------------------------------------------------------------------
template<class Type>
typename enable_if
<mpl::and_< is_interval_container<Type> 
          , mpl::not_<is_continuous_interval<typename Type::interval_type> > >,
typename Type::element_reverse_iterator>::type
elements_rbegin(Type& object)
{
    return typename Type::element_reverse_iterator(object.rbegin());
}

template<class Type>
typename enable_if
<mpl::and_< is_interval_container<Type> 
          , mpl::not_<is_continuous_interval<typename Type::interval_type> > >,
typename Type::element_reverse_iterator>::type
elements_rend(Type& object)
{ 
    return typename Type::element_reverse_iterator(object.rend());
}

template<class Type>
typename enable_if
<mpl::and_< is_interval_container<Type> 
          , mpl::not_<is_continuous_interval<typename Type::interval_type> > >,
typename Type::element_const_reverse_iterator>::type
elements_rbegin(const Type& object)
{ 
    return typename Type::element_const_reverse_iterator(object.rbegin());
}

template<class Type>
typename enable_if
<mpl::and_< is_interval_container<Type> 
          , mpl::not_<is_continuous_interval<typename Type::interval_type> > >,
typename Type::element_const_reverse_iterator>::type
elements_rend(const Type& object)
{ 
    return typename Type::element_const_reverse_iterator(object.rend());
}

}} // namespace boost icl

#endif



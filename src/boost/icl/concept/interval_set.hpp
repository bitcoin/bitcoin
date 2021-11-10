/*-----------------------------------------------------------------------------+    
Copyright (c) 2010-2010: Joachim Faulhaber
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/
#ifndef BOOST_ICL_CONCEPT_INTERVAL_SET_HPP_JOFA_100920
#define BOOST_ICL_CONCEPT_INTERVAL_SET_HPP_JOFA_100920

#include <boost/icl/type_traits/is_combinable.hpp>
#include <boost/icl/type_traits/interval_type_of.hpp>
#include <boost/icl/detail/set_algo.hpp>
#include <boost/icl/detail/interval_set_algo.hpp>
#include <boost/icl/concept/interval.hpp>

namespace boost{ namespace icl
{

//==============================================================================
//= Containedness<IntervalSet>
//==============================================================================
//------------------------------------------------------------------------------
//- bool contains(c T&, c P&) T:{S} P:{e i S} fragment_types
//------------------------------------------------------------------------------
template<class Type>
typename enable_if<is_interval_set<Type>, bool>::type
contains(const Type& super, const typename Type::element_type& element)
{
    return !(icl::find(super, element) == super.end());
}

template<class Type>
typename enable_if<is_interval_set<Type>, bool>::type
contains(const Type& super, const typename Type::segment_type& inter_val)
{ 
    typedef typename Type::const_iterator const_iterator;
    if(icl::is_empty(inter_val)) 
        return true;

    std::pair<const_iterator, const_iterator> exterior 
        = super.equal_range(inter_val);
    if(exterior.first == exterior.second)
        return false;

    const_iterator last_overlap = cyclic_prior(super, exterior.second);

    return 
        icl::contains(hull(*(exterior.first), *last_overlap), inter_val)
    &&  Interval_Set::is_joinable(super, exterior.first, last_overlap);
}

template<class Type, class OperandT>
typename enable_if<has_same_concept<is_interval_set, Type, OperandT>, 
                   bool>::type 
contains(const Type& super, const OperandT& sub)
{
    return Interval_Set::contains(super, sub);
}

//==============================================================================
//= Addition<IntervalSet>
//==============================================================================
//------------------------------------------------------------------------------
//- T& add(T&, c P&) T:{S} P:{e i} fragment_types
//------------------------------------------------------------------------------
template<class Type>
typename enable_if<is_interval_set<Type>, Type>::type&
add(Type& object, const typename Type::segment_type& operand)
{
    return object.add(operand);
}

template<class Type>
inline typename enable_if<is_interval_set<Type>, Type>::type&
add(Type& object, const typename Type::element_type& operand)
{
    typedef typename Type::segment_type segment_type;
    return icl::add(object, icl::singleton<segment_type>(operand));
}

//------------------------------------------------------------------------------
//- T& add(T&, J, c P&) T:{S} P:{i} interval_type
//------------------------------------------------------------------------------
template<class Type>
inline typename enable_if<is_interval_set<Type>, typename Type::iterator>::type
add(Type& object, typename Type::iterator      prior, 
            const typename Type::segment_type& operand)
{
    return object.add(prior, operand);
}

//==============================================================================
//= Insertion<IntervalSet>
//==============================================================================
//------------------------------------------------------------------------------
//- T& insert(T&, c P&) T:{S} P:{e i} fragment_types
//------------------------------------------------------------------------------
template<class Type>
inline typename enable_if<is_interval_set<Type>, Type>::type&
insert(Type& object, const typename Type::segment_type& operand)
{
    return icl::add(object, operand);
}

template<class Type>
inline typename enable_if<is_interval_set<Type>, Type>::type&
insert(Type& object, const typename Type::element_type& operand)
{
    return icl::add(object, operand);
}

//------------------------------------------------------------------------------
//- T& insert(T&, J, c P&) T:{S} P:{i} with hint
//------------------------------------------------------------------------------
template<class Type>
inline typename enable_if<is_interval_set<Type>, typename Type::iterator>::type
insert(Type& object, typename Type::iterator      prior,
               const typename Type::segment_type& operand)
{
    return icl::add(object, prior, operand);
}

//==============================================================================
//= Subtraction<IntervalSet>
//==============================================================================
//------------------------------------------------------------------------------
//- T& subtract(T&, c P&) T:{S} P:{e i} fragment_type
//------------------------------------------------------------------------------
template<class Type>
typename enable_if<is_interval_set<Type>, Type>::type&
subtract(Type& object, const typename Type::segment_type& operand)
{
    return object.subtract(operand);
}

template<class Type>
inline typename enable_if<is_interval_set<Type>, Type>::type&
subtract(Type& object, const typename Type::element_type& operand)
{
    typedef typename Type::segment_type segment_type;
    return icl::subtract(object, icl::singleton<segment_type>(operand));
}

//==============================================================================
//= Erasure<IntervalSet>
//==============================================================================
//------------------------------------------------------------------------------
//- T& erase(T&, c P&) T:{S} P:{e i} fragment_types
//------------------------------------------------------------------------------
template<class Type>
typename enable_if<is_interval_set<Type>, Type>::type&
erase(Type& object, const typename Type::segment_type& minuend)
{
    return icl::subtract(object, minuend);
}

template<class Type>
typename enable_if<is_interval_set<Type>, Type>::type&
erase(Type& object, const typename Type::element_type& minuend)
{
    return icl::subtract(object, minuend);
}

//==============================================================================
//= Intersection
//==============================================================================
//------------------------------------------------------------------------------
//- void add_intersection(T&, c T&, c P&) T:{S} P:{e i} fragment_types
//------------------------------------------------------------------------------
template<class Type>
typename enable_if<is_interval_set<Type>, void>::type
add_intersection(Type& section, const Type& object, 
                 const typename Type::element_type& operand)
{
    typedef typename Type::const_iterator const_iterator;
    const_iterator found = icl::find(object, operand);
    if(found != object.end())
        icl::add(section, operand);
}


template<class Type>
typename enable_if<is_interval_set<Type>, void>::type
add_intersection(Type& section, const Type& object, 
                 const typename Type::segment_type& segment)
{
    typedef typename Type::const_iterator const_iterator;
    typedef typename Type::iterator       iterator;
    typedef typename Type::interval_type  interval_type;

    if(icl::is_empty(segment)) 
        return;

    std::pair<const_iterator, const_iterator> exterior 
        = object.equal_range(segment);
    if(exterior.first == exterior.second)
        return;

    iterator prior_ = section.end();
    for(const_iterator it_=exterior.first; it_ != exterior.second; it_++) 
    {
        interval_type common_interval = key_value<Type>(it_) & segment;
        if(!icl::is_empty(common_interval))
            prior_ = section.insert(prior_, common_interval);
    }
}

//==============================================================================
//= Symmetric difference<IntervalSet>
//==============================================================================
//------------------------------------------------------------------------------
//- T& flip(T&, c P&) T:{S} P:{e i S'} fragment_types
//------------------------------------------------------------------------------
template<class Type>
typename enable_if<is_interval_set<Type>, Type>::type&
flip(Type& object, const typename Type::element_type& operand)
{
    if(icl::contains(object, operand))
        return object -= operand;
    else
        return object += operand;
}

template<class Type>
typename enable_if<is_interval_set<Type>, Type>::type&
flip(Type& object, const typename Type::segment_type& segment)
{
    typedef typename Type::const_iterator const_iterator;
    typedef typename Type::interval_type  interval_type;
    // That which is common shall be subtracted
    // That which is not shall be added
    // So x has to be 'complementary added' or flipped
    interval_type span = segment;
    std::pair<const_iterator, const_iterator> exterior 
        = object.equal_range(span);

    const_iterator fst_ = exterior.first;
    const_iterator end_ = exterior.second;

    interval_type covered, left_over;
    const_iterator it_ = fst_;
    while(it_ != end_) 
    {
        covered = *it_++; 
        //[a      ...  : span
        //     [b ...  : covered
        //[a  b)       : left_over
        left_over = right_subtract(span, covered);
        icl::subtract(object, span & covered); //That which is common shall be subtracted
        icl::add(object, left_over);           //That which is not shall be added

        //...      d) : span
        //... c)      : covered
        //     [c  d) : span'
        span = left_subtract(span, covered);
    }

    //If span is not empty here, it_ is not in the set so it_ shall be added
    icl::add(object, span);
    return object;
}


template<class Type, class OperandT>
typename enable_if<is_concept_compatible<is_interval_set, Type, OperandT>, Type>::type&
flip(Type& object, const OperandT& operand)
{
    typedef typename OperandT::const_iterator const_iterator;

    if(operand.empty())
        return object;

    const_iterator common_lwb, common_upb;

    if(!Set::common_range(common_lwb, common_upb, operand, object))
        return object += operand;

    const_iterator it_ = operand.begin();

    // All elements of operand left of the common range are added
    while(it_ != common_lwb)
        icl::add(object, *it_++);
    // All elements of operand in the common range are symmertrically subtracted
    while(it_ != common_upb)
        icl::flip(object, *it_++);
    // All elements of operand right of the common range are added
    while(it_ != operand.end())
        icl::add(object, *it_++);

    return object;
}

//==============================================================================
//= Set selection
//==============================================================================
template<class Type>
typename enable_if<is_interval_set<Type>, Type>::type&
domain(Type& dom, const Type& object)
{
    typedef typename Type::const_iterator const_iterator;
    typedef typename Type::iterator       iterator;
    dom.clear();
    const_iterator it_    = object.begin();
    iterator       prior_ = dom.end();

    while(it_ != object.end())
        prior_ = icl::insert(dom, prior_, *it_++);

    return dom;
}

template<class Type>
typename enable_if<is_interval_set<Type>, Type>::type&
between(Type& in_between, const Type& object)
{
    typedef typename Type::const_iterator const_iterator;
    typedef typename Type::iterator       iterator;
    in_between.clear();
    const_iterator it_ = object.begin(), pred_;
    iterator prior_ = in_between.end();

    if(it_ != object.end())
        pred_ = it_++;

    while(it_ != object.end())
        prior_ = icl::insert(in_between, prior_, 
                             icl::between(*pred_++, *it_++));

    return in_between;
}


//==============================================================================
//= Streaming
//==============================================================================
template<class CharType, class CharTraits, class Type>
typename enable_if<is_interval_set<Type>, 
                   std::basic_ostream<CharType, CharTraits> >::type& 
operator << (std::basic_ostream<CharType, CharTraits>& stream, const Type& object)
{
    stream << "{";
    ICL_const_FORALL(typename Type, it_, object)
        stream << (*it_);

    return stream << "}";
}


}} // namespace boost icl

#endif



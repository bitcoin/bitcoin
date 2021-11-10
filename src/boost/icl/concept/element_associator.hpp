/*-----------------------------------------------------------------------------+
Copyright (c) 2010-2010: Joachim Faulhaber
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/
#ifndef BOOST_ICL_CONCEPT_ELEMENT_ASSOCIATOR_HPP_JOFA_100921
#define BOOST_ICL_CONCEPT_ELEMENT_ASSOCIATOR_HPP_JOFA_100921

#include <boost/config.hpp>
#include <boost/icl/type_traits/is_associative_element_container.hpp>
#include <boost/icl/type_traits/is_key_container_of.hpp>
#include <boost/icl/type_traits/is_combinable.hpp>
#include <boost/icl/detail/subset_comparer.hpp>
#include <boost/icl/concept/element_set.hpp>
#include <boost/icl/concept/element_map.hpp>

namespace boost{ namespace icl
{

//==============================================================================
//= Size
//==============================================================================
template<class Type>
typename enable_if<is_element_container<Type>, std::size_t>::type
iterative_size(const Type& object)
{
    return object.size();
}

template<class Type>
typename enable_if<is_associative_element_container<Type>, typename Type::size_type>::type
size(const Type& object)
{
    return icl::iterative_size(object);
}

template<class Type>
typename enable_if<is_associative_element_container<Type>, typename Type::size_type>::type
cardinality(const Type& object)
{
    return icl::iterative_size(object);
}


//==============================================================================
//= Containedness<ElementSet|ElementMap>
//==============================================================================
//------------------------------------------------------------------------------
//- bool within(c P&, c T&) T:{s}|{m} P:{e}|{i} fragment_types|key_types
//------------------------------------------------------------------------------
/** Checks if a key is in the associative container */
template<class Type>
typename enable_if<is_associative_element_container<Type>, bool>::type
within(const typename Type::key_type& key, const Type& super)
{
    return !(super.find(key) == super.end());
}

//------------------------------------------------------------------------------
//- bool within(c P&, c T&) T:{s}|{m} P:{s'} fragment_types|key_types
//------------------------------------------------------------------------------
template<class SubT, class SuperT>
typename enable_if<mpl::and_< is_associative_element_container<SuperT>
                            , is_key_container_of<SubT, SuperT> >,
                   bool>::type
within(const SubT& sub, const SuperT& super)
{
    if(icl::is_empty(sub))                return true;
    if(icl::is_empty(super))              return false;
    if(icl::size(super) < icl::size(sub)) return false;

    typename SubT::const_iterator common_lwb_;
    typename SubT::const_iterator common_upb_;
    if(!Set::common_range(common_lwb_, common_upb_, sub, super))
        return false;

    typename SubT::const_iterator sub_ = sub.begin();
    typename SuperT::const_iterator super_;
    while(sub_ != sub.end())
    {
        super_ = super.find(key_value<SubT>(sub_));
        if(super_ == super.end())
            return false;
        else if(!co_equal(sub_, super_, &sub, &super))
            return false;

        ++sub_;
    }
    return true;
}

//------------------------------------------------------------------------------
//- bool contains(c T&, c P&) T:{s}|{m} P:{e}|{i} fragment_types|key_types
//------------------------------------------------------------------------------
template<class Type>
typename enable_if<is_associative_element_container<Type>, bool>::type
contains(const Type& super, const typename Type::key_type& key)
{
    return icl::within(key, super);
}

//------------------------------------------------------------------------------
//- bool contains(c T&, c P&) T:{s}|{m} P:{s'} fragment_types|key_types
//------------------------------------------------------------------------------
template<class SubT, class SuperT>
typename enable_if<mpl::and_< is_associative_element_container<SuperT>
                            , is_key_container_of<SubT, SuperT> >,
                   bool>::type
contains(const SuperT& super, const SubT& sub)
{
    return icl::within(sub, super);
}

//==============================================================================
//= Equivalences and Orderings
//==============================================================================

#ifdef BOOST_MSVC
#pragma warning(push)
#pragma warning(disable:4996) //'std::equal': Function call with parameters that may be unsafe - this call relies on the caller to check that the passed values are correct. To disable this warning, use -D_SCL_SECURE_NO_WARNINGS. See documentation on how to use Visual C++ 'Checked Iterators'
#endif                        // I do guarantee here that I am using the parameters correctly :)

/** Standard equality, which is lexicographical equality of the sets
    as sequences, that are given by their Compare order. */
template<class Type>
inline typename enable_if<is_associative_element_container<Type>, bool>::type
operator == (const Type& left, const Type& right)
{
    return left.size() == right.size()
        && std::equal(left.begin(), left.end(), right.begin());
}

#ifdef BOOST_MSVC
#pragma warning(pop)
#endif

template<class Type>
inline typename enable_if<is_associative_element_container<Type>, bool>::type
is_element_equal(const Type& left, const Type& right)
{ return left == right; }


/* Strict weak less ordering which is given by the Compare order */
template<class Type>
inline typename enable_if<is_associative_element_container<Type>, bool>::type
operator < (const Type& left, const Type& right)
{
    return std::lexicographical_compare(
        left.begin(), left.end(), right.begin(), right.end(),
        typename Type::element_compare()
        );
}

template<class LeftT, class RightT>
typename enable_if<is_concept_equivalent<is_element_container,LeftT, RightT>,
                   int>::type
inclusion_compare(const LeftT& left, const RightT& right)
{
    return Set::subset_compare(left, right,
                               left.begin(), left.end(),
                               right.begin(), right.end());
}

//==============================================================================
//= Addition
//==============================================================================
template <class Type>
inline typename enable_if<is_associative_element_container<Type>, Type>::type&
operator += (Type& object, const typename Type::value_type& operand)
{
    return icl::add(object, operand);
}

template <class Type>
inline typename enable_if<is_associative_element_container<Type>, Type>::type
operator + (Type object, const typename Type::value_type& operand)
{
    return object += operand;
}

template <class Type>
inline typename enable_if<is_associative_element_container<Type>, Type>::type
operator + (const typename Type::value_type& operand, Type object)
{
    return object += operand;
}

template <class Type>
inline typename enable_if<is_associative_element_container<Type>, Type>::type&
operator += (Type& object, const Type& operand)
{
    if(&object == &operand)
        return object;

    typename Type::iterator prior_ = object.end();
    ICL_const_FORALL(typename Type, it_, operand)
        prior_ = icl::add(object, prior_, *it_);

    return object;
}

template <class Type>
inline typename enable_if<is_associative_element_container<Type>, Type>::type
operator + (Type object, const Type& operand)
{
    return object += operand;
}

//==============================================================================
template <class Type>
inline typename enable_if<is_associative_element_container<Type>, Type>::type&
operator |= (Type& object, const typename Type::value_type& operand)
{
    return icl::add(object, operand);
}

template <class Type>
inline typename enable_if<is_associative_element_container<Type>, Type>::type
operator | (Type object, const typename Type::value_type& operand)
{
    return object += operand;
}

template <class Type>
inline typename enable_if<is_associative_element_container<Type>, Type>::type
operator | (const typename Type::value_type& operand, Type object)
{
    return object += operand;
}

template <class Type>
inline typename enable_if<is_associative_element_container<Type>, Type>::type&
operator |= (Type& object, const Type& operand)
{
    return object += operand;
}

template <class Type>
inline typename enable_if<is_associative_element_container<Type>, Type>::type
operator | (Type object, const Type& operand)
{
    return object += operand;
}


//==============================================================================
//= Insertion
//==============================================================================
//------------------------------------------------------------------------------
//- V insert(T&, c P&) T:{s}|{m} P:{e}|{b} fragment_type
//------------------------------------------------------------------------------
template<class Type>
typename enable_if<is_associative_element_container<Type>,
                   std::pair<typename Type::iterator,bool> >::type
insert(Type& object, const typename Type::value_type& operand)
{
    return object.insert(operand);
}

template<class Type>
typename enable_if<is_associative_element_container<Type>,
                   typename Type::iterator>::type
insert(Type& object, typename Type::iterator      prior,
               const typename Type::value_type& operand)
{
    return object.insert(prior, operand);
}

//------------------------------------------------------------------------------
//- T insert(T&, c T&) T:{s m}  map fragment_type
//------------------------------------------------------------------------------
template<class Type>
typename enable_if<is_associative_element_container<Type>, Type>::type&
insert(Type& object, const Type& addend)
{
    typedef typename Type::iterator iterator;

    iterator prior_ = object.end();
    ICL_const_FORALL(typename Type, elem_, addend)
        icl::insert(object, prior_, *elem_);

    return object;
}


//==============================================================================
//= Erasure
//==============================================================================
template<class Type>
typename enable_if<is_associative_element_container<Type>, typename Type::size_type>::type
erase(Type& object, const typename Type::key_type& key_value)
{
    typedef typename Type::size_type size_type;
    typename Type::iterator it_ = object.find(key_value);
    if(it_ != object.end())
    {
        object.erase(it_);
        return unit_element<size_type>::value();
    }
    return identity_element<size_type>::value();
}

template<class Type>
typename enable_if<is_associative_element_container<Type>, Type>::type&
erase(Type& object, const Type& erasure)
{
    ICL_const_FORALL(typename Type, elem_, erasure)
        icl::erase(object, *elem_);

    return object;
}



//==============================================================================
//= Subtraction<ElementSet|ElementMap>
//==============================================================================
template <class Type>
inline typename enable_if<is_associative_element_container<Type>, Type>::type&
operator -= (Type& object, const typename Type::value_type& operand)
{
    return icl::subtract(object, operand);
}

template <class Type>
inline typename enable_if<is_associative_element_container<Type>, Type>::type
operator - (Type object, const typename Type::value_type& operand)
{
    return object -= operand;
}

template <class Type>
inline typename enable_if<is_associative_element_container<Type>, Type>::type&
operator -= (Type& object, const Type& subtrahend)
{
    ICL_const_FORALL(typename Type, it_, subtrahend)
        icl::subtract(object, *it_);

    return object;
}

template <class Type>
inline typename enable_if<is_associative_element_container<Type>, Type>::type
operator - (Type object, const Type& subtrahend)
{
    return object -= subtrahend;
}


//==============================================================================
//= Intersection
//==============================================================================
//------------------------------------------------------------------------------
//- void add_intersection(T&, c T&, c P&) T:{s}{m} P:{e}{e} key_type
//------------------------------------------------------------------------------
template<class Type>
inline typename enable_if<is_associative_element_container<Type>, void>::type
add_intersection(Type& section, const Type&              object,
                       const typename Type::key_type& operand)
{
    typedef typename Type::const_iterator const_iterator;
    const_iterator it_ = object.find(operand);
    if(it_ != object.end())
        icl::add(section, *it_);
}

//------------------------------------------------------------------------------
//- void add_intersection(T&, c T&, c P&) T:{s}{m} P:{s}{s} set key_type
//------------------------------------------------------------------------------
template<class Type>
inline typename enable_if<is_associative_element_container<Type>, void>::type
add_intersection(Type& section, const Type& object,
                 const typename key_container_type_of<Type>::type& operand)
{
    typedef typename key_container_type_of<Type>::type key_container_type;
    typedef typename key_container_type::const_iterator const_iterator;
    const_iterator common_lwb_, common_upb_;
    if(!Set::common_range(common_lwb_, common_upb_, operand, object))
        return;

    const_iterator sec_ = common_lwb_;
    while(sec_ != common_upb_)
        add_intersection(section, object, *sec_++);
}

//------------------------------------------------------------------------------
//- Intersection<ElementMap|ElementSet>
//------------------------------------------------------------------------------
template<class Type>
inline typename enable_if<is_associative_element_container<Type>, Type>::type&
operator &= (Type& object, const typename Type::key_type& operand)
{
    Type section;
    add_intersection(section, object, operand);
    object.swap(section);
    return object;
}

template<class Type>
inline typename enable_if<is_associative_element_container<Type>, Type>::type
operator & (Type object, const typename Type::key_type& operand)
{
    return object &= operand;
}

template<class Type>
inline typename enable_if<is_associative_element_container<Type>, Type>::type
operator & (const typename Type::key_type& operand, Type object)
{
    return object &= operand;
}

template<class Type>
inline typename enable_if<is_associative_element_container<Type>, Type>::type&
operator &= (Type& object, const typename key_container_type_of<Type>::type& operand)
{
    Type section;
    add_intersection(section, object, operand);
    object.swap(section);
    return object;
}

template<class Type>
inline typename enable_if<is_associative_element_container<Type>, Type>::type
operator & (Type object, const Type& operand)
{
    return object &= operand;
}
//------------------------------------------------------------------------------

template<class Type, class CoType>
inline typename enable_if<is_associative_element_container<Type>, bool>::type
disjoint(const Type& left, const Type& right)
{
    return !intersects(left, right);
}

//==============================================================================
//= Symmetric difference<ElementSet|ElementMap>
//==============================================================================
template<class Type>
inline typename enable_if<is_associative_element_container<Type>, Type>::type
operator ^ (Type object, const typename Type::value_type& operand)
{
    return icl::flip(object, operand);
}

template<class Type>
inline typename enable_if<is_associative_element_container<Type>, Type>::type
operator ^ (const typename Type::value_type& operand, Type object)
{
    return icl::flip(object, operand);
}

template<class Type>
inline typename enable_if<is_associative_element_container<Type>, Type>::type
operator ^ (Type object, const Type& operand)
{
    return object ^= operand;
}


//==============================================================================
//= Manipulation by predicates
//==============================================================================
template<class Type, class Predicate>
typename enable_if<is_associative_element_container<Type>, Type>::type&
erase_if(const Predicate& pred, Type& object)
{
    typename Type::iterator it_ = object.begin();
    while(it_ != object.end())
        if(pred(*it_))
            icl::erase(object, it_++);
        else ++it_;
    return object;
}

template<class Type, class Predicate>
inline typename enable_if<is_associative_element_container<Type>, Type>::type&
add_if(const Predicate& pred, Type& object, const Type& src)
{
    typename Type::const_iterator it_ = src.begin();
    while(it_ != src.end())
        if(pred(*it_))
            icl::add(object, *it_++);

    return object;
}

template<class Type, class Predicate>
inline typename enable_if<is_associative_element_container<Type>, Type>::type&
assign_if(const Predicate& pred, Type& object, const Type& src)
{
    icl::clear(object);
    return add_if(object, src, pred);
}



}} // namespace boost icl

#endif

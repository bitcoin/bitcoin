/*-----------------------------------------------------------------------------+
Copyright (c) 2008-2010: Joachim Faulhaber
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/
#ifndef BOOST_ICL_INTERVAL_SET_ALGO_HPP_JOFA_081005
#define BOOST_ICL_INTERVAL_SET_ALGO_HPP_JOFA_081005

#include <boost/next_prior.hpp>
#include <boost/icl/detail/notate.hpp>
#include <boost/icl/detail/relation_state.hpp>
#include <boost/icl/type_traits/identity_element.hpp>
#include <boost/icl/type_traits/is_map.hpp>
#include <boost/icl/type_traits/is_total.hpp>
#include <boost/icl/type_traits/is_combinable.hpp>
#include <boost/icl/concept/set_value.hpp>
#include <boost/icl/concept/map_value.hpp>
#include <boost/icl/interval_combining_style.hpp>
#include <boost/icl/detail/element_comparer.hpp>
#include <boost/icl/detail/interval_subset_comparer.hpp>
#include <boost/icl/detail/associated_value.hpp>

namespace boost{namespace icl
{

namespace Interval_Set
{

//------------------------------------------------------------------------------
// Lexicographical comparison on ranges of two interval container 
//------------------------------------------------------------------------------

template<class LeftT, class RightT>
bool is_element_equal(const LeftT& left, const RightT& right)
{
    return subset_compare
            (
                left, right, 
                left.begin(), left.end(), 
                right.begin(), right.end()
            ) == inclusion::equal;
}

template<class LeftT, class RightT>
bool is_element_less(const LeftT& left, const RightT& right)
{
    return element_compare
            (
                left, right, 
                left.begin(), left.end(), 
                right.begin(), right.end()
            )  == comparison::less;
}

template<class LeftT, class RightT>
bool is_element_greater(const LeftT& left, const RightT& right)
{
    return element_compare
            (
                left, right, 
                left.begin(), left.end(), 
                right.begin(), right.end()
            )  == comparison::greater;
}

//------------------------------------------------------------------------------
// Subset/superset compare on ranges of two interval container 
//------------------------------------------------------------------------------

template<class IntervalContainerT>
bool is_joinable(const IntervalContainerT& container, 
                 typename IntervalContainerT::const_iterator first, 
                 typename IntervalContainerT::const_iterator past) 
{
    if(first == container.end())
        return true;

    typename IntervalContainerT::const_iterator it_ = first, next_ = first;
    ++next_;

    while(next_ != container.end() && it_ != past)
        if(!icl::touches(key_value<IntervalContainerT>(it_++),
                         key_value<IntervalContainerT>(next_++)))
            return false;

    return true;
}


template<class LeftT, class RightT>
bool is_inclusion_equal(const LeftT& left, const RightT& right)
{
    return subset_compare
            (
                left, right, 
                left.begin(), left.end(), 
                right.begin(), right.end()
            ) == inclusion::equal;
}

template<class LeftT, class RightT>
typename enable_if<mpl::and_<is_concept_combinable<is_interval_set, is_interval_map, LeftT, RightT>, 
                             is_total<RightT> >,
                   bool>::type
within(const LeftT&, const RightT&)
{
    return true;
}

template<class LeftT, class RightT>
typename enable_if<mpl::and_<is_concept_combinable<is_interval_set, is_interval_map, LeftT, RightT>, 
                             mpl::not_<is_total<RightT> > >,
                   bool>::type
within(const LeftT& sub, const RightT& super)
{
    int result =
        subset_compare
        (
            sub, super, 
            sub.begin(), sub.end(), 
            super.begin(), super.end()
        );
    return result == inclusion::subset || result == inclusion::equal;
}


template<class LeftT, class RightT>
typename enable_if<is_concept_combinable<is_interval_map, is_interval_map, LeftT, RightT>, 
                   bool>::type
within(const LeftT& sub, const RightT& super)
{
    int result =
        subset_compare
        (
            sub, super, 
            sub.begin(), sub.end(), 
            super.begin(), super.end()
        );
    return result == inclusion::subset || result == inclusion::equal;
}

template<class LeftT, class RightT>
typename enable_if<is_concept_combinable<is_interval_set, is_interval_set, LeftT, RightT>, 
                   bool>::type
within(const LeftT& sub, const RightT& super)
{
    int result =
        subset_compare
        (
            sub, super, 
            sub.begin(), sub.end(), 
            super.begin(), super.end()
        );
    return result == inclusion::subset || result == inclusion::equal;
}



template<class LeftT, class RightT>
typename enable_if<mpl::and_<is_concept_combinable<is_interval_map, is_interval_set, LeftT, RightT>, 
                             is_total<LeftT> >,
                   bool>::type
contains(const LeftT&, const RightT&)
{
    return true;
}

template<class LeftT, class RightT>
typename enable_if<mpl::and_<is_concept_combinable<is_interval_map, is_interval_set, LeftT, RightT>, 
                             mpl::not_<is_total<LeftT> > >,
                   bool>::type
contains(const LeftT& super, const RightT& sub)
{
    int result =
        subset_compare
        (
            super, sub, 
            super.begin(), super.end(), 
            sub.begin(), sub.end()
        );
    return result == inclusion::superset || result == inclusion::equal;
}

template<class LeftT, class RightT>
typename enable_if<is_concept_combinable<is_interval_set, is_interval_set, LeftT, RightT>, 
                   bool>::type
contains(const LeftT& super, const RightT& sub)
{
    int result =
        subset_compare
        (
            super, sub, 
            super.begin(), super.end(), 
            sub.begin(), sub.end()
        );
    return result == inclusion::superset || result == inclusion::equal;
}

template<class IntervalContainerT>
bool is_dense(const IntervalContainerT& container, 
              typename IntervalContainerT::const_iterator first, 
              typename IntervalContainerT::const_iterator past) 
{
    if(first == container.end())
        return true;

    typename IntervalContainerT::const_iterator it_ = first, next_ = first;
    ++next_;

    while(next_ != container.end() && it_ != past)
        if(!icl::touches(key_value<IntervalContainerT>(it_++), 
                         key_value<IntervalContainerT>(next_++)))
            return false;

    return true;
}

} // namespace Interval_Set

namespace segmental
{

template<class Type>
inline bool joinable(const Type& _Type, typename Type::iterator& some, typename Type::iterator& next)
{
    // assert: next != end && some++ == next
    return touches(key_value<Type>(some), key_value<Type>(next)) 
        && co_equal(some, next, &_Type, &_Type); 
}

template<class Type>
inline void join_nodes(Type& object, typename Type::iterator& left_, 
                                     typename Type::iterator& right_)
{
    typedef typename Type::interval_type interval_type;
    interval_type right_interval = key_value<Type>(right_);
    object.erase(right_);
    const_cast<interval_type&>(key_value<Type>(left_)) 
        = hull(key_value<Type>(left_), right_interval);
}

template<class Type>
inline typename Type::iterator 
    join_on_left(Type& object, typename Type::iterator& left_, 
                               typename Type::iterator& right_)
{
    //CL typedef typename Type::interval_type interval_type;
    // both left and right are in the set and they are neighbours
    BOOST_ASSERT(exclusive_less(key_value<Type>(left_), key_value<Type>(right_)));
    BOOST_ASSERT(joinable(object, left_, right_));

    join_nodes(object, left_, right_);
    return left_;
}

template<class Type>
inline typename Type::iterator 
    join_on_right(Type& object, typename Type::iterator& left_, 
                                typename Type::iterator& right_)
{
    //CL typedef typename Type::interval_type interval_type;
    // both left and right are in the map and they are neighbours
    BOOST_ASSERT(exclusive_less(key_value<Type>(left_), key_value<Type>(right_)));
    BOOST_ASSERT(joinable(object, left_, right_));

    join_nodes(object, left_, right_);
    right_ = left_;
    return right_;
}

template<class Type>
typename Type::iterator join_left(Type& object, typename Type::iterator& it_)
{
    typedef typename Type::iterator iterator;

    if(it_ == object.begin())
        return it_;

    // there is a predecessor
    iterator pred_ = it_;
    if(joinable(object, --pred_, it_)) 
        return join_on_right(object, pred_, it_); 

    return it_;
}

template<class Type>
typename Type::iterator join_right(Type& object, typename Type::iterator& it_)
{
    typedef typename Type::iterator iterator;

    if(it_ == object.end())
        return it_;

    // there is a successor
    iterator succ_ = it_;

    if(++succ_ != object.end() && joinable(object, it_, succ_)) 
        return join_on_left(object, it_, succ_);

    return it_;
}

template<class Type>
typename Type::iterator join_neighbours(Type& object, typename Type::iterator& it_)
{
           join_left (object, it_); 
    return join_right(object, it_);
}

template<class Type>
inline typename Type::iterator 
    join_under(Type& object, const typename Type::value_type& addend)
{
    //ASSERT: There is at least one interval in object that overlaps with addend
    typedef typename Type::iterator      iterator;
    typedef typename Type::interval_type interval_type;
    typedef typename Type::value_type    value_type;

    std::pair<iterator,iterator> overlap = object.equal_range(addend);
    iterator first_ = overlap.first,
             end_   = overlap.second,
             last_  = end_; --last_;

    iterator second_= first_; ++second_;

    interval_type left_resid  = right_subtract(key_value<Type>(first_), addend);
    interval_type right_resid =  left_subtract(key_value<Type>(last_) , addend);

    object.erase(second_, end_);

    const_cast<value_type&>(key_value<Type>(first_)) 
        = hull(hull(left_resid, addend), right_resid);
    return first_;
}

template<class Type>
inline typename Type::iterator 
    join_under(Type& object, const typename Type::value_type& addend,
                                   typename Type::iterator last_)
{
    //ASSERT: There is at least one interval in object that overlaps with addend
    typedef typename Type::iterator      iterator;
    typedef typename Type::interval_type interval_type;
    typedef typename Type::value_type    value_type;

    iterator first_ = object.lower_bound(addend);
    //BOOST_ASSERT(next(last_) == this->_set.upper_bound(inter_val));
    iterator second_= boost::next(first_), end_ = boost::next(last_);

    interval_type left_resid  = right_subtract(key_value<Type>(first_), addend);
    interval_type right_resid =  left_subtract(key_value<Type>(last_) , addend);

    object.erase(second_, end_);

    const_cast<value_type&>(key_value<Type>(first_)) 
        = hull(hull(left_resid, addend), right_resid);
    return first_;
}

} // namespace segmental

namespace Interval_Set
{
using namespace segmental;

template<class Type, int combining_style>
struct on_style;

template<class Type>
struct on_style<Type, interval_combine::joining>
{
    typedef          on_style            type;
    typedef typename Type::interval_type interval_type;
    typedef typename Type::iterator      iterator;

    inline static iterator handle_inserted(Type& object, iterator inserted_)
    { return join_neighbours(object, inserted_); }

    inline static iterator add_over
        (Type& object, const interval_type& addend, iterator last_)
    {
        iterator joined_ = join_under(object, addend, last_);
        return join_neighbours(object, joined_);
    }

    inline static iterator add_over
        (Type& object, const interval_type& addend)
    {
        iterator joined_ = join_under(object, addend);
        return join_neighbours(object, joined_);
    }
};

template<class Type>
struct on_style<Type, interval_combine::separating>
{
    typedef          on_style            type;
    typedef typename Type::interval_type interval_type;
    typedef typename Type::iterator      iterator;

    inline static iterator handle_inserted(Type&, iterator inserted_)
    { return inserted_; }

    inline static iterator add_over
        (Type& object, const interval_type& addend, iterator last_)
    {
        return join_under(object, addend, last_);
    }

    inline static iterator add_over
        (Type& object, const interval_type& addend)
    {
        return join_under(object, addend);
    }
};

template<class Type>
struct on_style<Type, interval_combine::splitting>
{
    typedef          on_style            type;
    typedef typename Type::interval_type interval_type;
    typedef typename Type::iterator      iterator;

    inline static iterator handle_inserted(Type&, iterator inserted_)
    { return inserted_; }

    inline static iterator add_over
        (Type& object, const interval_type& addend, iterator last_)
    {
        iterator first_ = object.lower_bound(addend);
        //BOOST_ASSERT(next(last_) == this->_set.upper_bound(inter_val));

        iterator it_ = first_;
        interval_type rest_interval = addend;

        add_front(object, rest_interval, it_);
        add_main (object, rest_interval, it_, last_);
        add_rear (object, rest_interval, it_);
        return it_;
    }

    inline static iterator add_over
        (Type& object, const interval_type& addend)
    {
        std::pair<iterator,iterator> overlap = object.equal_range(addend);
        iterator first_ = overlap.first,
                 end_   = overlap.second,
                 last_  = end_; --last_;

        iterator it_ = first_;
        interval_type rest_interval = addend;

        add_front(object, rest_interval, it_);
        add_main (object, rest_interval, it_, last_);
        add_rear (object, rest_interval, it_);

        return it_;
    }
};


template<class Type>
void add_front(Type& object, const typename Type::interval_type& inter_val, 
                                   typename Type::iterator&      first_)
{
    typedef typename Type::interval_type interval_type;
    typedef typename Type::iterator      iterator;
    // If the collision sequence has a left residual 'left_resid' it will
    // be split, to provide a standardized start of algorithms:
    // The addend interval 'inter_val' covers the beginning of the collision sequence.

    // only for the first there can be a left_resid: a part of *first_ left of inter_val
    interval_type left_resid = right_subtract(key_value<Type>(first_), inter_val);

    if(!icl::is_empty(left_resid))
    {   //            [------------ . . .
        // [left_resid---first_ --- . . .
        iterator prior_ = cyclic_prior(object, first_);
        const_cast<interval_type&>(key_value<Type>(first_)) 
            = left_subtract(key_value<Type>(first_), left_resid);
        //NOTE: Only splitting
        object._insert(prior_, icl::make_value<Type>(left_resid, co_value<Type>(first_)));
    }

    //POST:
    // [----- inter_val ---- . . .
    // ...[-- first_ --...
}


template<class Type>
void add_segment(Type& object, const typename Type::interval_type& inter_val, 
                                     typename Type::iterator&      it_      )
{
    typedef typename Type::interval_type interval_type;
    interval_type lead_gap = right_subtract(inter_val, *it_);
    if(!icl::is_empty(lead_gap))
        //           [lead_gap--- . . .
        // [prior_)           [-- it_ ...
        object._insert(prior(it_), lead_gap);

    // . . . --------- . . . addend interval
    //      [-- it_ --)      has a common part with the first overval
    ++it_;
}


template<class Type>
void add_main(Type& object, typename Type::interval_type& rest_interval, 
                            typename Type::iterator&      it_,
                      const typename Type::iterator&      last_)
{
    typedef typename Type::interval_type interval_type;
    interval_type cur_interval;
    while(it_ != last_)
    {
        cur_interval = *it_ ;
        add_segment(object, rest_interval, it_);
        // shrink interval
        rest_interval = left_subtract(rest_interval, cur_interval);
    }
}


template<class Type>
void add_rear(Type& object, const typename Type::interval_type& inter_val, 
                                  typename Type::iterator&      it_      )
{
    typedef typename Type::interval_type interval_type;
    typedef typename Type::iterator      iterator;

    iterator prior_ = cyclic_prior(object, it_);
    interval_type cur_itv = *it_;

    interval_type lead_gap = right_subtract(inter_val, cur_itv);
    if(!icl::is_empty(lead_gap))
        //          [lead_gap--- . . .
        // [prior_)          [-- it_ ...
        object._insert(prior_, lead_gap);
    
    interval_type end_gap = left_subtract(inter_val, cur_itv);
    if(!icl::is_empty(end_gap))
        // [---------------end_gap)
        //      [-- it_ --)
        it_ = object._insert(it_, end_gap);
    else
    {
        // only for the last there can be a right_resid: a part of *it_ right of addend
        interval_type right_resid = left_subtract(cur_itv, inter_val);

        if(!icl::is_empty(right_resid))
        {
            // [--------------)
            //      [-- it_ --right_resid)
            const_cast<interval_type&>(*it_) = right_subtract(*it_, right_resid);
            it_ = object._insert(it_, right_resid);
        }
    }
}


//==============================================================================
//= Addition
//==============================================================================
template<class Type>
typename Type::iterator 
    add(Type& object, const typename Type::value_type& addend)
{
    //CL typedef typename Type::interval_type interval_type;
    typedef typename Type::iterator      iterator;
    typedef typename on_style<Type, Type::fineness>::type on_style_;

    if(icl::is_empty(addend)) 
        return object.end();

    std::pair<iterator,bool> insertion = object._insert(addend);

    if(insertion.second)
        return on_style_::handle_inserted(object, insertion.first);
    else
        return on_style_::add_over(object, addend, insertion.first);
}


template<class Type>
typename Type::iterator 
    add(Type& object, typename Type::iterator    prior_, 
                const typename Type::value_type& addend)
{
    //CL typedef typename Type::interval_type interval_type;
    typedef typename Type::iterator      iterator;
    typedef typename on_style<Type, Type::fineness>::type on_style_;

    if(icl::is_empty(addend)) 
        return prior_;

    iterator insertion = object._insert(prior_, addend);

    if(*insertion == addend)
        return on_style_::handle_inserted(object, insertion);
    else
        return on_style_::add_over(object, addend);
}


//==============================================================================
//= Subtraction
//==============================================================================
template<class Type>
void subtract(Type& object, const typename Type::value_type& minuend)
{
    typedef typename Type::iterator      iterator;
    typedef typename Type::interval_type interval_type;
    //CL typedef typename Type::value_type    value_type;

    if(icl::is_empty(minuend)) return;

    std::pair<iterator, iterator> exterior = object.equal_range(minuend);
    if(exterior.first == exterior.second) return;

    iterator first_ = exterior.first;
    iterator end_   = exterior.second;
    iterator last_  = end_; --last_;

    interval_type leftResid = right_subtract(*first_, minuend);
    interval_type rightResid; 
    if(first_ != end_  )
        rightResid = left_subtract(*last_ , minuend);

    object.erase(first_, end_);

    if(!icl::is_empty(leftResid))
        object._insert(leftResid);

    if(!icl::is_empty(rightResid))
        object._insert(rightResid);
}


} // namespace Interval_Set

}} // namespace icl boost

#endif 


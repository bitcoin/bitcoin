/*-----------------------------------------------------------------------------+
Copyright (c) 2008-2009: Joachim Faulhaber
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/
#ifndef BOOST_ICL_SUBSET_COMPARER_HPP_JOFA_090202
#define BOOST_ICL_SUBSET_COMPARER_HPP_JOFA_090202

#include <boost/mpl/and.hpp>
#include <boost/icl/type_traits/is_map.hpp>
#include <boost/icl/detail/notate.hpp>
#include <boost/icl/detail/relation_state.hpp>
#include <boost/icl/type_traits/identity_element.hpp>
#include <boost/icl/type_traits/codomain_type_of.hpp>
#include <boost/icl/type_traits/is_concept_equivalent.hpp>
#include <boost/icl/type_traits/is_element_container.hpp>
#include <boost/icl/concept/interval_set_value.hpp>
#include <boost/icl/concept/map_value.hpp>

namespace boost{namespace icl
{

#ifdef BOOST_MSVC 
#pragma warning(push)
#pragma warning(disable:4127) // conditional expression is constant
#endif                        

namespace Set
{

//------------------------------------------------------------------------------
template<class LeftT, class RightT>
struct settic_codomain_compare
{
    static int apply(typename LeftT::const_iterator& left_, typename RightT::const_iterator& right_)
    {
        return inclusion_compare( co_value<LeftT>(left_), 
                                 co_value<RightT>(right_));
    }
};

template<class LeftT, class RightT>
struct atomic_codomain_compare
{
    static int apply(typename LeftT::const_iterator& left_, typename RightT::const_iterator& right_)
    {
        if(co_value<LeftT>(left_) == co_value<RightT>(right_))
            return inclusion::equal;
        else
            return inclusion::unrelated;
    }
};

template<class LeftT, class RightT>
struct empty_codomain_compare
{
    static int apply(typename LeftT::const_iterator&, typename RightT::const_iterator&)
    {
        return inclusion::equal;
    }
};

template<class LeftT, class RightT>
struct map_codomain_compare
{
    static int apply(typename LeftT::const_iterator& left_, typename RightT::const_iterator& right_)
    {
        using namespace boost::mpl;
        typedef typename LeftT::codomain_type  LeftCodomainT;
        typedef typename RightT::codomain_type RightCodomainT;

        return
            if_<
                bool_<is_concept_equivalent<is_set,LeftCodomainT,
                                                   RightCodomainT>::value>,
                settic_codomain_compare<LeftT,RightT>,
                atomic_codomain_compare<LeftT,RightT>
            >
            ::type::apply(left_, right_);
    }
};


//------------------------------------------------------------------------------
template<class LeftT, class RightT>
class subset_comparer
{
private:
    subset_comparer& operator = (const subset_comparer&);
public:
    typedef typename LeftT::const_iterator  LeftIterT;
    typedef typename RightT::const_iterator RightIterT;

    BOOST_STATIC_CONSTANT(bool, 
        _compare_codomain = (mpl::and_<is_map<LeftT>, is_map<RightT> >::value)); 

    subset_comparer(const LeftT&      left,
                    const RightT&     right,
                    const LeftIterT&  left_end,
                    const RightIterT& right_end)
        : _left(left), _right(right),
          _left_end(left_end), _right_end(right_end), _result(equal)
    {}

    enum{nextboth, stop};

    enum
    {
        unrelated  = inclusion::unrelated, 
        subset     = inclusion::subset,     // left is_subset_of   right 
        superset   = inclusion::superset,   // left is_superset_of right
        equal      = inclusion::equal       // equal = subset | superset
    };

    int result()const{ return _result; }

    int co_compare(LeftIterT& left, RightIterT& right)
    {
        using namespace boost::mpl;
        //CL typedef typename codomain_type_of<LeftT>::type  LeftCodomainT;
        //CL typedef typename codomain_type_of<RightT>::type RightCodomainT;

        return  
            if_<
                bool_<is_concept_equivalent<is_element_map,LeftT,RightT>::value>,
                map_codomain_compare<LeftT,RightT>,
                empty_codomain_compare<LeftT,RightT>
            >
            ::type::apply(left,right);
    }

    int restrict_result(int state) { return _result &= state; }

    int next_both(LeftIterT& left, RightIterT& right)
    {
        if(left == _left_end && right == _right_end)
            return stop;
        else if(left == _left_end)
        {
            restrict_result(subset);
            return stop;
        }
        else if(right == _right_end)
        {
            restrict_result(superset);
            return stop;
        }
        else if(typename LeftT::key_compare()(key_value<LeftT>(left), key_value<RightT>(right)))
        {   // left:  *left . . *joint_     left could be superset
            // right:           *right ...  if joint_ exists
            restrict_result(superset);
            if(unrelated == _result)
                return stop;
            else
            {
                LeftIterT joint_ = _left.lower_bound(key_value<RightT>(right));
                if(    joint_ == _left.end() 
                    || typename LeftT::key_compare()(key_value<RightT>(right), key_value<LeftT>(joint_)))
                {
                    _result = unrelated;
                    return stop;
                }
                else
                    left = joint_;
            }
        }
        else if(typename LeftT::key_compare()(key_value<RightT>(right), key_value<LeftT>(left)))
        {   // left:             *left   left could be subset
            // right:*right . . .*joint_ if *joint_ exists 
            restrict_result(subset);
            if(unrelated == _result)
                return stop;
            else
            {
                RightIterT joint_ = _right.lower_bound(key_value<LeftT>(left));
                if(    joint_ == _right.end()
                    || typename LeftT::key_compare()(key_value<LeftT>(left), key_value<RightT>(joint_)))
                {
                    _result = unrelated;
                    return stop;
                }
                else
                    right = joint_;
            }
        }

        // left =key= right 
        if(_compare_codomain)
            if(unrelated == restrict_result(co_compare(left,right)))
                return stop;

        ++left;
        ++right;
        return nextboth;
    }

private:
    const LeftT&  _left;
    const RightT& _right;
    LeftIterT     _left_end;
    RightIterT    _right_end;
    int           _result;
};





//------------------------------------------------------------------------------
// Subset/superset comparison on ranges of two interval container 
//------------------------------------------------------------------------------
template<class LeftT, class RightT>
int subset_compare
(
    const LeftT& left,   //sub
    const RightT& right, //super
    typename LeftT::const_iterator  left_begin,   
    typename LeftT::const_iterator  left_end,
    typename RightT::const_iterator right_begin, 
    typename RightT::const_iterator right_end
)
{
    typedef subset_comparer<LeftT,RightT> Step;
    Step step(left, right, left_end, right_end);

    typename LeftT::const_iterator  left_  = left_begin;
    typename RightT::const_iterator right_ = right_begin;

    int state = Step::nextboth;
    while(state != Step::stop)
        state = step.next_both(left_, right_);

    return step.result();
}

template<class LeftT, class RightT>
int subset_compare(const LeftT& left, const RightT& right)
{
    return subset_compare
        (
            left, right,
            left.begin(), left.end(),
            right.begin(), right.end()
        );
}


} // namespace Set
    
#ifdef BOOST_MSVC
#pragma warning(pop)
#endif

}} // namespace icl boost

#endif 


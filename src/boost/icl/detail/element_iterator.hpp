/*-----------------------------------------------------------------------------+
Copyright (c) 2009-2009: Joachim Faulhaber
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/
#ifndef BOOST_ICL_DETAIL_ELEMENT_ITERATOR_HPP_JOFA_091104
#define BOOST_ICL_DETAIL_ELEMENT_ITERATOR_HPP_JOFA_091104

#include <boost/mpl/if.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/config/warning_disable.hpp>
#include <boost/icl/type_traits/succ_pred.hpp>
#include <boost/icl/detail/mapped_reference.hpp>

namespace boost{namespace icl
{

//------------------------------------------------------------------------------
template<class Type>
struct is_std_pair
{ 
    typedef is_std_pair<Type> type; 
    BOOST_STATIC_CONSTANT(bool, value = false);
};

template<class FirstT, class SecondT>
struct is_std_pair<std::pair<FirstT, SecondT> >
{ 
    typedef is_std_pair<std::pair<FirstT, SecondT> > type; 
    BOOST_STATIC_CONSTANT(bool, value = true);
};


//------------------------------------------------------------------------------
template<class Type>
struct first_element
{ 
    typedef Type type; 
};

template<class FirstT, class SecondT>
struct first_element<std::pair<FirstT, SecondT> >
{ 
    typedef FirstT type; 
};

//------------------------------------------------------------------------------
template <class SegmentIteratorT> class element_iterator;

template<class IteratorT>
struct is_reverse
{ 
    typedef is_reverse type; 
    BOOST_STATIC_CONSTANT(bool, value = false);
};

template<class BaseIteratorT>
struct is_reverse<std::reverse_iterator<BaseIteratorT> >
{ 
    typedef is_reverse<std::reverse_iterator<BaseIteratorT> > type; 
    BOOST_STATIC_CONSTANT(bool, value = true);
};

template<class BaseIteratorT>
struct is_reverse<icl::element_iterator<BaseIteratorT> >
{ 
    typedef is_reverse<icl::element_iterator<BaseIteratorT> > type; 
    BOOST_STATIC_CONSTANT(bool, value = is_reverse<BaseIteratorT>::value);
};

//------------------------------------------------------------------------------
template<class SegmentT>
struct elemental;

#ifdef ICL_USE_INTERVAL_TEMPLATE_TEMPLATE

    template<class DomainT, ICL_COMPARE Compare, ICL_INTERVAL(ICL_COMPARE) Interval>
    struct elemental<ICL_INTERVAL_TYPE(Interval,DomainT,Compare) >
    {
        typedef ICL_INTERVAL_TYPE(Interval,DomainT,Compare) segment_type;
        typedef segment_type              interval_type;
        typedef DomainT                   type;
        typedef DomainT                   domain_type;
        typedef DomainT                   codomain_type;
        typedef DomainT                   transit_type;
    };

    template< class DomainT, class CodomainT, 
              ICL_COMPARE Compare, ICL_INTERVAL(ICL_COMPARE) Interval >
    struct elemental<std::pair<ICL_INTERVAL_TYPE(Interval,DomainT,Compare) const, CodomainT> >
    {
        typedef std::pair<ICL_INTERVAL_TYPE(Interval,DomainT,Compare), CodomainT> segment_type;
        typedef ICL_INTERVAL_TYPE(Interval,DomainT,Compare)                       interval_type;
        typedef std::pair<DomainT, CodomainT>                   type;
        typedef DomainT                                         domain_type;
        typedef CodomainT                                       codomain_type;
        typedef mapped_reference<DomainT, CodomainT>            transit_type;
    };

#else //ICL_USE_INTERVAL_TEMPLATE_TYPE

    template<ICL_INTERVAL(ICL_COMPARE) Interval>
    struct elemental
    {
        typedef ICL_INTERVAL_TYPE(Interval,DomainT,Compare) segment_type;
        typedef segment_type                        interval_type;
        typedef typename interval_traits<interval_type>::domain_type domain_type;
        typedef domain_type                         type;
        typedef domain_type                         codomain_type;
        typedef domain_type                         transit_type;
    };

    template< class CodomainT, ICL_INTERVAL(ICL_COMPARE) Interval >
    struct elemental<std::pair<ICL_INTERVAL_TYPE(Interval,DomainT,Compare) const, CodomainT> >
    {
        typedef std::pair<ICL_INTERVAL_TYPE(Interval,DomainT,Compare), CodomainT> segment_type;
        typedef ICL_INTERVAL_TYPE(Interval,DomainT,Compare)                       interval_type;
        typedef typename interval_traits<interval_type>::domain_type domain_type;
        typedef CodomainT                                       codomain_type;
        typedef std::pair<domain_type, codomain_type>           type;
        typedef mapped_reference<domain_type, codomain_type>    transit_type;
    };

#endif //ICL_USE_INTERVAL_TEMPLATE_TEMPLATE


//------------------------------------------------------------------------------
//- struct segment_adapter
//------------------------------------------------------------------------------
template<class SegmentIteratorT, class SegmentT>
struct segment_adapter;

#ifdef ICL_USE_INTERVAL_TEMPLATE_TEMPLATE

template<class SegmentIteratorT, class DomainT, ICL_COMPARE Compare, ICL_INTERVAL(ICL_COMPARE) Interval>
struct segment_adapter<SegmentIteratorT, ICL_INTERVAL_TYPE(Interval,DomainT,Compare) >
{
    typedef segment_adapter                         type;
    typedef ICL_INTERVAL_TYPE(Interval,DomainT,Compare) segment_type;
    typedef segment_type                            interval_type;
    typedef typename interval_type::difference_type domain_difference_type;
    typedef DomainT                                 domain_type;
    typedef DomainT                                 codomain_type;
    typedef domain_type                             element_type;
    typedef domain_type&                            transit_type;

    static domain_type     first (const SegmentIteratorT& leaper){ return leaper->first(); } 
    static domain_type     last  (const SegmentIteratorT& leaper){ return leaper->last();  } 
    static domain_difference_type length(const SegmentIteratorT& leaper){ return leaper->length();}

    static transit_type transient_element(domain_type& inter_pos, const SegmentIteratorT& leaper, 
                                          const domain_difference_type& sneaker)
    { 
        inter_pos = is_reverse<SegmentIteratorT>::value ? leaper->last()  - sneaker
                                                        : leaper->first() + sneaker;
        return inter_pos; 
    }
};

template < class SegmentIteratorT, class DomainT, class CodomainT, 
           ICL_COMPARE Compare, ICL_INTERVAL(ICL_COMPARE) Interval >
struct segment_adapter<SegmentIteratorT, std::pair<ICL_INTERVAL_TYPE(Interval,DomainT,Compare) const, CodomainT> >
{
    typedef segment_adapter                         type;
    typedef ICL_INTERVAL_TYPE(Interval,DomainT,Compare)               interval_type;
    typedef DomainT                                 domain_type;
    typedef std::pair<DomainT, CodomainT>           element_type;
    typedef CodomainT                               codomain_type;
    typedef mapped_reference<DomainT, CodomainT>    transit_type;    
    typedef typename difference_type_of<interval_traits<interval_type> >::type 
                                                    domain_difference_type;

    static domain_type     first (const SegmentIteratorT& leaper){ return leaper->first.first(); } 
    static domain_type     last  (const SegmentIteratorT& leaper){ return leaper->first.last();  } 
    static domain_difference_type length(const SegmentIteratorT& leaper){ return leaper->first.length();}

    static transit_type transient_element(domain_type& inter_pos, const SegmentIteratorT& leaper,
                                          const domain_difference_type& sneaker)
    {
        inter_pos = is_reverse<SegmentIteratorT>::value ? leaper->first.last()  - sneaker
                                                        : leaper->first.first() + sneaker;
        return transit_type(inter_pos, leaper->second); 
    }
};

#else // ICL_USE_INTERVAL_TEMPLATE_TYPE

template<class SegmentIteratorT, ICL_INTERVAL(ICL_COMPARE) Interval>
struct segment_adapter 
{
    typedef segment_adapter                          type;
    typedef ICL_INTERVAL_TYPE(Interval,DomainT,Compare) segment_type;
    typedef segment_type                             interval_type;
    typedef typename interval_traits<interval_type>::domain_type domain_type;
    typedef domain_type                              codomain_type;
    typedef domain_type                              element_type;
    typedef domain_type&                             transit_type;
    typedef typename difference_type_of<interval_traits<interval_type> >::type 
                                                     domain_difference_type;

    static domain_type     first (const SegmentIteratorT& leaper){ return leaper->first(); } 
    static domain_type     last  (const SegmentIteratorT& leaper){ return leaper->last();  } 
    static domain_difference_type length(const SegmentIteratorT& leaper){ return icl::length(*leaper);}

    static transit_type transient_element(domain_type& inter_pos, const SegmentIteratorT& leaper, 
                                          const domain_difference_type& sneaker)
    { 
        inter_pos = is_reverse<SegmentIteratorT>::value ? icl::last(*leaper)  - sneaker
                                                        : icl::first(*leaper) + sneaker;
        return inter_pos; 
    }
};

template < class SegmentIteratorT, class CodomainT, ICL_INTERVAL(ICL_COMPARE) Interval >
struct segment_adapter<SegmentIteratorT, std::pair<ICL_INTERVAL_TYPE(Interval,DomainT,Compare) const, CodomainT> >
{
    typedef segment_adapter                                type;
    typedef ICL_INTERVAL_TYPE(Interval,DomainT,Compare)    interval_type;
    typedef typename interval_traits<interval_type>::domain_type domain_type;
    typedef CodomainT                                      codomain_type;
    typedef std::pair<domain_type, codomain_type>          element_type;
    typedef mapped_reference<domain_type, CodomainT>       transit_type;    
    typedef typename difference_type_of<interval_traits<interval_type> >::type 
                                                           domain_difference_type;

    static domain_type     first (const SegmentIteratorT& leaper){ return leaper->first.first(); } 
    static domain_type     last  (const SegmentIteratorT& leaper){ return leaper->first.last();  } 
    static domain_difference_type length(const SegmentIteratorT& leaper){ return icl::length(leaper->first);}

    static transit_type transient_element(domain_type& inter_pos, const SegmentIteratorT& leaper,
                                          const domain_difference_type& sneaker)
    {
        inter_pos = is_reverse<SegmentIteratorT>::value ? icl::last(leaper->first)  - sneaker
                                                        : icl::first(leaper->first) + sneaker;
        return transit_type(inter_pos, leaper->second); 
    }
};

#endif // ICL_USE_INTERVAL_TEMPLATE_TEMPLATE

template <class SegmentIteratorT>
class element_iterator
  : public boost::iterator_facade<
          element_iterator<SegmentIteratorT>
        , typename elemental<typename SegmentIteratorT::value_type>::transit_type
        , boost::bidirectional_traversal_tag
        , typename elemental<typename SegmentIteratorT::value_type>::transit_type
    >
{
public:
    typedef element_iterator                                type;
    typedef SegmentIteratorT                                segment_iterator;
    typedef typename SegmentIteratorT::value_type           segment_type;
    typedef typename first_element<segment_type>::type      interval_type;
    typedef typename elemental<segment_type>::type          element_type;
    typedef typename elemental<segment_type>::domain_type   domain_type;
    typedef typename elemental<segment_type>::codomain_type codomain_type;
    typedef typename elemental<segment_type>::transit_type  transit_type;
    typedef transit_type                                    value_type;
    typedef typename difference_type_of<interval_traits<interval_type> >::type 
                                                            domain_difference_type;

private:
    typedef typename segment_adapter<segment_iterator,segment_type>::type adapt;

    struct enabler{};

public:
    element_iterator()
        : _saltator(identity_element<segment_iterator>::value())
        , _reptator(identity_element<domain_difference_type>::value()){}

    explicit element_iterator(segment_iterator jumper)
        : _saltator(jumper), _reptator(identity_element<domain_difference_type>::value()) {}

    template <class SaltatorT>
    element_iterator
        ( element_iterator<SaltatorT> const& other
        , typename enable_if<boost::is_convertible<SaltatorT*,SegmentIteratorT*>, enabler>::type = enabler())
        : _saltator(other._saltator), _reptator(other._reptator) {}

private:
    friend class boost::iterator_core_access;
    template <class> friend class element_iterator;

    template <class SaltatorT>
    bool equal(element_iterator<SaltatorT> const& other) const
    {
        return this->_saltator == other._saltator
            && this->_reptator == other._reptator;
    }

    void increment()
    { 
        if(_reptator < icl::pred(adapt::length(_saltator)))
            ++_reptator; 
        else
        {
            ++_saltator;
            _reptator = identity_element<domain_difference_type>::value();
        }
    }

    void decrement()
    { 
        if(identity_element<domain_difference_type>::value() < _reptator)
            --_reptator; 
        else
        {
            --_saltator;
            _reptator = adapt::length(_saltator);
            --_reptator;
        }
    }

    value_type dereference()const
    {
        return adapt::transient_element(_inter_pos, _saltator, _reptator);
    }

private:
    segment_iterator               _saltator;  // satltare: to jump  : the fast moving iterator
    mutable domain_difference_type _reptator;  // reptare:  to sneak : the slow moving iterator 0 based
    mutable domain_type            _inter_pos; // inter position : Position within the current segment
                                               // _saltator->first.first() <= _inter_pos <= _saltator->first.last() 
};

}} // namespace icl boost

#endif // BOOST_ICL_DETAIL_ELEMENT_ITERATOR_HPP_JOFA_091104




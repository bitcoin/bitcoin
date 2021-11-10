/*-----------------------------------------------------------------------------+
Copyright (c) 2008-2012: Joachim Faulhaber
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/
#ifndef BOOST_ICL_INTERVAL_MAP_HPP_JOFA_080705
#define BOOST_ICL_INTERVAL_MAP_HPP_JOFA_080705

#include <boost/assert.hpp>
#include <boost/icl/type_traits/is_map.hpp>
#include <boost/icl/interval_set.hpp>
#include <boost/icl/interval_base_map.hpp>

namespace boost{namespace icl
{

template<class DomainT, class CodomainT, class Traits, 
         ICL_COMPARE Compare, ICL_COMBINE Combine, ICL_SECTION Section, 
         ICL_INTERVAL(ICL_COMPARE)  Interval, ICL_ALLOC Alloc>
class split_interval_map;

/** \brief implements a map as a map of intervals - on insertion 
    overlapping intervals are split and associated values are combined.*/
template
<
    typename DomainT,
    typename CodomainT,
    class Traits = icl::partial_absorber,
    ICL_COMPARE Compare  = ICL_COMPARE_INSTANCE(ICL_COMPARE_DEFAULT, DomainT),
    ICL_COMBINE Combine  = ICL_COMBINE_INSTANCE(icl::inplace_plus, CodomainT),
    ICL_SECTION Section  = ICL_SECTION_INSTANCE(icl::inter_section, CodomainT), 
    ICL_INTERVAL(ICL_COMPARE) Interval = ICL_INTERVAL_INSTANCE(ICL_INTERVAL_DEFAULT, DomainT, Compare),
    ICL_ALLOC   Alloc    = std::allocator
>
class interval_map:

    public interval_base_map<interval_map<DomainT,CodomainT,Traits,Compare,Combine,Section,Interval,Alloc>, 
                             DomainT,CodomainT,Traits,Compare,Combine,Section,Interval,Alloc>
{
public:
    typedef Traits traits;
    typedef       interval_map<DomainT,CodomainT,Traits,Compare,Combine,Section,Interval,Alloc> type;
    typedef split_interval_map<DomainT,CodomainT,Traits,Compare,Combine,Section,Interval,Alloc> split_type;
    typedef type overloadable_type;
    typedef type joint_type;
    typedef interval_base_map<type, 
                              DomainT,CodomainT,Traits,Compare,Combine,Section,Interval,Alloc> base_type;

    typedef ICL_INTERVAL_TYPE(Interval,DomainT,Compare) interval_type;
    typedef typename base_type::iterator      iterator;
    typedef typename base_type::value_type    value_type;
    typedef typename base_type::element_type  element_type;
    typedef typename base_type::segment_type  segment_type;
    typedef typename base_type::domain_type   domain_type;
    typedef typename base_type::codomain_type codomain_type;
    typedef typename base_type::domain_mapping_type domain_mapping_type;
    typedef typename base_type::interval_mapping_type interval_mapping_type;
    typedef typename base_type::ImplMapT ImplMapT;

    typedef typename base_type::size_type     size_type;
    typedef typename base_type::codomain_combine codomain_combine;

    typedef interval_set<DomainT,Compare,Interval,Alloc> interval_set_type;
    typedef interval_set_type set_type;
    typedef set_type          key_object_type;

    enum { fineness = 1 };

public:
    //==========================================================================
    //= Construct, copy, destruct
    //==========================================================================

    /// Default constructor for the empty object
    interval_map(): base_type() {}

    /// Copy constructor
    interval_map(const interval_map& src): base_type(src) {}

    /// Copy constructor for base_type
    template<class SubType>
    explicit interval_map
        (const interval_base_map<SubType,DomainT,CodomainT,
                                 Traits,Compare,Combine,Section,Interval,Alloc>& src)
    { this->assign(src); }

    explicit interval_map(const domain_mapping_type& base_pair): base_type()
    { this->add(base_pair); }

    explicit interval_map(const value_type& value_pair): base_type()
    { this->add(value_pair); }


    /// Assignment from a base interval_map.
    template<class SubType>
    void assign(const interval_base_map<SubType,DomainT,CodomainT,
                                        Traits,Compare,Combine,Section,Interval,Alloc>& src)
    {
        typedef interval_base_map<SubType,DomainT,CodomainT,
                                  Traits,Compare,Combine,Section,Interval,Alloc> base_map_type;
        this->clear();
        iterator prior_ = this->_map.end();
        ICL_const_FORALL(typename base_map_type, it_, src) 
            prior_ = this->add(prior_, *it_); 
    }

    /// Assignment operator for base type
    template<class SubType>
    interval_map& operator =
        (const interval_base_map<SubType,DomainT,CodomainT,
                                 Traits,Compare,Combine,Section,Interval,Alloc>& src)
    { 
        this->assign(src); 
        return *this; 
    }

#   ifndef BOOST_ICL_NO_CXX11_RVALUE_REFERENCES
    //==========================================================================
    //= Move semantics
    //==========================================================================

    /// Move constructor
    interval_map(interval_map&& src)
        : base_type(boost::move(src))
    {}

    /// Move assignment operator
    interval_map& operator = (interval_map src)
    { 
        base_type::operator=(boost::move(src));
        return *this;
    }

    //==========================================================================
#   else

    /// Assignment operator
    interval_map& operator = (const interval_map& src)
    { 
        base_type::operator=(src);
        return *this;
    }

#   endif // BOOST_ICL_NO_CXX11_RVALUE_REFERENCES

private:
    // Private functions that shall be accessible by the baseclass:
    friend class
        interval_base_map <interval_map<DomainT,CodomainT,Traits,Compare,Combine,Section,Interval,Alloc>, 
                                        DomainT,CodomainT,Traits,Compare,Combine,Section,Interval,Alloc>;

    iterator handle_inserted(iterator it_)
    {
        return segmental::join_neighbours(*this, it_); 
    }

    void handle_inserted(iterator prior_, iterator it_)
    {
        if(prior_ != this->_map.end() && segmental::joinable(*this, prior_, it_))
            segmental::join_on_right(*this, prior_, it_);
    }

    template<class Combiner>
    void handle_left_combined(iterator it_)
    {
        if(on_absorbtion<type,Combiner,Traits::absorbs_identities>::is_absorbable((*it_).second))
            this->_map.erase(it_);
        else
            segmental::join_left(*this, it_);
    }

    template<class Combiner>
    void handle_combined(iterator it_)
    {
        if(on_absorbtion<type,Combiner,Traits::absorbs_identities>::is_absorbable((*it_).second))
            this->_map.erase(it_);
        else
            segmental::join_neighbours(*this, it_);
    }

    template<class Combiner>
    void handle_preceeded_combined(iterator prior_, iterator& it_)
    {
        if(on_absorbtion<type,Combiner,Traits::absorbs_identities>::is_absorbable((*it_).second))
        {
            this->_map.erase(it_);
            it_ = prior_;
        }
        else // After a new combination (e.g. combiner=max) joining neighbours may be possible
            segmental::join_neighbours(*this, it_);
    }

    template<class Combiner>
    void handle_succeeded_combined(iterator it_, iterator next_)
    {
        if(on_absorbtion<type,Combiner,Traits::absorbs_identities>::is_absorbable((*it_).second))
        {
            this->_map.erase(it_);
            segmental::join_right(*this, next_);
        }
        else
        {
            segmental::join_left(*this, it_);
            segmental::join_neighbours(*this, next_);
        }
    }



    void handle_reinserted(iterator insertion_)
    { 
        segmental::join_right(*this, insertion_); 
    }


    template<class Combiner>
    void gap_insert_at(iterator& it_, iterator prior_, 
                       const interval_type& end_gap, const codomain_type& co_val)
    {
        if(on_absorbtion<type,Combiner,Traits::absorbs_identities>::is_absorbable((*it_).second))
        {
            this->_map.erase(it_);
            it_ = this->template gap_insert<Combiner>(prior_, end_gap, co_val);
            segmental::join_right(*this, it_);
        }
        else 
        {
            segmental::join_left(*this, it_);
            iterator inserted_ = this->template gap_insert<Combiner>(it_, end_gap, co_val);
            it_ = segmental::join_neighbours(*this, inserted_);
        }
    }

} ;


//-----------------------------------------------------------------------------
// type traits
//-----------------------------------------------------------------------------
template <class DomainT, class CodomainT, class Traits, ICL_COMPARE Compare, ICL_COMBINE Combine, ICL_SECTION Section, ICL_INTERVAL(ICL_COMPARE)  Interval, ICL_ALLOC Alloc>
struct is_map<icl::interval_map<DomainT,CodomainT,Traits,Compare,Combine,Section,Interval,Alloc> >
{ 
    typedef is_map<icl::interval_map<DomainT,CodomainT,Traits,Compare,Combine,Section,Interval,Alloc> > type;
    BOOST_STATIC_CONSTANT(bool, value = true); 
};

template <class DomainT, class CodomainT, class Traits, ICL_COMPARE Compare, ICL_COMBINE Combine, ICL_SECTION Section, ICL_INTERVAL(ICL_COMPARE)  Interval, ICL_ALLOC Alloc>
struct has_inverse<icl::interval_map<DomainT,CodomainT,Traits,Compare,Combine,Section,Interval,Alloc> >
{ 
    typedef has_inverse<icl::interval_map<DomainT,CodomainT,Traits,Compare,Combine,Section,Interval,Alloc> > type;
    BOOST_STATIC_CONSTANT(bool, value = (has_inverse<CodomainT>::value)); 
};


template <class DomainT, class CodomainT, class Traits, ICL_COMPARE Compare, ICL_COMBINE Combine, ICL_SECTION Section, ICL_INTERVAL(ICL_COMPARE)  Interval, ICL_ALLOC Alloc>
struct is_interval_container<icl::interval_map<DomainT,CodomainT,Traits,Compare,Combine,Section,Interval,Alloc> >
{
    typedef is_interval_container<icl::interval_map<DomainT,CodomainT,Traits,Compare,Combine,Section,Interval,Alloc> > type;
    BOOST_STATIC_CONSTANT(bool, value = true); 
};

template <class DomainT, class CodomainT, class Traits, ICL_COMPARE Compare, ICL_COMBINE Combine, ICL_SECTION Section, ICL_INTERVAL(ICL_COMPARE)  Interval, ICL_ALLOC Alloc>
struct absorbs_identities<icl::interval_map<DomainT,CodomainT,Traits,Compare,Combine,Section,Interval,Alloc> >
{
    typedef absorbs_identities<icl::interval_map<DomainT,CodomainT,Traits,Compare,Combine,Section,Interval,Alloc> > type;
    BOOST_STATIC_CONSTANT(bool, value = (Traits::absorbs_identities)); 
};

template <class DomainT, class CodomainT, class Traits, ICL_COMPARE Compare, ICL_COMBINE Combine, ICL_SECTION Section, ICL_INTERVAL(ICL_COMPARE)  Interval, ICL_ALLOC Alloc>
struct is_total<icl::interval_map<DomainT,CodomainT,Traits,Compare,Combine,Section,Interval,Alloc> >
{
    typedef is_total<icl::interval_map<DomainT,CodomainT,Traits,Compare,Combine,Section,Interval,Alloc> > type;
    BOOST_STATIC_CONSTANT(bool, value = (Traits::is_total)); 
};


//-----------------------------------------------------------------------------
// type representation
//-----------------------------------------------------------------------------
template <class DomainT, class CodomainT, class Traits, ICL_COMPARE Compare, ICL_COMBINE Combine, ICL_SECTION Section, ICL_INTERVAL(ICL_COMPARE)  Interval, ICL_ALLOC Alloc>
struct type_to_string<icl::interval_map<DomainT,CodomainT,Traits,Compare,Combine,Section,Interval,Alloc> >
{
    static std::string apply()
    { 
        return "itv_map<"+ type_to_string<DomainT>::apply()   + ","
                         + type_to_string<CodomainT>::apply() + ","
                         + type_to_string<Traits>::apply()    + ">"; 
    }
};

}} // namespace icl boost

#endif



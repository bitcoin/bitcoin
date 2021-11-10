/*-----------------------------------------------------------------------------+
Copyright (c) 2007-2012: Joachim Faulhaber
Copyright (c) 1999-2006: Cortex Software GmbH, Kantstrasse 57, Berlin
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/
#ifndef BOOST_ICL_INTERVAL_BASE_MAP_HPP_JOFA_990223
#define BOOST_ICL_INTERVAL_BASE_MAP_HPP_JOFA_990223

#include <limits>
#include <boost/mpl/and.hpp>
#include <boost/mpl/or.hpp>
#include <boost/mpl/not.hpp>

#include <boost/icl/detail/notate.hpp> 
#include <boost/icl/detail/design_config.hpp>
#include <boost/icl/detail/on_absorbtion.hpp>
#include <boost/icl/detail/interval_map_algo.hpp>
#include <boost/icl/detail/exclusive_less_than.hpp>

#include <boost/icl/associative_interval_container.hpp>

#include <boost/icl/type_traits/is_interval_splitter.hpp>
#include <boost/icl/map.hpp>

namespace boost{namespace icl
{

template<class DomainT, class CodomainT>
struct mapping_pair
{
    DomainT   key;
    CodomainT data;

    mapping_pair():key(), data(){}

    mapping_pair(const DomainT& key_value, const CodomainT& data_value)
        :key(key_value), data(data_value){}

    mapping_pair(const std::pair<DomainT,CodomainT>& std_pair)
        :key(std_pair.first), data(std_pair.second){}
};

/** \brief Implements a map as a map of intervals (base class) */
template
<
    class SubType,
    typename DomainT,
    typename CodomainT,
    class Traits = icl::partial_absorber,
    ICL_COMPARE Compare  = ICL_COMPARE_INSTANCE(ICL_COMPARE_DEFAULT, DomainT),
    ICL_COMBINE Combine  = ICL_COMBINE_INSTANCE(icl::inplace_plus, CodomainT),
    ICL_SECTION Section  = ICL_SECTION_INSTANCE(icl::inter_section, CodomainT), 
    ICL_INTERVAL(ICL_COMPARE) Interval = ICL_INTERVAL_INSTANCE(ICL_INTERVAL_DEFAULT, DomainT, Compare),
    ICL_ALLOC   Alloc    = std::allocator
>
class interval_base_map
{
public:
    //==========================================================================
    //= Associated types
    //==========================================================================
    typedef interval_base_map<SubType,DomainT,CodomainT,
                              Traits,Compare,Combine,Section,Interval,Alloc>
                              type;

    /// The designated \e derived or \e sub_type of this base class
    typedef SubType sub_type;

    /// Auxilliary type for overloadresolution
    typedef type overloadable_type;

    /// Traits of an itl map
    typedef Traits traits;

    //--------------------------------------------------------------------------
    //- Associated types: Related types
    //--------------------------------------------------------------------------
    /// The atomized type representing the corresponding container of elements
    typedef typename icl::map<DomainT,CodomainT,
                              Traits,Compare,Combine,Section,Alloc> atomized_type;

    //--------------------------------------------------------------------------
    //- Associated types: Data
    //--------------------------------------------------------------------------
    /// Domain type (type of the keys) of the map
    typedef DomainT   domain_type;
    typedef typename boost::call_traits<DomainT>::param_type domain_param;
    /// Domain type (type of the keys) of the map
    typedef CodomainT codomain_type;
    /// Auxiliary type to help the compiler resolve ambiguities when using std::make_pair
    typedef mapping_pair<domain_type,codomain_type> domain_mapping_type;
    /// Conceptual is a map a set of elements of type \c element_type
    typedef domain_mapping_type element_type;
    /// The interval type of the map
    typedef ICL_INTERVAL_TYPE(Interval,DomainT,Compare) interval_type;
    /// Auxiliary type for overload resolution
    typedef std::pair<interval_type,CodomainT> interval_mapping_type;
    /// Type of an interval containers segment, that is spanned by an interval
    typedef std::pair<interval_type,CodomainT> segment_type;

    //--------------------------------------------------------------------------
    //- Associated types: Size
    //--------------------------------------------------------------------------
    /// The difference type of an interval which is sometimes different form the domain_type
    typedef typename difference_type_of<domain_type>::type difference_type;
    /// The size type of an interval which is mostly std::size_t
    typedef typename size_type_of<domain_type>::type size_type;

    //--------------------------------------------------------------------------
    //- Associated types: Functors
    //--------------------------------------------------------------------------
    /// Comparison functor for domain values
    typedef ICL_COMPARE_DOMAIN(Compare,DomainT)      domain_compare;
    typedef ICL_COMPARE_DOMAIN(Compare,segment_type) segment_compare;
    /// Combine functor for codomain value aggregation
    typedef ICL_COMBINE_CODOMAIN(Combine,CodomainT)  codomain_combine;
    /// Inverse Combine functor for codomain value aggregation
    typedef typename inverse<codomain_combine>::type inverse_codomain_combine;
    /// Intersection functor for codomain values

    typedef typename mpl::if_
    <has_set_semantics<codomain_type>
    , ICL_SECTION_CODOMAIN(Section,CodomainT)     
    , codomain_combine
    >::type                                            codomain_intersect;


    /// Inverse Combine functor for codomain value intersection
    typedef typename inverse<codomain_intersect>::type inverse_codomain_intersect;

    /// Comparison functor for intervals which are keys as well
    typedef exclusive_less_than<interval_type> interval_compare;

    /// Comparison functor for keys
    typedef exclusive_less_than<interval_type> key_compare;

    //--------------------------------------------------------------------------
    //- Associated types: Implementation and stl related
    //--------------------------------------------------------------------------
    /// The allocator type of the set
    typedef Alloc<std::pair<const interval_type, codomain_type> > 
        allocator_type;

    /// Container type for the implementation 
    typedef ICL_IMPL_SPACE::map<interval_type,codomain_type,
                                key_compare,allocator_type> ImplMapT;

    /// key type of the implementing container
    typedef typename ImplMapT::key_type   key_type;
    /// value type of the implementing container
    typedef typename ImplMapT::value_type value_type;
    /// data type of the implementing container
    typedef typename ImplMapT::value_type::second_type data_type;

    /// pointer type
    typedef typename ImplMapT::pointer         pointer;
    /// const pointer type
    typedef typename ImplMapT::const_pointer   const_pointer;
    /// reference type
    typedef typename ImplMapT::reference       reference;
    /// const reference type
    typedef typename ImplMapT::const_reference const_reference;

    /// iterator for iteration over intervals
    typedef typename ImplMapT::iterator iterator;
    /// const_iterator for iteration over intervals
    typedef typename ImplMapT::const_iterator const_iterator;
    /// iterator for reverse iteration over intervals
    typedef typename ImplMapT::reverse_iterator reverse_iterator;
    /// const_iterator for iteration over intervals
    typedef typename ImplMapT::const_reverse_iterator const_reverse_iterator;

    /// element iterator: Depreciated, see documentation.
    typedef boost::icl::element_iterator<iterator> element_iterator; 
    /// const element iterator: Depreciated, see documentation.
    typedef boost::icl::element_iterator<const_iterator> element_const_iterator; 
    /// element reverse iterator: Depreciated, see documentation.
    typedef boost::icl::element_iterator<reverse_iterator> element_reverse_iterator; 
    /// element const reverse iterator: Depreciated, see documentation.
    typedef boost::icl::element_iterator<const_reverse_iterator> element_const_reverse_iterator; 
    
    typedef typename on_absorbtion<type, codomain_combine, 
                                Traits::absorbs_identities>::type on_codomain_absorbtion;

public:
    BOOST_STATIC_CONSTANT(bool, 
        is_total_invertible = (   Traits::is_total 
                               && has_inverse<codomain_type>::value));

    BOOST_STATIC_CONSTANT(int, fineness = 0); 

public:

    //==========================================================================
    //= Construct, copy, destruct
    //==========================================================================
    /** Default constructor for the empty object */
    interval_base_map()
    {
        BOOST_CONCEPT_ASSERT((DefaultConstructibleConcept<DomainT>));
        BOOST_CONCEPT_ASSERT((LessThanComparableConcept<DomainT>));
        BOOST_CONCEPT_ASSERT((DefaultConstructibleConcept<CodomainT>));
        BOOST_CONCEPT_ASSERT((EqualComparableConcept<CodomainT>));
    }

    /** Copy constructor */
    interval_base_map(const interval_base_map& src): _map(src._map)
    {
        BOOST_CONCEPT_ASSERT((DefaultConstructibleConcept<DomainT>));
        BOOST_CONCEPT_ASSERT((LessThanComparableConcept<DomainT>));
        BOOST_CONCEPT_ASSERT((DefaultConstructibleConcept<CodomainT>));
        BOOST_CONCEPT_ASSERT((EqualComparableConcept<CodomainT>));
    }

#   ifndef BOOST_ICL_NO_CXX11_RVALUE_REFERENCES
    //==========================================================================
    //= Move semantics
    //==========================================================================

    /** Move constructor */
    interval_base_map(interval_base_map&& src): _map(boost::move(src._map))
    {
        BOOST_CONCEPT_ASSERT((DefaultConstructibleConcept<DomainT>));
        BOOST_CONCEPT_ASSERT((LessThanComparableConcept<DomainT>));
        BOOST_CONCEPT_ASSERT((DefaultConstructibleConcept<CodomainT>));
        BOOST_CONCEPT_ASSERT((EqualComparableConcept<CodomainT>));
    }

    /** Move assignment operator */
    interval_base_map& operator = (interval_base_map src) 
    {                           //call by value sice 'src' is a "sink value" 
        this->_map = boost::move(src._map);
        return *this; 
    }

    //==========================================================================
#   else 

    /** Copy assignment operator */
    interval_base_map& operator = (const interval_base_map& src) 
    { 
        this->_map = src._map;
        return *this; 
    }

#   endif // BOOST_ICL_NO_CXX11_RVALUE_REFERENCES

    /** swap the content of containers */
    void swap(interval_base_map& object) { _map.swap(object._map); }

    //==========================================================================
    //= Containedness
    //==========================================================================
    /** clear the map */
    void clear() { icl::clear(*that()); }

    /** is the map empty? */
    bool empty()const { return icl::is_empty(*that()); }

    //==========================================================================
    //= Size
    //==========================================================================
    /** An interval map's size is it's cardinality */
    size_type size()const
    {
        return icl::cardinality(*that());
    }

    /** Size of the iteration over this container */
    std::size_t iterative_size()const 
    { 
        return _map.size(); 
    }

    //==========================================================================
    //= Selection
    //==========================================================================

    /** Find the interval value pair, that contains \c key */
    const_iterator find(const domain_type& key_value)const
    { 
        return icl::find(*this, key_value);
    }

    /** Find the first interval value pair, that collides with interval 
        \c key_interval */
    const_iterator find(const interval_type& key_interval)const
    { 
        return _map.find(key_interval); 
    }

    /** Total select function. */
    codomain_type operator()(const domain_type& key_value)const
    {
        const_iterator it_ = icl::find(*this, key_value);
        return it_==end() ? identity_element<codomain_type>::value()
                          : (*it_).second;
    }

    //==========================================================================
    //= Addition
    //==========================================================================

    /** Addition of a key value pair to the map */
    SubType& add(const element_type& key_value_pair) 
    {
        return icl::add(*that(), key_value_pair);
    }

    /** Addition of an interval value pair to the map. */
    SubType& add(const segment_type& interval_value_pair) 
    {
        this->template _add<codomain_combine>(interval_value_pair);
        return *that();
    }

    /** Addition of an interval value pair \c interval_value_pair to the map. 
        Iterator \c prior_ is a hint to the position \c interval_value_pair can be 
        inserted after. */
    iterator add(iterator prior_, const segment_type& interval_value_pair) 
    {
        return this->template _add<codomain_combine>(prior_, interval_value_pair);
    }

    //==========================================================================
    //= Subtraction
    //==========================================================================
    /** Subtraction of a key value pair from the map */
    SubType& subtract(const element_type& key_value_pair)
    { 
        return icl::subtract(*that(), key_value_pair);
    }

    /** Subtraction of an interval value pair from the map. */
    SubType& subtract(const segment_type& interval_value_pair)
    {
        on_invertible<type, is_total_invertible>
            ::subtract(*that(), interval_value_pair);
        return *that();
    }

    //==========================================================================
    //= Insertion
    //==========================================================================
    /** Insertion of a \c key_value_pair into the map. */
    SubType& insert(const element_type& key_value_pair) 
    {
        return icl::insert(*that(), key_value_pair);
    }

    /** Insertion of an \c interval_value_pair into the map. */
    SubType& insert(const segment_type& interval_value_pair)
    { 
        _insert(interval_value_pair); 
        return *that();
    }

    /** Insertion of an \c interval_value_pair into the map. Iterator \c prior_. 
        serves as a hint to insert after the element \c prior point to. */
    iterator insert(iterator prior, const segment_type& interval_value_pair)
    { 
        return _insert(prior, interval_value_pair);
    }

    /** With <tt>key_value_pair = (k,v)</tt> set value \c v for key \c k */
    SubType& set(const element_type& key_value_pair) 
    { 
        return icl::set_at(*that(), key_value_pair);
    }

    /** With <tt>interval_value_pair = (I,v)</tt> set value \c v 
        for all keys in interval \c I in the map. */
    SubType& set(const segment_type& interval_value_pair)
    { 
        return icl::set_at(*that(), interval_value_pair);
    }

    //==========================================================================
    //= Erasure
    //==========================================================================
    /** Erase a \c key_value_pair from the map. */
    SubType& erase(const element_type& key_value_pair) 
    { 
        icl::erase(*that(), key_value_pair);
        return *that();
    }

    /** Erase an \c interval_value_pair from the map. */
    SubType& erase(const segment_type& interval_value_pair);

    /** Erase a key value pair for \c key. */
    SubType& erase(const domain_type& key) 
    { 
        return icl::erase(*that(), key); 
    }

    /** Erase all value pairs within the range of the 
        interval <tt>inter_val</tt> from the map.   */
    SubType& erase(const interval_type& inter_val);


    /** Erase all value pairs within the range of the interval that iterator 
        \c position points to. */
    void erase(iterator position){ this->_map.erase(position); }

    /** Erase all value pairs for a range of iterators <tt>[first,past)</tt>. */
    void erase(iterator first, iterator past){ this->_map.erase(first, past); }

    //==========================================================================
    //= Intersection
    //==========================================================================
    /** The intersection of \c interval_value_pair and \c *this map is added to \c section. */
    void add_intersection(SubType& section, const segment_type& interval_value_pair)const
    {
        on_definedness<SubType, Traits::is_total>
            ::add_intersection(section, *that(), interval_value_pair);
    }

    //==========================================================================
    //= Symmetric difference
    //==========================================================================
    /** If \c *this map contains \c key_value_pair it is erased, otherwise it is added. */
    SubType& flip(const element_type& key_value_pair)
    { 
        return icl::flip(*that(), key_value_pair); 
    }

    /** If \c *this map contains \c interval_value_pair it is erased, otherwise it is added. */
    SubType& flip(const segment_type& interval_value_pair)
    {
        on_total_absorbable<SubType, Traits::is_total, Traits::absorbs_identities>
            ::flip(*that(), interval_value_pair);
        return *that();
    }

    //==========================================================================
    //= Iterator related
    //==========================================================================

    iterator lower_bound(const key_type& interval)
    { return _map.lower_bound(interval); }

    iterator upper_bound(const key_type& interval)
    { return _map.upper_bound(interval); }

    const_iterator lower_bound(const key_type& interval)const
    { return _map.lower_bound(interval); }

    const_iterator upper_bound(const key_type& interval)const
    { return _map.upper_bound(interval); }

    std::pair<iterator,iterator> equal_range(const key_type& interval)
    { 
        return std::pair<iterator,iterator>
               (lower_bound(interval), upper_bound(interval)); 
    }

    std::pair<const_iterator,const_iterator> 
        equal_range(const key_type& interval)const
    { 
        return std::pair<const_iterator,const_iterator>
               (lower_bound(interval), upper_bound(interval)); 
    }

    iterator begin() { return _map.begin(); }
    iterator end()   { return _map.end(); }
    const_iterator begin()const { return _map.begin(); }
    const_iterator end()const   { return _map.end(); }
    reverse_iterator rbegin() { return _map.rbegin(); }
    reverse_iterator rend()   { return _map.rend(); }
    const_reverse_iterator rbegin()const { return _map.rbegin(); }
    const_reverse_iterator rend()const   { return _map.rend(); }

private:
    template<class Combiner>
    iterator _add(const segment_type& interval_value_pair);

    template<class Combiner>
    iterator _add(iterator prior_, const segment_type& interval_value_pair);

    template<class Combiner>
    void _subtract(const segment_type& interval_value_pair);

    iterator _insert(const segment_type& interval_value_pair);
    iterator _insert(iterator prior_, const segment_type& interval_value_pair);

private:
    template<class Combiner>
    void add_segment(const interval_type& inter_val, const CodomainT& co_val, iterator& it_);

    template<class Combiner>
    void add_main(interval_type& inter_val, const CodomainT& co_val, 
                  iterator& it_, const iterator& last_);

    template<class Combiner>
    void add_rear(const interval_type& inter_val, const CodomainT& co_val, iterator& it_);

    void add_front(const interval_type& inter_val, iterator& first_);

private:
    void subtract_front(const interval_type& inter_val, iterator& first_);

    template<class Combiner>
    void subtract_main(const CodomainT& co_val, iterator& it_, const iterator& last_);

    template<class Combiner>
    void subtract_rear(interval_type& inter_val, const CodomainT& co_val, iterator& it_);

private:
    void insert_main(const interval_type&, const CodomainT&, iterator&, const iterator&);
    void erase_rest (      interval_type&, const CodomainT&, iterator&, const iterator&);

    template<class FragmentT>
    void total_add_intersection(SubType& section, const FragmentT& fragment)const
    {
        section += *that();
        section.add(fragment);
    }

    void partial_add_intersection(SubType& section, const segment_type& operand)const
    {
        interval_type inter_val = operand.first;
        if(icl::is_empty(inter_val)) 
            return;

        std::pair<const_iterator, const_iterator> exterior = equal_range(inter_val);
        if(exterior.first == exterior.second)
            return;

        for(const_iterator it_=exterior.first; it_ != exterior.second; it_++) 
        {
            interval_type common_interval = (*it_).first & inter_val; 
            if(!icl::is_empty(common_interval))
            {
                section.template _add<codomain_combine>  (value_type(common_interval, (*it_).second) );
                section.template _add<codomain_intersect>(value_type(common_interval, operand.second));
            }
        }
    }

    void partial_add_intersection(SubType& section, const element_type& operand)const
    {
        partial_add_intersection(section, make_segment<type>(operand));
    }


protected:

    template <class Combiner>
    iterator gap_insert(iterator prior_, const interval_type& inter_val, 
                                         const codomain_type& co_val   )
    {
        // inter_val is not conained in this map. Insertion will be successful
        BOOST_ASSERT(this->_map.find(inter_val) == this->_map.end());
        BOOST_ASSERT((!on_absorbtion<type,Combiner,Traits::absorbs_identities>::is_absorbable(co_val)));
        return this->_map.insert(prior_, value_type(inter_val, version<Combiner>()(co_val)));
    }

    template <class Combiner>
    std::pair<iterator, bool>
    add_at(const iterator& prior_, const interval_type& inter_val, 
                                   const codomain_type& co_val   )
    {
        // Never try to insert an identity element into an identity element absorber here:
        BOOST_ASSERT((!(on_absorbtion<type,Combiner,Traits::absorbs_identities>::is_absorbable(co_val))));

        iterator inserted_ 
            = this->_map.insert(prior_, value_type(inter_val, Combiner::identity_element()));

        if((*inserted_).first == inter_val && (*inserted_).second == Combiner::identity_element())
        {
            Combiner()((*inserted_).second, co_val);
            return std::pair<iterator,bool>(inserted_, true);
        }
        else
            return std::pair<iterator,bool>(inserted_, false);
    }

    std::pair<iterator, bool>
    insert_at(const iterator& prior_, const interval_type& inter_val, 
                                      const codomain_type& co_val   )
    {
        iterator inserted_
            = this->_map.insert(prior_, value_type(inter_val, co_val));

        if(inserted_ == prior_)
            return std::pair<iterator,bool>(inserted_, false);
        else if((*inserted_).first == inter_val)
            return std::pair<iterator,bool>(inserted_, true);
        else
            return std::pair<iterator,bool>(inserted_, false);
    }


protected:
    sub_type* that() { return static_cast<sub_type*>(this); }
    const sub_type* that()const { return static_cast<const sub_type*>(this); }

protected:
    ImplMapT _map;


private:
    //--------------------------------------------------------------------------
    template<class Type, bool is_total_invertible>
    struct on_invertible;

    template<class Type>
    struct on_invertible<Type, true>
    {
        typedef typename Type::segment_type segment_type;
        typedef typename Type::inverse_codomain_combine inverse_codomain_combine;

        static void subtract(Type& object, const segment_type& operand)
        { object.template _add<inverse_codomain_combine>(operand); }
    };

    template<class Type>
    struct on_invertible<Type, false>
    {
        typedef typename Type::segment_type segment_type;
        typedef typename Type::inverse_codomain_combine inverse_codomain_combine;

        static void subtract(Type& object, const segment_type& operand)
        { object.template _subtract<inverse_codomain_combine>(operand); }
    };

    friend struct on_invertible<type, true>;
    friend struct on_invertible<type, false>;
    //--------------------------------------------------------------------------

    //--------------------------------------------------------------------------
    template<class Type, bool is_total>
    struct on_definedness;

    template<class Type>
    struct on_definedness<Type, true>
    {
        static void add_intersection(Type& section, const Type& object, 
                                     const segment_type& operand)
        { object.total_add_intersection(section, operand); }
    };

    template<class Type>
    struct on_definedness<Type, false>
    {
        static void add_intersection(Type& section, const Type& object, 
                                     const segment_type& operand)
        { object.partial_add_intersection(section, operand); }
    };

    friend struct on_definedness<type, true>;
    friend struct on_definedness<type, false>;
    //--------------------------------------------------------------------------

    //--------------------------------------------------------------------------
    template<class Type, bool has_set_semantics> 
    struct on_codomain_model;

    template<class Type>
    struct on_codomain_model<Type, true>
    {
        typedef typename Type::interval_type interval_type;
        typedef typename Type::codomain_type codomain_type;
        typedef typename Type::segment_type  segment_type;
        typedef typename Type::codomain_combine codomain_combine;
        typedef typename Type::inverse_codomain_intersect inverse_codomain_intersect;

        static void add(Type& intersection, interval_type& common_interval, 
                        const codomain_type& flip_value, const codomain_type& co_value)
        {
            codomain_type common_value = flip_value;
            inverse_codomain_intersect()(common_value, co_value);
            intersection.template 
                _add<codomain_combine>(segment_type(common_interval, common_value));
        }
    };

    template<class Type>
    struct on_codomain_model<Type, false>
    {
        typedef typename Type::interval_type interval_type;
        typedef typename Type::codomain_type codomain_type;
        typedef typename Type::segment_type  segment_type;
        typedef typename Type::codomain_combine codomain_combine;

        static void add(Type& intersection, interval_type& common_interval, 
                        const codomain_type&, const codomain_type&)
        {
            intersection.template 
              _add<codomain_combine>(segment_type(common_interval, 
                                                  identity_element<codomain_type>::value()));
        }
    };

    friend struct on_codomain_model<type, true>;
    friend struct on_codomain_model<type, false>;
    //--------------------------------------------------------------------------


    //--------------------------------------------------------------------------
    template<class Type, bool is_total, bool absorbs_identities>
    struct on_total_absorbable;

    template<class Type>
    struct on_total_absorbable<Type, true, true>
    {
        static void flip(Type& object, const typename Type::segment_type&)
        { icl::clear(object); }
    };

#ifdef BOOST_MSVC 
#pragma warning(push)
#pragma warning(disable:4127) // conditional expression is constant
#endif                        

    template<class Type>
    struct on_total_absorbable<Type, true, false>
    {
        typedef typename Type::segment_type  segment_type;
        typedef typename Type::codomain_type codomain_type;

        static void flip(Type& object, const segment_type& operand)
        { 
            object += operand;
            ICL_FORALL(typename Type, it_, object)
                (*it_).second = identity_element<codomain_type>::value();

            if(mpl::not_<is_interval_splitter<Type> >::value)
                icl::join(object);
        }
    };

#ifdef BOOST_MSVC
#pragma warning(pop)
#endif

    template<class Type, bool absorbs_identities>
    struct on_total_absorbable<Type, false, absorbs_identities>
    {
        typedef typename Type::segment_type   segment_type;
        typedef typename Type::codomain_type  codomain_type;
        typedef typename Type::interval_type  interval_type;
        typedef typename Type::value_type     value_type;
        typedef typename Type::const_iterator const_iterator;
        typedef typename Type::set_type       set_type;
        typedef typename Type::inverse_codomain_intersect inverse_codomain_intersect;

        static void flip(Type& object, const segment_type& interval_value_pair)
        {
            // That which is common shall be subtracted
            // That which is not shall be added
            // So interval_value_pair has to be 'complementary added' or flipped
            interval_type span = interval_value_pair.first;
            std::pair<const_iterator, const_iterator> exterior 
                = object.equal_range(span);

            const_iterator first_ = exterior.first;
            const_iterator end_   = exterior.second;

            interval_type covered, left_over, common_interval;
            const codomain_type& x_value = interval_value_pair.second;
            const_iterator it_ = first_;

            set_type eraser;
            Type     intersection;

            while(it_ != end_  ) 
            {
                const codomain_type& co_value = (*it_).second;
                covered = (*it_++).first;
                //[a      ...  : span
                //     [b ...  : covered
                //[a  b)       : left_over
                left_over = right_subtract(span, covered);

                //That which is common ...
                common_interval = span & covered;
                if(!icl::is_empty(common_interval))
                {
                    // ... shall be subtracted
                    icl::add(eraser, common_interval);

                    on_codomain_model<Type, has_set_semantics<codomain_type>::value>
                        ::add(intersection, common_interval, x_value, co_value);
                }

                icl::add(object, value_type(left_over, x_value)); //That which is not shall be added
                // Because this is a collision free addition I don't have to distinguish codomain_types.

                //...      d) : span
                //... c)      : covered
                //     [c  d) : span'
                span = left_subtract(span, covered);
            }

            //If span is not empty here, it is not in the set so it shall be added
            icl::add(object, value_type(span, x_value));

            //finally rewrite the common segments
            icl::erase(object, eraser);
            object += intersection;
        }
    };
    //--------------------------------------------------------------------------
} ;


//==============================================================================
//= Addition detail
//==============================================================================
template <class SubType, class DomainT, class CodomainT, class Traits, ICL_COMPARE Compare, ICL_COMBINE Combine, ICL_SECTION Section, ICL_INTERVAL(ICL_COMPARE) Interval, ICL_ALLOC Alloc>
inline void interval_base_map<SubType,DomainT,CodomainT,Traits,Compare,Combine,Section,Interval,Alloc>
    ::add_front(const interval_type& inter_val, iterator& first_)
{
    // If the collision sequence has a left residual 'left_resid' it will
    // be split, to provide a standardized start of algorithms:
    // The addend interval 'inter_val' covers the beginning of the collision sequence.

    // only for the first there can be a left_resid: a part of *first_ left of inter_val
    interval_type left_resid = right_subtract((*first_).first, inter_val);

    if(!icl::is_empty(left_resid))
    {   //            [------------ . . .
        // [left_resid---first_ --- . . .
        iterator prior_ = cyclic_prior(*this, first_);
        const_cast<interval_type&>((*first_).first) 
            = left_subtract((*first_).first, left_resid);
        //NOTE: Only splitting
        this->_map.insert(prior_, segment_type(left_resid, (*first_).second));
    }
    //POST:
    // [----- inter_val ---- . . .
    // ...[-- first_ --...
}

template <class SubType, class DomainT, class CodomainT, class Traits, ICL_COMPARE Compare, ICL_COMBINE Combine, ICL_SECTION Section, ICL_INTERVAL(ICL_COMPARE) Interval, ICL_ALLOC Alloc>
    template<class Combiner>
inline void interval_base_map<SubType,DomainT,CodomainT,Traits,Compare,Combine,Section,Interval,Alloc>
    ::add_segment(const interval_type& inter_val, const CodomainT& co_val, iterator& it_)
{
    interval_type lead_gap = right_subtract(inter_val, (*it_).first);
    if(!icl::is_empty(lead_gap))
    {
        // [lead_gap--- . . .
        //          [-- it_ ...
        iterator prior_ = it_==this->_map.begin()? it_ : prior(it_); 
        iterator inserted_ = this->template gap_insert<Combiner>(prior_, lead_gap, co_val);
        that()->handle_inserted(prior_, inserted_);
    }

    // . . . --------- . . . addend interval
    //      [-- it_ --)      has a common part with the first overval
    Combiner()((*it_).second, co_val);
    that()->template handle_left_combined<Combiner>(it_++);
}


template <class SubType, class DomainT, class CodomainT, class Traits, ICL_COMPARE Compare, ICL_COMBINE Combine, ICL_SECTION Section, ICL_INTERVAL(ICL_COMPARE) Interval, ICL_ALLOC Alloc>
    template<class Combiner>
inline void interval_base_map<SubType,DomainT,CodomainT,Traits,Compare,Combine,Section,Interval,Alloc>
    ::add_main(interval_type& inter_val, const CodomainT& co_val, 
               iterator& it_, const iterator& last_)
{
    interval_type cur_interval;
    while(it_!=last_)
    {
        cur_interval = (*it_).first ;
        add_segment<Combiner>(inter_val, co_val, it_);
        // shrink interval
        inter_val = left_subtract(inter_val, cur_interval);
    }
}

template <class SubType, class DomainT, class CodomainT, class Traits, ICL_COMPARE Compare, ICL_COMBINE Combine, ICL_SECTION Section, ICL_INTERVAL(ICL_COMPARE) Interval, ICL_ALLOC Alloc>
    template<class Combiner>
inline void interval_base_map<SubType,DomainT,CodomainT,Traits,Compare,Combine,Section,Interval,Alloc>
    ::add_rear(const interval_type& inter_val, const CodomainT& co_val, iterator& it_)
{
    iterator prior_ = cyclic_prior(*that(), it_);
    interval_type cur_itv = (*it_).first ;

    interval_type lead_gap = right_subtract(inter_val, cur_itv);
    if(!icl::is_empty(lead_gap))
    {   //         [lead_gap--- . . .
        // [prior)          [-- it_ ...
        iterator inserted_ = this->template gap_insert<Combiner>(prior_, lead_gap, co_val);
        that()->handle_inserted(prior_, inserted_);
    }

    interval_type end_gap = left_subtract(inter_val, cur_itv);
    if(!icl::is_empty(end_gap))
    {
        // [----------------end_gap)
        //  . . . -- it_ --)
        Combiner()((*it_).second, co_val);
        that()->template gap_insert_at<Combiner>(it_, prior_, end_gap, co_val);
    }
    else
    {
        // only for the last there can be a right_resid: a part of *it_ right of x
        interval_type right_resid = left_subtract(cur_itv, inter_val);

        if(icl::is_empty(right_resid))
        {
            // [---------------)
            //      [-- it_ ---)
            Combiner()((*it_).second, co_val);
            that()->template handle_preceeded_combined<Combiner>(prior_, it_);
        }
        else
        {
            // [--------------)
            //      [-- it_ --right_resid)
            const_cast<interval_type&>((*it_).first) = right_subtract((*it_).first, right_resid);

            //NOTE: This is NOT an insertion that has to take care for correct application of
            // the Combiner functor. It only reestablished that state after splitting the
            // 'it_' interval value pair. Using _map_insert<Combiner> does not work here.
            iterator insertion_ = this->_map.insert(it_, value_type(right_resid, (*it_).second));
            that()->handle_reinserted(insertion_);

            Combiner()((*it_).second, co_val);
            that()->template handle_preceeded_combined<Combiner>(insertion_, it_);
        }
    }
}


//==============================================================================
//= Addition
//==============================================================================
template <class SubType, class DomainT, class CodomainT, class Traits, ICL_COMPARE Compare, ICL_COMBINE Combine, ICL_SECTION Section, ICL_INTERVAL(ICL_COMPARE) Interval, ICL_ALLOC Alloc>
    template<class Combiner>
inline typename interval_base_map<SubType,DomainT,CodomainT,Traits,Compare,Combine,Section,Interval,Alloc>::iterator
    interval_base_map<SubType,DomainT,CodomainT,Traits,Compare,Combine,Section,Interval,Alloc>
    ::_add(const segment_type& addend)
{
    typedef typename on_absorbtion<type,Combiner,
                                absorbs_identities<type>::value>::type on_absorbtion_;

    const interval_type& inter_val = addend.first;
    if(icl::is_empty(inter_val)) 
        return this->_map.end();

    const codomain_type& co_val = addend.second;
    if(on_absorbtion_::is_absorbable(co_val))
        return this->_map.end();

    std::pair<iterator,bool> insertion 
        = this->_map.insert(value_type(inter_val, version<Combiner>()(co_val)));

    if(insertion.second)
        return that()->handle_inserted(insertion.first);
    else
    {
        // Detect the first and the end iterator of the collision sequence
        iterator first_ = this->_map.lower_bound(inter_val),
                 last_  = prior(this->_map.upper_bound(inter_val));
        //assert(end_ == this->_map.upper_bound(inter_val));
        iterator it_ = first_;
        interval_type rest_interval = inter_val;

        add_front         (rest_interval,         it_       );
        add_main<Combiner>(rest_interval, co_val, it_, last_);
        add_rear<Combiner>(rest_interval, co_val, it_       );
        return it_;
    }
}

template <class SubType, class DomainT, class CodomainT, class Traits, ICL_COMPARE Compare, ICL_COMBINE Combine, ICL_SECTION Section, ICL_INTERVAL(ICL_COMPARE) Interval, ICL_ALLOC Alloc>
    template<class Combiner>
inline typename interval_base_map<SubType,DomainT,CodomainT,Traits,Compare,Combine,Section,Interval,Alloc>::iterator
    interval_base_map<SubType,DomainT,CodomainT,Traits,Compare,Combine,Section,Interval,Alloc>
    ::_add(iterator prior_, const segment_type& addend)
{
    typedef typename on_absorbtion<type,Combiner,
                                absorbs_identities<type>::value>::type on_absorbtion_;

    const interval_type& inter_val = addend.first;
    if(icl::is_empty(inter_val)) 
        return prior_;

    const codomain_type& co_val = addend.second;
    if(on_absorbtion_::is_absorbable(co_val))
        return prior_;

    std::pair<iterator,bool> insertion 
        = add_at<Combiner>(prior_, inter_val, co_val);

    if(insertion.second)
        return that()->handle_inserted(insertion.first);
    else
    {
        // Detect the first and the end iterator of the collision sequence
        std::pair<iterator,iterator> overlap = equal_range(inter_val);
        iterator it_   = overlap.first,
                 last_ = prior(overlap.second);
        interval_type rest_interval = inter_val;

        add_front         (rest_interval,         it_       );
        add_main<Combiner>(rest_interval, co_val, it_, last_);
        add_rear<Combiner>(rest_interval, co_val, it_       );
        return it_;
    }
}

//==============================================================================
//= Subtraction detail
//==============================================================================

template <class SubType, class DomainT, class CodomainT, class Traits, ICL_COMPARE Compare, ICL_COMBINE Combine, ICL_SECTION Section, ICL_INTERVAL(ICL_COMPARE) Interval, ICL_ALLOC Alloc>
inline void interval_base_map<SubType,DomainT,CodomainT,Traits,Compare,Combine,Section,Interval,Alloc>
    ::subtract_front(const interval_type& inter_val, iterator& it_)
{
    interval_type left_resid = right_subtract((*it_).first, inter_val);

    if(!icl::is_empty(left_resid)) //                     [--- inter_val ---)
    {                              //[prior_) [left_resid)[--- it_ . . .
        iterator prior_ = cyclic_prior(*this, it_); 
        const_cast<interval_type&>((*it_).first) = left_subtract((*it_).first, left_resid);
        this->_map.insert(prior_, value_type(left_resid, (*it_).second));
        // The segemnt *it_ is split at inter_val.first(), so as an invariant
        // segment *it_ is always "under" inter_val and a left_resid is empty.
    }
}


template <class SubType, class DomainT, class CodomainT, class Traits, ICL_COMPARE Compare, ICL_COMBINE Combine, ICL_SECTION Section, ICL_INTERVAL(ICL_COMPARE) Interval, ICL_ALLOC Alloc>
    template<class Combiner>
inline void interval_base_map<SubType,DomainT,CodomainT,Traits,Compare,Combine,Section,Interval,Alloc>
    ::subtract_main(const CodomainT& co_val, iterator& it_, const iterator& last_)
{
    while(it_ != last_)
    {
        Combiner()((*it_).second, co_val);
        that()->template handle_left_combined<Combiner>(it_++);
    }
}

template <class SubType, class DomainT, class CodomainT, class Traits, ICL_COMPARE Compare, ICL_COMBINE Combine, ICL_SECTION Section, ICL_INTERVAL(ICL_COMPARE) Interval, ICL_ALLOC Alloc>
    template<class Combiner>
inline void interval_base_map<SubType,DomainT,CodomainT,Traits,Compare,Combine,Section,Interval,Alloc>
    ::subtract_rear(interval_type& inter_val, const CodomainT& co_val, iterator& it_)
{
    interval_type right_resid = left_subtract((*it_).first, inter_val);

    if(icl::is_empty(right_resid))
    {
        Combiner()((*it_).second, co_val);
        that()->template handle_combined<Combiner>(it_);
    }
    else
    {
        const_cast<interval_type&>((*it_).first) = right_subtract((*it_).first, right_resid);
        iterator next_ = this->_map.insert(it_, value_type(right_resid, (*it_).second));
        Combiner()((*it_).second, co_val);
        that()->template handle_succeeded_combined<Combiner>(it_, next_);
    }
}

//==============================================================================
//= Subtraction
//==============================================================================
template <class SubType, class DomainT, class CodomainT, class Traits, ICL_COMPARE Compare, ICL_COMBINE Combine, ICL_SECTION Section, ICL_INTERVAL(ICL_COMPARE) Interval, ICL_ALLOC Alloc>
    template<class Combiner>
inline void interval_base_map<SubType,DomainT,CodomainT,Traits,Compare,Combine,Section,Interval,Alloc>
    ::_subtract(const segment_type& minuend)
{
    interval_type inter_val = minuend.first;
    if(icl::is_empty(inter_val)) 
        return;

    const codomain_type& co_val = minuend.second;
    if(on_absorbtion<type,Combiner,Traits::absorbs_identities>::is_absorbable(co_val)) 
        return;

    std::pair<iterator, iterator> exterior = equal_range(inter_val);
    if(exterior.first == exterior.second)
        return;

    iterator last_  = prior(exterior.second);
    iterator it_    = exterior.first;
    subtract_front          (inter_val,         it_       );
    subtract_main <Combiner>(           co_val, it_, last_);
    subtract_rear <Combiner>(inter_val, co_val, it_       );
}

//==============================================================================
//= Insertion
//==============================================================================
template <class SubType, class DomainT, class CodomainT, class Traits, ICL_COMPARE Compare, ICL_COMBINE Combine, ICL_SECTION Section, ICL_INTERVAL(ICL_COMPARE) Interval, ICL_ALLOC Alloc>
inline void interval_base_map<SubType,DomainT,CodomainT,Traits,Compare,Combine,Section,Interval,Alloc>
    ::insert_main(const interval_type& inter_val, const CodomainT& co_val, 
                  iterator& it_, const iterator& last_)
{
    iterator end_   = boost::next(last_);
    iterator prior_ = cyclic_prior(*this,it_), inserted_;
    interval_type rest_interval = inter_val, left_gap, cur_itv;
    interval_type last_interval = last_ ->first;

    while(it_ != end_  )
    {
        cur_itv = (*it_).first ;            
        left_gap = right_subtract(rest_interval, cur_itv);

        if(!icl::is_empty(left_gap))
        {
            inserted_ = this->_map.insert(prior_, value_type(left_gap, co_val));
            it_ = that()->handle_inserted(inserted_);
        }

        // shrink interval
        rest_interval = left_subtract(rest_interval, cur_itv);
        prior_ = it_;
        ++it_;
    }

    //insert_rear(rest_interval, co_val, last_):
    interval_type end_gap = left_subtract(rest_interval, last_interval);
    if(!icl::is_empty(end_gap))
    {
        inserted_ = this->_map.insert(prior_, value_type(end_gap, co_val));
        it_ = that()->handle_inserted(inserted_);
    }
    else
        it_ = prior_;
}


template <class SubType, class DomainT, class CodomainT, class Traits, ICL_COMPARE Compare, ICL_COMBINE Combine, ICL_SECTION Section, ICL_INTERVAL(ICL_COMPARE) Interval, ICL_ALLOC Alloc>
inline typename interval_base_map<SubType,DomainT,CodomainT,Traits,Compare,Combine,Section,Interval,Alloc>::iterator
    interval_base_map<SubType,DomainT,CodomainT,Traits,Compare,Combine,Section,Interval,Alloc>
    ::_insert(const segment_type& addend)
{
    interval_type inter_val = addend.first;
    if(icl::is_empty(inter_val)) 
        return this->_map.end();

    const codomain_type& co_val = addend.second;
    if(on_codomain_absorbtion::is_absorbable(co_val)) 
        return this->_map.end();

    std::pair<iterator,bool> insertion = this->_map.insert(addend);

    if(insertion.second)
        return that()->handle_inserted(insertion.first);
    else
    {
        // Detect the first and the end iterator of the collision sequence
        iterator first_ = this->_map.lower_bound(inter_val),
                 last_  = prior(this->_map.upper_bound(inter_val));
        //assert((++last_) == this->_map.upper_bound(inter_val));
        iterator it_ = first_;
        insert_main(inter_val, co_val, it_, last_);
        return it_;
    }
}


template <class SubType, class DomainT, class CodomainT, class Traits, ICL_COMPARE Compare, ICL_COMBINE Combine, ICL_SECTION Section, ICL_INTERVAL(ICL_COMPARE) Interval, ICL_ALLOC Alloc>
inline typename interval_base_map<SubType,DomainT,CodomainT,Traits,Compare,Combine,Section,Interval,Alloc>::iterator
    interval_base_map<SubType,DomainT,CodomainT,Traits,Compare,Combine,Section,Interval,Alloc>
    ::_insert(iterator prior_, const segment_type& addend)
{
    interval_type inter_val = addend.first;
    if(icl::is_empty(inter_val)) 
        return prior_;

    const codomain_type& co_val = addend.second;
    if(on_codomain_absorbtion::is_absorbable(co_val)) 
        return prior_;

    std::pair<iterator,bool> insertion = insert_at(prior_, inter_val, co_val);

    if(insertion.second)
        return that()->handle_inserted(insertion.first);
    {
        // Detect the first and the end iterator of the collision sequence
        std::pair<iterator,iterator> overlap = equal_range(inter_val);
        iterator it_    = overlap.first,
                 last_  = prior(overlap.second);
        insert_main(inter_val, co_val, it_, last_);
        return it_;
    }
}

//==============================================================================
//= Erasure segment_type
//==============================================================================
template <class SubType, class DomainT, class CodomainT, class Traits, ICL_COMPARE Compare, ICL_COMBINE Combine, ICL_SECTION Section, ICL_INTERVAL(ICL_COMPARE) Interval, ICL_ALLOC Alloc>
inline void interval_base_map<SubType,DomainT,CodomainT,Traits,Compare,Combine,Section,Interval,Alloc>
    ::erase_rest(interval_type& inter_val, const CodomainT& co_val, 
                 iterator& it_, const iterator& last_)
{
    // For all intervals within loop: (*it_).first are contained_in inter_val
    while(it_ != last_)
        if((*it_).second == co_val)
            this->_map.erase(it_++); 
        else it_++;

    //erase_rear:
    if((*it_).second == co_val)
    {
        interval_type right_resid = left_subtract((*it_).first, inter_val);
        if(icl::is_empty(right_resid))
            this->_map.erase(it_);
        else
            const_cast<interval_type&>((*it_).first) = right_resid;
    }
}

template <class SubType, class DomainT, class CodomainT, class Traits, ICL_COMPARE Compare, ICL_COMBINE Combine, ICL_SECTION Section, ICL_INTERVAL(ICL_COMPARE) Interval, ICL_ALLOC Alloc>
inline SubType& interval_base_map<SubType,DomainT,CodomainT,Traits,Compare,Combine,Section,Interval,Alloc>
    ::erase(const segment_type& minuend)
{
    interval_type inter_val = minuend.first;
    if(icl::is_empty(inter_val)) 
        return *that();

    const codomain_type& co_val = minuend.second;
    if(on_codomain_absorbtion::is_absorbable(co_val))
        return *that();

    std::pair<iterator,iterator> exterior = equal_range(inter_val);
    if(exterior.first == exterior.second)
        return *that();

    iterator first_ = exterior.first, end_ = exterior.second, 
             last_  = cyclic_prior(*this, end_);
    iterator second_= first_; ++second_;

    if(first_ == last_) 
    {   //     [----inter_val----)
        //   .....first_==last_.....
        // only for the last there can be a right_resid: a part of *it_ right of minuend
        interval_type right_resid = left_subtract((*first_).first, inter_val);

        if((*first_).second == co_val)
        {   
            interval_type left_resid = right_subtract((*first_).first, inter_val);
            if(!icl::is_empty(left_resid)) //            [----inter_val----)
            {                              // [left_resid)..first_==last_......
                const_cast<interval_type&>((*first_).first) = left_resid;
                if(!icl::is_empty(right_resid))
                    this->_map.insert(first_, value_type(right_resid, co_val));
            }
            else if(!icl::is_empty(right_resid))
                const_cast<interval_type&>((*first_).first) = right_resid;
            else
                this->_map.erase(first_);
        }
    }
    else
    {
        // first AND NOT last
        if((*first_).second == co_val)
        {
            interval_type left_resid = right_subtract((*first_).first, inter_val);
            if(icl::is_empty(left_resid))
                this->_map.erase(first_);
            else
                const_cast<interval_type&>((*first_).first) = left_resid;
        }

        erase_rest(inter_val, co_val, second_, last_);
    }

     return *that();
}

//==============================================================================
//= Erasure key_type
//==============================================================================
template <class SubType, class DomainT, class CodomainT, class Traits, ICL_COMPARE Compare, ICL_COMBINE Combine, ICL_SECTION Section, ICL_INTERVAL(ICL_COMPARE) Interval, ICL_ALLOC Alloc>
inline SubType& interval_base_map<SubType,DomainT,CodomainT,Traits,Compare,Combine,Section,Interval,Alloc>
    ::erase(const interval_type& minuend)
{
    if(icl::is_empty(minuend)) 
        return *that();

    std::pair<iterator, iterator> exterior = equal_range(minuend);
    if(exterior.first == exterior.second)
        return *that();

    iterator first_ = exterior.first,
             end_   = exterior.second,
             last_  = prior(end_);

    interval_type left_resid  = right_subtract((*first_).first, minuend);
    interval_type right_resid =  left_subtract(last_ ->first, minuend);

    if(first_ == last_ )
        if(!icl::is_empty(left_resid))
        {
            const_cast<interval_type&>((*first_).first) = left_resid;
            if(!icl::is_empty(right_resid))
                this->_map.insert(first_, value_type(right_resid, (*first_).second));
        }
        else if(!icl::is_empty(right_resid))
            const_cast<interval_type&>((*first_).first) = left_subtract((*first_).first, minuend);
        else
            this->_map.erase(first_);
    else
    {   //            [-------- minuend ---------)
        // [left_resid   fst)   . . . .    [lst  right_resid)
        iterator second_= first_; ++second_;

        iterator start_ = icl::is_empty(left_resid)? first_: second_;
        iterator stop_  = icl::is_empty(right_resid)? end_  : last_ ;
        this->_map.erase(start_, stop_); //erase [start_, stop_)

        if(!icl::is_empty(left_resid))
            const_cast<interval_type&>((*first_).first) = left_resid;

        if(!icl::is_empty(right_resid))
            const_cast<interval_type&>(last_ ->first) = right_resid;
    }

    return *that();
}

//-----------------------------------------------------------------------------
// type traits
//-----------------------------------------------------------------------------
template 
<
    class SubType,
    class DomainT, class CodomainT, class Traits, ICL_COMPARE Compare, ICL_COMBINE Combine, ICL_SECTION Section, ICL_INTERVAL(ICL_COMPARE)  Interval, ICL_ALLOC Alloc
>
struct is_map<icl::interval_base_map<SubType,DomainT,CodomainT,Traits,Compare,Combine,Section,Interval,Alloc> >
{ 
    typedef is_map<icl::interval_base_map<SubType,DomainT,CodomainT,Traits,Compare,Combine,Section,Interval,Alloc> > type;
    BOOST_STATIC_CONSTANT(bool, value = true); 
};

template 
<
    class SubType,
    class DomainT, class CodomainT, class Traits, ICL_COMPARE Compare, ICL_COMBINE Combine, ICL_SECTION Section, ICL_INTERVAL(ICL_COMPARE)  Interval, ICL_ALLOC Alloc
>
struct has_inverse<icl::interval_base_map<SubType,DomainT,CodomainT,Traits,Compare,Combine,Section,Interval,Alloc> >
{ 
    typedef has_inverse<icl::interval_base_map<SubType,DomainT,CodomainT,Traits,Compare,Combine,Section,Interval,Alloc> > type;
    BOOST_STATIC_CONSTANT(bool, value = (has_inverse<CodomainT>::value)); 
};

template 
<
    class SubType,
    class DomainT, class CodomainT, class Traits, ICL_COMPARE Compare, ICL_COMBINE Combine, ICL_SECTION Section, ICL_INTERVAL(ICL_COMPARE)  Interval, ICL_ALLOC Alloc
>
struct is_interval_container<icl::interval_base_map<SubType,DomainT,CodomainT,Traits,Compare,Combine,Section,Interval,Alloc> >
{ 
    typedef is_interval_container<icl::interval_base_map<SubType,DomainT,CodomainT,Traits,Compare,Combine,Section,Interval,Alloc> > type;
    BOOST_STATIC_CONSTANT(bool, value = true); 
};

template 
<
    class SubType,
    class DomainT, class CodomainT, class Traits, ICL_COMPARE Compare, ICL_COMBINE Combine, ICL_SECTION Section, ICL_INTERVAL(ICL_COMPARE)  Interval, ICL_ALLOC Alloc
>
struct absorbs_identities<icl::interval_base_map<SubType,DomainT,CodomainT,Traits,Compare,Combine,Section,Interval,Alloc> >
{
    typedef absorbs_identities<icl::interval_base_map<SubType,DomainT,CodomainT,Traits,Compare,Combine,Section,Interval,Alloc> > type;
    BOOST_STATIC_CONSTANT(bool, value = (Traits::absorbs_identities)); 
};

template 
<
    class SubType,
    class DomainT, class CodomainT, class Traits, ICL_COMPARE Compare, ICL_COMBINE Combine, ICL_SECTION Section, ICL_INTERVAL(ICL_COMPARE) Interval, ICL_ALLOC Alloc
>
struct is_total<icl::interval_base_map<SubType,DomainT,CodomainT,Traits,Compare,Combine,Section,Interval,Alloc> >
{
    typedef is_total<icl::interval_base_map<SubType,DomainT,CodomainT,Traits,Compare,Combine,Section,Interval,Alloc> > type;
    BOOST_STATIC_CONSTANT(bool, value = (Traits::is_total)); 
};



}} // namespace icl boost

#endif



// Boost.Bimap
//
// Copyright (c) 2006-2007 Matias Capeletto
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/// \file relation/mutant_relation.hpp
/// \brief Defines the mutant_relation class

#ifndef BOOST_BIMAP_RELATION_MUTANT_RELATION_HPP
#define BOOST_BIMAP_RELATION_MUTANT_RELATION_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/config.hpp>

#include <boost/mpl/vector.hpp>
#include <boost/operators.hpp>
#include <boost/call_traits.hpp>

#ifndef BOOST_BIMAP_DISABLE_SERIALIZATION
  #include <boost/serialization/nvp.hpp>
#endif // BOOST_BIMAP_DISABLE_SERIALIZATION

#include <boost/functional/hash/hash.hpp>

#include <boost/mpl/aux_/na.hpp>

// Boost.Bimap
#include <boost/bimap/tags/tagged.hpp>
#include <boost/bimap/tags/support/default_tagged.hpp>
#include <boost/bimap/tags/support/tag_of.hpp>
#include <boost/bimap/tags/support/value_type_of.hpp>

#include <boost/bimap/relation/member_at.hpp>
#include <boost/bimap/relation/detail/mutant.hpp>
#include <boost/bimap/relation/structured_pair.hpp>
#include <boost/bimap/relation/symmetrical_base.hpp>
#include <boost/bimap/relation/support/get.hpp>

namespace boost {
namespace bimaps {
namespace relation {

namespace detail {

// This class is included so structured_pair and mutant_relation share
// exactly the same class layout

template< class LeftType, class RightType, bool force_mutable >
class relation_storage :
    public symmetrical_base<LeftType,RightType,force_mutable>
{
    typedef symmetrical_base<LeftType,RightType,force_mutable> base_;

    typedef relation_storage storage_;

    public:

    typedef relation_storage<LeftType,RightType,false> non_mutable_storage;

    typedef ::boost::mpl::vector2
    <
        relation_storage< LeftType, RightType, true  >,
        relation_storage< LeftType, RightType, false >

    > mutant_views;

    //@{
        /// data
        BOOST_DEDUCED_TYPENAME base_::left_value_type  left;
        BOOST_DEDUCED_TYPENAME base_::right_value_type right;
    //@}

    relation_storage() {}

    relation_storage(BOOST_DEDUCED_TYPENAME ::boost::call_traits<
                         BOOST_DEDUCED_TYPENAME base_::left_value_type
                     >::param_type l,
                     BOOST_DEDUCED_TYPENAME ::boost::call_traits<
                        BOOST_DEDUCED_TYPENAME base_::right_value_type
                     >::param_type r)

        : left(l), right(r) {}

          BOOST_DEDUCED_TYPENAME base_:: left_value_type &  get_left()      { return left;  }
    const BOOST_DEDUCED_TYPENAME base_:: left_value_type &  get_left()const { return left;  }
          BOOST_DEDUCED_TYPENAME base_::right_value_type & get_right()      { return right; }
    const BOOST_DEDUCED_TYPENAME base_::right_value_type & get_right()const { return right; }
};



template< class TA, class TB, class Info, bool force_mutable >
class relation_info_hook : public
 ::boost::bimaps::relation::detail::relation_storage<TA,TB,force_mutable>
{
    typedef ::boost::bimaps::relation::detail::
                relation_storage<TA,TB,force_mutable> base_;

    typedef BOOST_DEDUCED_TYPENAME ::boost::bimaps::tags::support::
        default_tagged<Info,member_at::info>::type tagged_info_type;

    public:
    typedef BOOST_DEDUCED_TYPENAME tagged_info_type::value_type info_type;
    typedef BOOST_DEDUCED_TYPENAME tagged_info_type::tag        info_tag;

    info_type info;

    protected:

    relation_info_hook() {}

    relation_info_hook( BOOST_DEDUCED_TYPENAME ::boost::call_traits<
                           BOOST_DEDUCED_TYPENAME base_::left_value_type
                        >::param_type l,
                        BOOST_DEDUCED_TYPENAME ::boost::call_traits<
                            BOOST_DEDUCED_TYPENAME base_::right_value_type
                        >::param_type r,
                        BOOST_DEDUCED_TYPENAME ::boost::call_traits<
                            info_type
                        >::param_type i = info_type() )

        : base_(l,r), info(i) {}

    template< class Relation >
    relation_info_hook( const Relation & rel ) :
        base_(rel.left,rel.right),
        info(rel.info) {}

    template< class Relation >
    void change_to( const Relation & rel )
    {
        base_::left  = rel.left ;
        base_::right = rel.right;
        info         = rel.info ;
    }

    #ifndef BOOST_BIMAP_DISABLE_SERIALIZATION
    template< class Archive >
    void serialize(Archive & ar, const unsigned int)
    {
        ar & ::boost::serialization::make_nvp("left" , base_::left );
        ar & ::boost::serialization::make_nvp("right", base_::right);
        ar & ::boost::serialization::make_nvp("info" , info        );
    }
    #endif // BOOST_BIMAP_DISABLE_SERIALIZATION
};

template< class TA, class TB, bool force_mutable>
class relation_info_hook<TA,TB,::boost::mpl::na,force_mutable> :
    public ::boost::bimaps::relation::detail::relation_storage<TA,TB,force_mutable>
{
    typedef ::boost::bimaps::relation::detail::
                relation_storage<TA,TB,force_mutable> base_;

    public:
    typedef ::boost::mpl::na info_type;
    typedef member_at::info info_tag;

    protected:

    relation_info_hook() {}

    relation_info_hook( BOOST_DEDUCED_TYPENAME ::boost::call_traits<
                           BOOST_DEDUCED_TYPENAME base_::left_value_type
                        >::param_type l,
                        BOOST_DEDUCED_TYPENAME ::boost::call_traits<
                            BOOST_DEDUCED_TYPENAME base_::right_value_type
                        >::param_type r)

        : base_(l,r) {}

    template< class Relation >
    relation_info_hook( const Relation & rel ) :
        base_(rel.left,rel.right) {}

    template< class Relation >
    void change_to( const Relation & rel )
    {
        base_::left  = rel.left ;
        base_::right = rel.right;
    }

    #ifndef BOOST_BIMAP_DISABLE_SERIALIZATION
    template< class Archive >
    void serialize(Archive & ar, const unsigned int)
    {
        ar & ::boost::serialization::make_nvp("left" , base_::left );
        ar & ::boost::serialization::make_nvp("right", base_::right);
    }
    #endif // BOOST_BIMAP_DISABLE_SERIALIZATION
};


} // namespace detail

/// \brief Abstraction of a related pair of values, that extends the std::pair class.
/**
The mutant_relation is a mutant class. A mutant class can mutate
with zero overhead in other classes that are called views.
Each view has to be StorageCompatible with the base class
of the mutant. Note that all the views have the following
storage structure:

\verbatim
                        __________
                       |          |
                       |    TA    |
                       |__________|
                       |          |
                       |    TB    |
                       |__________|

\endverbatim

See also select_relation, standard_relation.
\ingroup relation_group
                                                           **/


template< class TA, class TB, class Info = ::boost::mpl::na, bool force_mutable = false >
class mutant_relation : public
    ::boost::bimaps::relation::detail::
        relation_info_hook<TA,TB,Info,force_mutable>
{
    typedef ::boost::bimaps::relation::detail::
        relation_info_hook<TA,TB,Info,force_mutable> base_;

    public:

    // We have to know the type of the base where the types are
    // defined because Boost.MultiIndex requires it.

    typedef ::boost::bimaps::relation::detail::
                relation_storage<TA,TB,force_mutable> storage_base;

    /// Above view, non mutable view of the relation

    typedef mutant_relation<TA,TB,Info,false> above_view;

    //@{
        /// A signature compatible std::pair that is a view of the relation.

        typedef structured_pair< TA, TB, Info, normal_layout >  left_pair;
        typedef structured_pair< TB, TA, Info, mirror_layout > right_pair;
    //@}

    typedef ::boost::mpl::vector4
    <
         left_pair,
        right_pair,

        mutant_relation< TA, TB, Info, true  >,
        mutant_relation< TA, TB, Info, false >

    > mutant_views;

    mutant_relation() {}

    mutant_relation(BOOST_DEDUCED_TYPENAME ::boost::call_traits<
                        BOOST_DEDUCED_TYPENAME base_:: left_value_type
                    >::param_type l,
                    BOOST_DEDUCED_TYPENAME ::boost::call_traits<
                        BOOST_DEDUCED_TYPENAME base_::right_value_type
                    >::param_type r) :
        base_(l,r) {}

    mutant_relation(BOOST_DEDUCED_TYPENAME ::boost::call_traits<
                        BOOST_DEDUCED_TYPENAME base_:: left_value_type
                    >::param_type l,
                    BOOST_DEDUCED_TYPENAME ::boost::call_traits<
                        BOOST_DEDUCED_TYPENAME base_::right_value_type
                    >::param_type r,
                    BOOST_DEDUCED_TYPENAME ::boost::call_traits<
                        BOOST_DEDUCED_TYPENAME base_::info_type
                    >::param_type i) :
        base_(l,r,i) {}

    mutant_relation(const mutant_relation<TA,TB,Info,false> & rel) :
        base_(rel) {}

    mutant_relation(const mutant_relation<TA,TB,Info,true> & rel) :
        base_(rel) {}

    // Operators

    template< bool FM >
    mutant_relation& operator=(const mutant_relation<TA,TB,Info,FM> & rel)
    {
        base_::change_to(rel);
        return *this;
    }

    // The following functions are redundant if you only consider this class.
    // They are included to make easier the construction of the get and the
    // pair_by metafunction. Remember that not all compiler supports the mutant
    // idiom.

    left_pair & get_left_pair()
    {
        return ::boost::bimaps::relation::detail::mutate<left_pair>(*this);
    }

    const left_pair & get_left_pair() const
    {
        return ::boost::bimaps::relation::detail::mutate<left_pair>(*this);
    }

    right_pair & get_right_pair()
    {
        return ::boost::bimaps::relation::detail::mutate<right_pair>(*this);
    }

    const right_pair & get_right_pair() const
    {
        return ::boost::bimaps::relation::detail::mutate<right_pair>(*this);
    }

    above_view & get_view()
    {
        return ::boost::bimaps::relation::detail::mutate<above_view>(*this);
    }

    const above_view & get_view() const
    {
        return ::boost::bimaps::relation::detail::mutate<above_view>(*this);
    }

    template< class Tag >
    const BOOST_DEDUCED_TYPENAME ::boost::bimaps::relation::support::
        result_of::get<Tag,const mutant_relation>::type
    get() const
    {
        return ::boost::bimaps::relation::support::get<Tag>(*this);
    }

    template< class Tag >
    BOOST_DEDUCED_TYPENAME ::boost::bimaps::relation::support::
        result_of::get<Tag,mutant_relation>::type
    get()
    {
        return ::boost::bimaps::relation::support::get<Tag>(*this);
    }
    
    #ifndef BOOST_BIMAP_DISABLE_SERIALIZATION

    private:
    friend class ::boost::serialization::access;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        base_::serialize(ar,version);
    }

    #endif // BOOST_BIMAP_DISABLE_SERIALIZATION
};

// hash value

template< class FirstType, class SecondType, bool FM >
std::size_t hash_value(const detail::relation_storage<FirstType,SecondType,FM> & r)
{
    std::size_t seed = 0;
    ::boost::hash_combine(seed, r. left );
    ::boost::hash_combine(seed, r.right );

    return seed;
}

// mutant_relation - mutant_relation

template< class FirstType, class SecondType, bool FM1, bool FM2 >
bool operator==(const detail::relation_storage<FirstType,SecondType,FM1> & a,
                const detail::relation_storage<FirstType,SecondType,FM2> & b)
{
    return ( ( a.left  == b.left  ) &&
             ( a.right == b.right ) );
}

template< class FirstType, class SecondType, bool FM1, bool FM2 >
bool operator!=(const detail::relation_storage<FirstType,SecondType,FM1> & a,
                const detail::relation_storage<FirstType,SecondType,FM2> & b)
{
    return ! ( a == b );
}

template< class FirstType, class SecondType, bool FM1, bool FM2 >
bool operator<(const detail::relation_storage<FirstType,SecondType,FM1> & a,
               const detail::relation_storage<FirstType,SecondType,FM2> & b)
{
    return (  ( a.left  <  b.left  ) ||
             (( a.left == b.left ) && ( a.right < b.right )));
}

template< class FirstType, class SecondType, bool FM1, bool FM2 >
bool operator<=(const detail::relation_storage<FirstType,SecondType,FM1> & a,
                const detail::relation_storage<FirstType,SecondType,FM2> & b)
{
    return (  ( a.left  <  b.left  ) ||
             (( a.left == b.left ) && ( a.right <= b.right )));
}

template< class FirstType, class SecondType, bool FM1, bool FM2 >
bool operator>(const detail::relation_storage<FirstType,SecondType,FM1> & a,
               const detail::relation_storage<FirstType,SecondType,FM2> & b)
{
    return ( ( a.left  >  b.left  ) ||
             (( a.left == b.left ) && ( a.right > b.right )));
}

template< class FirstType, class SecondType, bool FM1, bool FM2 >
bool operator>=(const detail::relation_storage<FirstType,SecondType,FM1> & a,
                const detail::relation_storage<FirstType,SecondType,FM2> & b)
{
    return ( ( a.left  >  b.left  ) ||
             (( a.left == b.left ) && ( a.right >= b.right )));
}

namespace detail {

template< class TA, class TB, class Info, bool force_mutable>
mutant_relation<TA,TB,Info,force_mutable> 
    copy_with_left_replaced(mutant_relation<TA,TB,Info,force_mutable> const& rel,
        BOOST_DEDUCED_TYPENAME ::boost::call_traits< BOOST_DEDUCED_TYPENAME 
            mutant_relation<TA,TB,Info,force_mutable>::left_value_type>
                ::param_type l)
{
    return mutant_relation<TA,TB,Info,force_mutable>(l,rel.right,rel.info);
}
    
template< class TA, class TB, bool force_mutable>
mutant_relation<TA,TB,::boost::mpl::na,force_mutable>
    copy_with_left_replaced(mutant_relation<TA,TB,::boost::mpl::na,force_mutable> const& rel,
        BOOST_DEDUCED_TYPENAME ::boost::call_traits< BOOST_DEDUCED_TYPENAME 
            mutant_relation<TA,TB,::boost::mpl::na,force_mutable>::left_value_type>
                ::param_type l)
{
    return mutant_relation<TA,TB,::boost::mpl::na,force_mutable>(l,rel.right);  
}
    
template< class TA, class TB, class Info, bool force_mutable>
mutant_relation<TA,TB,Info,force_mutable> 
    copy_with_right_replaced(mutant_relation<TA,TB,Info,force_mutable> const& rel,
        BOOST_DEDUCED_TYPENAME ::boost::call_traits< BOOST_DEDUCED_TYPENAME 
            mutant_relation<TA,TB,Info,force_mutable>::right_value_type>
                ::param_type r)
{
    return mutant_relation<TA,TB,Info,force_mutable>(rel.left,r,rel.info);
}
    
template< class TA, class TB, bool force_mutable>
mutant_relation<TA,TB,::boost::mpl::na,force_mutable>
    copy_with_right_replaced(mutant_relation<TA,TB,::boost::mpl::na,force_mutable> const& rel,
        BOOST_DEDUCED_TYPENAME ::boost::call_traits< BOOST_DEDUCED_TYPENAME 
            mutant_relation<TA,TB,::boost::mpl::na,force_mutable>::right_value_type>
                ::param_type r)
{
    return mutant_relation<TA,TB,::boost::mpl::na,force_mutable>(rel.left,r);  
}

} // namespace detail

} // namespace relation
} // namespace bimaps
} // namespace boost


#endif // BOOST_BIMAP_RELATION_MUTANT_RELATION_HPP




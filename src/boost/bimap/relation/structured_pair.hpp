// Boost.Bimap
//
// Copyright (c) 2006-2007 Matias Capeletto
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/// \file relation/structured_pair.hpp
/// \brief Defines the structured_pair class.

#ifndef BOOST_BIMAP_RELATION_STRUCTURED_PAIR_HPP
#define BOOST_BIMAP_RELATION_STRUCTURED_PAIR_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/config.hpp>

#include <utility>

#include <boost/type_traits/remove_const.hpp>

#include <boost/mpl/aux_/na.hpp>

#include <boost/call_traits.hpp>

#include <boost/utility/enable_if.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/vector.hpp>

#include <boost/bimap/detail/debug/static_error.hpp>
#include <boost/bimap/relation/pair_layout.hpp>
#include <boost/bimap/relation/symmetrical_base.hpp>
#include <boost/bimap/relation/support/get.hpp>
#include <boost/bimap/tags/support/value_type_of.hpp>



namespace boost {
namespace bimaps {
namespace relation {

namespace detail {

/// \brief Storage definition of the left view of a mutant relation.
/**

See also storage_finder, mirror_storage.
                                                                            **/

template< class FirstType, class SecondType >
class normal_storage :
    public symmetrical_base<FirstType,SecondType>
{
    typedef symmetrical_base<FirstType,SecondType> base_;

    public:

    typedef normal_storage storage_;

    typedef BOOST_DEDUCED_TYPENAME base_::left_value_type  first_type;
    typedef BOOST_DEDUCED_TYPENAME base_::right_value_type second_type;

    first_type   first;
    second_type  second;

    normal_storage() {}

    normal_storage(BOOST_DEDUCED_TYPENAME ::boost::call_traits<
                        first_type >::param_type f,
                   BOOST_DEDUCED_TYPENAME ::boost::call_traits<
                        second_type>::param_type s)

        : first(f), second(s) {}

          BOOST_DEDUCED_TYPENAME base_:: left_value_type &  get_left()      { return first;  }
    const BOOST_DEDUCED_TYPENAME base_:: left_value_type &  get_left()const { return first;  }
          BOOST_DEDUCED_TYPENAME base_::right_value_type & get_right()      { return second; }
    const BOOST_DEDUCED_TYPENAME base_::right_value_type & get_right()const { return second; }
};

/// \brief Storage definition of the right view of a mutant relation.
/**

See also storage_finder, normal_storage.
                                                                            **/

template< class FirstType, class SecondType >
class mirror_storage :
    public symmetrical_base<SecondType,FirstType>
{
    typedef symmetrical_base<SecondType,FirstType> base_;

    public:

    typedef mirror_storage storage_;

    typedef BOOST_DEDUCED_TYPENAME base_::left_value_type   second_type;
    typedef BOOST_DEDUCED_TYPENAME base_::right_value_type  first_type;

    second_type  second;
    first_type   first;

    mirror_storage() {}

    mirror_storage(BOOST_DEDUCED_TYPENAME ::boost::call_traits<first_type  >::param_type f,
                   BOOST_DEDUCED_TYPENAME ::boost::call_traits<second_type >::param_type s)

        : second(s), first(f)  {}

          BOOST_DEDUCED_TYPENAME base_:: left_value_type &  get_left()      { return second; }
    const BOOST_DEDUCED_TYPENAME base_:: left_value_type &  get_left()const { return second; }
          BOOST_DEDUCED_TYPENAME base_::right_value_type & get_right()      { return first;  }
    const BOOST_DEDUCED_TYPENAME base_::right_value_type & get_right()const { return first;  }
};

/** \struct boost::bimaps::relation::storage_finder
\brief Obtain the a storage with the correct layout.

\code
template< class FirstType, class SecondType, class Layout >
struct storage_finder
{
    typedef {normal/mirror}_storage<FirstType,SecondType> type;
};
\endcode

See also normal_storage, mirror_storage.
                                                                        **/

#ifndef BOOST_BIMAP_DOXYGEN_WILL_NOT_PROCESS_THE_FOLLOWING_LINES

template
<
    class FirstType,
    class SecondType,
    class Layout
>
struct storage_finder
{
    typedef normal_storage<FirstType,SecondType> type;
};

template
<
    class FirstType,
    class SecondType
>
struct storage_finder<FirstType,SecondType,mirror_layout>
{
    typedef mirror_storage<FirstType,SecondType> type;
};

#endif // BOOST_BIMAP_DOXYGEN_WILL_NOT_PROCESS_THE_FOLLOWING_LINES


template< class TA, class TB, class Info, class Layout >
class pair_info_hook :
    public ::boost::bimaps::relation::detail::storage_finder<TA,TB,Layout>::type
{
    typedef BOOST_DEDUCED_TYPENAME ::boost::bimaps::relation::detail::storage_finder<TA,TB,Layout>::type base_;

    typedef BOOST_DEDUCED_TYPENAME ::boost::bimaps::tags::support::
        default_tagged<Info,member_at::info>::type tagged_info_type;

    public:
    typedef BOOST_DEDUCED_TYPENAME tagged_info_type::value_type info_type;
    typedef BOOST_DEDUCED_TYPENAME tagged_info_type::tag        info_tag;

    info_type info;

    protected:

    pair_info_hook() {}

    pair_info_hook( BOOST_DEDUCED_TYPENAME ::boost::call_traits<
                        BOOST_DEDUCED_TYPENAME base_::first_type
                    >::param_type f,
                    BOOST_DEDUCED_TYPENAME ::boost::call_traits<
                        BOOST_DEDUCED_TYPENAME base_::second_type
                    >::param_type s,
                    BOOST_DEDUCED_TYPENAME ::boost::call_traits<
                        info_type
                    >::param_type i = info_type() )
        : base_(f,s), info(i) {}

    template< class Pair >
    pair_info_hook( const Pair & p) :
        base_(p.first,p.second),
        info(p.info) {}

    template< class Pair >
    void change_to( const Pair & p )
    {
        base_::first  = p.first ;
        base_::second = p.second;
        info          = p.info  ;
    }

    void clear_info()
    {
        info = info_type();
    };
};

template< class TA, class TB, class Layout>
class pair_info_hook<TA,TB,::boost::mpl::na,Layout> :
    public ::boost::bimaps::relation::detail::storage_finder<TA,TB,Layout>::type
{
    typedef BOOST_DEDUCED_TYPENAME ::boost::bimaps::relation::detail::storage_finder<TA,TB,Layout>::type base_;

    public:
    typedef ::boost::mpl::na info_type;
    typedef member_at::info info_tag;

    protected:

    pair_info_hook() {}

    pair_info_hook( BOOST_DEDUCED_TYPENAME ::boost::call_traits<
                        BOOST_DEDUCED_TYPENAME base_::first_type
                    >::param_type f,
                    BOOST_DEDUCED_TYPENAME ::boost::call_traits<
                        BOOST_DEDUCED_TYPENAME base_::second_type
                    >::param_type s)

        : base_(f,s) {}

    template< class Pair >
    pair_info_hook( const Pair & p ) :
        base_(p.first,p.second) {}

    template< class Pair >
    void change_to( const Pair & p )
    {
        base_::first  = p.first ;
        base_::second = p.second;
    }

    void clear_info() {};
};



} // namespace detail

template< class TA, class TB, class Info, bool FM >
class mutant_relation;


/// \brief A std::pair signature compatible class that allows you to control
///        the internal structure of the data.
/**
This class allows you to specify the order in which the two data types will be
in the layout of the class.
                                                                               **/

template< class FirstType, class SecondType, class Info, class Layout = normal_layout >
class structured_pair :

    public ::boost::bimaps::relation::detail::pair_info_hook
    <
        FirstType, SecondType,
        Info,
        Layout

    >

{
    typedef BOOST_DEDUCED_TYPENAME ::boost::bimaps::relation::detail::pair_info_hook
    <
        FirstType, SecondType,
        Info,
        Layout

    > base_;

    public:

    typedef ::boost::mpl::vector3<
        structured_pair< FirstType, SecondType, Info, normal_layout >,
        structured_pair< FirstType, SecondType, Info, mirror_layout >,
        BOOST_DEDUCED_TYPENAME ::boost::mpl::if_<
            BOOST_DEDUCED_TYPENAME ::boost::is_same<Layout, normal_layout>::type,
            mutant_relation< FirstType, SecondType, Info, true >,
            mutant_relation< SecondType, FirstType, Info, true >
        >::type

    > mutant_views;

    structured_pair() {}

    structured_pair(BOOST_DEDUCED_TYPENAME boost::call_traits<
                        BOOST_DEDUCED_TYPENAME base_::first_type  >::param_type f,
                    BOOST_DEDUCED_TYPENAME boost::call_traits<
                        BOOST_DEDUCED_TYPENAME base_::second_type >::param_type s)
        : base_(f,s) {}

    structured_pair(BOOST_DEDUCED_TYPENAME boost::call_traits<
                        BOOST_DEDUCED_TYPENAME base_::first_type  >::param_type f,
                    BOOST_DEDUCED_TYPENAME boost::call_traits<
                        BOOST_DEDUCED_TYPENAME base_::second_type >::param_type s,
                    BOOST_DEDUCED_TYPENAME boost::call_traits<
                        BOOST_DEDUCED_TYPENAME base_::info_type   >::param_type i)
        : base_(f,s,i) {}

    template< class OtherLayout >
    structured_pair(
        const structured_pair<FirstType,SecondType,Info,OtherLayout> & p)
        : base_(p) {}

    template< class OtherLayout >
    structured_pair& operator=(
        const structured_pair<FirstType,SecondType,OtherLayout> & p)
    {
        base_::change_to(p);
        return *this;
    }

    template< class First, class Second >
    structured_pair(const std::pair<First,Second> & p) :
        base_(p.first,p.second)
    {}

    template< class First, class Second >
    structured_pair& operator=(const std::pair<First,Second> & p)
    {
        base_::first  = p.first;
        base_::second = p.second;
        base_::clear_info();
        return *this;
    }

    template< class Tag >
    const BOOST_DEDUCED_TYPENAME ::boost::bimaps::relation::support::
        result_of::get<Tag,const structured_pair>::type
    get() const
    {
        return ::boost::bimaps::relation::support::get<Tag>(*this);
    }

    template< class Tag >
    BOOST_DEDUCED_TYPENAME ::boost::bimaps::relation::support::
        result_of::get<Tag,structured_pair>::type
    get()
    {
        return ::boost::bimaps::relation::support::get<Tag>(*this);
    }
};

// structured_pair - structured_pair

template< class FirstType, class SecondType, class Info, class Layout1, class Layout2 >
bool operator==(const structured_pair<FirstType,SecondType,Info,Layout1> & a,
                const structured_pair<FirstType,SecondType,Info,Layout2> & b)
{
    return ( ( a.first  == b.first  ) &&
             ( a.second == b.second ) );
}

template< class FirstType, class SecondType, class Info, class Layout1, class Layout2 >
bool operator!=(const structured_pair<FirstType,SecondType,Info,Layout1> & a,
                const structured_pair<FirstType,SecondType,Info,Layout2> & b)
{
    return ! ( a == b );
}

template< class FirstType, class SecondType, class Info, class Layout1, class Layout2 >
bool operator<(const structured_pair<FirstType,SecondType,Info,Layout1> & a,
               const structured_pair<FirstType,SecondType,Info,Layout2> & b)
{
    return (  ( a.first  <  b.first  ) ||
             (( a.first == b.first ) && ( a.second < b.second )));
}

template< class FirstType, class SecondType, class Info, class Layout1, class Layout2 >
bool operator<=(const structured_pair<FirstType,SecondType,Info,Layout1> & a,
                const structured_pair<FirstType,SecondType,Info,Layout2> & b)
{
    return (  ( a.first  <  b.first  ) ||
             (( a.first == b.first ) && ( a.second <= b.second )));
}

template< class FirstType, class SecondType, class Info, class Layout1, class Layout2 >
bool operator>(const structured_pair<FirstType,SecondType,Info,Layout1> & a,
               const structured_pair<FirstType,SecondType,Info,Layout2> & b)
{
    return ( ( a.first  >  b.first  ) ||
             (( a.first == b.first ) && ( a.second > b.second )));
}

template< class FirstType, class SecondType, class Info, class Layout1, class Layout2 >
bool operator>=(const structured_pair<FirstType,SecondType,Info,Layout1> & a,
                const structured_pair<FirstType,SecondType,Info,Layout2> & b)
{
    return ( ( a.first  >  b.first  ) ||
             (( a.first == b.first ) && ( a.second >= b.second )));
}

// structured_pair - std::pair

template< class FirstType, class SecondType, class Info, class Layout, class F, class S >
bool operator==(const structured_pair<FirstType,SecondType,Info,Layout> & a,
                const std::pair<F,S> & b)
{
    return ( ( a.first  == b.first  ) &&
             ( a.second == b.second ) );
}

template< class FirstType, class SecondType, class Info, class Layout, class F, class S >
bool operator!=(const structured_pair<FirstType,SecondType,Info,Layout> & a,
                const std::pair<F,S> & b)
{
    return ! ( a == b );
}

template< class FirstType, class SecondType, class Info, class Layout, class F, class S >
bool operator<(const structured_pair<FirstType,SecondType,Info,Layout> & a,
               const std::pair<F,S> & b)
{
    return (  ( a.first  <  b.first  ) ||
             (( a.first == b.first ) && ( a.second < b.second )));
}

template< class FirstType, class SecondType, class Info, class Layout, class F, class S >
bool operator<=(const structured_pair<FirstType,SecondType,Info,Layout> & a,
                const std::pair<F,S> & b)
{
    return (  ( a.first  <  b.first  ) ||
             (( a.first == b.first ) && ( a.second <= b.second )));
}

template< class FirstType, class SecondType, class Info, class Layout, class F, class S >
bool operator>(const structured_pair<FirstType,SecondType,Info,Layout> & a,
               const std::pair<F,S> & b)
{
    return ( ( a.first  >  b.first  ) ||
             (( a.first == b.first ) && ( a.second > b.second )));
}

template< class FirstType, class SecondType, class Info, class Layout, class F, class S >
bool operator>=(const structured_pair<FirstType,SecondType,Info,Layout> & a,
                const std::pair<F,S> & b)
{
    return ( ( a.first  >  b.first  ) ||
             (( a.first == b.first ) && ( a.second >= b.second )));
}

// std::pair - sturctured_pair

template< class FirstType, class SecondType, class Info, class Layout, class F, class S >
bool operator==(const std::pair<F,S> & a,
                const structured_pair<FirstType,SecondType,Info,Layout> & b)
{
    return ( ( a.first  == b.first  ) &&
             ( a.second == b.second ) );
}

template< class FirstType, class SecondType, class Info, class Layout, class F, class S >
bool operator!=(const std::pair<F,S> & a,
                const structured_pair<FirstType,SecondType,Info,Layout> & b)
{
    return ! ( a == b );
}

template< class FirstType, class SecondType, class Info, class Layout, class F, class S >
bool operator<(const std::pair<F,S> & a,
               const structured_pair<FirstType,SecondType,Info,Layout> & b)
{
    return (  ( a.first  <  b.first  ) ||
             (( a.first == b.first ) && ( a.second < b.second )));
}

template< class FirstType, class SecondType, class Info, class Layout, class F, class S >
bool operator<=(const std::pair<F,S> & a,
                const structured_pair<FirstType,SecondType,Info,Layout> & b)
{
    return (  ( a.first  <  b.first  ) ||
             (( a.first == b.first ) && ( a.second <= b.second )));
}

template< class FirstType, class SecondType, class Info, class Layout, class F, class S >
bool operator>(const std::pair<F,S> & a,
               const structured_pair<FirstType,SecondType,Info,Layout> & b)
{
    return ( ( a.first  >  b.first  ) ||
             (( a.first == b.first ) && ( a.second > b.second )));
}

template< class FirstType, class SecondType, class Info, class Layout, class F, class S >
bool operator>=(const std::pair<F,S> & a,
                const structured_pair<FirstType,SecondType,Info,Layout> & b)
{
    return ( ( a.first  >  b.first  ) ||
             (( a.first == b.first ) && ( a.second >= b.second )));
}


namespace detail {

template< class FirstType, class SecondType, class Info, class Layout>
structured_pair<FirstType,SecondType,Info,Layout> 
    copy_with_first_replaced(structured_pair<FirstType,SecondType,Info,Layout> const& p,
        BOOST_DEDUCED_TYPENAME ::boost::call_traits< BOOST_DEDUCED_TYPENAME 
            structured_pair<FirstType,SecondType,Info,Layout>::first_type>
                ::param_type f)
{
    return structured_pair<FirstType,SecondType,Info,Layout>(f,p.second,p.info);
}
    
template< class FirstType, class SecondType, class Layout>
structured_pair<FirstType,SecondType,::boost::mpl::na,Layout> 
    copy_with_first_replaced(structured_pair<FirstType,SecondType,::boost::mpl::na,Layout> const& p,
        BOOST_DEDUCED_TYPENAME ::boost::call_traits< BOOST_DEDUCED_TYPENAME 
            structured_pair<FirstType,SecondType,::boost::mpl::na,Layout>::first_type>
                ::param_type f)
{
    return structured_pair<FirstType,SecondType,::boost::mpl::na,Layout>(f,p.second);
}
    
template< class FirstType, class SecondType, class Info, class Layout>
structured_pair<FirstType,SecondType,Info,Layout> 
    copy_with_second_replaced(structured_pair<FirstType,SecondType,Info,Layout> const& p,
        BOOST_DEDUCED_TYPENAME ::boost::call_traits< BOOST_DEDUCED_TYPENAME 
            structured_pair<FirstType,SecondType,Info,Layout>::second_type>
                ::param_type s)
{
    return structured_pair<FirstType,SecondType,Info,Layout>(p.first,s,p.info);
}
    
template< class FirstType, class SecondType, class Layout>
structured_pair<FirstType,SecondType,::boost::mpl::na,Layout> 
    copy_with_second_replaced(structured_pair<FirstType,SecondType,::boost::mpl::na,Layout> const& p,
        BOOST_DEDUCED_TYPENAME ::boost::call_traits< BOOST_DEDUCED_TYPENAME 
            structured_pair<FirstType,SecondType,::boost::mpl::na,Layout>::second_type>
                ::param_type s)
{
    return structured_pair<FirstType,SecondType,::boost::mpl::na,Layout>(p.first,s);
}

} // namespace detail


} // namespace relation
} // namespace bimaps
} // namespace boost

#endif // BOOST_BIMAP_RELATION_STRUCTURED_PAIR_HPP


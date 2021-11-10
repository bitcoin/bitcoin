/*
 [auto_generated]
 boost/numeric/odeint/algebra/detail/for_each.hpp

 [begin_description]
 Default for_each implementations.
 [end_description]

 Copyright 2010-2012 Karsten Ahnert
 Copyright 2011 Mario Mulansky

 Distributed under the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or
 copy at http://www.boost.org/LICENSE_1_0.txt)
 */


#ifndef BOOST_NUMERIC_ODEINT_ALGEBRA_DETAIL_FOR_EACH_HPP_INCLUDED
#define BOOST_NUMERIC_ODEINT_ALGEBRA_DETAIL_FOR_EACH_HPP_INCLUDED

namespace boost {
namespace numeric {
namespace odeint {
namespace detail {


    template< class Iterator1 , class Operation >
    inline void for_each1( Iterator1 first1 , Iterator1 last1 , Operation op )
    {
        for( ; first1 != last1 ; )
            op( *first1++ );
    }


    template< class Iterator1 , class Iterator2 , class Operation >
    inline void for_each2( Iterator1 first1 , Iterator1 last1 , Iterator2 first2 , Operation op )
    {
        for( ; first1 != last1 ; )
            op( *first1++ , *first2++ );
    }


    template< class Iterator1 , class Iterator2 , class Iterator3 , class Operation >
    inline void for_each3( Iterator1 first1 , Iterator1 last1 , Iterator2 first2 , Iterator3 first3, Operation op )
    {
        for( ; first1 != last1 ; )
            op( *first1++ , *first2++ , *first3++ );
    }


    template< class Iterator1 , class Iterator2 , class Iterator3 , class Iterator4 , class Operation >
    inline void for_each4( Iterator1 first1 , Iterator1 last1 , Iterator2 first2 , Iterator3 first3, Iterator4 first4, Operation op )
    {
        for( ; first1 != last1 ; )
            op( *first1++ , *first2++ , *first3++ , *first4++ );
    }


    template< class Iterator1 , class Iterator2 , class Iterator3 , class Iterator4 , class Iterator5 , class Operation >
    inline void for_each5( Iterator1 first1 , Iterator1 last1 , Iterator2 first2 , Iterator3 first3,
                           Iterator4 first4, Iterator5 first5, Operation op )
    {
        for( ; first1 != last1 ; )
            op( *first1++ , *first2++ , *first3++ , *first4++ , *first5++ );
    }


    template< class Iterator1 , class Iterator2 , class Iterator3 , class Iterator4 , class Iterator5 , class Iterator6 , class Operation >
    inline void for_each6( Iterator1 first1 , Iterator1 last1 , Iterator2 first2 , Iterator3 first3,
                           Iterator4 first4, Iterator5 first5, Iterator6 first6 , Operation op )
    {
        for( ; first1 != last1 ; )
            op( *first1++ , *first2++ , *first3++ , *first4++ , *first5++ , *first6++ );
    }


    template< class Iterator1 , class Iterator2 , class Iterator3 , class Iterator4 , class Iterator5 , class Iterator6 , class Iterator7 , class Operation >
    inline void for_each7( Iterator1 first1 , Iterator1 last1 , Iterator2 first2 , Iterator3 first3,
                           Iterator4 first4, Iterator5 first5, Iterator6 first6 , Iterator7 first7 , Operation op )
    {
        for( ; first1 != last1 ; )
            op( *first1++ , *first2++ , *first3++ , *first4++ , *first5++ , *first6++ , *first7++ );
    }

    template< class Iterator1 , class Iterator2 , class Iterator3 , class Iterator4 , class Iterator5 , class Iterator6 , class Iterator7 , class Iterator8 , class Operation >
    inline void for_each8( Iterator1 first1 , Iterator1 last1 , Iterator2 first2 , Iterator3 first3,
                           Iterator4 first4, Iterator5 first5, Iterator6 first6 , Iterator7 first7 , Iterator8 first8 , Operation op )
    {
        for( ; first1 != last1 ; )
            op( *first1++ , *first2++ , *first3++ , *first4++ , *first5++ , *first6++ , *first7++ , *first8++ );
    }

    template< class Iterator1 , class Iterator2 , class Iterator3 , class Iterator4 , class Iterator5 , class Iterator6 , class Iterator7 , class Iterator8 , class Iterator9 , class Operation >
    inline void for_each9( Iterator1 first1 , Iterator1 last1 , Iterator2 first2 , Iterator3 first3,
                           Iterator4 first4, Iterator5 first5, Iterator6 first6 , Iterator7 first7 , Iterator8 first8 ,
                           Iterator9 first9 , Operation op )
    {
        for( ; first1 != last1 ; )
            op( *first1++ , *first2++ , *first3++ , *first4++ , *first5++ , *first6++ , *first7++ , *first8++ , *first9++ );
    }

    template< class Iterator1 , class Iterator2 , class Iterator3 , class Iterator4 , class Iterator5 , class Iterator6 , class Iterator7 , class Iterator8 , class Iterator9 , class Iterator10 , class Operation >
    inline void for_each10( Iterator1 first1 , Iterator1 last1 , Iterator2 first2 , Iterator3 first3,
                            Iterator4 first4, Iterator5 first5, Iterator6 first6 , Iterator7 first7 , Iterator8 first8 ,
                            Iterator9 first9 , Iterator10 first10 , Operation op )
    {
        for( ; first1 != last1 ; )
            op( *first1++ , *first2++ , *first3++ , *first4++ , *first5++ , *first6++ , *first7++ , *first8++ , *first9++ , *first10++ );
    }

    template< class Iterator1 , class Iterator2 , class Iterator3 , class Iterator4 , class Iterator5 , class Iterator6 , class Iterator7 , class Iterator8 , class Iterator9 , class Iterator10 , class Iterator11 , class Operation >
    inline void for_each11( Iterator1 first1 , Iterator1 last1 , Iterator2 first2 , Iterator3 first3,
                            Iterator4 first4, Iterator5 first5, Iterator6 first6 , Iterator7 first7 , Iterator8 first8 ,
                            Iterator9 first9 , Iterator10 first10 , Iterator11 first11 , Operation op )
    {
        for( ; first1 != last1 ; )
            op( *first1++ , *first2++ , *first3++ , *first4++ , *first5++ , *first6++ , *first7++ , *first8++ , *first9++ , *first10++ , *first11++ );
    }

    template< class Iterator1 , class Iterator2 , class Iterator3 , class Iterator4 , class Iterator5 , class Iterator6 , class Iterator7 , class Iterator8 , class Iterator9 , class Iterator10 , class Iterator11 , class Iterator12 , class Operation >
    inline void for_each12( Iterator1 first1 , Iterator1 last1 , Iterator2 first2 , Iterator3 first3,
                            Iterator4 first4, Iterator5 first5, Iterator6 first6 , Iterator7 first7 , Iterator8 first8 ,
                            Iterator9 first9 , Iterator10 first10 , Iterator11 first11 , Iterator12 first12 , Operation op )
    {
        for( ; first1 != last1 ; )
            op( *first1++ , *first2++ , *first3++ , *first4++ , *first5++ , *first6++ , *first7++ , *first8++ , *first9++ , *first10++ , *first11++ , *first12++ );
    }

    template< class Iterator1 , class Iterator2 , class Iterator3 , class Iterator4 , class Iterator5 , class Iterator6 , class Iterator7 , class Iterator8 , class Iterator9 , class Iterator10 , class Iterator11 , class Iterator12 , class Iterator13 , class Operation >
    inline void for_each13( Iterator1 first1 , Iterator1 last1 , Iterator2 first2 , Iterator3 first3,
                            Iterator4 first4, Iterator5 first5, Iterator6 first6 , Iterator7 first7 , Iterator8 first8 ,
                            Iterator9 first9 , Iterator10 first10 , Iterator11 first11 , Iterator12 first12 , Iterator13 first13 , Operation op )
    {
        for( ; first1 != last1 ; )
            op( *first1++ , *first2++ , *first3++ , *first4++ , *first5++ , *first6++ , *first7++ , *first8++ , *first9++ , *first10++ , *first11++ , *first12++ , *first13++ );
    }

    template< class Iterator1 , class Iterator2 , class Iterator3 , class Iterator4 , class Iterator5 , class Iterator6 , class Iterator7 , class Iterator8 , class Iterator9 , class Iterator10 , class Iterator11 , class Iterator12 , class Iterator13 , class Iterator14 , class Operation >
    inline void for_each14( Iterator1 first1 , Iterator1 last1 , Iterator2 first2 , Iterator3 first3,
                            Iterator4 first4, Iterator5 first5, Iterator6 first6 , Iterator7 first7 , Iterator8 first8 ,
                            Iterator9 first9 , Iterator10 first10 , Iterator11 first11 , Iterator12 first12 , Iterator13 first13 ,
                            Iterator14 first14 , Operation op )
    {
        for( ; first1 != last1 ; )
            op( *first1++ , *first2++ , *first3++ , *first4++ , *first5++ , *first6++ , *first7++ , *first8++ , *first9++ , *first10++ , *first11++ , *first12++ , *first13++ , *first14++ );
    }

    template< class Iterator1 , class Iterator2 , class Iterator3 , class Iterator4 , class Iterator5 , class Iterator6 , class Iterator7 , class Iterator8 , class Iterator9 , class Iterator10 , class Iterator11 , class Iterator12 , class Iterator13 , class Iterator14 , class Iterator15 , class Operation >
    inline void for_each15( Iterator1 first1 , Iterator1 last1 , Iterator2 first2 , Iterator3 first3,
                            Iterator4 first4, Iterator5 first5, Iterator6 first6 , Iterator7 first7 , Iterator8 first8 ,
                            Iterator9 first9 , Iterator10 first10 , Iterator11 first11 , Iterator12 first12 , Iterator13 first13 ,
                            Iterator14 first14 , Iterator15 first15 , Operation op )
    {
        for( ; first1 != last1 ; )
            op( *first1++ , *first2++ , *first3++ , *first4++ , *first5++ , *first6++ , *first7++ , *first8++ , *first9++ , *first10++ , *first11++ , *first12++ , *first13++ , *first14++ , *first15++ );
    }


} // detail
} // odeint
} // numeric
} // boost


#endif // BOOST_NUMERIC_ODEINT_ALGEBRA_DETAIL_FOR_EACH_HPP_INCLUDED

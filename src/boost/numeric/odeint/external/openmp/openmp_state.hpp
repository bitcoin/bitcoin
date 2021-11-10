/*
 [auto_generated]
 boost/numeric/odeint/external/openmp/openmp_state.hpp

 [begin_description]
 Wrappers for OpenMP.
 [end_description]

 Copyright 2013 Karsten Ahnert
 Copyright 2013 Mario Mulansky
 Copyright 2013 Pascal Germroth

 Distributed under the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or
 copy at http://www.boost.org/LICENSE_1_0.txt)
 */


#ifndef BOOST_NUMERIC_ODEINT_EXTERNAL_OPENMP_OPENMP_STATE_HPP_INCLUDED
#define BOOST_NUMERIC_ODEINT_EXTERNAL_OPENMP_OPENMP_STATE_HPP_INCLUDED

#include <omp.h>
#include <vector>
#include <algorithm>
#include <boost/range/adaptor/sliced.hpp>
#include <boost/numeric/odeint/util/copy.hpp>
#include <boost/numeric/odeint/util/split.hpp>
#include <boost/numeric/odeint/util/resize.hpp>
#include <boost/numeric/odeint/external/openmp/openmp_nested_algebra.hpp>

namespace boost {
namespace numeric {
namespace odeint {

/** \brief A container that is split into distinct parts, for threading.
 * Just a wrapper for vector<vector<T>>, use `copy` for splitting/joining.
 */
template< class T >
struct openmp_state : public std::vector< std::vector< T > >
{
    openmp_state() {}

    openmp_state(size_t n, const std::vector<T>& val = std::vector<T>())
    : std::vector< std::vector< T > >(n, val) {}

    template<class InputIterator>
    openmp_state(InputIterator first, InputIterator last)
    : std::vector< std::vector< T > >(first, last) {}

    openmp_state(const std::vector< std::vector< T > > &orig)
    : std::vector< std::vector< T > >(orig) {}

};




template< class T >
struct is_resizeable< openmp_state< T > > : boost::true_type { };


template< class T >
struct same_size_impl< openmp_state< T > , openmp_state< T > >
{
    static bool same_size( const openmp_state< T > &x , const openmp_state< T > &y )
    {
        if( x.size() != y.size() ) return false;
        for( size_t i = 0 ; i != x.size() ; i++ )
            if( x[i].size() != y[i].size() ) return false;
        return true;
    }
};


template< class T >
struct resize_impl< openmp_state< T > , openmp_state< T > >
{
    static void resize( openmp_state< T > &x , const openmp_state< T > &y )
    {
        x.resize( y.size() );
#       pragma omp parallel for schedule(dynamic)
        for(size_t i = 0 ; i < x.size() ; i++)
            x[i].resize( y[i].size() );
    }
};


/** \brief Copy data between openmp_states of same size. */
template< class T >
struct copy_impl< openmp_state< T >, openmp_state< T > >
{
    static void copy( const openmp_state< T > &from, openmp_state< T > &to )
    {
#       pragma omp parallel for schedule(dynamic)
        for(size_t i = 0 ; i < from.size() ; i++)
            std::copy( from[i].begin() , from[i].end() , to.begin() );
    }
};



/** \brief Copy data from some container to an openmp_state and resize it.
 * Target container size will determine number of blocks to split into.
 * If it is empty, it will be resized to the maximum number of OpenMP threads.
 * SourceContainer must support `s::value_type`, `s::const_iterator`, `s.begin()`, `s.end()` and `s.size()`,
 * with Random Access Iterators; i.e. it must be a Random Access Container. */
template< class SourceContainer >
struct split_impl< SourceContainer, openmp_state< typename SourceContainer::value_type > >
{
    static void split( const SourceContainer &from, openmp_state< typename SourceContainer::value_type > &to )
    {
        if(to.size() == 0) to.resize( omp_get_max_threads() );
        const size_t part = from.size() / to.size();
#       pragma omp parallel for schedule(dynamic)
        for(size_t i = 0 ; i < to.size() ; i++) {
            typedef typename SourceContainer::const_iterator it_t;
            const it_t begin = from.begin() + i * part;
            it_t end = begin + part;
            // for cases where from.size() % to.size() > 0
            if(i + 1 == to.size() || end > from.end()) end = from.end();
            to[i].resize(end - begin);
            std::copy(begin, end, to[i].begin());
        }
    }
};

/** \brief Copy data from an openmp_state to some container and resize it.
 * TargetContainer must support `s::value_type`, `s::iterator`, `s.begin()` and `s.resize(n)`,
 * i.e. it must be a `std::vector`. */
template< class TargetContainer >
struct unsplit_impl< openmp_state< typename TargetContainer::value_type >, TargetContainer >
{
    static void unsplit( const openmp_state< typename TargetContainer::value_type > &from , TargetContainer &to )
    {
        // resize target
        size_t total_size = 0;
        for(size_t i = 0 ; i < from.size() ; i++)
            total_size += from[i].size();
        to.resize( total_size );
        // copy parts
        typename TargetContainer::iterator out = to.begin();
        for(size_t i = 0 ; i < from.size() ; i++)
            out = std::copy(from[i].begin(), from[i].end(), out);
    }
};




/** \brief OpenMP-parallelized algebra.
 * For use with openmp_state.
 */
typedef openmp_nested_algebra< range_algebra > openmp_algebra;



/** \brief Use `openmp_algebra` for `openmp_state`. */
template< class T >
struct algebra_dispatcher< openmp_state< T > >
{
    typedef openmp_algebra algebra_type;
};


}
}
}


#endif


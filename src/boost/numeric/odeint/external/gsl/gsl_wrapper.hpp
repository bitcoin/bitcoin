/*
 [auto_generated]
 boost/numeric/odeint/external/gsl/gsl_wrapper.hpp

 [begin_description]
 Wrapper for gsl_vector.
 [end_description]

 Copyright 2011-2012 Mario Mulansky
 Copyright 2011 Karsten Ahnert

 Distributed under the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or
 copy at http://www.boost.org/LICENSE_1_0.txt)
 */


#ifndef BOOST_NUMERIC_ODEINT_EXTERNAL_GSL_GSL_WRAPPER_HPP_INCLUDED
#define BOOST_NUMERIC_ODEINT_EXTERNAL_GSL_GSL_WRAPPER_HPP_INCLUDED

#include <new>

#include <gsl/gsl_vector.h>

#include <boost/type_traits/integral_constant.hpp>
#include <boost/range.hpp>
#include <boost/iterator/iterator_facade.hpp>


#include <boost/numeric/odeint/util/state_wrapper.hpp>
#include <boost/numeric/odeint/util/is_resizeable.hpp>
#include <boost/numeric/odeint/util/copy.hpp>

class const_gsl_vector_iterator;

/*
 * defines an iterator for gsl_vector
 */
class gsl_vector_iterator : public boost::iterator_facade< gsl_vector_iterator , double , boost::random_access_traversal_tag >
{
public :

    gsl_vector_iterator( void ): m_p(0) , m_stride( 0 ) { }
    explicit gsl_vector_iterator( gsl_vector *p ) : m_p( p->data ) , m_stride( p->stride ) { }
    friend gsl_vector_iterator end_iterator( gsl_vector * );

private :

    friend class boost::iterator_core_access;
    friend class const_gsl_vector_iterator;

    void increment( void ) { m_p += m_stride; }
    void decrement( void ) { m_p -= m_stride; }
    void advance( ptrdiff_t n ) { m_p += n*m_stride; }
    bool equal( const gsl_vector_iterator &other ) const { return this->m_p == other.m_p; }
    bool equal( const const_gsl_vector_iterator &other ) const;
    double& dereference( void ) const { return *m_p; }

    double *m_p;
    size_t m_stride;
};



/*
 * defines an const iterator for gsl_vector
 */
class const_gsl_vector_iterator : public boost::iterator_facade< const_gsl_vector_iterator , const double , boost::random_access_traversal_tag >
{
public :

    const_gsl_vector_iterator( void ): m_p(0) , m_stride( 0 ) { }
    explicit const_gsl_vector_iterator( const gsl_vector *p ) : m_p( p->data ) , m_stride( p->stride ) { }
    const_gsl_vector_iterator( const gsl_vector_iterator &p ) : m_p( p.m_p ) , m_stride( p.m_stride ) { }

private :

    friend class boost::iterator_core_access;
    friend class gsl_vector_iterator;
    friend const_gsl_vector_iterator end_iterator( const gsl_vector * );

    void increment( void ) { m_p += m_stride; }
    void decrement( void ) { m_p -= m_stride; }
    void advance( ptrdiff_t n ) { m_p += n*m_stride; }
    bool equal( const const_gsl_vector_iterator &other ) const { return this->m_p == other.m_p; }
    bool equal( const gsl_vector_iterator &other ) const { return this->m_p == other.m_p; }
    const double& dereference( void ) const { return *m_p; }

    const double *m_p;
    size_t m_stride;
};


bool gsl_vector_iterator::equal( const const_gsl_vector_iterator &other ) const { return this->m_p == other.m_p; }


gsl_vector_iterator end_iterator( gsl_vector *x )
{
    gsl_vector_iterator iter( x );
    iter.m_p += iter.m_stride * x->size;
    return iter;
}

const_gsl_vector_iterator end_iterator( const gsl_vector *x )
{
    const_gsl_vector_iterator iter( x );
    iter.m_p += iter.m_stride * x->size;
    return iter;
}




namespace boost
{
template<>
struct range_mutable_iterator< gsl_vector* >
{
    typedef gsl_vector_iterator type;
};

template<>
struct range_const_iterator< gsl_vector* >
{
    typedef const_gsl_vector_iterator type;
};
} // namespace boost




// template<>
inline gsl_vector_iterator range_begin( gsl_vector *x )
{
    return gsl_vector_iterator( x );
}

// template<>
inline const_gsl_vector_iterator range_begin( const gsl_vector *x )
{
    return const_gsl_vector_iterator( x );
}

// template<>
inline gsl_vector_iterator range_end( gsl_vector *x )
{
    return end_iterator( x );
}

// template<>
inline const_gsl_vector_iterator range_end( const gsl_vector *x )
{
    return end_iterator( x );
}







namespace boost {
namespace numeric {
namespace odeint {


template<>
struct is_resizeable< gsl_vector* >
{
    //struct type : public boost::true_type { };
    typedef boost::true_type type;
    const static bool value = type::value;
};

template <>
struct same_size_impl< gsl_vector* , gsl_vector* >
{
    static bool same_size( const gsl_vector* x , const gsl_vector* y )
    {
        return x->size == y->size;
    }
};

template <>
struct resize_impl< gsl_vector* , gsl_vector* >
{
    static void resize( gsl_vector* &x , const gsl_vector* y )
    {
        gsl_vector_free( x );
        x = gsl_vector_alloc( y->size );
    }
};

template<>
struct state_wrapper< gsl_vector* >
{
    typedef double value_type;
    typedef gsl_vector* state_type;
    typedef state_wrapper< gsl_vector* > state_wrapper_type;

    state_type m_v;

    state_wrapper( )
    {
        m_v = gsl_vector_alloc( 1 );
    }

    state_wrapper( const state_wrapper_type &x )
    {
        resize( m_v , x.m_v );
        gsl_vector_memcpy( m_v , x.m_v );
    }


    ~state_wrapper()
    {
        gsl_vector_free( m_v );
    }

};

} // odeint
} // numeric
} // boost




#endif // BOOST_NUMERIC_ODEINT_EXTERNAL_GSL_GSL_WRAPPER_HPP_INCLUDED

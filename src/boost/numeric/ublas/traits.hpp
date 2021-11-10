//
//  Copyright (c) 2000-2002
//  Joerg Walter, Mathias Koch
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//  The authors gratefully acknowledge the support of
//  GeNeSys mbH & Co. KG in producing this work.
//

#ifndef _BOOST_UBLAS_TRAITS_
#define _BOOST_UBLAS_TRAITS_

#include <iterator>
#include <complex>
#include <boost/config/no_tr1/cmath.hpp>

#include <boost/numeric/ublas/detail/config.hpp>
#include <boost/numeric/ublas/detail/iterator.hpp>
#include <boost/numeric/ublas/detail/returntype_deduction.hpp>
#ifdef BOOST_UBLAS_USE_INTERVAL
#include <boost/numeric/interval.hpp>
#endif

#include <boost/type_traits.hpp>
#include <complex>
#include <boost/typeof/typeof.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/type_traits/is_float.hpp>
#include <boost/type_traits/is_integral.hpp>
#include <boost/type_traits/is_unsigned.hpp>
#include <boost/mpl/and.hpp>
#include <boost/mpl/if.hpp>
#include <boost/typeof/typeof.hpp>


// anonymous namespace to avoid ADL issues
namespace {
  template<class T>
    typename boost::mpl::if_c<boost::is_integral<T>::value,
                              double,
                              T>::type
  boost_numeric_ublas_sqrt (const T& t) {
    using namespace std;
    // we'll find either std::sqrt or else another version via ADL:
    return sqrt (t);
  }

template<typename T>
inline typename boost::disable_if<
    boost::is_unsigned<T>, T >::type
    boost_numeric_ublas_abs (const T &t ) {
        using namespace std;
        // force a type conversion back to T for char and short types
        return static_cast<T>(abs( t ));
    }

template<typename T>
inline typename boost::enable_if<
    boost::is_unsigned<T>, T >::type
    boost_numeric_ublas_abs (const T &t ) {
        return t;
    }
}

namespace boost { namespace numeric { namespace ublas {


    template<typename R, typename I>
    typename boost::enable_if<
      mpl::and_<
        boost::is_float<R>,
        boost::is_integral<I>
        >,
      std::complex<R> >::type inline operator+ (I in1, std::complex<R> const& in2 ) {
      return R (in1) + in2;
    }

    template<typename R, typename I>
    typename boost::enable_if<
      mpl::and_<
        boost::is_float<R>,
        boost::is_integral<I>
        >,
      std::complex<R> >::type inline operator+ (std::complex<R> const& in1, I in2) {
      return in1 + R (in2);
    }

    template<typename R, typename I>
    typename boost::enable_if<
      mpl::and_<
        boost::is_float<R>,
        boost::is_integral<I>
        >,
      std::complex<R> >::type inline operator- (I in1, std::complex<R> const& in2) {
      return R (in1) - in2;
    }

    template<typename R, typename I>
    typename boost::enable_if<
      mpl::and_<
        boost::is_float<R>,
        boost::is_integral<I>
        >,
      std::complex<R> >::type inline operator- (std::complex<R> const& in1, I in2) {
      return in1 - R (in2);
    }

    template<typename R, typename I>
    typename boost::enable_if<
      mpl::and_<
        boost::is_float<R>,
        boost::is_integral<I>
        >,
      std::complex<R> >::type inline operator* (I in1, std::complex<R> const& in2) {
      return R (in1) * in2;
    }

    template<typename R, typename I>
    typename boost::enable_if<
      mpl::and_<
        boost::is_float<R>,
        boost::is_integral<I>
        >,
      std::complex<R> >::type inline operator* (std::complex<R> const& in1, I in2) {
      return in1 * R(in2);
    }

    template<typename R, typename I>
    typename boost::enable_if<
      mpl::and_<
        boost::is_float<R>,
        boost::is_integral<I>
        >,
      std::complex<R> >::type inline operator/ (I in1, std::complex<R> const& in2) {
      return R(in1) / in2;
    }

    template<typename R, typename I>
    typename boost::enable_if<
      mpl::and_<
        boost::is_float<R>,
        boost::is_integral<I>
        >,
      std::complex<R> >::type inline operator/ (std::complex<R> const& in1, I in2) {
      return in1 / R (in2);
    }

    // uBLAS assumes a common return type for all binary arithmetic operators
    template<class X, class Y>
    struct promote_traits {
        typedef BOOST_TYPEOF_TPL(X() + Y()) promote_type;
    };



    // Type traits - generic numeric properties and functions
    template<class T>
    struct type_traits;
        
    // Define properties for a generic scalar type
    template<class T>
    struct scalar_traits {
        typedef scalar_traits<T> self_type;
        typedef T value_type;
        typedef const T &const_reference;
        typedef T &reference;

        typedef T real_type;
        typedef real_type precision_type;       // we do not know what type has more precision then the real_type

        static const unsigned plus_complexity = 1;
        static const unsigned multiplies_complexity = 1;

        static
        BOOST_UBLAS_INLINE
        real_type real (const_reference t) {
                return t;
        }
        static
        BOOST_UBLAS_INLINE
        real_type imag (const_reference /*t*/) {
                return 0;
        }
        static
        BOOST_UBLAS_INLINE
        value_type conj (const_reference t) {
                return t;
        }

        static
        BOOST_UBLAS_INLINE
        real_type type_abs (const_reference t) {
            return boost_numeric_ublas_abs (t);
        }
        static
        BOOST_UBLAS_INLINE
        value_type type_sqrt (const_reference t) {
            // force a type conversion back to value_type for intgral types
            return value_type (boost_numeric_ublas_sqrt (t));
        }

        static
        BOOST_UBLAS_INLINE
        real_type norm_1 (const_reference t) {
            return self_type::type_abs (t);
        }
        static
        BOOST_UBLAS_INLINE
        real_type norm_2 (const_reference t) {
            return self_type::type_abs (t);
        }
        static
        BOOST_UBLAS_INLINE
        real_type norm_inf (const_reference t) {
            return self_type::type_abs (t);
        }

        static
        BOOST_UBLAS_INLINE
        bool equals (const_reference t1, const_reference t2) {
            return self_type::norm_inf (t1 - t2) < BOOST_UBLAS_TYPE_CHECK_EPSILON *
                   (std::max) ((std::max) (self_type::norm_inf (t1),
                                       self_type::norm_inf (t2)),
                             BOOST_UBLAS_TYPE_CHECK_MIN);
        }
    };

    // Define default type traits, assume T is a scalar type
    template<class T>
    struct type_traits : scalar_traits <T> {
        typedef type_traits<T> self_type;
        typedef T value_type;
        typedef const T &const_reference;
        typedef T &reference;

        typedef T real_type;
        typedef real_type precision_type;
        static const unsigned multiplies_complexity = 1;

    };

    // Define real type traits
    template<>
    struct type_traits<float> : scalar_traits<float> {
        typedef type_traits<float> self_type;
        typedef float value_type;
        typedef const value_type &const_reference;
        typedef value_type &reference;
        typedef value_type real_type;
        typedef double precision_type;
    };
    template<>
    struct type_traits<double> : scalar_traits<double> {
        typedef type_traits<double> self_type;
        typedef double value_type;
        typedef const value_type &const_reference;
        typedef value_type &reference;
        typedef value_type real_type;
        typedef long double precision_type;
    };
    template<>
    struct type_traits<long double>  : scalar_traits<long double> {
        typedef type_traits<long double> self_type;
        typedef long double value_type;
        typedef const value_type &const_reference;
        typedef value_type &reference;
        typedef value_type real_type;
        typedef value_type precision_type;
    };

    // Define properties for a generic complex type
    template<class T>
    struct complex_traits {
        typedef complex_traits<T> self_type;
        typedef T value_type;
        typedef const T &const_reference;
        typedef T &reference;

        typedef typename T::value_type real_type;
        typedef real_type precision_type;       // we do not know what type has more precision then the real_type

        static const unsigned plus_complexity = 2;
        static const unsigned multiplies_complexity = 6;

        static
        BOOST_UBLAS_INLINE
        real_type real (const_reference t) {
                return std::real (t);
        }
        static
        BOOST_UBLAS_INLINE
        real_type imag (const_reference t) {
                return std::imag (t);
        }
        static
        BOOST_UBLAS_INLINE
        value_type conj (const_reference t) {
                return std::conj (t);
        }

        static
        BOOST_UBLAS_INLINE
        real_type type_abs (const_reference t) {
                return abs (t);
        }
        static
        BOOST_UBLAS_INLINE
        value_type type_sqrt (const_reference t) {
                return sqrt (t);
        }

        static
        BOOST_UBLAS_INLINE
        real_type norm_1 (const_reference t) {
            return self_type::type_abs (t);
            // original computation has been replaced because a complex number should behave like a scalar type
            // return type_traits<real_type>::type_abs (self_type::real (t)) +
            //       type_traits<real_type>::type_abs (self_type::imag (t));
        }
        static
        BOOST_UBLAS_INLINE
        real_type norm_2 (const_reference t) {
            return self_type::type_abs (t);
        }
        static
        BOOST_UBLAS_INLINE
        real_type norm_inf (const_reference t) {
            return self_type::type_abs (t);
            // original computation has been replaced because a complex number should behave like a scalar type
            // return (std::max) (type_traits<real_type>::type_abs (self_type::real (t)),
            //                 type_traits<real_type>::type_abs (self_type::imag (t)));
        }

        static
        BOOST_UBLAS_INLINE
        bool equals (const_reference t1, const_reference t2) {
            return self_type::norm_inf (t1 - t2) < BOOST_UBLAS_TYPE_CHECK_EPSILON *
                   (std::max) ((std::max) (self_type::norm_inf (t1),
                                       self_type::norm_inf (t2)),
                             BOOST_UBLAS_TYPE_CHECK_MIN);
        }
    };
    
    // Define complex type traits
    template<>
    struct type_traits<std::complex<float> > : complex_traits<std::complex<float> >{
        typedef type_traits<std::complex<float> > self_type;
        typedef std::complex<float> value_type;
        typedef const value_type &const_reference;
        typedef value_type &reference;
        typedef float real_type;
        typedef std::complex<double> precision_type;

    };
    template<>
    struct type_traits<std::complex<double> > : complex_traits<std::complex<double> >{
        typedef type_traits<std::complex<double> > self_type;
        typedef std::complex<double> value_type;
        typedef const value_type &const_reference;
        typedef value_type &reference;
        typedef double real_type;
        typedef std::complex<long double> precision_type;
    };
    template<>
    struct type_traits<std::complex<long double> > : complex_traits<std::complex<long double> > {
        typedef type_traits<std::complex<long double> > self_type;
        typedef std::complex<long double> value_type;
        typedef const value_type &const_reference;
        typedef value_type &reference;
        typedef long double real_type;
        typedef value_type precision_type;
    };

#ifdef BOOST_UBLAS_USE_INTERVAL
    // Define scalar interval type traits
    template<>
    struct type_traits<boost::numeric::interval<float> > : scalar_traits<boost::numeric::interval<float> > {
        typedef type_traits<boost::numeric::interval<float> > self_type;
        typedef boost::numeric::interval<float> value_type;
        typedef const value_type &const_reference;
        typedef value_type &reference;
        typedef value_type real_type;
        typedef boost::numeric::interval<double> precision_type;

    };
    template<>
    struct type_traits<boost::numeric::interval<double> > : scalar_traits<boost::numeric::interval<double> > {
        typedef type_traits<boost::numeric::interval<double> > self_type;
        typedef boost::numeric::interval<double> value_type;
        typedef const value_type &const_reference;
        typedef value_type &reference;
        typedef value_type real_type;
        typedef boost::numeric::interval<long double> precision_type;
    };
    template<>
    struct type_traits<boost::numeric::interval<long double> > : scalar_traits<boost::numeric::interval<long double> > {
        typedef type_traits<boost::numeric::interval<long double> > self_type;
        typedef boost::numeric::interval<long double> value_type;
        typedef const value_type &const_reference;
        typedef value_type &reference;
        typedef value_type real_type;
        typedef value_type precision_type;
    };
#endif


    // Storage tags -- hierarchical definition of storage characteristics

    struct unknown_storage_tag {};
    struct sparse_proxy_tag: public unknown_storage_tag {};
    struct sparse_tag: public sparse_proxy_tag {};
    struct packed_proxy_tag: public sparse_proxy_tag {};
    struct packed_tag: public packed_proxy_tag {};
    struct dense_proxy_tag: public packed_proxy_tag {};
    struct dense_tag: public dense_proxy_tag {};

    template<class S1, class S2>
    struct storage_restrict_traits {
        typedef S1 storage_category;
    };

    template<>
    struct storage_restrict_traits<sparse_tag, dense_proxy_tag> {
        typedef sparse_proxy_tag storage_category;
    };
    template<>
    struct storage_restrict_traits<sparse_tag, packed_proxy_tag> {
        typedef sparse_proxy_tag storage_category;
    };
    template<>
    struct storage_restrict_traits<sparse_tag, sparse_proxy_tag> {
        typedef sparse_proxy_tag storage_category;
    };

    template<>
    struct storage_restrict_traits<packed_tag, dense_proxy_tag> {
        typedef packed_proxy_tag storage_category;
    };
    template<>
    struct storage_restrict_traits<packed_tag, packed_proxy_tag> {
        typedef packed_proxy_tag storage_category;
    };
    template<>
    struct storage_restrict_traits<packed_tag, sparse_proxy_tag> {
        typedef sparse_proxy_tag storage_category;
    };

    template<>
    struct storage_restrict_traits<packed_proxy_tag, sparse_proxy_tag> {
        typedef sparse_proxy_tag storage_category;
    };

    template<>
    struct storage_restrict_traits<dense_tag, dense_proxy_tag> {
        typedef dense_proxy_tag storage_category;
    };
    template<>
    struct storage_restrict_traits<dense_tag, packed_proxy_tag> {
        typedef packed_proxy_tag storage_category;
    };
    template<>
    struct storage_restrict_traits<dense_tag, sparse_proxy_tag> {
        typedef sparse_proxy_tag storage_category;
    };

    template<>
    struct storage_restrict_traits<dense_proxy_tag, packed_proxy_tag> {
        typedef packed_proxy_tag storage_category;
    };
    template<>
    struct storage_restrict_traits<dense_proxy_tag, sparse_proxy_tag> {
        typedef sparse_proxy_tag storage_category;
    };


    // Iterator tags -- hierarchical definition of storage characteristics

    struct sparse_bidirectional_iterator_tag : public std::bidirectional_iterator_tag {};
    struct packed_random_access_iterator_tag : public std::random_access_iterator_tag {};
    struct dense_random_access_iterator_tag : public packed_random_access_iterator_tag {};

    // Thanks to Kresimir Fresl for convincing Comeau with iterator_base_traits ;-)
    template<class IC>
    struct iterator_base_traits {};

    template<>
    struct iterator_base_traits<std::forward_iterator_tag> {
        template<class I, class T>
        struct iterator_base {
            typedef forward_iterator_base<std::forward_iterator_tag, I, T> type;
        };
    };

    template<>
    struct iterator_base_traits<std::bidirectional_iterator_tag> {
        template<class I, class T>
        struct iterator_base {
            typedef bidirectional_iterator_base<std::bidirectional_iterator_tag, I, T> type;
        };
    };
    template<>
    struct iterator_base_traits<sparse_bidirectional_iterator_tag> {
        template<class I, class T>
        struct iterator_base {
            typedef bidirectional_iterator_base<sparse_bidirectional_iterator_tag, I, T> type;
        };
    };

    template<>
    struct iterator_base_traits<std::random_access_iterator_tag> {
        template<class I, class T>
        struct iterator_base {
            typedef random_access_iterator_base<std::random_access_iterator_tag, I, T> type;
        };
    };
    template<>
    struct iterator_base_traits<packed_random_access_iterator_tag> {
        template<class I, class T>
        struct iterator_base {
            typedef random_access_iterator_base<packed_random_access_iterator_tag, I, T> type;
        };
    };
    template<>
    struct iterator_base_traits<dense_random_access_iterator_tag> {
        template<class I, class T>
        struct iterator_base {
            typedef random_access_iterator_base<dense_random_access_iterator_tag, I, T> type;
        };
    };

    template<class I1, class I2>
    struct iterator_restrict_traits {
        typedef I1 iterator_category;
    };

    template<>
    struct iterator_restrict_traits<packed_random_access_iterator_tag, sparse_bidirectional_iterator_tag> {
        typedef sparse_bidirectional_iterator_tag iterator_category;
    };
    template<>
    struct iterator_restrict_traits<sparse_bidirectional_iterator_tag, packed_random_access_iterator_tag> {
        typedef sparse_bidirectional_iterator_tag iterator_category;
    };

    template<>
    struct iterator_restrict_traits<dense_random_access_iterator_tag, sparse_bidirectional_iterator_tag> {
        typedef sparse_bidirectional_iterator_tag iterator_category;
    };
    template<>
    struct iterator_restrict_traits<sparse_bidirectional_iterator_tag, dense_random_access_iterator_tag> {
        typedef sparse_bidirectional_iterator_tag iterator_category;
    };

    template<>
    struct iterator_restrict_traits<dense_random_access_iterator_tag, packed_random_access_iterator_tag> {
        typedef packed_random_access_iterator_tag iterator_category;
    };
    template<>
    struct iterator_restrict_traits<packed_random_access_iterator_tag, dense_random_access_iterator_tag> {
        typedef packed_random_access_iterator_tag iterator_category;
    };

    template<class I>
    BOOST_UBLAS_INLINE
    void increment (I &it, const I &it_end, typename I::difference_type compare, packed_random_access_iterator_tag) {
        it += (std::min) (compare, it_end - it);
    }
    template<class I>
    BOOST_UBLAS_INLINE
    void increment (I &it, const I &/* it_end */, typename I::difference_type /* compare */, sparse_bidirectional_iterator_tag) {
        ++ it;
    }
    template<class I>
    BOOST_UBLAS_INLINE
    void increment (I &it, const I &it_end, typename I::difference_type compare) {
        increment (it, it_end, compare, typename I::iterator_category ());
    }

    template<class I>
    BOOST_UBLAS_INLINE
    void increment (I &it, const I &it_end) {
#if BOOST_UBLAS_TYPE_CHECK
        I cit (it);
        while (cit != it_end) {
            BOOST_UBLAS_CHECK (*cit == typename I::value_type/*zero*/(), internal_logic ());
            ++ cit;
        }
#endif
        it = it_end;
    }

    namespace detail {

        // specialisation which define whether a type has a trivial constructor
        // or not. This is used by array types.
        template<typename T>
        struct has_trivial_constructor : public boost::has_trivial_constructor<T> {};

        template<typename T>
        struct has_trivial_destructor : public boost::has_trivial_destructor<T> {};

        template<typename FLT>
        struct has_trivial_constructor<std::complex<FLT> > : public has_trivial_constructor<FLT> {};
        
        template<typename FLT>
        struct has_trivial_destructor<std::complex<FLT> > : public has_trivial_destructor<FLT> {};

    }


    /**  \brief Traits class to extract type information from a constant matrix or vector CONTAINER.
     *
     */
    template < class E >
    struct container_view_traits {
        /// type of indices
        typedef typename E::size_type             size_type;
        /// type of differences of indices
        typedef typename E::difference_type       difference_type;

        /// storage category: \c unknown_storage_tag, \c dense_tag, \c packed_tag, ...
        typedef typename E::storage_category      storage_category;

        /// type of elements
        typedef typename E::value_type            value_type;
        /// const reference to an element
        typedef typename E::const_reference       const_reference;
  
        /// type used in expressions to mark a reference to this class (usually a const container_reference<const E> or the class itself)
        typedef typename E::const_closure_type    const_closure_type;
    };

    /**  \brief Traits class to extract additional type information from a mutable matrix or vector CONTAINER.
     *
     */
    template < class E >
    struct mutable_container_traits {
        /// reference to an element
        typedef typename E::reference             reference;
  
        /// type used in expressions to mark a reference to this class (usually a container_reference<E> or the class itself)
        typedef typename E::closure_type          closure_type;
    };

    /**  \brief Traits class to extract type information from a matrix or vector CONTAINER.
     *
     */
    template < class E >
    struct container_traits 
        : container_view_traits<E>, mutable_container_traits<E> {

    };


    /**  \brief Traits class to extract type information from a constant MATRIX.
     *
     */
    template < class MATRIX >
    struct matrix_view_traits : container_view_traits <MATRIX> {

        /// orientation of the matrix, either \c row_major_tag, \c column_major_tag or \c unknown_orientation_tag
        typedef typename MATRIX::orientation_category  orientation_category;
  
        /// row iterator for the matrix
        typedef typename MATRIX::const_iterator1  const_iterator1;

        /// column iterator for the matrix
        typedef typename MATRIX::const_iterator2  const_iterator2;
    };

    /**  \brief Traits class to extract additional type information from a mutable MATRIX.
     *
     */
    template < class MATRIX >
    struct mutable_matrix_traits 
        : mutable_container_traits <MATRIX> {

        /// row iterator for the matrix
        typedef typename MATRIX::iterator1  iterator1;

        /// column iterator for the matrix
        typedef typename MATRIX::iterator2  iterator2;
    };


    /**  \brief Traits class to extract type information from a MATRIX.
     *
     */
    template < class MATRIX >
    struct matrix_traits 
        : matrix_view_traits <MATRIX>, mutable_matrix_traits <MATRIX> {
    };

    /**  \brief Traits class to extract type information from a VECTOR.
     *
     */
    template < class VECTOR >
    struct vector_view_traits : container_view_traits <VECTOR> {

        /// iterator for the VECTOR
        typedef typename VECTOR::const_iterator  const_iterator;

        /// iterator pointing to the first element
        static
        const_iterator begin(const VECTOR & v) {
            return v.begin();
        }
        /// iterator pointing behind the last element
        static
        const_iterator end(const VECTOR & v) {
            return v.end();
        }

    };

    /**  \brief Traits class to extract type information from a VECTOR.
     *
     */
    template < class VECTOR >
    struct mutable_vector_traits : mutable_container_traits <VECTOR> {
        /// iterator for the VECTOR
        typedef typename VECTOR::iterator  iterator;

        /// iterator pointing to the first element
        static
        iterator begin(VECTOR & v) {
            return v.begin();
        }

        /// iterator pointing behind the last element
        static
        iterator end(VECTOR & v) {
            return v.end();
        }
    };

    /**  \brief Traits class to extract type information from a VECTOR.
     *
     */
    template < class VECTOR >
    struct vector_traits 
        : vector_view_traits <VECTOR>, mutable_vector_traits <VECTOR> {
    };


    // Note: specializations for T[N] and T[M][N] have been moved to traits/c_array.hpp

}}}

#endif

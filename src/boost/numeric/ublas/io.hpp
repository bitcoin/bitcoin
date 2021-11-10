//
//  Copyright (c) 2000-2010
//  Joerg Walter, Mathias Koch, David Bellot
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//  The authors gratefully acknowledge the support of
//  GeNeSys mbH & Co. KG in producing this work.
//

#ifndef _BOOST_UBLAS_IO_
#define _BOOST_UBLAS_IO_

// Only forward definition required to define stream operations
#include <iosfwd>
#include <sstream>
#include <boost/numeric/ublas/matrix_expression.hpp>


namespace boost { namespace numeric { namespace ublas {

    /** \brief output stream operator for vector expressions
     *
     * Any vector expressions can be written to a standard output stream
     * as defined in the C++ standard library. For example:
     * \code
     * vector<float> v1(3),v2(3);
     * for(size_t i=0; i<3; i++)
     * {
     *       v1(i) = i+0.2;
     *       v2(i) = i+0.3;
     * }
     * cout << v1+v2 << endl;
     * \endcode
     * will display the some of the 2 vectors like this:
     * \code
     * [3](0.5,2.5,4.5)
     * \endcode
     *
     * \param os is a standard basic output stream
     * \param v is a vector expression
     * \return a reference to the resulting output stream
     */
    template<class E, class T, class VE>
    // BOOST_UBLAS_INLINE This function seems to be big. So we do not let the compiler inline it.
    std::basic_ostream<E, T> &operator << (std::basic_ostream<E, T> &os,
                                           const vector_expression<VE> &v) {
        typedef typename VE::size_type size_type;
        size_type size = v ().size ();
        std::basic_ostringstream<E, T, std::allocator<E> > s;
        s.flags (os.flags ());
        s.imbue (os.getloc ());
        s.precision (os.precision ());
        s << '[' << size << "](";
        if (size > 0)
            s << v () (0);
        for (size_type i = 1; i < size; ++ i)
            s << ',' << v () (i);
        s << ')';
        return os << s.str ().c_str ();
    }

    /** \brief input stream operator for vectors
     *
     * This is used to feed in vectors with data stored as an ASCII representation
     * from a standard input stream.
     *
     * From a file or any valid stream, the format is: 
     * \c [<vector size>](<data1>,<data2>,...<dataN>) like for example:
     * \code
     * [5](1,2.1,3.2,3.14,0.2)
     * \endcode
     *
     * You can use it like this
     * \code
     * my_input_stream >> my_vector;
     * \endcode
     *
     * You can only put data into a valid \c vector<> not a \c vector_expression
     *
     * \param is is a standard basic input stream
     * \param v is a vector
     * \return a reference to the resulting input stream
     */
    template<class E, class T, class VT, class VA>
    // BOOST_UBLAS_INLINE This function seems to be big. So we do not let the compiler inline it.
    std::basic_istream<E, T> &operator >> (std::basic_istream<E, T> &is,
                                           vector<VT, VA> &v) {
        typedef typename vector<VT, VA>::size_type size_type;
        E ch;
        size_type size;
        if (is >> ch && ch != '[') {
            is.putback (ch);
            is.setstate (std::ios_base::failbit);
        } else if (is >> size >> ch && ch != ']') {
            is.putback (ch);
            is.setstate (std::ios_base::failbit);
        } else if (! is.fail ()) {
            vector<VT, VA> s (size);
            if (is >> ch && ch != '(') {
                is.putback (ch);
                is.setstate (std::ios_base::failbit);
            } else if (! is.fail ()) {
                for (size_type i = 0; i < size; i ++) {
                    if (is >> s (i) >> ch && ch != ',') {
                        is.putback (ch);
                        if (i < size - 1)
                            is.setstate (std::ios_base::failbit);
                        break;
                    }
                }
                if (is >> ch && ch != ')') {
                    is.putback (ch);
                    is.setstate (std::ios_base::failbit);
                }
            }
            if (! is.fail ())
                v.swap (s);
        }
        return is;
    }

    /** \brief output stream operator for matrix expressions
     *
     * it outpus the content of a \f$(M \times N)\f$ matrix to a standard output 
     * stream using the following format:
     * \c[<rows>,<columns>]((<m00>,<m01>,...,<m0N>),...,(<mM0>,<mM1>,...,<mMN>))
     *
     * For example:
     * \code
     * matrix<float> m(3,3) = scalar_matrix<float>(3,3,1.0) - diagonal_matrix<float>(3,3,1.0);
     * cout << m << endl;
     * \encode
     * will display
     * \code
     * [3,3]((0,1,1),(1,0,1),(1,1,0))
     * \endcode
     * This output is made for storing and retrieving matrices in a simple way but you can
     * easily recognize the following: 
     * \f[ \left( \begin{array}{ccc} 1 & 1 & 1\\ 1 & 1 & 1\\ 1 & 1 & 1 \end{array} \right) - \left( \begin{array}{ccc} 1 & 0 & 0\\ 0 & 1 & 0\\ 0 & 0 & 1 \end{array} \right) = \left( \begin{array}{ccc} 0 & 1 & 1\\ 1 & 0 & 1\\ 1 & 1 & 0 \end{array} \right) \f]
     *
     * \param os is a standard basic output stream
     * \param m is a matrix expression
     * \return a reference to the resulting output stream
     */
    template<class E, class T, class ME>
    // BOOST_UBLAS_INLINE This function seems to be big. So we do not let the compiler inline it.
    std::basic_ostream<E, T> &operator << (std::basic_ostream<E, T> &os,
                                           const matrix_expression<ME> &m) {
        typedef typename ME::size_type size_type;
        size_type size1 = m ().size1 ();
        size_type size2 = m ().size2 ();
        std::basic_ostringstream<E, T, std::allocator<E> > s;
        s.flags (os.flags ());
        s.imbue (os.getloc ());
        s.precision (os.precision ());
        s << '[' << size1 << ',' << size2 << "](";
        if (size1 > 0) {
            s << '(' ;
            if (size2 > 0)
                s << m () (0, 0);
            for (size_type j = 1; j < size2; ++ j)
                s << ',' << m () (0, j);
            s << ')';
        }
        for (size_type i = 1; i < size1; ++ i) {
            s << ",(" ;
            if (size2 > 0)
                s << m () (i, 0);
            for (size_type j = 1; j < size2; ++ j)
                s << ',' << m () (i, j);
            s << ')';
        }
        s << ')';
        return os << s.str ().c_str ();
    }

    /** \brief input stream operator for matrices
     *
     * This is used to feed in matrices with data stored as an ASCII representation
     * from a standard input stream.
     *
     * From a file or any valid standard stream, the format is:
     * \c[<rows>,<columns>]((<m00>,<m01>,...,<m0N>),...,(<mM0>,<mM1>,...,<mMN>))
     *
     * You can use it like this
     * \code
     * my_input_stream >> my_matrix;
     * \endcode
     *
     * You can only put data into a valid \c matrix<> not a \c matrix_expression
     *
     * \param is is a standard basic input stream
     * \param m is a matrix
     * \return a reference to the resulting input stream
     */
    template<class E, class T, class MT, class MF, class MA>
    // BOOST_UBLAS_INLINE This function seems to be big. So we do not let the compiler inline it.
    std::basic_istream<E, T> &operator >> (std::basic_istream<E, T> &is,
                                           matrix<MT, MF, MA> &m) {
        typedef typename matrix<MT, MF, MA>::size_type size_type;
        E ch;
        size_type size1, size2;
        if (is >> ch && ch != '[') {
            is.putback (ch);
            is.setstate (std::ios_base::failbit);
        } else if (is >> size1 >> ch && ch != ',') {
            is.putback (ch);
            is.setstate (std::ios_base::failbit);
        } else if (is >> size2 >> ch && ch != ']') {
            is.putback (ch);
            is.setstate (std::ios_base::failbit);
        } else if (! is.fail ()) {
            matrix<MT, MF, MA> s (size1, size2);
            if (is >> ch && ch != '(') {
                is.putback (ch);
                is.setstate (std::ios_base::failbit);
            } else if (! is.fail ()) {
                for (size_type i = 0; i < size1; i ++) {
                    if (is >> ch && ch != '(') {
                        is.putback (ch);
                        is.setstate (std::ios_base::failbit);
                        break;
                    }
                    for (size_type j = 0; j < size2; j ++) {
                        if (is >> s (i, j) >> ch && ch != ',') {
                            is.putback (ch);
                            if (j < size2 - 1) {
                                is.setstate (std::ios_base::failbit);
                                break;
                            }
                        }
                    }
                    if (is >> ch && ch != ')') {
                        is.putback (ch);
                        is.setstate (std::ios_base::failbit);
                        break;
                    }
                    if (is >> ch && ch != ',') {
                       is.putback (ch);
                       if (i < size1 - 1) {
                            is.setstate (std::ios_base::failbit);
                            break;
                       }
                    }
                }
                if (is >> ch && ch != ')') {
                    is.putback (ch);
                    is.setstate (std::ios_base::failbit);
                }
            }
            if (! is.fail ())
                m.swap (s);
        }
        return is;
    }

    /** \brief special input stream operator for symmetric matrices
     *
     * This is used to feed in symmetric matrices with data stored as an ASCII 
     * representation from a standard input stream.
     *
     * You can simply write your matrices in a file or any valid stream and read them again 
     * at a later time with this function. The format is the following:
     * \code [<rows>,<columns>]((<m00>,<m01>,...,<m0N>),...,(<mM0>,<mM1>,...,<mMN>)) \endcode
     *
     * You can use it like this
     * \code
     * my_input_stream >> my_symmetric_matrix;
     * \endcode
     *
     * You can only put data into a valid \c symmetric_matrix<>, not in a \c matrix_expression
     * This function also checks that input data form a valid symmetric matrix
     *
     * \param is is a standard basic input stream
     * \param m is a \c symmetric_matrix
     * \return a reference to the resulting input stream
     */
    template<class E, class T, class MT, class MF1, class MF2, class MA>
    // BOOST_UBLAS_INLINE This function seems to be big. So we do not let the compiler inline it.
    std::basic_istream<E, T> &operator >> (std::basic_istream<E, T> &is,
                                           symmetric_matrix<MT, MF1, MF2, MA> &m) {
        typedef typename symmetric_matrix<MT, MF1, MF2, MA>::size_type size_type;
        E ch;
        size_type size1, size2;
        MT value;
        if (is >> ch && ch != '[') {
            is.putback (ch);
            is.setstate (std::ios_base::failbit);
        } else if (is >> size1 >> ch && ch != ',') {
            is.putback (ch);
            is.setstate (std::ios_base::failbit);
        } else if (is >> size2 >> ch && (size2 != size1 || ch != ']')) { // symmetric matrix must be square
            is.putback (ch);
            is.setstate (std::ios_base::failbit);
        } else if (! is.fail ()) {
            symmetric_matrix<MT, MF1, MF2, MA> s (size1, size2);
            if (is >> ch && ch != '(') {
                is.putback (ch);
                is.setstate (std::ios_base::failbit);
             } else if (! is.fail ()) {
                for (size_type i = 0; i < size1; i ++) {
                    if (is >> ch && ch != '(') {
                        is.putback (ch);
                        is.setstate (std::ios_base::failbit);
                        break;
                    }
                    for (size_type j = 0; j < size2; j ++) {
                        if (is >> value >> ch && ch != ',') {
                            is.putback (ch);
                            if (j < size2 - 1) {
                                is.setstate (std::ios_base::failbit);
                                break;
                            }
                        }
                        if (i <= j) { 
                             // this is the first time we read this element - set the value
                            s(i,j) = value;
                        }
                        else if ( s(i,j) != value ) {
                            // matrix is not symmetric
                            is.setstate (std::ios_base::failbit);
                            break;
                        }
                     }
                     if (is >> ch && ch != ')') {
                         is.putback (ch);
                         is.setstate (std::ios_base::failbit);
                         break;
                     }
                     if (is >> ch && ch != ',') {
                        is.putback (ch);
                        if (i < size1 - 1) {
                             is.setstate (std::ios_base::failbit);
                             break;
                        }
                     }
                }
                if (is >> ch && ch != ')') {
                    is.putback (ch);
                    is.setstate (std::ios_base::failbit);
                }
            }
            if (! is.fail ())
                m.swap (s);
        }
        return is;
    }
 

}}}

#endif

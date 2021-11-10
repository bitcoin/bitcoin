// Boost.Range library concept checks
//
//  Copyright Neil Groves 2009. Use, modification and distribution
//  are subject to the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//  Copyright Daniel Walker 2006. Use, modification and distribution
//  are subject to the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org/libs/range/
//

#ifndef BOOST_RANGE_CONCEPTS_HPP
#define BOOST_RANGE_CONCEPTS_HPP

#include <boost/concept_check.hpp>
#include <boost/iterator/iterator_concepts.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/range/iterator.hpp>
#include <boost/range/value_type.hpp>
#include <boost/range/detail/misc_concept.hpp>
#include <boost/type_traits/remove_reference.hpp>

#include <iterator>

/*!
 * \file
 * \brief Concept checks for the Boost Range library.
 *
 * The structures in this file may be used in conjunction with the
 * Boost Concept Check library to insure that the type of a function
 * parameter is compatible with a range concept. If not, a meaningful
 * compile time error is generated. Checks are provided for the range
 * concepts related to iterator traversal categories. For example, the
 * following line checks that the type T models the ForwardRange
 * concept.
 *
 * \code
 * BOOST_CONCEPT_ASSERT((ForwardRangeConcept<T>));
 * \endcode
 *
 * A different concept check is required to ensure writeable value
 * access. For example to check for a ForwardRange that can be written
 * to, the following code is required.
 *
 * \code
 * BOOST_CONCEPT_ASSERT((WriteableForwardRangeConcept<T>));
 * \endcode
 *
 * \see http://www.boost.org/libs/range/doc/range.html for details
 * about range concepts.
 * \see http://www.boost.org/libs/iterator/doc/iterator_concepts.html
 * for details about iterator concepts.
 * \see http://www.boost.org/libs/concept_check/concept_check.htm for
 * details about concept checks.
 */

namespace boost {

    namespace range_detail {

#ifndef BOOST_RANGE_ENABLE_CONCEPT_ASSERT

// List broken compiler versions here:
#ifndef __clang__
    #ifdef __GNUC__
        // GNUC 4.2 has strange issues correctly detecting compliance with the Concepts
        // hence the least disruptive approach is to turn-off the concept checking for
        // this version of the compiler.
        #if __GNUC__ == 4 && __GNUC_MINOR__ == 2
            #define BOOST_RANGE_ENABLE_CONCEPT_ASSERT 0
        #endif
    #endif

    #ifdef __GCCXML__
        // GCC XML, unsurprisingly, has the same issues
        #if __GCCXML_GNUC__ == 4 && __GCCXML_GNUC_MINOR__ == 2
            #define BOOST_RANGE_ENABLE_CONCEPT_ASSERT 0
        #endif
    #endif
#endif

    #ifdef BOOST_BORLANDC
        #define BOOST_RANGE_ENABLE_CONCEPT_ASSERT 0
    #endif

    #ifdef __PATHCC__
        #define BOOST_RANGE_ENABLE_CONCEPT_ASSERT 0
    #endif

// Default to using the concept asserts unless we have defined it off
// during the search for black listed compilers.
    #ifndef BOOST_RANGE_ENABLE_CONCEPT_ASSERT
        #define BOOST_RANGE_ENABLE_CONCEPT_ASSERT 1
    #endif

#endif

#if BOOST_RANGE_ENABLE_CONCEPT_ASSERT
    #define BOOST_RANGE_CONCEPT_ASSERT( x ) BOOST_CONCEPT_ASSERT( x )
#else
    #define BOOST_RANGE_CONCEPT_ASSERT( x )
#endif

        // Rationale for the inclusion of redefined iterator concept
        // classes:
        //
        // The Range algorithms often do not require that the iterators are
        // Assignable or default constructable, but the correct standard
        // conformant iterators do require the iterators to be a model of the
        // Assignable concept.
        // Iterators that contains a functor that is not assignable therefore
        // are not correct models of the standard iterator concepts,
        // despite being adequate for most algorithms. An example of this
        // use case is the combination of the boost::adaptors::filtered
        // class with a boost::lambda::bind generated functor.
        // Ultimately modeling the range concepts using composition
        // with the Boost.Iterator concepts would render the library
        // incompatible with many common Boost.Lambda expressions.
        template<class Iterator>
        struct IncrementableIteratorConcept : CopyConstructible<Iterator>
        {
#if BOOST_RANGE_ENABLE_CONCEPT_ASSERT
            typedef BOOST_DEDUCED_TYPENAME iterator_traversal<Iterator>::type traversal_category;

            BOOST_RANGE_CONCEPT_ASSERT((
                Convertible<
                    traversal_category,
                    incrementable_traversal_tag
                >));

            BOOST_CONCEPT_USAGE(IncrementableIteratorConcept)
            {
                ++i;
                (void)i++;
            }
        private:
            Iterator i;
#endif
        };

        template<class Iterator>
        struct SinglePassIteratorConcept
            : IncrementableIteratorConcept<Iterator>
            , EqualityComparable<Iterator>
        {
#if BOOST_RANGE_ENABLE_CONCEPT_ASSERT
            BOOST_RANGE_CONCEPT_ASSERT((
                Convertible<
                    BOOST_DEDUCED_TYPENAME SinglePassIteratorConcept::traversal_category,
                    single_pass_traversal_tag
                >));

            BOOST_CONCEPT_USAGE(SinglePassIteratorConcept)
            {
                Iterator i2(++i);
                boost::ignore_unused_variable_warning(i2);

                // deliberately we are loose with the postfix version for the single pass
                // iterator due to the commonly poor adherence to the specification means that
                // many algorithms would be unusable, whereas actually without the check they
                // work
                (void)(i++);

                BOOST_DEDUCED_TYPENAME std::iterator_traits<Iterator>::reference r1(*i);
                boost::ignore_unused_variable_warning(r1);

                BOOST_DEDUCED_TYPENAME std::iterator_traits<Iterator>::reference r2(*(++i));
                boost::ignore_unused_variable_warning(r2);
            }
        private:
            Iterator i;
#endif
        };

        template<class Iterator>
        struct ForwardIteratorConcept
            : SinglePassIteratorConcept<Iterator>
            , DefaultConstructible<Iterator>
        {
#if BOOST_RANGE_ENABLE_CONCEPT_ASSERT
            typedef BOOST_DEDUCED_TYPENAME std::iterator_traits<Iterator>::difference_type difference_type;

            BOOST_MPL_ASSERT((is_integral<difference_type>));
            BOOST_MPL_ASSERT_RELATION(std::numeric_limits<difference_type>::is_signed, ==, true);

            BOOST_RANGE_CONCEPT_ASSERT((
                Convertible<
                    BOOST_DEDUCED_TYPENAME ForwardIteratorConcept::traversal_category,
                    forward_traversal_tag
                >));

            BOOST_CONCEPT_USAGE(ForwardIteratorConcept)
            {
                // See the above note in the SinglePassIteratorConcept about the handling of the
                // postfix increment. Since with forward and better iterators there is no need
                // for a proxy, we can sensibly require that the dereference result
                // is convertible to reference.
                Iterator i2(i++);
                boost::ignore_unused_variable_warning(i2);
                BOOST_DEDUCED_TYPENAME std::iterator_traits<Iterator>::reference r(*(i++));
                boost::ignore_unused_variable_warning(r);
            }
        private:
            Iterator i;
#endif
         };

         template<class Iterator>
         struct BidirectionalIteratorConcept
             : ForwardIteratorConcept<Iterator>
         {
 #if BOOST_RANGE_ENABLE_CONCEPT_ASSERT
             BOOST_RANGE_CONCEPT_ASSERT((
                 Convertible<
                     BOOST_DEDUCED_TYPENAME BidirectionalIteratorConcept::traversal_category,
                     bidirectional_traversal_tag
                 >));

             BOOST_CONCEPT_USAGE(BidirectionalIteratorConcept)
             {
                 --i;
                 (void)i--;
             }
         private:
             Iterator i;
 #endif
         };

         template<class Iterator>
         struct RandomAccessIteratorConcept
             : BidirectionalIteratorConcept<Iterator>
         {
 #if BOOST_RANGE_ENABLE_CONCEPT_ASSERT
             BOOST_RANGE_CONCEPT_ASSERT((
                 Convertible<
                     BOOST_DEDUCED_TYPENAME RandomAccessIteratorConcept::traversal_category,
                     random_access_traversal_tag
                 >));

             BOOST_CONCEPT_USAGE(RandomAccessIteratorConcept)
             {
                 i += n;
                 i = i + n;
                 i = n + i;
                 i -= n;
                 i = i - n;
                 n = i - j;
             }
         private:
             BOOST_DEDUCED_TYPENAME BidirectionalIteratorConcept<Iterator>::difference_type n;
             Iterator i;
             Iterator j;
 #endif
         };

    } // namespace range_detail

    //! Check if a type T models the SinglePassRange range concept.
    template<class T>
    struct SinglePassRangeConcept
    {
#if BOOST_RANGE_ENABLE_CONCEPT_ASSERT
        // A few compilers don't like the rvalue reference T types so just
        // remove it.
        typedef BOOST_DEDUCED_TYPENAME remove_reference<T>::type Rng;

        typedef BOOST_DEDUCED_TYPENAME range_iterator<
            Rng const
        >::type const_iterator;

        typedef BOOST_DEDUCED_TYPENAME range_iterator<Rng>::type iterator;

        BOOST_RANGE_CONCEPT_ASSERT((
                range_detail::SinglePassIteratorConcept<iterator>));

        BOOST_RANGE_CONCEPT_ASSERT((
                range_detail::SinglePassIteratorConcept<const_iterator>));

        BOOST_CONCEPT_USAGE(SinglePassRangeConcept)
        {
            // This has been modified from assigning to this->i
            // (where i was a member variable) to improve
            // compatibility with Boost.Lambda
            iterator i1 = boost::begin(*m_range);
            iterator i2 = boost::end(*m_range);

            boost::ignore_unused_variable_warning(i1);
            boost::ignore_unused_variable_warning(i2);

            const_constraints(*m_range);
        }

    private:
        void const_constraints(const Rng& const_range)
        {
            const_iterator ci1 = boost::begin(const_range);
            const_iterator ci2 = boost::end(const_range);

            boost::ignore_unused_variable_warning(ci1);
            boost::ignore_unused_variable_warning(ci2);
        }

       // Rationale:
       // The type of m_range is T* rather than T because it allows
       // T to be an abstract class. The other obvious alternative of
       // T& produces a warning on some compilers.
       Rng* m_range;
#endif
    };

    //! Check if a type T models the ForwardRange range concept.
    template<class T>
    struct ForwardRangeConcept : SinglePassRangeConcept<T>
    {
#if BOOST_RANGE_ENABLE_CONCEPT_ASSERT
        BOOST_RANGE_CONCEPT_ASSERT((range_detail::ForwardIteratorConcept<BOOST_DEDUCED_TYPENAME ForwardRangeConcept::iterator>));
        BOOST_RANGE_CONCEPT_ASSERT((range_detail::ForwardIteratorConcept<BOOST_DEDUCED_TYPENAME ForwardRangeConcept::const_iterator>));
#endif
    };

    template<class T>
    struct WriteableRangeConcept
    {
#if BOOST_RANGE_ENABLE_CONCEPT_ASSERT
        typedef BOOST_DEDUCED_TYPENAME range_iterator<T>::type iterator;

        BOOST_CONCEPT_USAGE(WriteableRangeConcept)
        {
            *i = v;
        }
    private:
        iterator i;
        BOOST_DEDUCED_TYPENAME range_value<T>::type v;
#endif
    };

    //! Check if a type T models the WriteableForwardRange range concept.
    template<class T>
    struct WriteableForwardRangeConcept
        : ForwardRangeConcept<T>
        , WriteableRangeConcept<T>
    {
    };

    //! Check if a type T models the BidirectionalRange range concept.
    template<class T>
    struct BidirectionalRangeConcept : ForwardRangeConcept<T>
    {
#if BOOST_RANGE_ENABLE_CONCEPT_ASSERT
        BOOST_RANGE_CONCEPT_ASSERT((range_detail::BidirectionalIteratorConcept<BOOST_DEDUCED_TYPENAME BidirectionalRangeConcept::iterator>));
        BOOST_RANGE_CONCEPT_ASSERT((range_detail::BidirectionalIteratorConcept<BOOST_DEDUCED_TYPENAME BidirectionalRangeConcept::const_iterator>));
#endif
    };

    //! Check if a type T models the WriteableBidirectionalRange range concept.
    template<class T>
    struct WriteableBidirectionalRangeConcept
        : BidirectionalRangeConcept<T>
        , WriteableRangeConcept<T>
    {
    };

    //! Check if a type T models the RandomAccessRange range concept.
    template<class T>
    struct RandomAccessRangeConcept : BidirectionalRangeConcept<T>
    {
#if BOOST_RANGE_ENABLE_CONCEPT_ASSERT
        BOOST_RANGE_CONCEPT_ASSERT((range_detail::RandomAccessIteratorConcept<BOOST_DEDUCED_TYPENAME RandomAccessRangeConcept::iterator>));
        BOOST_RANGE_CONCEPT_ASSERT((range_detail::RandomAccessIteratorConcept<BOOST_DEDUCED_TYPENAME RandomAccessRangeConcept::const_iterator>));
#endif
    };

    //! Check if a type T models the WriteableRandomAccessRange range concept.
    template<class T>
    struct WriteableRandomAccessRangeConcept
        : RandomAccessRangeConcept<T>
        , WriteableRangeConcept<T>
    {
    };

} // namespace boost

#endif // BOOST_RANGE_CONCEPTS_HPP

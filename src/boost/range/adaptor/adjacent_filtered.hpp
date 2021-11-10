// Boost.Range library
//
//  Copyright Thorsten Ottosen, Neil Groves 2006 - 2008. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org/libs/range/
//

#ifndef BOOST_RANGE_ADAPTOR_ADJACENT_FILTER_IMPL_HPP
#define BOOST_RANGE_ADAPTOR_ADJACENT_FILTER_IMPL_HPP

#include <boost/config.hpp>
#ifdef BOOST_MSVC
#pragma warning( push )
#pragma warning( disable : 4355 )
#endif

#include <boost/range/adaptor/argument_fwd.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/range/concepts.hpp>
#include <boost/iterator/iterator_adaptor.hpp>
#include <boost/next_prior.hpp>


namespace boost
{
    namespace range_detail
    {
        template< class Iter, class Pred, bool default_pass >
        class skip_iterator
          : public boost::iterator_adaptor<
                    skip_iterator<Iter,Pred,default_pass>,
                    Iter,
                    BOOST_DEDUCED_TYPENAME std::iterator_traits<Iter>::value_type,
                    boost::forward_traversal_tag,
                    BOOST_DEDUCED_TYPENAME std::iterator_traits<Iter>::reference,
                    BOOST_DEDUCED_TYPENAME std::iterator_traits<Iter>::difference_type
                >
          , private Pred
        {
        private:
            typedef boost::iterator_adaptor<
                        skip_iterator<Iter,Pred,default_pass>,
                        Iter,
                        BOOST_DEDUCED_TYPENAME std::iterator_traits<Iter>::value_type,
                        boost::forward_traversal_tag,
                        BOOST_DEDUCED_TYPENAME std::iterator_traits<Iter>::reference,
                        BOOST_DEDUCED_TYPENAME std::iterator_traits<Iter>::difference_type
                    > base_t;

        public:
            typedef Pred pred_t;
            typedef Iter iter_t;

            skip_iterator() : m_last() {}

            skip_iterator(iter_t it, iter_t last, const Pred& pred)
                : base_t(it)
                , pred_t(pred)
                , m_last(last)
            {
            }

            template<class OtherIter>
            skip_iterator( const skip_iterator<OtherIter, pred_t, default_pass>& other )
            : base_t(other.base())
            , pred_t(other)
            , m_last(other.m_last)
            {
            }

            void increment()
            {
                iter_t& it = this->base_reference();
                BOOST_ASSERT( it != m_last );
                pred_t& bi_pred = *this;
                iter_t prev = it;
                ++it;
                if (it != m_last)
                {
                    if (default_pass)
                    {
                        while (it != m_last && !bi_pred(*prev, *it))
                        {
                            ++it;
                            ++prev;
                        }
                    }
                    else
                    {
                        for (; it != m_last; ++it, ++prev)
                        {
                            if (bi_pred(*prev, *it))
                            {
                                break;
                            }
                        }
                    }
                }
            }

            iter_t m_last;
        };

        template< class P, class R, bool default_pass >
        struct adjacent_filtered_range
            : iterator_range< skip_iterator<
                                BOOST_DEDUCED_TYPENAME range_iterator<R>::type,
                                P,
                                default_pass
                            >
                        >
        {
        private:
            typedef skip_iterator<
                        BOOST_DEDUCED_TYPENAME range_iterator<R>::type,
                        P,
                        default_pass
                     >
                skip_iter;

            typedef iterator_range<skip_iter>
                base_range;

            typedef BOOST_DEDUCED_TYPENAME range_iterator<R>::type raw_iterator;

        public:
            adjacent_filtered_range( const P& p, R& r )
            : base_range(skip_iter(boost::begin(r), boost::end(r), p),
                         skip_iter(boost::end(r), boost::end(r), p))
            {
            }
        };

        template< class T >
        struct adjacent_holder : holder<T>
        {
            adjacent_holder( T r ) : holder<T>(r)
            { }
        };

        template< class T >
        struct adjacent_excl_holder : holder<T>
        {
            adjacent_excl_holder( T r ) : holder<T>(r)
            { }
        };

        template< class ForwardRng, class BinPredicate >
        inline adjacent_filtered_range<BinPredicate, ForwardRng, true>
        operator|( ForwardRng& r,
                   const adjacent_holder<BinPredicate>& f )
        {
            BOOST_RANGE_CONCEPT_ASSERT((ForwardRangeConcept<ForwardRng>));

            return adjacent_filtered_range<BinPredicate, ForwardRng, true>( f.val, r );
        }

        template< class ForwardRng, class BinPredicate >
        inline adjacent_filtered_range<BinPredicate, const ForwardRng, true>
        operator|( const ForwardRng& r,
                   const adjacent_holder<BinPredicate>& f )
        {
            BOOST_RANGE_CONCEPT_ASSERT((ForwardRangeConcept<const ForwardRng>));

            return adjacent_filtered_range<BinPredicate,
                                           const ForwardRng, true>( f.val, r );
        }

        template< class ForwardRng, class BinPredicate >
        inline adjacent_filtered_range<BinPredicate, ForwardRng, false>
        operator|( ForwardRng& r,
                   const adjacent_excl_holder<BinPredicate>& f )
        {
            BOOST_RANGE_CONCEPT_ASSERT((ForwardRangeConcept<ForwardRng>));
            return adjacent_filtered_range<BinPredicate, ForwardRng, false>( f.val, r );
        }

        template< class ForwardRng, class BinPredicate >
        inline adjacent_filtered_range<BinPredicate, const ForwardRng, false>
        operator|( const ForwardRng& r,
                   const adjacent_excl_holder<BinPredicate>& f )
        {
            BOOST_RANGE_CONCEPT_ASSERT((ForwardRangeConcept<const ForwardRng>));
            return adjacent_filtered_range<BinPredicate,
                                           const ForwardRng, false>( f.val, r );
        }

    } // 'range_detail'

    // Bring adjacent_filter_range into the boost namespace so that users of
    // this library may specify the return type of the '|' operator and
    // adjacent_filter()
    using range_detail::adjacent_filtered_range;

    namespace adaptors
    {
        namespace
        {
            const range_detail::forwarder<range_detail::adjacent_holder>
                adjacent_filtered =
                   range_detail::forwarder<range_detail::adjacent_holder>();

            const range_detail::forwarder<range_detail::adjacent_excl_holder>
                adjacent_filtered_excl =
                    range_detail::forwarder<range_detail::adjacent_excl_holder>();
        }

        template<class ForwardRng, class BinPredicate>
        inline adjacent_filtered_range<BinPredicate, ForwardRng, true>
        adjacent_filter(ForwardRng& rng, BinPredicate filter_pred)
        {
            BOOST_RANGE_CONCEPT_ASSERT((ForwardRangeConcept<ForwardRng>));
            return adjacent_filtered_range<BinPredicate, ForwardRng, true>(filter_pred, rng);
        }

        template<class ForwardRng, class BinPredicate>
        inline adjacent_filtered_range<BinPredicate, const ForwardRng, true>
        adjacent_filter(const ForwardRng& rng, BinPredicate filter_pred)
        {
            BOOST_RANGE_CONCEPT_ASSERT((ForwardRangeConcept<const ForwardRng>));
            return adjacent_filtered_range<BinPredicate, const ForwardRng, true>(filter_pred, rng);
        }

    } // 'adaptors'

}

#ifdef BOOST_MSVC
#pragma warning( pop )
#endif

#endif

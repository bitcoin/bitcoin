// Boost.Range library
//
//  Copyright Neil Groves 2007. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//
// For more information, see http://www.boost.org/libs/range/
//
#ifndef BOOST_RANGE_ADAPTOR_STRIDED_HPP_INCLUDED
#define BOOST_RANGE_ADAPTOR_STRIDED_HPP_INCLUDED

#include <boost/range/adaptor/argument_fwd.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <iterator>

namespace boost
{
    namespace range_detail
    {
        // strided_iterator for wrapping a forward traversal iterator
        template<class BaseIterator, class Category>
        class strided_iterator
            : public iterator_facade<
                strided_iterator<BaseIterator, Category>
                , typename iterator_value<BaseIterator>::type
                , forward_traversal_tag
                , typename iterator_reference<BaseIterator>::type
                , typename iterator_difference<BaseIterator>::type
            >
        {
            friend class ::boost::iterator_core_access;

            typedef iterator_facade<
                strided_iterator<BaseIterator, Category>
                , typename iterator_value<BaseIterator>::type
                , forward_traversal_tag
                , typename iterator_reference<BaseIterator>::type
                , typename iterator_difference<BaseIterator>::type
            > super_t;

        public:
            typedef typename super_t::difference_type difference_type;
            typedef typename super_t::reference reference;
            typedef BaseIterator base_iterator;
            typedef std::forward_iterator_tag iterator_category;

            strided_iterator()
                : m_it()
                , m_last()
                , m_stride()
            {
            }

            strided_iterator(base_iterator   it,
                             base_iterator   last,
                             difference_type stride)
                : m_it(it)
                , m_last(last)
                , m_stride(stride)
            {
            }

            template<class OtherIterator>
            strided_iterator(
                const strided_iterator<OtherIterator, Category>& other,
                typename enable_if_convertible<
                    OtherIterator,
                    base_iterator
                >::type* = 0
            )
                : m_it(other.base())
                , m_last(other.base_end())
                , m_stride(other.get_stride())
            {
            }

            base_iterator base() const
            {
                return m_it;
            }

            base_iterator base_end() const
            {
                return m_last;
            }

            difference_type get_stride() const
            {
                return m_stride;
            }

        private:
            void increment()
            {
                for (difference_type i = 0;
                        (m_it != m_last) && (i < m_stride); ++i)
                {
                    ++m_it;
                }
            }

            reference dereference() const
            {
                return *m_it;
            }

            template<class OtherIterator>
            bool equal(
                const strided_iterator<OtherIterator, Category>& other,
                typename enable_if_convertible<
                    OtherIterator,
                    base_iterator
                >::type* = 0) const
            {
                return m_it == other.m_it;
            }

            base_iterator m_it;
            base_iterator m_last;
            difference_type m_stride;
        };

        // strided_iterator for wrapping a bidirectional iterator
        template<class BaseIterator>
        class strided_iterator<BaseIterator, bidirectional_traversal_tag>
            : public iterator_facade<
                strided_iterator<BaseIterator, bidirectional_traversal_tag>
                , typename iterator_value<BaseIterator>::type
                , bidirectional_traversal_tag
                , typename iterator_reference<BaseIterator>::type
                , typename iterator_difference<BaseIterator>::type
            >
        {
            friend class ::boost::iterator_core_access;

            typedef iterator_facade<
                strided_iterator<BaseIterator, bidirectional_traversal_tag>
                , typename iterator_value<BaseIterator>::type
                , bidirectional_traversal_tag
                , typename iterator_reference<BaseIterator>::type
                , typename iterator_difference<BaseIterator>::type
            > super_t;
        public:
            typedef typename super_t::difference_type difference_type;
            typedef typename super_t::reference reference;
            typedef BaseIterator base_iterator;
            typedef typename boost::make_unsigned<difference_type>::type
                        size_type;
            typedef std::bidirectional_iterator_tag iterator_category;

            strided_iterator()
                : m_it()
                , m_offset()
                , m_index()
                , m_stride()
            {
            }

            strided_iterator(base_iterator   it,
                             size_type       index,
                             difference_type stride)
                : m_it(it)
                , m_offset()
                , m_index(index)
                , m_stride(stride)
            {
                if (stride && ((m_index % stride) != 0))
                    m_index += (stride - (m_index % stride));
            }

            template<class OtherIterator>
            strided_iterator(
                const strided_iterator<
                    OtherIterator,
                    bidirectional_traversal_tag
                >& other,
                typename enable_if_convertible<
                    OtherIterator,
                    base_iterator
                >::type* = 0
            )
                : m_it(other.base())
                , m_offset(other.get_offset())
                , m_index(other.get_index())
                , m_stride(other.get_stride())
            {
            }

            base_iterator base() const
            {
                return m_it;
            }

            difference_type get_offset() const
            {
                return m_offset;
            }

            size_type get_index() const
            {
                return m_index;
            }

            difference_type get_stride() const
            {
                return m_stride;
            }

        private:
            void increment()
            {
                m_offset += m_stride;
            }

            void decrement()
            {
                m_offset -= m_stride;
            }

            reference dereference() const
            {
                update();
                return *m_it;
            }

            void update() const
            {
                std::advance(m_it, m_offset);
                m_index += m_offset;
                m_offset = 0;
            }

            template<class OtherIterator>
            bool equal(
                const strided_iterator<
                    OtherIterator,
                    bidirectional_traversal_tag
                >& other,
                typename enable_if_convertible<
                    OtherIterator,
                    base_iterator
                >::type* = 0) const
            {
                return (m_index + m_offset) ==
                            (other.get_index() + other.get_offset());
            }

            mutable base_iterator m_it;
            mutable difference_type m_offset;
            mutable size_type m_index;
            difference_type m_stride;
        };

        // strided_iterator implementation for wrapping a random access iterator
        template<class BaseIterator>
        class strided_iterator<BaseIterator, random_access_traversal_tag>
            : public iterator_facade<
                strided_iterator<BaseIterator, random_access_traversal_tag>
                , typename iterator_value<BaseIterator>::type
                , random_access_traversal_tag
                , typename iterator_reference<BaseIterator>::type
                , typename iterator_difference<BaseIterator>::type
            >
        {
            friend class ::boost::iterator_core_access;

            typedef iterator_facade<
                strided_iterator<BaseIterator, random_access_traversal_tag>
                , typename iterator_value<BaseIterator>::type
                , random_access_traversal_tag
                , typename iterator_reference<BaseIterator>::type
                , typename iterator_difference<BaseIterator>::type
            > super_t;
        public:
            typedef typename super_t::difference_type difference_type;
            typedef typename super_t::reference reference;
            typedef BaseIterator base_iterator;
            typedef std::random_access_iterator_tag iterator_category;

            strided_iterator()
                : m_it()
                , m_first()
                , m_index(0)
                , m_stride()
            {
            }

            strided_iterator(
                base_iterator   first,
                base_iterator   it,
                difference_type stride
            )
                : m_it(it)
                , m_first(first)
                , m_index(stride ? (it - first) : difference_type())
                , m_stride(stride)
            {
                if (stride && ((m_index % stride) != 0))
                    m_index += (stride - (m_index % stride));
            }

            template<class OtherIterator>
            strided_iterator(
                const strided_iterator<
                    OtherIterator,
                    random_access_traversal_tag
                >& other,
                typename enable_if_convertible<
                    OtherIterator,
                    base_iterator
                >::type* = 0
            )
                : m_it(other.base())
                , m_first(other.base_begin())
                , m_index(other.get_index())
                , m_stride(other.get_stride())
            {
            }

            base_iterator base_begin() const
            {
                return m_first;
            }

            base_iterator base() const
            {
                return m_it;
            }

            difference_type get_stride() const
            {
                return m_stride;
            }

            difference_type get_index() const
            {
                return m_index;
            }

        private:
            void increment()
            {
                m_index += m_stride;
            }

            void decrement()
            {
                m_index -= m_stride;
            }

            void advance(difference_type offset)
            {
                m_index += (m_stride * offset);
            }

            // Implementation detail: only update the actual underlying iterator
            // at the point of dereference. This is done so that the increment
            // and decrement can overshoot the valid sequence as is required
            // by striding. Since we can do all comparisons just with the index
            // simply, and all dereferences must be within the valid range.
            void update() const
            {
                m_it = m_first + m_index;
            }

            template<class OtherIterator>
            difference_type distance_to(
                const strided_iterator<
                    OtherIterator,
                    random_access_traversal_tag
                >& other,
                typename enable_if_convertible<
                            OtherIterator, base_iterator>::type* = 0) const
            {
                BOOST_ASSERT((other.m_index - m_index) % m_stride == difference_type());
                return (other.m_index - m_index) / m_stride;
            }

            template<class OtherIterator>
            bool equal(
                const strided_iterator<
                    OtherIterator,
                    random_access_traversal_tag
                >& other,
                typename enable_if_convertible<
                            OtherIterator, base_iterator>::type* = 0) const
            {
                return m_index == other.m_index;
            }

            reference dereference() const
            {
                update();
                return *m_it;
            }

        private:
            mutable base_iterator m_it;
            base_iterator m_first;
            difference_type m_index;
            difference_type m_stride;
        };

        template<class Rng, class Difference> inline
        strided_iterator<
            typename range_iterator<Rng>::type,
            forward_traversal_tag
        >
        make_begin_strided_iterator(
            Rng& rng,
            Difference stride,
            forward_traversal_tag)
        {
            return strided_iterator<
                typename range_iterator<Rng>::type,
                forward_traversal_tag
            >(boost::begin(rng), boost::end(rng), stride);
        }

        template<class Rng, class Difference> inline
        strided_iterator<
            typename range_iterator<const Rng>::type,
            forward_traversal_tag
        >
        make_begin_strided_iterator(
            const Rng& rng,
            Difference stride,
            forward_traversal_tag)
        {
            return strided_iterator<
                typename range_iterator<const Rng>::type,
                forward_traversal_tag
            >(boost::begin(rng), boost::end(rng), stride);
        }

        template<class Rng, class Difference> inline
        strided_iterator<
            typename range_iterator<Rng>::type,
            forward_traversal_tag
        >
        make_end_strided_iterator(
            Rng& rng,
            Difference stride,
            forward_traversal_tag)
        {
            return strided_iterator<
                typename range_iterator<Rng>::type,
                forward_traversal_tag
            >(boost::end(rng), boost::end(rng), stride);
        }

        template<class Rng, class Difference> inline
        strided_iterator<
            typename range_iterator<const Rng>::type,
            forward_traversal_tag
        >
        make_end_strided_iterator(
            const Rng& rng,
            Difference stride,
            forward_traversal_tag)
        {
            return strided_iterator<
                typename range_iterator<const Rng>::type,
                forward_traversal_tag
            >(boost::end(rng), boost::end(rng), stride);
        }

        template<class Rng, class Difference> inline
        strided_iterator<
            typename range_iterator<Rng>::type,
            bidirectional_traversal_tag
        >
        make_begin_strided_iterator(
            Rng& rng,
            Difference stride,
            bidirectional_traversal_tag)
        {
            typedef typename range_difference<Rng>::type difference_type;

            return strided_iterator<
                typename range_iterator<Rng>::type,
                bidirectional_traversal_tag
            >(boost::begin(rng), difference_type(), stride);
        }

        template<class Rng, class Difference> inline
        strided_iterator<
            typename range_iterator<const Rng>::type,
            bidirectional_traversal_tag
        >
        make_begin_strided_iterator(
            const Rng& rng,
            Difference stride,
            bidirectional_traversal_tag)
        {
            typedef typename range_difference<const Rng>::type difference_type;

            return strided_iterator<
                typename range_iterator<const Rng>::type,
                bidirectional_traversal_tag
            >(boost::begin(rng), difference_type(), stride);
        }

        template<class Rng, class Difference> inline
        strided_iterator<
            typename range_iterator<Rng>::type,
            bidirectional_traversal_tag
        >
        make_end_strided_iterator(
            Rng& rng,
            Difference stride,
            bidirectional_traversal_tag)
        {
            return strided_iterator<
                typename range_iterator<Rng>::type,
                bidirectional_traversal_tag
            >(boost::end(rng), boost::size(rng), stride);
        }

        template<class Rng, class Difference> inline
        strided_iterator<
            typename range_iterator<const Rng>::type,
            bidirectional_traversal_tag
        >
        make_end_strided_iterator(
            const Rng& rng,
            Difference stride,
            bidirectional_traversal_tag)
        {
            return strided_iterator<
                typename range_iterator<const Rng>::type,
                bidirectional_traversal_tag
            >(boost::end(rng), boost::size(rng), stride);
        }

        template<class Rng, class Difference> inline
        strided_iterator<
            typename range_iterator<Rng>::type,
            random_access_traversal_tag
        >
        make_begin_strided_iterator(
            Rng& rng,
            Difference stride,
            random_access_traversal_tag)
        {
            return strided_iterator<
                typename range_iterator<Rng>::type,
                random_access_traversal_tag
            >(boost::begin(rng), boost::begin(rng), stride);
        }

        template<class Rng, class Difference> inline
        strided_iterator<
            typename range_iterator<const Rng>::type,
            random_access_traversal_tag
        >
        make_begin_strided_iterator(
            const Rng& rng,
            Difference stride,
            random_access_traversal_tag)
        {
            return strided_iterator<
                typename range_iterator<const Rng>::type,
                random_access_traversal_tag
            >(boost::begin(rng), boost::begin(rng), stride);
        }

        template<class Rng, class Difference> inline
        strided_iterator<
            typename range_iterator<Rng>::type,
            random_access_traversal_tag
        >
        make_end_strided_iterator(
            Rng& rng,
            Difference stride,
            random_access_traversal_tag)
        {
            return strided_iterator<
                typename range_iterator<Rng>::type,
                random_access_traversal_tag
            >(boost::begin(rng), boost::end(rng), stride);
        }

        template<class Rng, class Difference> inline
        strided_iterator<
            typename range_iterator<const Rng>::type,
            random_access_traversal_tag
        >
        make_end_strided_iterator(
            const Rng& rng,
            Difference stride,
            random_access_traversal_tag)
        {
            return strided_iterator<
                typename range_iterator<const Rng>::type,
                random_access_traversal_tag
            >(boost::begin(rng), boost::end(rng), stride);
        }

        template<
            class Rng,
            class Category =
                typename iterators::pure_iterator_traversal<
                    typename range_iterator<Rng>::type
                >::type
        >
        class strided_range
            : public iterator_range<
                range_detail::strided_iterator<
                    typename range_iterator<Rng>::type,
                    Category
                >
            >
        {
            typedef range_detail::strided_iterator<
                typename range_iterator<Rng>::type,
                Category
            > iter_type;
            typedef iterator_range<iter_type> super_t;
        public:
            template<class Difference>
            strided_range(Difference stride, Rng& rng)
                : super_t(
                    range_detail::make_begin_strided_iterator(
                        rng, stride,
                        typename iterator_traversal<
                            typename range_iterator<Rng>::type
                        >::type()),
                    range_detail::make_end_strided_iterator(
                        rng, stride,
                        typename iterator_traversal<
                            typename range_iterator<Rng>::type
                        >::type()))
            {
                BOOST_ASSERT( stride >= 0 );
            }
        };

        template<class Difference>
        class strided_holder : public holder<Difference>
        {
        public:
            explicit strided_holder(Difference value)
                : holder<Difference>(value)
            {
            }
        };

        template<class Rng, class Difference>
        inline strided_range<Rng>
        operator|(Rng& rng, const strided_holder<Difference>& stride)
        {
            return strided_range<Rng>(stride.val, rng);
        }

        template<class Rng, class Difference>
        inline strided_range<const Rng>
        operator|(const Rng& rng, const strided_holder<Difference>& stride)
        {
            return strided_range<const Rng>(stride.val, rng);
        }

    } // namespace range_detail

    using range_detail::strided_range;

    namespace adaptors
    {

        namespace
        {
            const range_detail::forwarder<range_detail::strided_holder>
                strided = range_detail::forwarder<
                            range_detail::strided_holder>();
        }

        template<class Range, class Difference>
        inline strided_range<Range>
        stride(Range& rng, Difference step)
        {
            return strided_range<Range>(step, rng);
        }

        template<class Range, class Difference>
        inline strided_range<const Range>
        stride(const Range& rng, Difference step)
        {
            return strided_range<const Range>(step, rng);
        }

    } // namespace 'adaptors'
} // namespace 'boost'

#endif

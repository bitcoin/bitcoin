// Boost.Range library
//
//  Copyright Neil Groves 2010. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//
// For more information, see http://www.boost.org/libs/range/
//
#ifndef BOOST_RANGE_IRANGE_HPP_INCLUDED
#define BOOST_RANGE_IRANGE_HPP_INCLUDED

#include <boost/assert.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/range/iterator_range.hpp>

namespace boost
{
    namespace range_detail
    {
        // integer_iterator is an iterator over an integer sequence that
        // is bounded only by the limits of the underlying integer
        // representation.
        //
        // This is useful for implementing the irange(first, last)
        // function.
        //
        // Note:
        // This use of this iterator and irange is appreciably less
        // performant than the corresponding hand-written integer
        // loop on many compilers.
        template<typename Integer>
        class integer_iterator
            : public boost::iterator_facade<
                        integer_iterator<Integer>,
                        Integer,
                        boost::random_access_traversal_tag,
                        Integer,
                        std::ptrdiff_t
                    >
        {
            typedef boost::iterator_facade<
                        integer_iterator<Integer>,
                        Integer,
                        boost::random_access_traversal_tag,
                        Integer,
                        std::ptrdiff_t
                    > base_t;
        public:
            typedef typename base_t::value_type value_type;
            typedef typename base_t::difference_type difference_type;
            typedef typename base_t::reference reference;
            typedef std::random_access_iterator_tag iterator_category;

            integer_iterator() : m_value() {}
            explicit integer_iterator(value_type x) : m_value(x) {}

        private:
            void increment()
            {
                ++m_value;
            }

            void decrement()
            {
                --m_value;
            }

            void advance(difference_type offset)
            {
                m_value += offset;
            }

            difference_type distance_to(const integer_iterator& other) const
            {
                return is_signed<value_type>::value
                    ? (other.m_value - m_value)
                    : (other.m_value >= m_value)
                        ? static_cast<difference_type>(other.m_value - m_value)
                        : -static_cast<difference_type>(m_value - other.m_value);
            }

            bool equal(const integer_iterator& other) const
            {
                return m_value == other.m_value;
            }

            reference dereference() const
            {
                return m_value;
            }

            friend class ::boost::iterator_core_access;
            value_type m_value;
        };

        // integer_iterator_with_step is similar in nature to the
        // integer_iterator but provides the ability to 'move' in
        // a number of steps specified at construction time.
        //
        // The three variable implementation provides the best guarantees
        // of loop termination upon various combinations of input.
        //
        // While this design is less performant than some less
        // safe alternatives, the use of ranges and iterators to
        // perform counting will never be optimal anyhow, hence
        // if optimal performance is desired a hand-coded loop
        // is the solution.
        template<typename Integer>
        class integer_iterator_with_step
            : public boost::iterator_facade<
                        integer_iterator_with_step<Integer>,
                        Integer,
                        boost::random_access_traversal_tag,
                        Integer,
                        std::ptrdiff_t
                    >
        {
            typedef boost::iterator_facade<
                        integer_iterator_with_step<Integer>,
                        Integer,
                        boost::random_access_traversal_tag,
                        Integer,
                        std::ptrdiff_t
                    > base_t;
        public:
            typedef typename base_t::value_type value_type;
            typedef typename base_t::difference_type difference_type;
            typedef typename base_t::reference reference;
            typedef std::random_access_iterator_tag iterator_category;

            integer_iterator_with_step(value_type first, difference_type step, value_type step_size)
                : m_first(first)
                , m_step(step)
                , m_step_size(step_size)
            {
            }

        private:
            void increment()
            {
                ++m_step;
            }

            void decrement()
            {
                --m_step;
            }

            void advance(difference_type offset)
            {
                m_step += offset;
            }

            difference_type distance_to(const integer_iterator_with_step& other) const
            {
                return other.m_step - m_step;
            }

            bool equal(const integer_iterator_with_step& other) const
            {
                return m_step == other.m_step;
            }

            reference dereference() const
            {
                return m_first + (m_step * m_step_size);
            }

            friend class ::boost::iterator_core_access;
            value_type m_first;
            difference_type m_step;
            difference_type m_step_size;
        };

    } // namespace range_detail

    template<typename Integer>
    class integer_range
        : public iterator_range< range_detail::integer_iterator<Integer> >
    {
        typedef range_detail::integer_iterator<Integer> iterator_t;
        typedef iterator_range<iterator_t> base_t;
    public:
        integer_range(Integer first, Integer last)
            : base_t(iterator_t(first), iterator_t(last))
        {
        }
    };

    template<typename Integer>
    class strided_integer_range
    : public iterator_range< range_detail::integer_iterator_with_step<Integer> >
    {
        typedef range_detail::integer_iterator_with_step<Integer> iterator_t;
        typedef iterator_range<iterator_t> base_t;
    public:
        template<typename Iterator>
        strided_integer_range(Iterator first, Iterator last)
            : base_t(first, last)
        {
        }
    };

    template<typename Integer>
    integer_range<Integer>
    irange(Integer first, Integer last)
    {
        BOOST_ASSERT( first <= last );
        return integer_range<Integer>(first, last);
    }

    template<typename Integer, typename StepSize>
    strided_integer_range<Integer>
        irange(Integer first, Integer last, StepSize step_size)
    {
        BOOST_ASSERT( step_size != 0 );
        BOOST_ASSERT( (step_size > 0) ? (last >= first) : (last <= first) );

        typedef typename range_detail::integer_iterator_with_step<Integer> iterator_t;

        const std::ptrdiff_t sz = static_cast<std::ptrdiff_t>(step_size >= 0 ? step_size : -step_size);
        const Integer l = step_size >= 0 ? last : first;
        const Integer f = step_size >= 0 ? first : last;
        const std::ptrdiff_t num_steps = (l - f) / sz + ((l - f) % sz ? 1 : 0);
        BOOST_ASSERT(num_steps >= 0);

        return strided_integer_range<Integer>(
            iterator_t(first, 0, step_size),
            iterator_t(first, num_steps, step_size));
    }

    template<typename Integer>
    integer_range<Integer>
    irange(Integer last)
    {
        return integer_range<Integer>(static_cast<Integer>(0), last);
    }

} // namespace boost

#endif // include guard

///////////////////////////////////////////////////////////////////////////////
// tail.hpp
//
//  Copyright 2005 Eric Niebler, Michael Gauckler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_ACCUMULATORS_STATISTICS_TAIL_HPP_EAN_28_10_2005
#define BOOST_ACCUMULATORS_STATISTICS_TAIL_HPP_EAN_28_10_2005

#include <vector>
#include <functional>
#include <boost/assert.hpp>
#include <boost/range.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/or.hpp>
#include <boost/mpl/placeholders.hpp>
#include <boost/parameter/keyword.hpp>
#include <boost/iterator/reverse_iterator.hpp>
#include <boost/iterator/permutation_iterator.hpp>
#include <boost/accumulators/accumulators_fwd.hpp>
#include <boost/accumulators/framework/accumulator_base.hpp>
#include <boost/accumulators/framework/extractor.hpp>
#include <boost/accumulators/numeric/functional.hpp>
#include <boost/accumulators/framework/parameters/sample.hpp>
#include <boost/accumulators/framework/depends_on.hpp>
#include <boost/accumulators/statistics_fwd.hpp>

namespace boost { namespace accumulators
{
///////////////////////////////////////////////////////////////////////////////
// cache_size named parameters
BOOST_PARAMETER_NESTED_KEYWORD(tag, right_tail_cache_size, cache_size)
BOOST_PARAMETER_NESTED_KEYWORD(tag, left_tail_cache_size, cache_size)

BOOST_ACCUMULATORS_IGNORE_GLOBAL(right_tail_cache_size)
BOOST_ACCUMULATORS_IGNORE_GLOBAL(left_tail_cache_size)

namespace detail
{
    ///////////////////////////////////////////////////////////////////////////////
    // tail_range
    /// INTERNAL ONLY
    ///
    template<typename ElementIterator, typename IndexIterator>
    struct tail_range
    {
        typedef boost::iterator_range<
            boost::reverse_iterator<boost::permutation_iterator<ElementIterator, IndexIterator> >
        > type;
    };

    ///////////////////////////////////////////////////////////////////////////////
    // make_tail_range
    /// INTERNAL ONLY
    ///
    template<typename ElementIterator, typename IndexIterator>
    typename tail_range<ElementIterator, IndexIterator>::type
    make_tail_range(ElementIterator elem_begin, IndexIterator index_begin, IndexIterator index_end)
    {
        return boost::make_iterator_range(
            boost::make_reverse_iterator(
                boost::make_permutation_iterator(elem_begin, index_end)
            )
          , boost::make_reverse_iterator(
                boost::make_permutation_iterator(elem_begin, index_begin)
            )
        );
    }

    ///////////////////////////////////////////////////////////////////////////////
    // stat_assign_visitor
    /// INTERNAL ONLY
    ///
    template<typename Args>
    struct stat_assign_visitor
    {
        stat_assign_visitor(Args const &a, std::size_t i)
          : args(a)
          , index(i)
        {
        }

        template<typename Stat>
        void operator ()(Stat &stat) const
        {
            stat.assign(this->args, this->index);
        }

    private:
        stat_assign_visitor &operator =(stat_assign_visitor const &);
        Args const &args;
        std::size_t index;
    };

    ///////////////////////////////////////////////////////////////////////////////
    // stat_assign
    /// INTERNAL ONLY
    ///
    template<typename Args>
    inline stat_assign_visitor<Args> const stat_assign(Args const &args, std::size_t index)
    {
        return stat_assign_visitor<Args>(args, index);
    }

    ///////////////////////////////////////////////////////////////////////////////
    // is_tail_variate_feature
    /// INTERNAL ONLY
    ///
    template<typename Stat, typename LeftRight>
    struct is_tail_variate_feature
      : mpl::false_
    {
    };

    /// INTERNAL ONLY
    ///
    template<typename VariateType, typename VariateTag, typename LeftRight>
    struct is_tail_variate_feature<tag::tail_variate<VariateType, VariateTag, LeftRight>, LeftRight>
      : mpl::true_
    {
    };

    /// INTERNAL ONLY
    ///
    template<typename LeftRight>
    struct is_tail_variate_feature<tag::tail_weights<LeftRight>, LeftRight>
      : mpl::true_
    {
    };

} // namespace detail

namespace impl
{
    ///////////////////////////////////////////////////////////////////////////////
    // tail_impl
    template<typename Sample, typename LeftRight>
    struct tail_impl
      : accumulator_base
    {
        // LeftRight must be either right or left
        BOOST_MPL_ASSERT((
            mpl::or_<is_same<LeftRight, right>, is_same<LeftRight, left> >
        ));

        typedef
            typename mpl::if_<
                is_same<LeftRight, right>
              , numeric::functional::greater<Sample const, Sample const>
              , numeric::functional::less<Sample const, Sample const>
            >::type
        predicate_type;

        // for boost::result_of
        typedef typename detail::tail_range<
            typename std::vector<Sample>::const_iterator
          , std::vector<std::size_t>::iterator
        >::type result_type;

        template<typename Args>
        tail_impl(Args const &args)
          : is_sorted(false)
          , indices()
          , samples(args[tag::tail<LeftRight>::cache_size], args[sample | Sample()])
        {
            this->indices.reserve(this->samples.size());
        }

        tail_impl(tail_impl const &that)
          : is_sorted(that.is_sorted)
          , indices(that.indices)
          , samples(that.samples)
        {
            this->indices.reserve(this->samples.size());
        }

        // This just stores the heap and the samples.
        // In operator()() below, if we are adding a new sample
        // to the sample cache, we force all the
        // tail_variates to update also. (It's not
        // good enough to wait for the accumulator_set to do it
        // for us because then information about whether a sample
        // was stored and where is lost, and would need to be
        // queried at runtime, which would be slow.) This is
        // implemented as a filtered visitation over the stats,
        // which we can access because args[accumulator] gives us
        // all the stats.

        template<typename Args>
        void operator ()(Args const &args)
        {
            if(this->indices.size() < this->samples.size())
            {
                this->indices.push_back(this->indices.size());
                this->assign(args, this->indices.back());
            }
            else if(predicate_type()(args[sample], this->samples[this->indices[0]]))
            {
                std::pop_heap(this->indices.begin(), this->indices.end(), indirect_cmp(this->samples));
                this->assign(args, this->indices.back());
            }
        }

        result_type result(dont_care) const
        {
            if(!this->is_sorted)
            {
                // Must use the same predicate here as in push_heap/pop_heap above.
                std::sort_heap(this->indices.begin(), this->indices.end(), indirect_cmp(this->samples));
                // sort_heap puts elements in reverse order. Calling std::reverse
                // turns the sorted sequence back into a valid heap.
                std::reverse(this->indices.begin(), this->indices.end());
                this->is_sorted = true;
            }

            return detail::make_tail_range(
                this->samples.begin()
              , this->indices.begin()
              , this->indices.end()
            );
        }

    private:

        struct is_tail_variate
        {
            template<typename T>
            struct apply
              : detail::is_tail_variate_feature<
                    typename detail::feature_tag<T>::type
                  , LeftRight
                >
            {};
        };

        template<typename Args>
        void assign(Args const &args, std::size_t index)
        {
            BOOST_ASSERT(index < this->samples.size());
            this->samples[index] = args[sample];
            std::push_heap(this->indices.begin(), this->indices.end(), indirect_cmp(this->samples));
            this->is_sorted = false;
            // Tell the tail variates to store their values also
            args[accumulator].template visit_if<is_tail_variate>(detail::stat_assign(args, index));
        }

        ///////////////////////////////////////////////////////////////////////////////
        //
        struct indirect_cmp
        {
            typedef std::size_t first_argument_type;
            typedef std::size_t second_argument_type;
            typedef bool result_type;

            indirect_cmp(std::vector<Sample> const &s)
              : samples(s)
            {
            }

            bool operator ()(std::size_t left, std::size_t right) const
            {
                return predicate_type()(this->samples[left], this->samples[right]);
            }

        private:
            indirect_cmp &operator =(indirect_cmp const &);
            std::vector<Sample> const &samples;
        };

    public:
        // make this accumulator serializeable
        template<class Archive>
        void serialize(Archive & ar, const unsigned int file_version)
        { 
            ar & is_sorted;
            ar & indices;
            ar & samples;
        }

    private:
        mutable bool is_sorted;
        mutable std::vector<std::size_t> indices;
        std::vector<Sample> samples;
    };

} // namespace impl

// TODO The templatized tag::tail below should inherit from the correct named parameter.
// The following lines provide a workaround, but there must be a better way of doing this.
template<typename T>
struct tail_cache_size_named_arg
{
};
template<>
struct tail_cache_size_named_arg<left>
  : tag::left_tail_cache_size
{
};
template<>
struct tail_cache_size_named_arg<right>
  : tag::right_tail_cache_size
{
};

///////////////////////////////////////////////////////////////////////////////
// tag::tail<>
//
namespace tag
{
    template<typename LeftRight>
    struct tail
      : depends_on<>
      , tail_cache_size_named_arg<LeftRight>
    {
        /// INTERNAL ONLY
        ///
        typedef accumulators::impl::tail_impl<mpl::_1, LeftRight> impl;

        #ifdef BOOST_ACCUMULATORS_DOXYGEN_INVOKED
        /// tag::tail<LeftRight>::cache_size named parameter
        static boost::parameter::keyword<tail_cache_size_named_arg<LeftRight> > const cache_size;
        #endif
    };

    struct abstract_tail
      : depends_on<>
    {
    };
}

///////////////////////////////////////////////////////////////////////////////
// extract::tail
//
namespace extract
{
    extractor<tag::abstract_tail> const tail = {};

    BOOST_ACCUMULATORS_IGNORE_GLOBAL(tail)
}

using extract::tail;

template<typename LeftRight>
struct feature_of<tag::tail<LeftRight> >
  : feature_of<tag::abstract_tail>
{
};

}} // namespace boost::accumulators

#endif

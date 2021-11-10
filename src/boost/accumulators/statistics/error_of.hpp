///////////////////////////////////////////////////////////////////////////////
// error_of.hpp
//
//  Copyright 2005 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_ACCUMULATORS_STATISTICS_ERROR_OF_HPP_EAN_29_11_2005
#define BOOST_ACCUMULATORS_STATISTICS_ERROR_OF_HPP_EAN_29_11_2005

#include <boost/mpl/placeholders.hpp>
#include <boost/accumulators/framework/accumulator_base.hpp>
#include <boost/accumulators/framework/extractor.hpp>
#include <boost/accumulators/framework/depends_on.hpp>
#include <boost/accumulators/statistics_fwd.hpp>

namespace boost { namespace accumulators
{

namespace impl
{
    /// INTERNAL ONLY
    ///
    template<typename Feature>
    struct this_feature_has_no_error_calculation
      : mpl::false_
    {
    };

    ///////////////////////////////////////////////////////////////////////////////
    // error_of_impl
    /// INTERNAL ONLY
    ///
    template<typename Sample, typename Feature>
    struct error_of_impl
      : accumulator_base
    {
        // TODO: specialize this on the specific features that have errors we're
        // interested in.
        BOOST_MPL_ASSERT((this_feature_has_no_error_calculation<Feature>));

        // for boost::result_of
        typedef int result_type;

        error_of_impl(dont_care)
        {
        }

        result_type result(dont_care) const
        {
            return 0;
        }
    };

} // namespace impl

///////////////////////////////////////////////////////////////////////////////
// tag::error_of
//
namespace tag
{
    template<typename Feature>
    struct error_of
      : depends_on<Feature>
    {
        /// INTERNAL ONLY
        ///
        typedef accumulators::impl::error_of_impl<mpl::_1, Feature> impl;
    };
}

///////////////////////////////////////////////////////////////////////////////
// extract::error_of
//
namespace extract
{
    BOOST_ACCUMULATORS_DEFINE_EXTRACTOR(tag, error_of, (typename))
}

using extract::error_of;

// make tag::error_of<tag::feature(modifier)> work
template<typename Feature>
struct as_feature<tag::error_of<Feature> >
{
    typedef tag::error_of<typename as_feature<Feature>::type> type;
};

// make error_of<tag::mean> work with non-void weights (should become
// error_of<tag::weighted_mean>
template<typename Feature>
struct as_weighted_feature<tag::error_of<Feature> >
{
    typedef tag::error_of<typename as_weighted_feature<Feature>::type> type;
};

}} // namespace boost::accumulators

#endif

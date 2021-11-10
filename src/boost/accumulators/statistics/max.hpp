///////////////////////////////////////////////////////////////////////////////
// max.hpp
//
//  Copyright 2005 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_ACCUMULATORS_STATISTICS_MAX_HPP_EAN_28_10_2005
#define BOOST_ACCUMULATORS_STATISTICS_MAX_HPP_EAN_28_10_2005

#include <limits>
#include <boost/mpl/placeholders.hpp>
#include <boost/accumulators/framework/accumulator_base.hpp>
#include <boost/accumulators/framework/extractor.hpp>
#include <boost/accumulators/framework/parameters/sample.hpp>
#include <boost/accumulators/numeric/functional.hpp>
#include <boost/accumulators/framework/depends_on.hpp>
#include <boost/accumulators/statistics_fwd.hpp>

namespace boost { namespace accumulators
{

namespace impl
{
    ///////////////////////////////////////////////////////////////////////////////
    // max_impl
    template<typename Sample>
    struct max_impl
      : accumulator_base
    {
        // for boost::result_of
        typedef Sample result_type;

        template<typename Args>
        max_impl(Args const &args)
          : max_(numeric::as_min(args[sample | Sample()]))
        {
        }

        template<typename Args>
        void operator ()(Args const &args)
        {
            numeric::max_assign(this->max_, args[sample]);
        }

        result_type result(dont_care) const
        {
            return this->max_;
        }

        // make this accumulator serializeable
        template<class Archive>
        void serialize(Archive & ar, const unsigned int file_version)
        { 
            ar & max_;
        }

    private:
        Sample max_;
    };

} // namespace impl

///////////////////////////////////////////////////////////////////////////////
// tag::max
//
namespace tag
{
    struct max
      : depends_on<>
    {
        /// INTERNAL ONLY
        ///
        typedef accumulators::impl::max_impl<mpl::_1> impl;
    };
}

///////////////////////////////////////////////////////////////////////////////
// extract::max
//
namespace extract
{
    extractor<tag::max> const max = {};

    BOOST_ACCUMULATORS_IGNORE_GLOBAL(max)
}

using extract::max;

}} // namespace boost::accumulators

#endif

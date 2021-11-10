///////////////////////////////////////////////////////////////////////////////
// min.hpp
//
//  Copyright 2005 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_ACCUMULATORS_STATISTICS_MIN_HPP_EAN_28_10_2005
#define BOOST_ACCUMULATORS_STATISTICS_MIN_HPP_EAN_28_10_2005

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
    // min_impl
    template<typename Sample>
    struct min_impl
      : accumulator_base
    {
        // for boost::result_of
        typedef Sample result_type;

        template<typename Args>
        min_impl(Args const &args)
          : min_(numeric::as_max(args[sample | Sample()]))
        {
        }

        template<typename Args>
        void operator ()(Args const &args)
        {
            numeric::min_assign(this->min_, args[sample]);
        }

        result_type result(dont_care) const
        {
            return this->min_;
        }

        // make this accumulator serializeable
        template<class Archive>
        void serialize(Archive & ar, const unsigned int file_version)
        { 
            ar & min_;
        }

    private:
        Sample min_;
    };

} // namespace impl

///////////////////////////////////////////////////////////////////////////////
// tag::min
//
namespace tag
{
    struct min
      : depends_on<>
    {
        /// INTERNAL ONLY
        ///
        typedef accumulators::impl::min_impl<mpl::_1> impl;
    };
}

///////////////////////////////////////////////////////////////////////////////
// extract::min
//
namespace extract
{
    extractor<tag::min> const min = {};

    BOOST_ACCUMULATORS_IGNORE_GLOBAL(min)
}

using extract::min;

}} // namespace boost::accumulators

#endif

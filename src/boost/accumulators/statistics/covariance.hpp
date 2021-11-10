///////////////////////////////////////////////////////////////////////////////
// covariance.hpp
//
//  Copyright 2006 Daniel Egloff, Olivier Gygi. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_ACCUMULATORS_STATISTICS_COVARIANCE_HPP_DE_01_01_2006
#define BOOST_ACCUMULATORS_STATISTICS_COVARIANCE_HPP_DE_01_01_2006

#include <vector>
#include <limits>
#include <numeric>
#include <functional>
#include <complex>
#include <boost/mpl/assert.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/range.hpp>
#include <boost/parameter/keyword.hpp>
#include <boost/mpl/placeholders.hpp>
#include <boost/numeric/ublas/io.hpp>
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/type_traits/is_scalar.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/accumulators/framework/accumulator_base.hpp>
#include <boost/accumulators/framework/extractor.hpp>
#include <boost/accumulators/numeric/functional.hpp>
#include <boost/accumulators/framework/parameters/sample.hpp>
#include <boost/accumulators/statistics_fwd.hpp>
#include <boost/accumulators/statistics/count.hpp>
#include <boost/accumulators/statistics/mean.hpp>

namespace boost { namespace numeric
{
    namespace functional
    {
        struct std_vector_tag;

        ///////////////////////////////////////////////////////////////////////////////
        // functional::outer_product
        template<typename Left, typename Right, typename EnableIf = void>
        struct outer_product_base
          : functional::multiplies<Left, Right>
        {};

        template<typename Left, typename Right, typename LeftTag = typename tag<Left>::type, typename RightTag = typename tag<Right>::type>
        struct outer_product
          : outer_product_base<Left, Right, void>
        {};

        template<typename Left, typename Right>
        struct outer_product<Left, Right, std_vector_tag, std_vector_tag>
        {
            typedef Left first_argument_type;
            typedef Right second_argument_type;
            typedef
                ublas::matrix<
                    typename functional::multiplies<
                        typename Left::value_type
                      , typename Right::value_type
                    >::result_type
                >
            result_type;

            result_type
            operator ()(Left & left, Right & right) const
            {
                std::size_t left_size = left.size();
                std::size_t right_size = right.size();
                result_type result(left_size, right_size);
                for (std::size_t i = 0; i < left_size; ++i)
                    for (std::size_t j = 0; j < right_size; ++j)
                        result(i,j) = numeric::multiplies(left[i], right[j]);
                return result;
            }
        };
    }

    namespace op
    {
        struct outer_product
          : boost::detail::function2<functional::outer_product<_1, _2, functional::tag<_1>, functional::tag<_2> > >
        {};
    }

    namespace
    {
        op::outer_product const &outer_product = boost::detail::pod_singleton<op::outer_product>::instance;
    }

}}

namespace boost { namespace accumulators
{

namespace impl
{
    ///////////////////////////////////////////////////////////////////////////////
    // covariance_impl
    //
    /**
        @brief Covariance Estimator

        An iterative Monte Carlo estimator for the covariance \f$\mathrm{Cov}(X,X')\f$, where \f$X\f$ is a sample
        and \f$X'\f$ is a variate, is given by:

        \f[
            \hat{c}_n = \frac{n-1}{n} \hat{c}_{n-1} + \frac{1}{n-1}(X_n - \hat{\mu}_n)(X_n' - \hat{\mu}_n'),\quad n\ge2,\quad\hat{c}_1 = 0,
        \f]

        \f$\hat{\mu}_n\f$ and \f$\hat{\mu}_n'\f$ being the means of the samples and variates.
    */
    template<typename Sample, typename VariateType, typename VariateTag>
    struct covariance_impl
      : accumulator_base
    {
        typedef typename numeric::functional::fdiv<Sample, std::size_t>::result_type sample_type;
        typedef typename numeric::functional::fdiv<VariateType, std::size_t>::result_type variate_type;
        // for boost::result_of
        typedef typename numeric::functional::outer_product<sample_type, variate_type>::result_type result_type;

        template<typename Args>
        covariance_impl(Args const &args)
          : cov_(
                numeric::outer_product(
                    numeric::fdiv(args[sample | Sample()], (std::size_t)1)
                  , numeric::fdiv(args[parameter::keyword<VariateTag>::get() | VariateType()], (std::size_t)1)
                )
            )
        {
        }

        template<typename Args>
        void operator ()(Args const &args)
        {
            std::size_t cnt = count(args);

            if (cnt > 1)
            {
                extractor<tag::mean_of_variates<VariateType, VariateTag> > const some_mean_of_variates = {};

                this->cov_ = this->cov_*(cnt-1.)/cnt
                           + numeric::outer_product(
                                 some_mean_of_variates(args) - args[parameter::keyword<VariateTag>::get()]
                               , mean(args) - args[sample]
                             ) / (cnt-1.);
            }
        }

        result_type result(dont_care) const
        {
            return this->cov_;
        }

        // make this accumulator serializeable
        template<class Archive>
        void serialize(Archive & ar, const unsigned int file_version)
        { 
            ar & cov_;
        }

    private:
        result_type cov_;
    };

} // namespace impl

///////////////////////////////////////////////////////////////////////////////
// tag::covariance
//
namespace tag
{
    template<typename VariateType, typename VariateTag>
    struct covariance
      : depends_on<count, mean, mean_of_variates<VariateType, VariateTag> >
    {
        typedef accumulators::impl::covariance_impl<mpl::_1, VariateType, VariateTag> impl;
    };

    struct abstract_covariance
      : depends_on<>
    {
    };
}

///////////////////////////////////////////////////////////////////////////////
// extract::covariance
//
namespace extract
{
    extractor<tag::abstract_covariance> const covariance = {};

    BOOST_ACCUMULATORS_IGNORE_GLOBAL(covariance)
}

using extract::covariance;

template<typename VariateType, typename VariateTag>
struct feature_of<tag::covariance<VariateType, VariateTag> >
  : feature_of<tag::abstract_covariance>
{
};

// So that covariance can be automatically substituted with
// weighted_covariance when the weight parameter is non-void.
template<typename VariateType, typename VariateTag>
struct as_weighted_feature<tag::covariance<VariateType, VariateTag> >
{
    typedef tag::weighted_covariance<VariateType, VariateTag> type;
};

template<typename VariateType, typename VariateTag>
struct feature_of<tag::weighted_covariance<VariateType, VariateTag> >
  : feature_of<tag::covariance<VariateType, VariateTag> >
{};

}} // namespace boost::accumulators

#endif

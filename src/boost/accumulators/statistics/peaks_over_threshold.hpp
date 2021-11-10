///////////////////////////////////////////////////////////////////////////////
// peaks_over_threshold.hpp
//
//  Copyright 2006 Daniel Egloff, Olivier Gygi. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_ACCUMULATORS_STATISTICS_PEAKS_OVER_THRESHOLD_HPP_DE_01_01_2006
#define BOOST_ACCUMULATORS_STATISTICS_PEAKS_OVER_THRESHOLD_HPP_DE_01_01_2006

#include <vector>
#include <limits>
#include <numeric>
#include <functional>
#include <boost/config/no_tr1/cmath.hpp> // pow
#include <sstream> // stringstream
#include <stdexcept> // runtime_error
#include <boost/throw_exception.hpp>
#include <boost/range.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/int.hpp>
#include <boost/mpl/placeholders.hpp>
#include <boost/parameter/keyword.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/accumulators/accumulators_fwd.hpp>
#include <boost/accumulators/framework/accumulator_base.hpp>
#include <boost/accumulators/framework/extractor.hpp>
#include <boost/accumulators/numeric/functional.hpp>
#include <boost/accumulators/framework/parameters/sample.hpp>
#include <boost/accumulators/framework/depends_on.hpp>
#include <boost/accumulators/statistics_fwd.hpp>
#include <boost/accumulators/statistics/parameters/quantile_probability.hpp>
#include <boost/accumulators/statistics/count.hpp>
#include <boost/accumulators/statistics/tail.hpp>

#ifdef _MSC_VER
# pragma warning(push)
# pragma warning(disable: 4127) // conditional expression is constant
#endif

namespace boost { namespace accumulators
{

///////////////////////////////////////////////////////////////////////////////
// threshold_probability and threshold named parameters
//
BOOST_PARAMETER_NESTED_KEYWORD(tag, pot_threshold_value, threshold_value)
BOOST_PARAMETER_NESTED_KEYWORD(tag, pot_threshold_probability, threshold_probability)

BOOST_ACCUMULATORS_IGNORE_GLOBAL(pot_threshold_value)
BOOST_ACCUMULATORS_IGNORE_GLOBAL(pot_threshold_probability)

namespace impl
{
    ///////////////////////////////////////////////////////////////////////////////
    // peaks_over_threshold_impl
    //  works with an explicit threshold value and does not depend on order statistics
    /**
        @brief Peaks over Threshold Method for Quantile and Tail Mean Estimation

        According to the theorem of Pickands-Balkema-de Haan, the distribution function \f$F_u(x)\f$ of
        the excesses \f$x\f$ over some sufficiently high threshold \f$u\f$ of a distribution function \f$F(x)\f$
        may be approximated by a generalized Pareto distribution
        \f[
            G_{\xi,\beta}(x) =
            \left\{
            \begin{array}{ll}
                \beta^{-1}\left(1+\frac{\xi x}{\beta}\right)^{-1/\xi-1} & \textrm{if }\xi\neq0\\
                \beta^{-1}\exp\left(-\frac{x}{\beta}\right) & \textrm{if }\xi=0,
            \end{array}
            \right.
        \f]
        with suitable parameters \f$\xi\f$ and \f$\beta\f$ that can be estimated, e.g., with the method of moments, cf.
        Hosking and Wallis (1987),
        \f[
            \begin{array}{lll}
            \hat{\xi} & = & \frac{1}{2}\left[1-\frac{(\hat{\mu}-u)^2}{\hat{\sigma}^2}\right]\\
            \hat{\beta} & = & \frac{\hat{\mu}-u}{2}\left[\frac{(\hat{\mu}-u)^2}{\hat{\sigma}^2}+1\right],
            \end{array}
        \f]
        \f$\hat{\mu}\f$ and \f$\hat{\sigma}^2\f$ being the empirical mean and variance of the samples over
        the threshold \f$u\f$. Equivalently, the distribution function
        \f$F_u(x-u)\f$ of the exceedances \f$x-u\f$ can be approximated by
        \f$G_{\xi,\beta}(x-u)=G_{\xi,\beta,u}(x)\f$. Since for \f$x\geq u\f$ the distribution function \f$F(x)\f$
        can be written as
        \f[
            F(x) = [1 - \P(X \leq u)]F_u(x - u) + \P(X \leq u)
        \f]
        and the probability \f$\P(X \leq u)\f$ can be approximated by the empirical distribution function
        \f$F_n(u)\f$ evaluated at \f$u\f$, an estimator of \f$F(x)\f$ is given by
        \f[
            \widehat{F}(x) = [1 - F_n(u)]G_{\xi,\beta,u}(x) + F_n(u).
        \f]
        It can be shown that \f$\widehat{F}(x)\f$ is a generalized
        Pareto distribution \f$G_{\xi,\bar{\beta},\bar{u}}(x)\f$ with \f$\bar{\beta}=\beta[1-F_n(u)]^{\xi}\f$
        and \f$\bar{u}=u-\bar{\beta}\left\{[1-F_n(u)]^{-\xi}-1\right\}/\xi\f$. By inverting \f$\widehat{F}(x)\f$,
        one obtains an estimator for the \f$\alpha\f$-quantile,
        \f[
            \hat{q}_{\alpha} = \bar{u} + \frac{\bar{\beta}}{\xi}\left[(1-\alpha)^{-\xi}-1\right],
        \f]
        and similarly an estimator for the (coherent) tail mean,
        \f[
            \widehat{CTM}_{\alpha} = \hat{q}_{\alpha} - \frac{\bar{\beta}}{\xi-1}(1-\alpha)^{-\xi},
        \f]
        cf. McNeil and Frey (2000).

        Note that in case extreme values of the left tail are fitted, the distribution is mirrored with respect to the
        \f$y\f$ axis such that the left tail can be treated as a right tail. The computed fit parameters thus define
        the Pareto distribution that fits the mirrored left tail. When quantities like a quantile or a tail mean are
        computed using the fit parameters obtained from the mirrored data, the result is mirrored back, yielding the
        correct result.

        For further details, see

        J. R. M. Hosking and J. R. Wallis, Parameter and quantile estimation for the generalized Pareto distribution,
        Technometrics, Volume 29, 1987, p. 339-349

        A. J. McNeil and R. Frey, Estimation of Tail-Related Risk Measures for Heteroscedastic Financial Time Series:
        an Extreme Value Approach, Journal of Empirical Finance, Volume 7, 2000, p. 271-300

        @param quantile_probability
        @param pot_threshold_value
    */
    template<typename Sample, typename LeftRight>
    struct peaks_over_threshold_impl
      : accumulator_base
    {
        typedef typename numeric::functional::fdiv<Sample, std::size_t>::result_type float_type;
        // for boost::result_of
        typedef boost::tuple<float_type, float_type, float_type> result_type;
        // for left tail fitting, mirror the extreme values
        typedef mpl::int_<is_same<LeftRight, left>::value ? -1 : 1> sign;

        template<typename Args>
        peaks_over_threshold_impl(Args const &args)
          : Nu_(0)
          , mu_(sign::value * numeric::fdiv(args[sample | Sample()], (std::size_t)1))
          , sigma2_(numeric::fdiv(args[sample | Sample()], (std::size_t)1))
          , threshold_(sign::value * args[pot_threshold_value])
          , fit_parameters_(boost::make_tuple(0., 0., 0.))
          , is_dirty_(true)
        {
        }

        template<typename Args>
        void operator ()(Args const &args)
        {
            this->is_dirty_ = true;

            if (sign::value * args[sample] > this->threshold_)
            {
                this->mu_ += args[sample];
                this->sigma2_ += args[sample] * args[sample];
                ++this->Nu_;
            }
        }

        template<typename Args>
        result_type result(Args const &args) const
        {
            if (this->is_dirty_)
            {
                this->is_dirty_ = false;

                std::size_t cnt = count(args);

                this->mu_ = sign::value * numeric::fdiv(this->mu_, this->Nu_);
                this->sigma2_ = numeric::fdiv(this->sigma2_, this->Nu_);
                this->sigma2_ -= this->mu_ * this->mu_;

                float_type threshold_probability = numeric::fdiv(cnt - this->Nu_, cnt);

                float_type tmp = numeric::fdiv(( this->mu_ - this->threshold_ )*( this->mu_ - this->threshold_ ), this->sigma2_);
                float_type xi_hat = 0.5 * ( 1. - tmp );
                float_type beta_hat = 0.5 * ( this->mu_ - this->threshold_ ) * ( 1. + tmp );
                float_type beta_bar = beta_hat * std::pow(1. - threshold_probability, xi_hat);
                float_type u_bar = this->threshold_ - beta_bar * ( std::pow(1. - threshold_probability, -xi_hat) - 1.)/xi_hat;
                this->fit_parameters_ = boost::make_tuple(u_bar, beta_bar, xi_hat);
            }

            return this->fit_parameters_;
        }

        // make this accumulator serializeable
        // TODO: do we need to split to load/save and verify that threshold did not change?
        template<class Archive>
        void serialize(Archive & ar, const unsigned int file_version)
        { 
            ar & Nu_;
            ar & mu_;
            ar & sigma2_;
            ar & threshold_;
            ar & get<0>(fit_parameters_);
            ar & get<1>(fit_parameters_);
            ar & get<2>(fit_parameters_);
            ar & is_dirty_;
        }

    private:
        std::size_t Nu_;                     // number of samples larger than threshold
        mutable float_type mu_;              // mean of Nu_ largest samples
        mutable float_type sigma2_;          // variance of Nu_ largest samples
        float_type threshold_;
        mutable result_type fit_parameters_; // boost::tuple that stores fit parameters
        mutable bool is_dirty_;
    };

    ///////////////////////////////////////////////////////////////////////////////
    // peaks_over_threshold_prob_impl
    //  determines threshold from a given threshold probability using order statistics
    /**
        @brief Peaks over Threshold Method for Quantile and Tail Mean Estimation

        @sa peaks_over_threshold_impl

        @param quantile_probability
        @param pot_threshold_probability
    */
    template<typename Sample, typename LeftRight>
    struct peaks_over_threshold_prob_impl
      : accumulator_base
    {
        typedef typename numeric::functional::fdiv<Sample, std::size_t>::result_type float_type;
        // for boost::result_of
        typedef boost::tuple<float_type, float_type, float_type> result_type;
        // for left tail fitting, mirror the extreme values
        typedef mpl::int_<is_same<LeftRight, left>::value ? -1 : 1> sign;

        template<typename Args>
        peaks_over_threshold_prob_impl(Args const &args)
          : mu_(sign::value * numeric::fdiv(args[sample | Sample()], (std::size_t)1))
          , sigma2_(numeric::fdiv(args[sample | Sample()], (std::size_t)1))
          , threshold_probability_(args[pot_threshold_probability])
          , fit_parameters_(boost::make_tuple(0., 0., 0.))
          , is_dirty_(true)
        {
        }

        void operator ()(dont_care)
        {
            this->is_dirty_ = true;
        }

        template<typename Args>
        result_type result(Args const &args) const
        {
            if (this->is_dirty_)
            {
                this->is_dirty_ = false;

                std::size_t cnt = count(args);

                // the n'th cached sample provides an approximate threshold value u
                std::size_t n = static_cast<std::size_t>(
                    std::ceil(
                        cnt * ( ( is_same<LeftRight, left>::value ) ? this->threshold_probability_ : 1. - this->threshold_probability_ )
                    )
                );

                // If n is in a valid range, return result, otherwise return NaN or throw exception
                if ( n >= static_cast<std::size_t>(tail(args).size()))
                {
                    if (std::numeric_limits<float_type>::has_quiet_NaN)
                    {
                        return boost::make_tuple(
                            std::numeric_limits<float_type>::quiet_NaN()
                          , std::numeric_limits<float_type>::quiet_NaN()
                          , std::numeric_limits<float_type>::quiet_NaN()
                        );
                    }
                    else
                    {
                        std::ostringstream msg;
                        msg << "index n = " << n << " is not in valid range [0, " << tail(args).size() << ")";
                        boost::throw_exception(std::runtime_error(msg.str()));
                        return boost::make_tuple(Sample(0), Sample(0), Sample(0));
                    }
                }
                else
                {
                    float_type u = *(tail(args).begin() + n - 1) * sign::value;

                    // compute mean and variance of samples above/under threshold value u
                    for (std::size_t i = 0; i < n; ++i)
                    {
                        mu_ += *(tail(args).begin() + i);
                        sigma2_ += *(tail(args).begin() + i) * (*(tail(args).begin() + i));
                    }

                    this->mu_ = sign::value * numeric::fdiv(this->mu_, n);
                    this->sigma2_ = numeric::fdiv(this->sigma2_, n);
                    this->sigma2_ -= this->mu_ * this->mu_;

                    if (is_same<LeftRight, left>::value)
                        this->threshold_probability_ = 1. - this->threshold_probability_;

                    float_type tmp = numeric::fdiv(( this->mu_ - u )*( this->mu_ - u ), this->sigma2_);
                    float_type xi_hat = 0.5 * ( 1. - tmp );
                    float_type beta_hat = 0.5 * ( this->mu_ - u ) * ( 1. + tmp );
                    float_type beta_bar = beta_hat * std::pow(1. - threshold_probability_, xi_hat);
                    float_type u_bar = u - beta_bar * ( std::pow(1. - threshold_probability_, -xi_hat) - 1.)/xi_hat;
                    this->fit_parameters_ = boost::make_tuple(u_bar, beta_bar, xi_hat);
                }
            }

            return this->fit_parameters_;
        }

        // make this accumulator serializeable
        // TODO: do we need to split to load/save and verify that threshold did not change?
        template<class Archive>
        void serialize(Archive & ar, const unsigned int file_version)
        { 
            ar & mu_;
            ar & sigma2_;
            ar & threshold_probability_;
            ar & get<0>(fit_parameters_);
            ar & get<1>(fit_parameters_);
            ar & get<2>(fit_parameters_);
            ar & is_dirty_;
        }

    private:
        mutable float_type mu_;                     // mean of samples above threshold u
        mutable float_type sigma2_;                 // variance of samples above threshold u
        mutable float_type threshold_probability_;
        mutable result_type fit_parameters_;        // boost::tuple that stores fit parameters
        mutable bool is_dirty_;
    };

} // namespace impl

///////////////////////////////////////////////////////////////////////////////
// tag::peaks_over_threshold
//
namespace tag
{
    template<typename LeftRight>
    struct peaks_over_threshold
      : depends_on<count>
      , pot_threshold_value
    {
        /// INTERNAL ONLY
        ///
        typedef accumulators::impl::peaks_over_threshold_impl<mpl::_1, LeftRight> impl;
    };

    template<typename LeftRight>
    struct peaks_over_threshold_prob
      : depends_on<count, tail<LeftRight> >
      , pot_threshold_probability
    {
        /// INTERNAL ONLY
        ///
        typedef accumulators::impl::peaks_over_threshold_prob_impl<mpl::_1, LeftRight> impl;
    };

    struct abstract_peaks_over_threshold
      : depends_on<>
    {
    };
}

///////////////////////////////////////////////////////////////////////////////
// extract::peaks_over_threshold
//
namespace extract
{
    extractor<tag::abstract_peaks_over_threshold> const peaks_over_threshold = {};

    BOOST_ACCUMULATORS_IGNORE_GLOBAL(peaks_over_threshold)
}

using extract::peaks_over_threshold;

// peaks_over_threshold<LeftRight>(with_threshold_value) -> peaks_over_threshold<LeftRight>
template<typename LeftRight>
struct as_feature<tag::peaks_over_threshold<LeftRight>(with_threshold_value)>
{
    typedef tag::peaks_over_threshold<LeftRight> type;
};

// peaks_over_threshold<LeftRight>(with_threshold_probability) -> peaks_over_threshold_prob<LeftRight>
template<typename LeftRight>
struct as_feature<tag::peaks_over_threshold<LeftRight>(with_threshold_probability)>
{
    typedef tag::peaks_over_threshold_prob<LeftRight> type;
};

template<typename LeftRight>
struct feature_of<tag::peaks_over_threshold<LeftRight> >
  : feature_of<tag::abstract_peaks_over_threshold>
{
};

template<typename LeftRight>
struct feature_of<tag::peaks_over_threshold_prob<LeftRight> >
  : feature_of<tag::abstract_peaks_over_threshold>
{
};

// So that peaks_over_threshold can be automatically substituted
// with weighted_peaks_over_threshold when the weight parameter is non-void.
template<typename LeftRight>
struct as_weighted_feature<tag::peaks_over_threshold<LeftRight> >
{
    typedef tag::weighted_peaks_over_threshold<LeftRight> type;
};

template<typename LeftRight>
struct feature_of<tag::weighted_peaks_over_threshold<LeftRight> >
  : feature_of<tag::peaks_over_threshold<LeftRight> >
{};

// So that peaks_over_threshold_prob can be automatically substituted
// with weighted_peaks_over_threshold_prob when the weight parameter is non-void.
template<typename LeftRight>
struct as_weighted_feature<tag::peaks_over_threshold_prob<LeftRight> >
{
    typedef tag::weighted_peaks_over_threshold_prob<LeftRight> type;
};

template<typename LeftRight>
struct feature_of<tag::weighted_peaks_over_threshold_prob<LeftRight> >
  : feature_of<tag::peaks_over_threshold_prob<LeftRight> >
{};

}} // namespace boost::accumulators

#ifdef _MSC_VER
# pragma warning(pop)
#endif

#endif

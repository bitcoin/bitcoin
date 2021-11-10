//  Copyright Benjamin Sobotta 2012

//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_STATS_SKEW_NORMAL_HPP
#define BOOST_STATS_SKEW_NORMAL_HPP

// http://en.wikipedia.org/wiki/Skew_normal_distribution
// http://azzalini.stat.unipd.it/SN/
// Also:
// Azzalini, A. (1985). "A class of distributions which includes the normal ones".
// Scand. J. Statist. 12: 171-178.

#include <boost/math/distributions/fwd.hpp> // TODO add skew_normal distribution to fwd.hpp!
#include <boost/math/special_functions/owens_t.hpp> // Owen's T function
#include <boost/math/distributions/complement.hpp>
#include <boost/math/distributions/normal.hpp>
#include <boost/math/distributions/detail/common_error_handling.hpp>
#include <boost/math/constants/constants.hpp>
#include <boost/math/tools/tuple.hpp>
#include <boost/math/tools/roots.hpp> // Newton-Raphson
#include <boost/math/tools/assert.hpp>
#include <boost/math/distributions/detail/generic_mode.hpp> // pdf max finder.

#include <utility>
#include <algorithm> // std::lower_bound, std::distance

namespace boost{ namespace math{

  namespace detail
  {
    template <class RealType, class Policy>
    inline bool check_skew_normal_shape(
      const char* function,
      RealType shape,
      RealType* result,
      const Policy& pol)
    {
      if(!(boost::math::isfinite)(shape))
      {
        *result =
          policies::raise_domain_error<RealType>(function,
          "Shape parameter is %1%, but must be finite!",
          shape, pol);
        return false;
      }
      return true;
    }

  } // namespace detail

  template <class RealType = double, class Policy = policies::policy<> >
  class skew_normal_distribution
  {
  public:
    typedef RealType value_type;
    typedef Policy policy_type;

    skew_normal_distribution(RealType l_location = 0, RealType l_scale = 1, RealType l_shape = 0)
      : location_(l_location), scale_(l_scale), shape_(l_shape)
    { // Default is a 'standard' normal distribution N01. (shape=0 results in the normal distribution with no skew)
      static const char* function = "boost::math::skew_normal_distribution<%1%>::skew_normal_distribution";

      RealType result;
      detail::check_scale(function, l_scale, &result, Policy());
      detail::check_location(function, l_location, &result, Policy());
      detail::check_skew_normal_shape(function, l_shape, &result, Policy());
    }

    RealType location()const
    { 
      return location_;
    }

    RealType scale()const
    { 
      return scale_;
    }

    RealType shape()const
    { 
      return shape_;
    }


  private:
    //
    // Data members:
    //
    RealType location_;  // distribution location.
    RealType scale_;    // distribution scale.
    RealType shape_;    // distribution shape.
  }; // class skew_normal_distribution

  typedef skew_normal_distribution<double> skew_normal;

  template <class RealType, class Policy>
  inline const std::pair<RealType, RealType> range(const skew_normal_distribution<RealType, Policy>& /*dist*/)
  { // Range of permissible values for random variable x.
    using boost::math::tools::max_value;
    return std::pair<RealType, RealType>(
       std::numeric_limits<RealType>::has_infinity ? -std::numeric_limits<RealType>::infinity() : -max_value<RealType>(), 
       std::numeric_limits<RealType>::has_infinity ? std::numeric_limits<RealType>::infinity() : max_value<RealType>()); // - to + max value.
  }

  template <class RealType, class Policy>
  inline const std::pair<RealType, RealType> support(const skew_normal_distribution<RealType, Policy>& /*dist*/)
  { // Range of supported values for random variable x.
    // This is range where cdf rises from 0 to 1, and outside it, the pdf is zero.

    using boost::math::tools::max_value;
    return std::pair<RealType, RealType>(-max_value<RealType>(),  max_value<RealType>()); // - to + max value.
  }

  template <class RealType, class Policy>
  inline RealType pdf(const skew_normal_distribution<RealType, Policy>& dist, const RealType& x)
  {
    const RealType scale = dist.scale();
    const RealType location = dist.location();
    const RealType shape = dist.shape();

    static const char* function = "boost::math::pdf(const skew_normal_distribution<%1%>&, %1%)";

    RealType result = 0;
    if(false == detail::check_scale(function, scale, &result, Policy()))
    {
      return result;
    }
    if(false == detail::check_location(function, location, &result, Policy()))
    {
      return result;
    }
    if(false == detail::check_skew_normal_shape(function, shape, &result, Policy()))
    {
      return result;
    }
    if((boost::math::isinf)(x))
    {
       return 0; // pdf + and - infinity is zero.
    }
    // Below produces MSVC 4127 warnings, so the above used instead.
    //if(std::numeric_limits<RealType>::has_infinity && abs(x) == std::numeric_limits<RealType>::infinity())
    //{ // pdf + and - infinity is zero.
    //  return 0;
    //}
    if(false == detail::check_x(function, x, &result, Policy()))
    {
      return result;
    }

    const RealType transformed_x = (x-location)/scale;

    normal_distribution<RealType, Policy> std_normal;

    result = pdf(std_normal, transformed_x) * cdf(std_normal, shape*transformed_x) * 2 / scale;

    return result;
  } // pdf

  template <class RealType, class Policy>
  inline RealType cdf(const skew_normal_distribution<RealType, Policy>& dist, const RealType& x)
  {
    const RealType scale = dist.scale();
    const RealType location = dist.location();
    const RealType shape = dist.shape();

    static const char* function = "boost::math::cdf(const skew_normal_distribution<%1%>&, %1%)";
    RealType result = 0;
    if(false == detail::check_scale(function, scale, &result, Policy()))
    {
      return result;
    }
    if(false == detail::check_location(function, location, &result, Policy()))
    {
      return result;
    }
    if(false == detail::check_skew_normal_shape(function, shape, &result, Policy()))
    {
      return result;
    }
    if((boost::math::isinf)(x))
    {
      if(x < 0) return 0; // -infinity
      return 1; // + infinity
    }
    // These produce MSVC 4127 warnings, so the above used instead.
    //if(std::numeric_limits<RealType>::has_infinity && x == std::numeric_limits<RealType>::infinity())
    //{ // cdf +infinity is unity.
    //  return 1;
    //}
    //if(std::numeric_limits<RealType>::has_infinity && x == -std::numeric_limits<RealType>::infinity())
    //{ // cdf -infinity is zero.
    //  return 0;
    //}
    if(false == detail::check_x(function, x, &result, Policy()))
    {
      return result;
    }

    const RealType transformed_x = (x-location)/scale;

    normal_distribution<RealType, Policy> std_normal;

    result = cdf(std_normal, transformed_x) - owens_t(transformed_x, shape)*static_cast<RealType>(2);

    return result;
  } // cdf

  template <class RealType, class Policy>
  inline RealType cdf(const complemented2_type<skew_normal_distribution<RealType, Policy>, RealType>& c)
  {
    const RealType scale = c.dist.scale();
    const RealType location = c.dist.location();
    const RealType shape = c.dist.shape();
    const RealType x = c.param;

    static const char* function = "boost::math::cdf(const complement(skew_normal_distribution<%1%>&), %1%)";

    if((boost::math::isinf)(x))
    {
      if(x < 0) return 1; // cdf complement -infinity is unity.
      return 0; // cdf complement +infinity is zero
    }
    // These produce MSVC 4127 warnings, so the above used instead.
    //if(std::numeric_limits<RealType>::has_infinity && x == std::numeric_limits<RealType>::infinity())
    //{ // cdf complement +infinity is zero.
    //  return 0;
    //}
    //if(std::numeric_limits<RealType>::has_infinity && x == -std::numeric_limits<RealType>::infinity())
    //{ // cdf complement -infinity is unity.
    //  return 1;
    //}
    RealType result = 0;
    if(false == detail::check_scale(function, scale, &result, Policy()))
      return result;
    if(false == detail::check_location(function, location, &result, Policy()))
      return result;
    if(false == detail::check_skew_normal_shape(function, shape, &result, Policy()))
      return result;
    if(false == detail::check_x(function, x, &result, Policy()))
      return result;

    const RealType transformed_x = (x-location)/scale;

    normal_distribution<RealType, Policy> std_normal;

    result = cdf(complement(std_normal, transformed_x)) + owens_t(transformed_x, shape)*static_cast<RealType>(2);
    return result;
  } // cdf complement

  template <class RealType, class Policy>
  inline RealType location(const skew_normal_distribution<RealType, Policy>& dist)
  {
    return dist.location();
  }

  template <class RealType, class Policy>
  inline RealType scale(const skew_normal_distribution<RealType, Policy>& dist)
  {
    return dist.scale();
  }

  template <class RealType, class Policy>
  inline RealType shape(const skew_normal_distribution<RealType, Policy>& dist)
  {
    return dist.shape();
  }

  template <class RealType, class Policy>
  inline RealType mean(const skew_normal_distribution<RealType, Policy>& dist)
  {
    BOOST_MATH_STD_USING  // for ADL of std functions

    using namespace boost::math::constants;

    //const RealType delta = dist.shape() / sqrt(static_cast<RealType>(1)+dist.shape()*dist.shape());

    //return dist.location() + dist.scale() * delta * root_two_div_pi<RealType>();

    return dist.location() + dist.scale() * dist.shape() / sqrt(pi<RealType>()+pi<RealType>()*dist.shape()*dist.shape()) * root_two<RealType>();
  }

  template <class RealType, class Policy>
  inline RealType variance(const skew_normal_distribution<RealType, Policy>& dist)
  {
    using namespace boost::math::constants;

    const RealType delta2 = dist.shape() != 0 ? static_cast<RealType>(1) / (static_cast<RealType>(1)+static_cast<RealType>(1)/(dist.shape()*dist.shape())) : static_cast<RealType>(0);
    //const RealType inv_delta2 = static_cast<RealType>(1)+static_cast<RealType>(1)/(dist.shape()*dist.shape());

    RealType variance = dist.scale()*dist.scale()*(static_cast<RealType>(1)-two_div_pi<RealType>()*delta2);
    //RealType variance = dist.scale()*dist.scale()*(static_cast<RealType>(1)-two_div_pi<RealType>()/inv_delta2);

    return variance;
  }

  namespace detail
  {
    /*
      TODO No closed expression for mode, so use max of pdf.
    */
    
    template <class RealType, class Policy>
    inline RealType mode_fallback(const skew_normal_distribution<RealType, Policy>& dist)
    { // mode.
        static const char* function = "mode(skew_normal_distribution<%1%> const&)";
        const RealType scale = dist.scale();
        const RealType location = dist.location();
        const RealType shape = dist.shape();
        
        RealType result;
        if(!detail::check_scale(
          function,
          scale, &result, Policy())
          ||
        !detail::check_skew_normal_shape(
          function,
          shape,
          &result,
          Policy()))
        return result;

        if( shape == 0 )
        {
          return location;
        }

        if( shape < 0 )
        {
          skew_normal_distribution<RealType, Policy> D(0, 1, -shape);
          result = mode_fallback(D);
          result = location-scale*result;
          return result;
        }
        
        BOOST_MATH_STD_USING

        // 21 elements
        static const RealType shapes[] = {
          0.0,
          1.000000000000000e-004,
          2.069138081114790e-004,
          4.281332398719396e-004,
          8.858667904100824e-004,
          1.832980710832436e-003,
          3.792690190732250e-003,
          7.847599703514606e-003,
          1.623776739188722e-002,
          3.359818286283781e-002,
          6.951927961775606e-002,
          1.438449888287663e-001,
          2.976351441631319e-001,
          6.158482110660261e-001,
          1.274274985703135e+000,
          2.636650898730361e+000,
          5.455594781168514e+000,
          1.128837891684688e+001,
          2.335721469090121e+001,
          4.832930238571753e+001,
          1.000000000000000e+002};

        // 21 elements
        static const RealType guess[] = {
          0.0,
          5.000050000525391e-005,
          1.500015000148736e-004,
          3.500035000350010e-004,
          7.500075000752560e-004,
          1.450014500145258e-003,
          3.050030500305390e-003,
          6.250062500624765e-003,
          1.295012950129504e-002,
          2.675026750267495e-002,
          5.525055250552491e-002,
          1.132511325113255e-001,
          2.249522495224952e-001,
          3.992539925399257e-001,
          5.353553535535358e-001,
          4.954549545495457e-001,
          3.524535245352451e-001,
          2.182521825218249e-001,
          1.256512565125654e-001,
          6.945069450694508e-002,
          3.735037350373460e-002
        };

        const RealType* result_ptr = std::lower_bound(shapes, shapes+21, shape);

        typedef typename std::iterator_traits<RealType*>::difference_type diff_type;
        
        const diff_type d = std::distance(shapes, result_ptr);
        
        BOOST_MATH_ASSERT(d > static_cast<diff_type>(0));

        // refine
        if(d < static_cast<diff_type>(21)) // shape smaller 100
        {
          result = guess[d-static_cast<diff_type>(1)]
            + (guess[d]-guess[d-static_cast<diff_type>(1)])/(shapes[d]-shapes[d-static_cast<diff_type>(1)])
            * (shape-shapes[d-static_cast<diff_type>(1)]);
        }
        else // shape greater 100
        {
          result = 1e-4;
        }

        skew_normal_distribution<RealType, Policy> helper(0, 1, shape);
        
        result = detail::generic_find_mode_01(helper, result, function);
        
        result = result*scale + location;
        
        return result;
    } // mode_fallback
    
    
    /*
     * TODO No closed expression for mode, so use f'(x) = 0
     */
    template <class RealType, class Policy>
    struct skew_normal_mode_functor
    { 
      skew_normal_mode_functor(const boost::math::skew_normal_distribution<RealType, Policy> dist)
        : distribution(dist)
      {
      }

      boost::math::tuple<RealType, RealType> operator()(RealType const& x)
      {
        normal_distribution<RealType, Policy> std_normal;
        const RealType shape = distribution.shape();
        const RealType pdf_x = pdf(distribution, x);
        const RealType normpdf_x = pdf(std_normal, x);
        const RealType normpdf_ax = pdf(std_normal, x*shape);
        RealType fx = static_cast<RealType>(2)*shape*normpdf_ax*normpdf_x - x*pdf_x;
        RealType dx = static_cast<RealType>(2)*shape*x*normpdf_x*normpdf_ax*(static_cast<RealType>(1) + shape*shape) + pdf_x + x*fx;
        // return both function evaluation difference f(x) and 1st derivative f'(x).
        return boost::math::make_tuple(fx, -dx);
      }
    private:
      const boost::math::skew_normal_distribution<RealType, Policy> distribution;
    };
    
  } // namespace detail
  
  template <class RealType, class Policy>
  inline RealType mode(const skew_normal_distribution<RealType, Policy>& dist)
  {
    const RealType scale = dist.scale();
    const RealType location = dist.location();
    const RealType shape = dist.shape();

    static const char* function = "boost::math::mode(const skew_normal_distribution<%1%>&, %1%)";

    RealType result = 0;
    if(false == detail::check_scale(function, scale, &result, Policy()))
      return result;
    if(false == detail::check_location(function, location, &result, Policy()))
      return result;
    if(false == detail::check_skew_normal_shape(function, shape, &result, Policy()))
      return result;

    if( shape == 0 )
    {
      return location;
    }

    if( shape < 0 )
    {
      skew_normal_distribution<RealType, Policy> D(0, 1, -shape);
      result = mode(D);
      result = location-scale*result;
      return result;
    }

    // 21 elements
    static const RealType shapes[] = {
      0.0,
      static_cast<RealType>(1.000000000000000e-004),
      static_cast<RealType>(2.069138081114790e-004),
      static_cast<RealType>(4.281332398719396e-004),
      static_cast<RealType>(8.858667904100824e-004),
      static_cast<RealType>(1.832980710832436e-003),
      static_cast<RealType>(3.792690190732250e-003),
      static_cast<RealType>(7.847599703514606e-003),
      static_cast<RealType>(1.623776739188722e-002),
      static_cast<RealType>(3.359818286283781e-002),
      static_cast<RealType>(6.951927961775606e-002),
      static_cast<RealType>(1.438449888287663e-001),
      static_cast<RealType>(2.976351441631319e-001),
      static_cast<RealType>(6.158482110660261e-001),
      static_cast<RealType>(1.274274985703135e+000),
      static_cast<RealType>(2.636650898730361e+000),
      static_cast<RealType>(5.455594781168514e+000),
      static_cast<RealType>(1.128837891684688e+001),
      static_cast<RealType>(2.335721469090121e+001),
      static_cast<RealType>(4.832930238571753e+001),
      static_cast<RealType>(1.000000000000000e+002)
    };

    // 21 elements
    static const RealType guess[] = {
      0.0,
      static_cast<RealType>(5.000050000525391e-005),
      static_cast<RealType>(1.500015000148736e-004),
      static_cast<RealType>(3.500035000350010e-004),
      static_cast<RealType>(7.500075000752560e-004),
      static_cast<RealType>(1.450014500145258e-003),
      static_cast<RealType>(3.050030500305390e-003),
      static_cast<RealType>(6.250062500624765e-003),
      static_cast<RealType>(1.295012950129504e-002),
      static_cast<RealType>(2.675026750267495e-002),
      static_cast<RealType>(5.525055250552491e-002),
      static_cast<RealType>(1.132511325113255e-001),
      static_cast<RealType>(2.249522495224952e-001),
      static_cast<RealType>(3.992539925399257e-001),
      static_cast<RealType>(5.353553535535358e-001),
      static_cast<RealType>(4.954549545495457e-001),
      static_cast<RealType>(3.524535245352451e-001),
      static_cast<RealType>(2.182521825218249e-001),
      static_cast<RealType>(1.256512565125654e-001),
      static_cast<RealType>(6.945069450694508e-002),
      static_cast<RealType>(3.735037350373460e-002)
    };

    const RealType* result_ptr = std::lower_bound(shapes, shapes+21, shape);

    typedef typename std::iterator_traits<RealType*>::difference_type diff_type;
    
    const diff_type d = std::distance(shapes, result_ptr);
    
    BOOST_MATH_ASSERT(d > static_cast<diff_type>(0));

    // TODO: make the search bounds smarter, depending on the shape parameter
    RealType search_min = 0; // below zero was caught above
    RealType search_max = 0.55f; // will never go above 0.55

    // refine
    if(d < static_cast<diff_type>(21)) // shape smaller 100
    {
      // it is safe to assume that d > 0, because shape==0.0 is caught earlier
      result = guess[d-static_cast<diff_type>(1)]
        + (guess[d]-guess[d-static_cast<diff_type>(1)])/(shapes[d]-shapes[d-static_cast<diff_type>(1)])
        * (shape-shapes[d-static_cast<diff_type>(1)]);
    }
    else // shape greater 100
    {
      result = 1e-4f;
      search_max = guess[19]; // set 19 instead of 20 to have a safety margin because the table may not be exact @ shape=100
    }
    
    const int get_digits = policies::digits<RealType, Policy>();// get digits from policy, 
    std::uintmax_t m = policies::get_max_root_iterations<Policy>(); // and max iterations.

    skew_normal_distribution<RealType, Policy> helper(0, 1, shape);

    result = tools::newton_raphson_iterate(detail::skew_normal_mode_functor<RealType, Policy>(helper), result,
      search_min, search_max, get_digits, m);
    
    result = result*scale + location;

    return result;
  }
  

  
  template <class RealType, class Policy>
  inline RealType skewness(const skew_normal_distribution<RealType, Policy>& dist)
  {
    BOOST_MATH_STD_USING  // for ADL of std functions
    using namespace boost::math::constants;

    static const RealType factor = four_minus_pi<RealType>()/static_cast<RealType>(2);
    const RealType delta = dist.shape() / sqrt(static_cast<RealType>(1)+dist.shape()*dist.shape());

    return factor * pow(root_two_div_pi<RealType>() * delta, 3) /
      pow(static_cast<RealType>(1)-two_div_pi<RealType>()*delta*delta, static_cast<RealType>(1.5));
  }

  template <class RealType, class Policy>
  inline RealType kurtosis(const skew_normal_distribution<RealType, Policy>& dist)
  {
    return kurtosis_excess(dist)+static_cast<RealType>(3);
  }

  template <class RealType, class Policy>
  inline RealType kurtosis_excess(const skew_normal_distribution<RealType, Policy>& dist)
  {
    using namespace boost::math::constants;

    static const RealType factor = pi_minus_three<RealType>()*static_cast<RealType>(2);

    const RealType delta2 = dist.shape() != 0 ? static_cast<RealType>(1) / (static_cast<RealType>(1)+static_cast<RealType>(1)/(dist.shape()*dist.shape())) : static_cast<RealType>(0);

    const RealType x = static_cast<RealType>(1)-two_div_pi<RealType>()*delta2;
    const RealType y = two_div_pi<RealType>() * delta2;

    return factor * y*y / (x*x);
  }

  namespace detail
  {

    template <class RealType, class Policy>
    struct skew_normal_quantile_functor
    { 
      skew_normal_quantile_functor(const boost::math::skew_normal_distribution<RealType, Policy> dist, RealType const& p)
        : distribution(dist), prob(p)
      {
      }

      boost::math::tuple<RealType, RealType> operator()(RealType const& x)
      {
        RealType c = cdf(distribution, x);
        RealType fx = c - prob;  // Difference cdf - value - to minimize.
        RealType dx = pdf(distribution, x); // pdf is 1st derivative.
        // return both function evaluation difference f(x) and 1st derivative f'(x).
        return boost::math::make_tuple(fx, dx);
      }
    private:
      const boost::math::skew_normal_distribution<RealType, Policy> distribution;
      RealType prob; 
    };

  } // namespace detail

  template <class RealType, class Policy>
  inline RealType quantile(const skew_normal_distribution<RealType, Policy>& dist, const RealType& p)
  {
    const RealType scale = dist.scale();
    const RealType location = dist.location();
    const RealType shape = dist.shape();

    static const char* function = "boost::math::quantile(const skew_normal_distribution<%1%>&, %1%)";

    RealType result = 0;
    if(false == detail::check_scale(function, scale, &result, Policy()))
      return result;
    if(false == detail::check_location(function, location, &result, Policy()))
      return result;
    if(false == detail::check_skew_normal_shape(function, shape, &result, Policy()))
      return result;
    if(false == detail::check_probability(function, p, &result, Policy()))
      return result;

    // Compute initial guess via Cornish-Fisher expansion.
    RealType x = -boost::math::erfc_inv(2 * p, Policy()) * constants::root_two<RealType>();

    // Avoid unnecessary computations if there is no skew.
    if(shape != 0)
    {
      const RealType skew = skewness(dist);
      const RealType exk = kurtosis_excess(dist);

      x = x + (x*x-static_cast<RealType>(1))*skew/static_cast<RealType>(6)
      + x*(x*x-static_cast<RealType>(3))*exk/static_cast<RealType>(24)
      - x*(static_cast<RealType>(2)*x*x-static_cast<RealType>(5))*skew*skew/static_cast<RealType>(36);
    } // if(shape != 0)

    result = standard_deviation(dist)*x+mean(dist);

    // handle special case of non-skew normal distribution.
    if(shape == 0)
      return result;

    // refine the result by numerically searching the root of (p-cdf)

    const RealType search_min = range(dist).first;
    const RealType search_max = range(dist).second;

    const int get_digits = policies::digits<RealType, Policy>();// get digits from policy, 
    std::uintmax_t m = policies::get_max_root_iterations<Policy>(); // and max iterations.

    result = tools::newton_raphson_iterate(detail::skew_normal_quantile_functor<RealType, Policy>(dist, p), result,
      search_min, search_max, get_digits, m);

    return result;
  } // quantile

  template <class RealType, class Policy>
  inline RealType quantile(const complemented2_type<skew_normal_distribution<RealType, Policy>, RealType>& c)
  {
    const RealType scale = c.dist.scale();
    const RealType location = c.dist.location();
    const RealType shape = c.dist.shape();

    static const char* function = "boost::math::quantile(const complement(skew_normal_distribution<%1%>&), %1%)";
    RealType result = 0;
    if(false == detail::check_scale(function, scale, &result, Policy()))
      return result;
    if(false == detail::check_location(function, location, &result, Policy()))
      return result;
    if(false == detail::check_skew_normal_shape(function, shape, &result, Policy()))
      return result;
    RealType q = c.param;
    if(false == detail::check_probability(function, q, &result, Policy()))
      return result;

    skew_normal_distribution<RealType, Policy> D(-location, scale, -shape);

    result = -quantile(D, q);

    return result;
  } // quantile


} // namespace math
} // namespace boost

// This include must be at the end, *after* the accessors
// for this distribution have been defined, in order to
// keep compilers that support two-phase lookup happy.
#include <boost/math/distributions/detail/derived_accessors.hpp>

#endif // BOOST_STATS_SKEW_NORMAL_HPP



// Copyright 2018 Hans Dembinski
//
// Distributed under the Boost Software License, version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_HISTOGRAM_ACCUMULATORS_WEIGHTED_MEAN_HPP
#define BOOST_HISTOGRAM_ACCUMULATORS_WEIGHTED_MEAN_HPP

#include <boost/core/nvp.hpp>
#include <boost/histogram/detail/square.hpp>
#include <boost/histogram/fwd.hpp> // for weighted_mean<>
#include <boost/histogram/weight.hpp>
#include <cassert>
#include <type_traits>

namespace boost {
namespace histogram {
namespace accumulators {

/**
  Calculates mean and variance of weighted sample.

  Uses West's incremental algorithm to improve numerical stability
  of mean and variance computation.
*/
template <class ValueType>
class weighted_mean {
public:
  using value_type = ValueType;
  using const_reference = const value_type&;

  weighted_mean() = default;

  /// Allow implicit conversion from other weighted_means.
  template <class T>
  weighted_mean(const weighted_mean<T>& o)
      : sum_of_weights_{o.sum_of_weights_}
      , sum_of_weights_squared_{o.sum_of_weights_squared_}
      , weighted_mean_{o.weighted_mean_}
      , sum_of_weighted_deltas_squared_{o.sum_of_weighted_deltas_squared_} {}

  /// Initialize to external sum of weights, sum of weights squared, mean, and variance.
  weighted_mean(const_reference wsum, const_reference wsum2, const_reference mean,
                const_reference variance)
      : sum_of_weights_(wsum)
      , sum_of_weights_squared_(wsum2)
      , weighted_mean_(mean)
      , sum_of_weighted_deltas_squared_(
            variance * (sum_of_weights_ - sum_of_weights_squared_ / sum_of_weights_)) {}

  /// Insert sample x.
  void operator()(const_reference x) { operator()(weight(1), x); }

  /// Insert sample x with weight w.
  void operator()(const weight_type<value_type>& w, const_reference x) {
    sum_of_weights_ += w.value;
    sum_of_weights_squared_ += w.value * w.value;
    const auto delta = x - weighted_mean_;
    weighted_mean_ += w.value * delta / sum_of_weights_;
    sum_of_weighted_deltas_squared_ += w.value * delta * (x - weighted_mean_);
  }

  /// Add another weighted_mean.
  weighted_mean& operator+=(const weighted_mean& rhs) {
    if (rhs.sum_of_weights_ == 0) return *this;

    // see mean.hpp for derivation of correct formula

    const auto n1 = sum_of_weights_;
    const auto mu1 = weighted_mean_;
    const auto n2 = rhs.sum_of_weights_;
    const auto mu2 = rhs.weighted_mean_;

    sum_of_weights_ += rhs.sum_of_weights_;
    sum_of_weights_squared_ += rhs.sum_of_weights_squared_;
    weighted_mean_ = (n1 * mu1 + n2 * mu2) / sum_of_weights_;

    sum_of_weighted_deltas_squared_ += rhs.sum_of_weighted_deltas_squared_;
    sum_of_weighted_deltas_squared_ += n1 * detail::square(weighted_mean_ - mu1);
    sum_of_weighted_deltas_squared_ += n2 * detail::square(weighted_mean_ - mu2);

    return *this;
  }

  /** Scale by value.

   This acts as if all samples were scaled by the value.
  */
  weighted_mean& operator*=(const_reference s) noexcept {
    weighted_mean_ *= s;
    sum_of_weighted_deltas_squared_ *= s * s;
    return *this;
  }

  bool operator==(const weighted_mean& rhs) const noexcept {
    return sum_of_weights_ == rhs.sum_of_weights_ &&
           sum_of_weights_squared_ == rhs.sum_of_weights_squared_ &&
           weighted_mean_ == rhs.weighted_mean_ &&
           sum_of_weighted_deltas_squared_ == rhs.sum_of_weighted_deltas_squared_;
  }

  bool operator!=(const weighted_mean& rhs) const noexcept { return !operator==(rhs); }

  /// Return sum of weights.
  const_reference sum_of_weights() const noexcept { return sum_of_weights_; }

  /// Return sum of weights squared (variance of weight distribution).
  const_reference sum_of_weights_squared() const noexcept {
    return sum_of_weights_squared_;
  }

  /** Return effective counts.

    This corresponds to the equivalent number of unweighted samples that would
    have the same variance as this sample. count() should be used to check whether
    value() and variance() are defined, see documentation of value() and variance().
    count() can be used to compute the variance of the mean by dividing variance()
    by count().
  */
  value_type count() const noexcept {
    // see https://en.wikipedia.org/wiki/Effective_sample_size#weighted_samples
    return detail::square(sum_of_weights_) / sum_of_weights_squared_;
  }

  /** Return mean value of accumulated weighted samples.

    The result is undefined, if count() == 0.
  */
  const_reference value() const noexcept { return weighted_mean_; }

  /** Return variance of accumulated weighted samples.

    The result is undefined, if count() == 0 or count() == 1.
  */
  value_type variance() const {
    // see https://en.wikipedia.org/wiki/Weighted_arithmetic_mean#Reliability_weights
    return sum_of_weighted_deltas_squared_ /
           (sum_of_weights_ - sum_of_weights_squared_ / sum_of_weights_);
  }

  template <class Archive>
  void serialize(Archive& ar, unsigned /* version */) {
    ar& make_nvp("sum_of_weights", sum_of_weights_);
    ar& make_nvp("sum_of_weights_squared", sum_of_weights_squared_);
    ar& make_nvp("weighted_mean", weighted_mean_);
    ar& make_nvp("sum_of_weighted_deltas_squared", sum_of_weighted_deltas_squared_);
  }

private:
  value_type sum_of_weights_{};
  value_type sum_of_weights_squared_{};
  value_type weighted_mean_{};
  value_type sum_of_weighted_deltas_squared_{};
};

} // namespace accumulators
} // namespace histogram
} // namespace boost

#ifndef BOOST_HISTOGRAM_DOXYGEN_INVOKED
namespace std {
template <class T, class U>
/// Specialization for boost::histogram::accumulators::weighted_mean.
struct common_type<boost::histogram::accumulators::weighted_mean<T>,
                   boost::histogram::accumulators::weighted_mean<U>> {
  using type = boost::histogram::accumulators::weighted_mean<common_type_t<T, U>>;
};
} // namespace std
#endif

#endif

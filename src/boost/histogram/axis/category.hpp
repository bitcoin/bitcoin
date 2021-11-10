// Copyright 2015-2018 Hans Dembinski
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_HISTOGRAM_AXIS_CATEGORY_HPP
#define BOOST_HISTOGRAM_AXIS_CATEGORY_HPP

#include <algorithm>
#include <boost/core/nvp.hpp>
#include <boost/histogram/axis/iterator.hpp>
#include <boost/histogram/axis/metadata_base.hpp>
#include <boost/histogram/axis/option.hpp>
#include <boost/histogram/detail/detect.hpp>
#include <boost/histogram/detail/relaxed_equal.hpp>
#include <boost/histogram/fwd.hpp>
#include <boost/throw_exception.hpp>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace boost {
namespace histogram {
namespace axis {

/**
  Maps at a set of unique values to bin indices.

  The axis maps a set of values to bins, following the order of arguments in the
  constructor. The optional overflow bin for this axis counts input values that
  are not part of the set. Binning has O(N) complexity, but with a very small
  factor. For small N (the typical use case) it beats other kinds of lookup.

  @tparam Value input value type, must be equal-comparable.
  @tparam MetaData type to store meta data.
  @tparam Options see boost::histogram::axis::option.
  @tparam Allocator allocator to use for dynamic memory management.

  The options `underflow` and `circular` are not allowed. The options `growth`
  and `overflow` are mutually exclusive.
*/
template <class Value, class MetaData, class Options, class Allocator>
class category : public iterator_mixin<category<Value, MetaData, Options, Allocator>>,
                 public metadata_base_t<MetaData> {
  // these must be private, so that they are not automatically inherited
  using value_type = Value;
  using metadata_base = metadata_base_t<MetaData>;
  using metadata_type = typename metadata_base::metadata_type;
  using options_type = detail::replace_default<Options, option::overflow_t>;
  using allocator_type = Allocator;
  using vector_type = std::vector<value_type, allocator_type>;

  static_assert(!options_type::test(option::underflow),
                "category axis cannot have underflow");
  static_assert(!options_type::test(option::circular),
                "category axis cannot be circular");
  static_assert(!(options_type::test(option::growth) &&
                  options_type::test(option::overflow)),
                "growing category axis cannot have entries in overflow bin");

public:
  constexpr category() = default;
  explicit category(allocator_type alloc) : vec_(alloc) {}

  /** Construct from iterator range of unique values.
   *
   * \param begin     begin of category range of unique values.
   * \param end       end of category range of unique values.
   * \param meta      description of the axis.
   * \param alloc     allocator instance to use.
   */
  template <class It, class = detail::requires_iterator<It>>
  category(It begin, It end, metadata_type meta = {}, allocator_type alloc = {})
      : metadata_base(std::move(meta)), vec_(alloc) {
    if (std::distance(begin, end) < 0)
      BOOST_THROW_EXCEPTION(
          std::invalid_argument("end must be reachable by incrementing begin"));
    vec_.reserve(std::distance(begin, end));
    while (begin != end) vec_.emplace_back(*begin++);
  }

  /** Construct axis from iterable sequence of unique values.
   *
   * \param iterable sequence of unique values.
   * \param meta     description of the axis.
   * \param alloc    allocator instance to use.
   */
  template <class C, class = detail::requires_iterable<C>>
  category(const C& iterable, metadata_type meta = {}, allocator_type alloc = {})
      : category(std::begin(iterable), std::end(iterable), std::move(meta),
                 std::move(alloc)) {}

  /** Construct axis from an initializer list of unique values.
   *
   * \param list   `std::initializer_list` of unique values.
   * \param meta   description of the axis.
   * \param alloc  allocator instance to use.
   */
  template <class U>
  category(std::initializer_list<U> list, metadata_type meta = {},
           allocator_type alloc = {})
      : category(list.begin(), list.end(), std::move(meta), std::move(alloc)) {}

  /// Constructor used by algorithm::reduce to shrink and rebin (not for users).
  category(const category& src, index_type begin, index_type end, unsigned merge)
      // LCOV_EXCL_START: gcc-8 is missing the delegated ctor for no reason
      : category(src.vec_.begin() + begin, src.vec_.begin() + end, src.metadata(),
                 src.get_allocator())
  // LCOV_EXCL_STOP
  {
    if (merge > 1)
      BOOST_THROW_EXCEPTION(std::invalid_argument("cannot merge bins for category axis"));
  }

  /// Return index for value argument.
  index_type index(const value_type& x) const noexcept {
    const auto beg = vec_.begin();
    const auto end = vec_.end();
    return static_cast<index_type>(std::distance(beg, std::find(beg, end, x)));
  }

  /// Returns index and shift (if axis has grown) for the passed argument.
  std::pair<index_type, index_type> update(const value_type& x) {
    const auto i = index(x);
    if (i < size()) return {i, 0};
    vec_.emplace_back(x);
    return {i, -1};
  }

  /// Return value for index argument.
  /// Throws `std::out_of_range` if the index is out of bounds.
  auto value(index_type idx) const
      -> std::conditional_t<std::is_scalar<value_type>::value, value_type,
                            const value_type&> {
    if (idx < 0 || idx >= size())
      BOOST_THROW_EXCEPTION(std::out_of_range("category index out of range"));
    return vec_[idx];
  }

  /// Return value for index argument; alias for value(...).
  decltype(auto) bin(index_type idx) const { return value(idx); }

  /// Returns the number of bins, without over- or underflow.
  index_type size() const noexcept { return static_cast<index_type>(vec_.size()); }

  /// Returns the options.
  static constexpr unsigned options() noexcept { return options_type::value; }

  /// Whether the axis is inclusive (see axis::traits::is_inclusive).
  static constexpr bool inclusive() noexcept {
    return options() & (option::overflow | option::growth);
  }

  /// Indicate that the axis is not ordered.
  static constexpr bool ordered() noexcept { return false; }

  template <class V, class M, class O, class A>
  bool operator==(const category<V, M, O, A>& o) const noexcept {
    const auto& a = vec_;
    const auto& b = o.vec_;
    return std::equal(a.begin(), a.end(), b.begin(), b.end(), detail::relaxed_equal{}) &&
           detail::relaxed_equal{}(this->metadata(), o.metadata());
  }

  template <class V, class M, class O, class A>
  bool operator!=(const category<V, M, O, A>& o) const noexcept {
    return !operator==(o);
  }

  allocator_type get_allocator() const { return vec_.get_allocator(); }

  template <class Archive>
  void serialize(Archive& ar, unsigned /* version */) {
    ar& make_nvp("seq", vec_);
    ar& make_nvp("meta", this->metadata());
  }

private:
  vector_type vec_;

  template <class V, class M, class O, class A>
  friend class category;
};

#if __cpp_deduction_guides >= 201606

template <class T>
category(std::initializer_list<T>)
    ->category<detail::replace_cstring<std::decay_t<T>>, null_type>;

template <class T, class M>
category(std::initializer_list<T>, M)
    ->category<detail::replace_cstring<std::decay_t<T>>,
               detail::replace_cstring<std::decay_t<M>>>;

#endif

} // namespace axis
} // namespace histogram
} // namespace boost

#endif

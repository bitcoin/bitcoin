// Copyright 2015-2016 Hans Dembinski
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_HISTOGRAM_INDEXED_HPP
#define BOOST_HISTOGRAM_INDEXED_HPP

#include <array>
#include <boost/config.hpp> // BOOST_ATTRIBUTE_NODISCARD
#include <boost/histogram/axis/traits.hpp>
#include <boost/histogram/detail/axes.hpp>
#include <boost/histogram/detail/detect.hpp>
#include <boost/histogram/detail/iterator_adaptor.hpp>
#include <boost/histogram/detail/operators.hpp>
#include <boost/histogram/fwd.hpp>
#include <boost/histogram/unsafe_access.hpp>
#include <iterator>
#include <type_traits>
#include <utility> // std::get

namespace boost {
namespace histogram {

namespace detail {
using std::get;

template <std::size_t I, class T>
auto get(T&& t) -> decltype(t[0]) {
  return t[I];
}
} // namespace detail

/** Coverage mode of the indexed range generator.

  Defines options for the iteration strategy.
*/
enum class coverage {
  inner, /*!< iterate over inner bins, exclude underflow and overflow */
  all,   /*!< iterate over all bins, including underflow and overflow */
};

/** Input iterator range over histogram bins with multi-dimensional index.

  The iterator returned by begin() can only be incremented. If several copies of the
  input iterators exist, the other copies become invalid if one of them is incremented.
*/
template <class Histogram>
class BOOST_ATTRIBUTE_NODISCARD indexed_range {
private:
  using histogram_type = Histogram;
  static constexpr unsigned buffer_size =
      detail::buffer_size<typename std::decay_t<histogram_type>::axes_type>::value;

public:
  /// implementation detail
  using value_iterator = std::conditional_t<std::is_const<histogram_type>::value,
                                            typename histogram_type::const_iterator,
                                            typename histogram_type::iterator>;
  /// implementation detail
  using value_reference = typename std::iterator_traits<value_iterator>::reference;
  /// implementation detail
  using value_type = typename std::iterator_traits<value_iterator>::value_type;

  class iterator;
  using range_iterator [[deprecated("use iterator instead")]] = iterator; ///< deprecated

  /** Lightweight view to access value and index of current cell.

    The methods provide access to the current cell indices and bins. It acts like a
    pointer to the cell value, and in a limited way also like a reference. To interoperate
    with the algorithms of the standard library, the accessor is implicitly convertible to
    a cell value. Assignments and comparisons are passed through to the cell. An accessor
    is coupled to its parent indexed_range::iterator. Moving the parent iterator
    forward also updates the linked accessor. Accessors are not copyable. They cannot be
    stored in containers, but indexed_range::iterator can be stored.
  */
  class BOOST_ATTRIBUTE_NODISCARD accessor : detail::mirrored<accessor, void> {
  public:
    /// Array-like view into the current multi-dimensional index.
    class index_view {
      using index_pointer = const typename iterator::index_data*;

    public:
      using const_reference = const axis::index_type&;
      using reference [[deprecated("use const_reference instead")]] =
          const_reference; ///< deprecated

      /// implementation detail
      class const_iterator
          : public detail::iterator_adaptor<const_iterator, index_pointer,
                                            const_reference> {
      public:
        const_reference operator*() const noexcept { return const_iterator::base()->idx; }

      private:
        explicit const_iterator(index_pointer i) noexcept
            : const_iterator::iterator_adaptor_(i) {}

        friend class index_view;
      };

      const_iterator begin() const noexcept { return const_iterator{begin_}; }
      const_iterator end() const noexcept { return const_iterator{end_}; }
      std::size_t size() const noexcept {
        return static_cast<std::size_t>(end_ - begin_);
      }
      const_reference operator[](unsigned d) const noexcept { return begin_[d].idx; }
      const_reference at(unsigned d) const { return begin_[d].idx; }

    private:
      /// implementation detail
      index_view(index_pointer b, index_pointer e) : begin_(b), end_(e) {}

      index_pointer begin_, end_;
      friend class accessor;
    };

    // assignment is pass-through
    accessor& operator=(const accessor& o) {
      get() = o.get();
      return *this;
    }

    // assignment is pass-through
    template <class T>
    accessor& operator=(const T& x) {
      get() = x;
      return *this;
    }

    /// Returns the cell reference.
    value_reference get() const noexcept { return *(iter_.iter_); }
    /// @copydoc get()
    value_reference operator*() const noexcept { return get(); }
    /// Access fields and methods of the cell object.
    value_iterator operator->() const noexcept { return iter_.iter_; }

    /// Access current index.
    /// @param d axis dimension.
    axis::index_type index(unsigned d = 0) const noexcept {
      return iter_.indices_[d].idx;
    }

    /// Access indices as an iterable range.
    index_view indices() const noexcept {
      assert(iter_.indices_.hist_);
      return {iter_.indices_.begin(), iter_.indices_.end()};
    }

    /// Access current bin.
    /// @tparam N axis dimension.
    template <unsigned N = 0>
    decltype(auto) bin(std::integral_constant<unsigned, N> = {}) const {
      assert(iter_.indices_.hist_);
      return iter_.indices_.hist_->axis(std::integral_constant<unsigned, N>())
          .bin(index(N));
    }

    /// Access current bin.
    /// @param d axis dimension.
    decltype(auto) bin(unsigned d) const {
      assert(iter_.indices_.hist_);
      return iter_.indices_.hist_->axis(d).bin(index(d));
    }

    /** Computes density in current cell.

      The density is computed as the cell value divided by the product of bin widths. Axes
      without bin widths, like axis::category, are treated as having unit bin with.
    */
    double density() const {
      assert(iter_.indices_.hist_);
      double x = 1;
      unsigned d = 0;
      iter_.indices_.hist_->for_each_axis([&](const auto& a) {
        const auto w = axis::traits::width_as<double>(a, this->index(d++));
        x *= w != 0 ? w : 1;
      });
      return get() / x;
    }

    // forward all comparison operators to the value
    bool operator<(const accessor& o) noexcept { return get() < o.get(); }
    bool operator>(const accessor& o) noexcept { return get() > o.get(); }
    bool operator==(const accessor& o) noexcept { return get() == o.get(); }
    bool operator!=(const accessor& o) noexcept { return get() != o.get(); }
    bool operator<=(const accessor& o) noexcept { return get() <= o.get(); }
    bool operator>=(const accessor& o) noexcept { return get() >= o.get(); }

    template <class U>
    bool operator<(const U& o) const noexcept {
      return get() < o;
    }

    template <class U>
    bool operator>(const U& o) const noexcept {
      return get() > o;
    }

    template <class U>
    bool operator==(const U& o) const noexcept {
      return get() == o;
    }

    template <class U>
    bool operator!=(const U& o) const noexcept {
      return get() != o;
    }

    template <class U>
    bool operator<=(const U& o) const noexcept {
      return get() <= o;
    }

    template <class U>
    bool operator>=(const U& o) const noexcept {
      return get() >= o;
    }

    operator value_type() const noexcept { return get(); }

  private:
    accessor(iterator& i) noexcept : iter_(i) {}

    accessor(const accessor&) = default; // only callable by indexed_range::iterator

    iterator& iter_;

    friend class iterator;
  };

  /// implementation detail
  class iterator {
  public:
    using value_type = typename indexed_range::value_type;
    using reference = accessor;

  private:
    struct pointer_proxy {
      reference* operator->() noexcept { return std::addressof(ref_); }
      reference ref_;
    };

  public:
    using pointer = pointer_proxy;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::forward_iterator_tag;

    reference operator*() noexcept { return *this; }
    pointer operator->() noexcept { return pointer_proxy{operator*()}; }

    iterator& operator++() {
      assert(iter_ < indices_.hist_->end());
      const auto cbeg = indices_.begin();
      auto c = cbeg;
      ++iter_;
      ++c->idx;
      if (c->idx < c->end) return *this;
      while (c->idx == c->end) {
        iter_ += c->end_skip;
        if (++c == indices_.end()) return *this;
        ++c->idx;
      }
      while (c-- != cbeg) {
        c->idx = c->begin;
        iter_ += c->begin_skip;
      }
      return *this;
    }

    iterator operator++(int) {
      auto prev = *this;
      operator++();
      return prev;
    }

    bool operator==(const iterator& x) const noexcept { return iter_ == x.iter_; }
    bool operator!=(const iterator& x) const noexcept { return !operator==(x); }

    // make iterator ready for C++17 sentinels
    bool operator==(const value_iterator& x) const noexcept { return iter_ == x; }
    bool operator!=(const value_iterator& x) const noexcept { return !operator==(x); }

    // useful for iterator debugging
    std::size_t offset() const noexcept { return iter_ - indices_.hist_->begin(); }

  private:
    iterator(value_iterator i, histogram_type& h) : iter_(i), indices_(&h) {}

    value_iterator iter_;

    struct index_data {
      axis::index_type idx, begin, end;
      std::size_t begin_skip, end_skip;
    };

    struct indices_t : private std::array<index_data, buffer_size> {
      using base_type = std::array<index_data, buffer_size>;
      using pointer = index_data*;
      using const_pointer = const index_data*;

      indices_t(histogram_type* h) noexcept : hist_{h} {}

      using base_type::operator[];
      unsigned size() const noexcept { return hist_->rank(); }
      pointer begin() noexcept { return base_type::data(); }
      const_pointer begin() const noexcept { return base_type::data(); }
      pointer end() noexcept { return begin() + size(); }
      const_pointer end() const noexcept { return begin() + size(); }

      histogram_type* hist_;
    } indices_;

    friend class indexed_range;
  };

  indexed_range(histogram_type& hist, coverage cov)
      : indexed_range(hist, make_range(hist, cov)) {}

  template <class Iterable, class = detail::requires_iterable<Iterable>>
  indexed_range(histogram_type& hist, Iterable&& range)
      : begin_(hist.begin(), hist), end_(hist.end(), hist) {
    auto r_begin = std::begin(range);
    assert(std::distance(r_begin, std::end(range)) == static_cast<int>(hist.rank()));

    begin_.indices_.hist_->for_each_axis([ca = begin_.indices_.begin(), r_begin,
                                          stride = std::size_t{1},
                                          this](const auto& a) mutable {
      const auto size = a.size();

      using opt = axis::traits::get_options<std::decay_t<decltype(a)>>;
      constexpr axis::index_type start = opt::test(axis::option::underflow) ? -1 : 0;
      const auto stop = size + (opt::test(axis::option::overflow) ? 1 : 0);

      ca->begin = std::max(start, detail::get<0>(*r_begin));
      ca->end = std::min(stop, detail::get<1>(*r_begin));
      assert(ca->begin <= ca->end);
      ca->idx = ca->begin;

      ca->begin_skip = static_cast<std::size_t>(ca->begin - start) * stride;
      ca->end_skip = static_cast<std::size_t>(stop - ca->end) * stride;
      begin_.iter_ += ca->begin_skip;

      stride *= stop - start;

      ++ca;
      ++r_begin;
    });
  }

  iterator begin() noexcept { return begin_; }
  iterator end() noexcept { return end_; }

private:
  auto make_range(histogram_type& hist, coverage cov) {
    using range_item = std::array<axis::index_type, 2>;
    auto b = detail::make_stack_buffer<range_item>(unsafe_access::axes(hist));
    hist.for_each_axis([cov, it = std::begin(b)](const auto& a) mutable {
      (*it)[0] = 0;
      (*it)[1] = a.size();
      if (cov == coverage::all) {
        (*it)[0] -= 1;
        (*it)[1] += 1;
      } else
        assert(cov == coverage::inner);
      ++it;
    });
    return b;
  }

  iterator begin_, end_;
};

/** Generates an indexed range of <a
  href="https://en.cppreference.com/w/cpp/named_req/ForwardIterator">forward iterators</a>
  over the histogram cells.

  Use this in a range-based for loop:

  ```
  for (auto&& x : indexed(hist)) { ... }
  ```

  This generates an optimized loop which is nearly always faster than a hand-written loop
  over the histogram cells. The iterators dereference to an indexed_range::accessor, which
  has methods to query the current indices and bins and acts like a pointer to the cell
  value. The returned iterators are forward iterators. They can be stored in a container,
  but may not be used after the life-time of the histogram ends.

  @returns indexed_range

  @param hist Reference to the histogram.
  @param cov  Iterate over all or only inner bins (optional, default: inner).
 */
template <class Histogram>
auto indexed(Histogram&& hist, coverage cov = coverage::inner) {
  return indexed_range<std::remove_reference_t<Histogram>>{std::forward<Histogram>(hist),
                                                           cov};
}

/** Generates and indexed range <a
  href="https://en.cppreference.com/w/cpp/named_req/ForwardIterator">forward iterators</a>
  over a rectangular region of histogram cells.

  Use this in a range-based for loop. Example:
  ```
  auto hist = make_histogram(axis::integer<>(0, 4), axis::integer<>(2, 6));
  axis::index_type range[2] = {{1, 3}, {0, 2}};
  for (auto&& x : indexed(hist, range)) { ... }
  ```
  This skips the first and last index of the first axis, and the last two indices of the
  second.

  @returns indexed_range

  @param hist  Reference to the histogram.
  @param range Iterable over items with two axis::index_type values, which mark the
               begin and end index of each axis. The length of the iterable must be
               equal to the rank of the histogram. The begin index must be smaller than
               the end index. Index ranges wider than the actual range are reduced to
               the actual range including underflow and overflow indices.
*/
template <class Histogram, class Iterable, class = detail::requires_iterable<Iterable>>
auto indexed(Histogram&& hist, Iterable&& range) {
  return indexed_range<std::remove_reference_t<Histogram>>{std::forward<Histogram>(hist),
                                                           std::forward<Iterable>(range)};
}

} // namespace histogram
} // namespace boost

#endif

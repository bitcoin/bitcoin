// Copyright David Abrahams 2003.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef COUNTING_ITERATOR_DWA200348_HPP
# define COUNTING_ITERATOR_DWA200348_HPP

# include <boost/iterator/iterator_adaptor.hpp>
# include <boost/detail/numeric_traits.hpp>
# include <boost/mpl/bool.hpp>
# include <boost/mpl/if.hpp>
# include <boost/mpl/identity.hpp>
# include <boost/mpl/eval_if.hpp>

namespace boost {
namespace iterators {

template <
    class Incrementable
  , class CategoryOrTraversal
  , class Difference
>
class counting_iterator;

namespace detail
{
  // Try to detect numeric types at compile time in ways compatible
  // with the limitations of the compiler and library.
  template <class T>
  struct is_numeric_impl
  {
      // For a while, this wasn't true, but we rely on it below. This is a regression assert.
      BOOST_STATIC_ASSERT(::boost::is_integral<char>::value);

# ifndef BOOST_NO_LIMITS_COMPILE_TIME_CONSTANTS

      BOOST_STATIC_CONSTANT(bool, value = std::numeric_limits<T>::is_specialized);

# else

#  if !BOOST_WORKAROUND(BOOST_BORLANDC, BOOST_TESTED_AT(0x551))
      BOOST_STATIC_CONSTANT(
          bool, value = (
              boost::is_convertible<int,T>::value
           && boost::is_convertible<T,int>::value
      ));
#  else
    BOOST_STATIC_CONSTANT(bool, value = ::boost::is_arithmetic<T>::value);
#  endif

# endif
  };

  template <class T>
  struct is_numeric
    : mpl::bool_<(::boost::iterators::detail::is_numeric_impl<T>::value)>
  {};

#  if defined(BOOST_HAS_LONG_LONG)
  template <>
  struct is_numeric< ::boost::long_long_type>
    : mpl::true_ {};

  template <>
  struct is_numeric< ::boost::ulong_long_type>
    : mpl::true_ {};
#  endif

  // Some compilers fail to have a numeric_limits specialization
  template <>
  struct is_numeric<wchar_t>
    : mpl::true_ {};

  template <class T>
  struct numeric_difference
  {
      typedef typename boost::detail::numeric_traits<T>::difference_type type;
  };

  BOOST_STATIC_ASSERT(is_numeric<int>::value);

  template <class Incrementable, class CategoryOrTraversal, class Difference>
  struct counting_iterator_base
  {
      typedef typename detail::ia_dflt_help<
          CategoryOrTraversal
        , mpl::eval_if<
              is_numeric<Incrementable>
            , mpl::identity<random_access_traversal_tag>
            , iterator_traversal<Incrementable>
          >
      >::type traversal;

      typedef typename detail::ia_dflt_help<
          Difference
        , mpl::eval_if<
              is_numeric<Incrementable>
            , numeric_difference<Incrementable>
            , iterator_difference<Incrementable>
          >
      >::type difference;

      typedef iterator_adaptor<
          counting_iterator<Incrementable, CategoryOrTraversal, Difference> // self
        , Incrementable                                           // Base
        , Incrementable                                           // Value
# ifndef BOOST_ITERATOR_REF_CONSTNESS_KILLS_WRITABILITY
          const  // MSVC won't strip this.  Instead we enable Thomas'
                 // criterion (see boost/iterator/detail/facade_iterator_category.hpp)
# endif
        , traversal
        , Incrementable const&                                    // reference
        , difference
      > type;
  };

  // Template class distance_policy_select -- choose a policy for computing the
  // distance between counting_iterators at compile-time based on whether or not
  // the iterator wraps an integer or an iterator, using "poor man's partial
  // specialization".

  template <bool is_integer> struct distance_policy_select;

  // A policy for wrapped iterators
  template <class Difference, class Incrementable1, class Incrementable2>
  struct iterator_distance
  {
      static Difference distance(Incrementable1 x, Incrementable2 y)
      {
          return y - x;
      }
  };

  // A policy for wrapped numbers
  template <class Difference, class Incrementable1, class Incrementable2>
  struct number_distance
  {
      static Difference distance(Incrementable1 x, Incrementable2 y)
      {
          return boost::detail::numeric_distance(x, y);
      }
  };
}

template <
    class Incrementable
  , class CategoryOrTraversal = use_default
  , class Difference = use_default
>
class counting_iterator
  : public detail::counting_iterator_base<
        Incrementable, CategoryOrTraversal, Difference
    >::type
{
    typedef typename detail::counting_iterator_base<
        Incrementable, CategoryOrTraversal, Difference
    >::type super_t;

    friend class iterator_core_access;

 public:
    typedef typename super_t::difference_type difference_type;

    counting_iterator() { }

    counting_iterator(counting_iterator const& rhs) : super_t(rhs.base()) {}

    counting_iterator(Incrementable x)
      : super_t(x)
    {
    }

# if 0
    template<class OtherIncrementable>
    counting_iterator(
        counting_iterator<OtherIncrementable, CategoryOrTraversal, Difference> const& t
      , typename enable_if_convertible<OtherIncrementable, Incrementable>::type* = 0
    )
      : super_t(t.base())
    {}
# endif

 private:

    typename super_t::reference dereference() const
    {
        return this->base_reference();
    }

    template <class OtherIncrementable>
    difference_type
    distance_to(counting_iterator<OtherIncrementable, CategoryOrTraversal, Difference> const& y) const
    {
      typedef typename mpl::if_<
          detail::is_numeric<Incrementable>
        , detail::number_distance<difference_type, Incrementable, OtherIncrementable>
        , detail::iterator_distance<difference_type, Incrementable, OtherIncrementable>
      >::type d;

      return d::distance(this->base(), y.base());
    }
};

// Manufacture a counting iterator for an arbitrary incrementable type
template <class Incrementable>
inline counting_iterator<Incrementable>
make_counting_iterator(Incrementable x)
{
  typedef counting_iterator<Incrementable> result_t;
  return result_t(x);
}

} // namespace iterators

using iterators::counting_iterator;
using iterators::make_counting_iterator;

} // namespace boost

#endif // COUNTING_ITERATOR_DWA200348_HPP

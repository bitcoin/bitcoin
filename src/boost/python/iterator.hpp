// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef ITERATOR_DWA2002512_HPP
# define ITERATOR_DWA2002512_HPP

# include <boost/python/detail/prefix.hpp>

# include <boost/python/detail/target.hpp>
# include <boost/python/detail/type_traits.hpp>
# include <boost/python/object/iterator.hpp>
# include <boost/python/object_core.hpp>

# if defined(BOOST_MSVC) && (BOOST_MSVC == 1400) /*
> warning C4180: qualifier applied to function type has no meaning; ignored
Peter Dimov wrote:
This warning is caused by an overload resolution bug in VC8 that cannot be
worked around and will probably not be fixed by MS in the VC8 line. The
problematic overload is only instantiated and never called, and the code
works correctly. */
#  pragma warning(disable: 4180)
# endif

# include <boost/bind/bind.hpp>
# include <boost/bind/protect.hpp>

namespace boost { namespace python { 

namespace detail
{
  // Adds an additional layer of binding to
  // objects::make_iterator(...), which allows us to pass member
  // function and member data pointers.
  template <class Target, class Accessor1, class Accessor2, class NextPolicies>
  inline object make_iterator(
      Accessor1 get_start
    , Accessor2 get_finish
    , NextPolicies next_policies
    , Target&(*)()
  )
  {
      using namespace boost::placeholders;
      return objects::make_iterator_function<Target>(
          boost::protect(boost::bind(get_start, _1))
        , boost::protect(boost::bind(get_finish, _1))
        , next_policies
      );
  }

  // Guts of template class iterators<>, below.
  template <bool const_ = false>
  struct iterators_impl
  {
      template <class T>
      struct apply
      {
          typedef typename T::iterator iterator;
          static iterator begin(T& x) { return x.begin(); }
          static iterator end(T& x) { return x.end(); }
      };
  };

  template <>
  struct iterators_impl<true>
  {
      template <class T>
      struct apply
      {
          typedef typename T::const_iterator iterator;
          static iterator begin(T& x) { return x.begin(); }
          static iterator end(T& x) { return x.end(); }
      };
  };
}

// An "ordinary function generator" which contains static begin(x) and
// end(x) functions that invoke T::begin() and T::end(), respectively.
template <class T>
struct iterators
    : detail::iterators_impl<
        detail::is_const<T>::value
      >::template apply<T>
{
};

// Create an iterator-building function which uses the given
// accessors. Deduce the Target type from the accessors. The iterator
// returns copies of the inderlying elements.
template <class Accessor1, class Accessor2>
object range(Accessor1 start, Accessor2 finish)
{
    return detail::make_iterator(
        start, finish
      , objects::default_iterator_call_policies()
      , detail::target(start)
    );
}

// Create an iterator-building function which uses the given accessors
// and next() policies. Deduce the Target type.
template <class NextPolicies, class Accessor1, class Accessor2>
object range(Accessor1 start, Accessor2 finish, NextPolicies* = 0)
{
    return detail::make_iterator(start, finish, NextPolicies(), detail::target(start));
}

// Create an iterator-building function which uses the given accessors
// and next() policies, operating on the given Target type
template <class NextPolicies, class Target, class Accessor1, class Accessor2>
object range(Accessor1 start, Accessor2 finish, NextPolicies* = 0, boost::type<Target>* = 0)
{
    // typedef typename add_reference<Target>::type target;
    return detail::make_iterator(start, finish, NextPolicies(), (Target&(*)())0);
}

// A Python callable object which produces an iterator traversing
// [x.begin(), x.end()), where x is an instance of the Container
// type. NextPolicies are used as the CallPolicies for the iterator's
// next() function.
template <class Container
          , class NextPolicies = objects::default_iterator_call_policies>
struct iterator : object
{
    iterator()
        : object(
            python::range<NextPolicies>(
                &iterators<Container>::begin, &iterators<Container>::end
                ))
    {
    }
};

}} // namespace boost::python

#endif // ITERATOR_DWA2002512_HPP

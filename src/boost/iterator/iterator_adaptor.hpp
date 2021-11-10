// (C) Copyright David Abrahams 2002.
// (C) Copyright Jeremy Siek    2002.
// (C) Copyright Thomas Witt    2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_ITERATOR_ADAPTOR_23022003THW_HPP
#define BOOST_ITERATOR_ADAPTOR_23022003THW_HPP

#include <boost/static_assert.hpp>

#include <boost/core/use_default.hpp>

#include <boost/iterator/iterator_categories.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/iterator/detail/enable_if.hpp>

#include <boost/mpl/and.hpp>
#include <boost/mpl/not.hpp>
#include <boost/mpl/or.hpp>

#include <boost/type_traits/is_same.hpp>
#include <boost/type_traits/is_convertible.hpp>

#ifdef BOOST_ITERATOR_REF_CONSTNESS_KILLS_WRITABILITY
# include <boost/type_traits/remove_reference.hpp>
#endif

#include <boost/type_traits/add_reference.hpp>
#include <boost/iterator/detail/config_def.hpp>

#include <boost/iterator/iterator_traits.hpp>

namespace boost {
namespace iterators {

  // Used as a default template argument internally, merely to
  // indicate "use the default", this can also be passed by users
  // explicitly in order to specify that the default should be used.
  using boost::use_default;

} // namespace iterators

// the incompleteness of use_default causes massive problems for
// is_convertible (naturally).  This workaround is fortunately not
// needed for vc6/vc7.
template<class To>
struct is_convertible<use_default,To>
  : mpl::false_ {};

namespace iterators {

  namespace detail
  {

    //
    // Result type used in enable_if_convertible meta function.
    // This can be an incomplete type, as only pointers to
    // enable_if_convertible< ... >::type are used.
    // We could have used void for this, but conversion to
    // void* is just to easy.
    //
    struct enable_type;
  }


  //
  // enable_if for use in adapted iterators constructors.
  //
  // In order to provide interoperability between adapted constant and
  // mutable iterators, adapted iterators will usually provide templated
  // conversion constructors of the following form
  //
  // template <class BaseIterator>
  // class adapted_iterator :
  //   public iterator_adaptor< adapted_iterator<Iterator>, Iterator >
  // {
  // public:
  //
  //   ...
  //
  //   template <class OtherIterator>
  //   adapted_iterator(
  //       OtherIterator const& it
  //     , typename enable_if_convertible<OtherIterator, Iterator>::type* = 0);
  //
  //   ...
  // };
  //
  // enable_if_convertible is used to remove those overloads from the overload
  // set that cannot be instantiated. For all practical purposes only overloads
  // for constant/mutable interaction will remain. This has the advantage that
  // meta functions like boost::is_convertible do not return false positives,
  // as they can only look at the signature of the conversion constructor
  // and not at the actual instantiation.
  //
  // enable_if_interoperable can be safely used in user code. It falls back to
  // always enabled for compilers that don't support enable_if or is_convertible.
  // There is no need for compiler specific workarounds in user code.
  //
  // The operators implementation relies on boost::is_convertible not returning
  // false positives for user/library defined iterator types. See comments
  // on operator implementation for consequences.
  //
#  if defined(BOOST_NO_IS_CONVERTIBLE) || defined(BOOST_NO_SFINAE)

  template <class From, class To>
  struct enable_if_convertible
  {
      typedef boost::iterators::detail::enable_type type;
  };

#  elif BOOST_WORKAROUND(_MSC_FULL_VER, BOOST_TESTED_AT(13102292))

  // For some reason vc7.1 needs us to "cut off" instantiation
  // of is_convertible in a few cases.
  template<typename From, typename To>
  struct enable_if_convertible
    : iterators::enable_if<
        mpl::or_<
            is_same<From,To>
          , is_convertible<From, To>
        >
      , boost::iterators::detail::enable_type
    >
  {};

#  else

  template<typename From, typename To>
  struct enable_if_convertible
    : iterators::enable_if<
          is_convertible<From, To>
        , boost::iterators::detail::enable_type
      >
  {};

# endif

  //
  // Default template argument handling for iterator_adaptor
  //
  namespace detail
  {
    // If T is use_default, return the result of invoking
    // DefaultNullaryFn, otherwise return T.
    template <class T, class DefaultNullaryFn>
    struct ia_dflt_help
      : mpl::eval_if<
            is_same<T, use_default>
          , DefaultNullaryFn
          , mpl::identity<T>
        >
    {
    };

    // A metafunction which computes an iterator_adaptor's base class,
    // a specialization of iterator_facade.
    template <
        class Derived
      , class Base
      , class Value
      , class Traversal
      , class Reference
      , class Difference
    >
    struct iterator_adaptor_base
    {
        typedef iterator_facade<
            Derived

# ifdef BOOST_ITERATOR_REF_CONSTNESS_KILLS_WRITABILITY
          , typename boost::iterators::detail::ia_dflt_help<
                Value
              , mpl::eval_if<
                    is_same<Reference,use_default>
                  , iterator_value<Base>
                  , remove_reference<Reference>
                >
            >::type
# else
          , typename boost::iterators::detail::ia_dflt_help<
                Value, iterator_value<Base>
            >::type
# endif

          , typename boost::iterators::detail::ia_dflt_help<
                Traversal
              , iterator_traversal<Base>
            >::type

          , typename boost::iterators::detail::ia_dflt_help<
                Reference
              , mpl::eval_if<
                    is_same<Value,use_default>
                  , iterator_reference<Base>
                  , add_reference<Value>
                >
            >::type

          , typename boost::iterators::detail::ia_dflt_help<
                Difference, iterator_difference<Base>
            >::type
        >
        type;
    };

    // workaround for aC++ CR JAGaf33512
    template <class Tr1, class Tr2>
    inline void iterator_adaptor_assert_traversal ()
    {
      BOOST_STATIC_ASSERT((is_convertible<Tr1, Tr2>::value));
    }
  }

  //
  // Iterator Adaptor
  //
  // The parameter ordering changed slightly with respect to former
  // versions of iterator_adaptor The idea is that when the user needs
  // to fiddle with the reference type it is highly likely that the
  // iterator category has to be adjusted as well.  Any of the
  // following four template arguments may be ommitted or explicitly
  // replaced by use_default.
  //
  //   Value - if supplied, the value_type of the resulting iterator, unless
  //      const. If const, a conforming compiler strips constness for the
  //      value_type. If not supplied, iterator_traits<Base>::value_type is used
  //
  //   Category - the traversal category of the resulting iterator. If not
  //      supplied, iterator_traversal<Base>::type is used.
  //
  //   Reference - the reference type of the resulting iterator, and in
  //      particular, the result type of operator*(). If not supplied but
  //      Value is supplied, Value& is used. Otherwise
  //      iterator_traits<Base>::reference is used.
  //
  //   Difference - the difference_type of the resulting iterator. If not
  //      supplied, iterator_traits<Base>::difference_type is used.
  //
  template <
      class Derived
    , class Base
    , class Value        = use_default
    , class Traversal    = use_default
    , class Reference    = use_default
    , class Difference   = use_default
  >
  class iterator_adaptor
    : public boost::iterators::detail::iterator_adaptor_base<
        Derived, Base, Value, Traversal, Reference, Difference
      >::type
  {
      friend class iterator_core_access;

   protected:
      typedef typename boost::iterators::detail::iterator_adaptor_base<
          Derived, Base, Value, Traversal, Reference, Difference
      >::type super_t;
   public:
      iterator_adaptor() {}

      explicit iterator_adaptor(Base const &iter)
          : m_iterator(iter)
      {
      }

      typedef Base base_type;

      Base const& base() const
        { return m_iterator; }

   protected:
      // for convenience in derived classes
      typedef iterator_adaptor<Derived,Base,Value,Traversal,Reference,Difference> iterator_adaptor_;

      //
      // lvalue access to the Base object for Derived
      //
      Base const& base_reference() const
        { return m_iterator; }

      Base& base_reference()
        { return m_iterator; }

   private:
      //
      // Core iterator interface for iterator_facade.  This is private
      // to prevent temptation for Derived classes to use it, which
      // will often result in an error.  Derived classes should use
      // base_reference(), above, to get direct access to m_iterator.
      //
      typename super_t::reference dereference() const
        { return *m_iterator; }

      template <
      class OtherDerived, class OtherIterator, class V, class C, class R, class D
      >
      bool equal(iterator_adaptor<OtherDerived, OtherIterator, V, C, R, D> const& x) const
      {
        // Maybe readd with same_distance
        //           BOOST_STATIC_ASSERT(
        //               (detail::same_category_and_difference<Derived,OtherDerived>::value)
        //               );
          return m_iterator == x.base();
      }

      typedef typename iterator_category_to_traversal<
          typename super_t::iterator_category
      >::type my_traversal;

# define BOOST_ITERATOR_ADAPTOR_ASSERT_TRAVERSAL(cat) \
      boost::iterators::detail::iterator_adaptor_assert_traversal<my_traversal, cat>();

      void advance(typename super_t::difference_type n)
      {
          BOOST_ITERATOR_ADAPTOR_ASSERT_TRAVERSAL(random_access_traversal_tag)
          m_iterator += n;
      }

      void increment() { ++m_iterator; }

      void decrement()
      {
          BOOST_ITERATOR_ADAPTOR_ASSERT_TRAVERSAL(bidirectional_traversal_tag)
           --m_iterator;
      }

      template <
          class OtherDerived, class OtherIterator, class V, class C, class R, class D
      >
      typename super_t::difference_type distance_to(
          iterator_adaptor<OtherDerived, OtherIterator, V, C, R, D> const& y) const
      {
          BOOST_ITERATOR_ADAPTOR_ASSERT_TRAVERSAL(random_access_traversal_tag)
          // Maybe readd with same_distance
          //           BOOST_STATIC_ASSERT(
          //               (detail::same_category_and_difference<Derived,OtherDerived>::value)
          //               );
          return y.base() - m_iterator;
      }

# undef BOOST_ITERATOR_ADAPTOR_ASSERT_TRAVERSAL

   private: // data members
      Base m_iterator;
  };

} // namespace iterators

using iterators::iterator_adaptor;
using iterators::enable_if_convertible;

} // namespace boost

#include <boost/iterator/detail/config_undef.hpp>

#endif // BOOST_ITERATOR_ADAPTOR_23022003THW_HPP

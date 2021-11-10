// Copyright David Abrahams and Thomas Becker 2000-2006.
// Copyright Kohei Takahashi 2012-2014.
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_ZIP_ITERATOR_TMB_07_13_2003_HPP_
# define BOOST_ZIP_ITERATOR_TMB_07_13_2003_HPP_

#include <stddef.h>
#include <boost/iterator/iterator_traits.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/iterator/iterator_adaptor.hpp> // for enable_if_convertible
#include <boost/iterator/iterator_categories.hpp>

#include <boost/iterator/minimum_category.hpp>

#include <utility> // for std::pair
#include <boost/fusion/adapted/boost_tuple.hpp> // for backward compatibility

#include <boost/type_traits/remove_reference.hpp>
#include <boost/type_traits/remove_cv.hpp>

#include <boost/mpl/at.hpp>
#include <boost/mpl/fold.hpp>
#include <boost/mpl/transform.hpp>
#include <boost/mpl/placeholders.hpp>

#include <boost/fusion/algorithm/iteration/for_each.hpp>
#include <boost/fusion/algorithm/transformation/transform.hpp>
#include <boost/fusion/sequence/convert.hpp>
#include <boost/fusion/sequence/intrinsic/at_c.hpp>
#include <boost/fusion/sequence/comparison/equal_to.hpp>
#include <boost/fusion/support/tag_of_fwd.hpp>

namespace boost {
namespace iterators {

  // Zip iterator forward declaration for zip_iterator_base
  template<typename IteratorTuple>
  class zip_iterator;

  namespace detail
  {

    // Functors to be used with tuple algorithms
    //
    template<typename DiffType>
    class advance_iterator
    {
    public:
      advance_iterator(DiffType step) : m_step(step) {}

      template<typename Iterator>
      void operator()(Iterator& it) const
      { it += m_step; }

    private:
      DiffType m_step;
    };
    //
    struct increment_iterator
    {
      template<typename Iterator>
      void operator()(Iterator& it) const
      { ++it; }
    };
    //
    struct decrement_iterator
    {
      template<typename Iterator>
      void operator()(Iterator& it) const
      { --it; }
    };
    //
    struct dereference_iterator
    {
      template<typename>
      struct result;

      template<typename This, typename Iterator>
      struct result<This(Iterator)>
      {
        typedef typename
          remove_cv<typename remove_reference<Iterator>::type>::type
        iterator;

        typedef typename iterator_reference<iterator>::type type;
      };

      template<typename Iterator>
        typename result<dereference_iterator(Iterator)>::type
        operator()(Iterator const& it) const
      { return *it; }
    };

    // Metafunction to obtain the type of the tuple whose element types
    // are the reference types of an iterator tuple.
    //
    template<typename IteratorTuple>
    struct tuple_of_references
      : mpl::transform<
            IteratorTuple,
            iterator_reference<mpl::_1>
          >
    {
    };

    // Specialization for std::pair
    template<typename Iterator1, typename Iterator2>
    struct tuple_of_references<std::pair<Iterator1, Iterator2> >
    {
        typedef std::pair<
            typename iterator_reference<Iterator1>::type
          , typename iterator_reference<Iterator2>::type
        > type;
    };

    // Metafunction to obtain the minimal traversal tag in a tuple
    // of iterators.
    //
    template<typename IteratorTuple>
    struct minimum_traversal_category_in_iterator_tuple
    {
      typedef typename mpl::transform<
          IteratorTuple
        , pure_traversal_tag<iterator_traversal<> >
      >::type tuple_of_traversal_tags;

      typedef typename mpl::fold<
          tuple_of_traversal_tags
        , random_access_traversal_tag
        , minimum_category<>
      >::type type;
    };

    template<typename Iterator1, typename Iterator2>
    struct minimum_traversal_category_in_iterator_tuple<std::pair<Iterator1, Iterator2> >
    {
        typedef typename pure_traversal_tag<
            typename iterator_traversal<Iterator1>::type
        >::type iterator1_traversal;
        typedef typename pure_traversal_tag<
            typename iterator_traversal<Iterator2>::type
        >::type iterator2_traversal;

        typedef typename minimum_category<
            iterator1_traversal
          , typename minimum_category<
                iterator2_traversal
              , random_access_traversal_tag
            >::type
        >::type type;
    };

    ///////////////////////////////////////////////////////////////////
    //
    // Class zip_iterator_base
    //
    // Builds and exposes the iterator facade type from which the zip
    // iterator will be derived.
    //
    template<typename IteratorTuple>
    struct zip_iterator_base
    {
     private:
        // Reference type is the type of the tuple obtained from the
        // iterators' reference types.
        typedef typename
        detail::tuple_of_references<IteratorTuple>::type reference;

        // Value type is the same as reference type.
        typedef reference value_type;

        // Difference type is the first iterator's difference type
        typedef typename iterator_difference<
            typename mpl::at_c<IteratorTuple, 0>::type
        >::type difference_type;

        // Traversal catetgory is the minimum traversal category in the
        // iterator tuple.
        typedef typename
        detail::minimum_traversal_category_in_iterator_tuple<
            IteratorTuple
        >::type traversal_category;
     public:

        // The iterator facade type from which the zip iterator will
        // be derived.
        typedef iterator_facade<
            zip_iterator<IteratorTuple>,
            value_type,
            traversal_category,
            reference,
            difference_type
        > type;
    };

    template <>
    struct zip_iterator_base<int>
    {
        typedef int type;
    };

    template <typename reference>
    struct converter
    {
        template <typename Seq>
        static reference call(Seq seq)
        {
            typedef typename fusion::traits::tag_of<reference>::type tag;
            return fusion::convert<tag>(seq);
        }
    };

    template <typename Reference1, typename Reference2>
    struct converter<std::pair<Reference1, Reference2> >
    {
        typedef std::pair<Reference1, Reference2> reference;
        template <typename Seq>
        static reference call(Seq seq)
        {
            return reference(
                fusion::at_c<0>(seq)
              , fusion::at_c<1>(seq));
        }
    };
  }

  /////////////////////////////////////////////////////////////////////
  //
  // zip_iterator class definition
  //
  template<typename IteratorTuple>
  class zip_iterator :
    public detail::zip_iterator_base<IteratorTuple>::type
  {

   // Typedef super_t as our base class.
   typedef typename
     detail::zip_iterator_base<IteratorTuple>::type super_t;

   // iterator_core_access is the iterator's best friend.
   friend class iterator_core_access;

  public:

    // Construction
    // ============

    // Default constructor
    zip_iterator() { }

    // Constructor from iterator tuple
    zip_iterator(IteratorTuple iterator_tuple)
      : m_iterator_tuple(iterator_tuple)
    { }

    // Copy constructor
    template<typename OtherIteratorTuple>
    zip_iterator(
       const zip_iterator<OtherIteratorTuple>& other,
       typename enable_if_convertible<
         OtherIteratorTuple,
         IteratorTuple
         >::type* = 0
    ) : m_iterator_tuple(other.get_iterator_tuple())
    {}

    // Get method for the iterator tuple.
    const IteratorTuple& get_iterator_tuple() const
    { return m_iterator_tuple; }

  private:

    // Implementation of Iterator Operations
    // =====================================

    // Dereferencing returns a tuple built from the dereferenced
    // iterators in the iterator tuple.
    typename super_t::reference dereference() const
    {
        typedef typename super_t::reference reference;
        typedef detail::converter<reference> gen;
        return gen::call(fusion::transform(
          get_iterator_tuple(),
          detail::dereference_iterator()));
    }

    // Two zip iterators are equal if all iterators in the iterator
    // tuple are equal. NOTE: It should be possible to implement this
    // as
    //
    // return get_iterator_tuple() == other.get_iterator_tuple();
    //
    // but equality of tuples currently (7/2003) does not compile
    // under several compilers. No point in bringing in a bunch
    // of #ifdefs here.
    //
    template<typename OtherIteratorTuple>
    bool equal(const zip_iterator<OtherIteratorTuple>& other) const
    {
        return fusion::equal_to(
          get_iterator_tuple(),
          other.get_iterator_tuple());
    }

    // Advancing a zip iterator means to advance all iterators in the
    // iterator tuple.
    void advance(typename super_t::difference_type n)
    {
        fusion::for_each(
          m_iterator_tuple,
          detail::advance_iterator<BOOST_DEDUCED_TYPENAME super_t::difference_type>(n));
    }
    // Incrementing a zip iterator means to increment all iterators in
    // the iterator tuple.
    void increment()
    {
        fusion::for_each(
          m_iterator_tuple,
          detail::increment_iterator());
    }

    // Decrementing a zip iterator means to decrement all iterators in
    // the iterator tuple.
    void decrement()
    {
        fusion::for_each(
          m_iterator_tuple,
          detail::decrement_iterator());
    }

    // Distance is calculated using the first iterator in the tuple.
    template<typename OtherIteratorTuple>
      typename super_t::difference_type distance_to(
        const zip_iterator<OtherIteratorTuple>& other
        ) const
    {
        return fusion::at_c<0>(other.get_iterator_tuple()) -
            fusion::at_c<0>(this->get_iterator_tuple());
    }

    // Data Members
    // ============

    // The iterator tuple.
    IteratorTuple m_iterator_tuple;

  };

  // Make function for zip iterator
  //
  template<typename IteratorTuple>
  inline zip_iterator<IteratorTuple>
  make_zip_iterator(IteratorTuple t)
  { return zip_iterator<IteratorTuple>(t); }

} // namespace iterators

using iterators::zip_iterator;
using iterators::make_zip_iterator;

} // namespace boost

#endif

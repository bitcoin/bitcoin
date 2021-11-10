// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef DEF_HELPER_DWA200287_HPP
# define DEF_HELPER_DWA200287_HPP

# include <boost/python/args.hpp>
# include <boost/python/detail/indirect_traits.hpp>
# include <boost/python/detail/type_traits.hpp>
# include <boost/mpl/not.hpp>
# include <boost/mpl/and.hpp>
# include <boost/mpl/or.hpp>
# include <boost/mpl/lambda.hpp>
# include <boost/mpl/apply.hpp>
# include <boost/tuple/tuple.hpp>
# include <boost/python/detail/not_specified.hpp>
# include <boost/python/detail/def_helper_fwd.hpp>

namespace boost { namespace python {

struct default_call_policies;

namespace detail
{
  // tuple_extract<Tuple,Predicate>::extract(t) returns the first
  // element of a Tuple whose type E satisfies the given Predicate
  // applied to add_reference<E>. The Predicate must be an MPL
  // metafunction class.
  template <class Tuple, class Predicate>
  struct tuple_extract;

  // Implementation class for when the tuple's head type does not
  // satisfy the Predicate
  template <bool matched>
  struct tuple_extract_impl
  {
      template <class Tuple, class Predicate>
      struct apply
      {
          typedef typename Tuple::head_type result_type;
          
          static typename Tuple::head_type extract(Tuple const& x)
          {
              return x.get_head();
          }
      };
  };

  // Implementation specialization for when the tuple's head type
  // satisfies the predicate
  template <>
  struct tuple_extract_impl<false>
  {
      template <class Tuple, class Predicate>
      struct apply
      {
          // recursive application of tuple_extract on the tail of the tuple
          typedef tuple_extract<typename Tuple::tail_type, Predicate> next;
          typedef typename next::result_type result_type;
          
          static result_type extract(Tuple const& x)
          {
              return next::extract(x.get_tail());
          }
      };
  };

  // A metafunction which selects a version of tuple_extract_impl to
  // use for the implementation of tuple_extract
  template <class Tuple, class Predicate>
  struct tuple_extract_base_select
  {
      typedef typename Tuple::head_type head_type;
      typedef typename mpl::apply1<Predicate,
              typename add_lvalue_reference<head_type>::type>::type match_t;
      BOOST_STATIC_CONSTANT(bool, match = match_t::value);
      typedef typename tuple_extract_impl<match>::template apply<Tuple,Predicate> type;
  };
  
  template <class Tuple, class Predicate>
  struct tuple_extract
      : tuple_extract_base_select<
         Tuple
         , typename mpl::lambda<Predicate>::type
      >::type
  {
  };


  //
  // Specialized extractors for the docstring, keywords, CallPolicies,
  // and default implementation of virtual functions
  //

  template <class Tuple>
  struct doc_extract
      : tuple_extract<
        Tuple
        , mpl::not_<
           mpl::or_<
               indirect_traits::is_reference_to_class<mpl::_1>
             , indirect_traits::is_reference_to_member_function_pointer<mpl::_1 >
           >
        >
     >
  {
  };
  
  template <class Tuple>
  struct keyword_extract
      : tuple_extract<Tuple, is_reference_to_keywords<mpl::_1 > >
  {
  };

  template <class Tuple>
  struct policy_extract
      : tuple_extract<
          Tuple
          , mpl::and_<
             mpl::not_<is_same<not_specified const&,mpl::_1> >
              , indirect_traits::is_reference_to_class<mpl::_1 >
              , mpl::not_<is_reference_to_keywords<mpl::_1 > >
          >
        >
  {
  };

  template <class Tuple>
  struct default_implementation_extract
      : tuple_extract<
          Tuple
          , indirect_traits::is_reference_to_member_function_pointer<mpl::_1 >
          >
  {
  };

  //
  // A helper class for decoding the optional arguments to def()
  // invocations, which can be supplied in any order and are
  // discriminated by their type properties. The template parameters
  // are expected to be the types of the actual (optional) arguments
  // passed to def().
  //
  template <class T1, class T2, class T3, class T4>
  struct def_helper
  {
      // A tuple type which begins with references to the supplied
      // arguments and ends with actual representatives of the default
      // types.
      typedef boost::tuples::tuple<
          T1 const&
          , T2 const&
          , T3 const&
          , T4 const&
          , default_call_policies
          , detail::keywords<0>
          , char const*
          , void(not_specified::*)()   // A function pointer type which is never an
                                       // appropriate default implementation
          > all_t;

      // Constructors; these initialize an member of the tuple type
      // shown above.
      def_helper(T1 const& a1) : m_all(a1,m_nil,m_nil,m_nil) {}
      def_helper(T1 const& a1, T2 const& a2) : m_all(a1,a2,m_nil,m_nil) {}
      def_helper(T1 const& a1, T2 const& a2, T3 const& a3) : m_all(a1,a2,a3,m_nil) {}
      def_helper(T1 const& a1, T2 const& a2, T3 const& a3, T4 const& a4) : m_all(a1,a2,a3,a4) {}

   private: // types
      typedef typename default_implementation_extract<all_t>::result_type default_implementation_t;
      
   public: // Constants which can be used for static assertions.

      // Users must not supply a default implementation for non-class
      // methods.
      BOOST_STATIC_CONSTANT(
          bool, has_default_implementation = (
              !is_same<default_implementation_t, void(not_specified::*)()>::value));
      
   public: // Extractor functions which pull the appropriate value out
           // of the tuple
      char const* doc() const
      {
          return doc_extract<all_t>::extract(m_all);
      }
      
      typename keyword_extract<all_t>::result_type keywords() const
      {
          return keyword_extract<all_t>::extract(m_all);
      }
      
      typename policy_extract<all_t>::result_type policies() const
      {
          return policy_extract<all_t>::extract(m_all);
      }

      default_implementation_t default_implementation() const
      {
          return default_implementation_extract<all_t>::extract(m_all);
      }
      
   private: // data members
      all_t m_all; 
      not_specified m_nil; // for filling in not_specified slots
  };
}

}} // namespace boost::python::detail

#endif // DEF_HELPER_DWA200287_HPP

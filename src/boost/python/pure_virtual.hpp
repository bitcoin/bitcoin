// Copyright David Abrahams 2003.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef PURE_VIRTUAL_DWA2003810_HPP
# define PURE_VIRTUAL_DWA2003810_HPP

# include <boost/python/def_visitor.hpp>
# include <boost/python/default_call_policies.hpp>
# include <boost/mpl/push_front.hpp>
# include <boost/mpl/pop_front.hpp>

# include <boost/python/detail/nullary_function_adaptor.hpp>

namespace boost { namespace python { 

namespace detail
{
  //
  // @group Helpers for pure_virtual_visitor. {
  //
  
  // Raises a Python RuntimeError reporting that a pure virtual
  // function was called.
  void BOOST_PYTHON_DECL pure_virtual_called();

  // Replace the two front elements of S with T1 and T2
  template <class S, class T1, class T2>
  struct replace_front2
  {
      // Metafunction forwarding seemed to confound vc6 
      typedef typename mpl::push_front<
          typename mpl::push_front<
              typename mpl::pop_front<
                  typename mpl::pop_front<
                      S
                  >::type
              >::type
            , T2
          >::type
        , T1
      >::type type;
  };

  // Given an MPL sequence representing a member function [object]
  // signature, returns a new MPL sequence whose return type is
  // replaced by void, and whose first argument is replaced by C&.
  template <class C, class S>
  typename replace_front2<S,void,C&>::type
  error_signature(S)
  {
      typedef typename replace_front2<S,void,C&>::type r;
      return r();
  }

  //
  // } 
  //

  //
  // A def_visitor which defines a method as usual, then adds a
  // corresponding function which raises a "pure virtual called"
  // exception unless it's been overridden.
  //
  template <class PointerToMemberFunction>
  struct pure_virtual_visitor
    : def_visitor<pure_virtual_visitor<PointerToMemberFunction> >
  {
      pure_virtual_visitor(PointerToMemberFunction pmf)
        : m_pmf(pmf)
      {}
      
   private:
      friend class python::def_visitor_access;
      
      template <class C_, class Options>
      void visit(C_& c, char const* name, Options& options) const
      {
          // This should probably be a nicer error message
          BOOST_STATIC_ASSERT(!Options::has_default_implementation);

          // Add the virtual function dispatcher
          c.def(
              name
            , m_pmf
            , options.doc()
            , options.keywords()
            , options.policies()
          );

          typedef BOOST_DEDUCED_TYPENAME C_::metadata::held_type held_type;
          
          // Add the default implementation which raises the exception
          c.def(
              name
            , make_function(
                  detail::nullary_function_adaptor<void(*)()>(pure_virtual_called)
                , default_call_policies()
                , detail::error_signature<held_type>(detail::get_signature(m_pmf))
              )
          );
      }
      
   private: // data members
      PointerToMemberFunction m_pmf;
  };
}

//
// Passed a pointer to member function, generates a def_visitor which
// creates a method that only dispatches to Python if the function has
// been overridden, either in C++ or in Python, raising a "pure
// virtual called" exception otherwise.
//
template <class PointerToMemberFunction>
detail::pure_virtual_visitor<PointerToMemberFunction>
pure_virtual(PointerToMemberFunction pmf)
{
    return detail::pure_virtual_visitor<PointerToMemberFunction>(pmf);
}

}} // namespace boost::python

#endif // PURE_VIRTUAL_DWA2003810_HPP

// Copyright David Abrahams 2001.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef MAKE_CONSTRUCTOR_DWA20011221_HPP
# define MAKE_CONSTRUCTOR_DWA20011221_HPP

# include <boost/python/detail/prefix.hpp>

# include <boost/python/default_call_policies.hpp>
# include <boost/python/args.hpp>
# include <boost/python/object_fwd.hpp>

# include <boost/python/object/function_object.hpp>
# include <boost/python/object/make_holder.hpp>
# include <boost/python/object/pointer_holder.hpp>
# include <boost/python/converter/context_result_converter.hpp>

# include <boost/python/detail/caller.hpp>
# include <boost/python/detail/none.hpp>

# include <boost/mpl/size.hpp>
# include <boost/mpl/int.hpp>
# include <boost/mpl/push_front.hpp>
# include <boost/mpl/pop_front.hpp>
# include <boost/mpl/assert.hpp>

namespace boost { namespace python {

namespace detail
{
  template <class T>
  struct install_holder : converter::context_result_converter
  {
      install_holder(PyObject* args_)
        : m_self(PyTuple_GetItem(args_, 0)) {}

      PyObject* operator()(T x) const
      {
          dispatch(x, is_pointer<T>());
          return none();
      }

   private:
      template <class U>
      void dispatch(U* x, detail::true_) const
      {
#if defined(BOOST_NO_CXX11_SMART_PTR)
	std::auto_ptr<U> owner(x);
	dispatch(owner, detail::false_());
#else
	std::unique_ptr<U> owner(x);
	dispatch(std::move(owner), detail::false_());
#endif
      }
      
      template <class Ptr>
      void dispatch(Ptr x, detail::false_) const
      {
          typedef typename pointee<Ptr>::type value_type;
          typedef objects::pointer_holder<Ptr,value_type> holder;
          typedef objects::instance<holder> instance_t;

          void* memory = holder::allocate(this->m_self, offsetof(instance_t, storage), sizeof(holder));
          try {
#if defined(BOOST_NO_CXX11_SMART_PTR)
              (new (memory) holder(x))->install(this->m_self);
#else
              (new (memory) holder(std::move(x)))->install(this->m_self);
#endif
          }
          catch(...) {
              holder::deallocate(this->m_self, memory);
              throw;
          }
      }
      
      PyObject* m_self;
  };
  
  struct constructor_result_converter
  {
      template <class T>
      struct apply
      {
          typedef install_holder<T> type;
      };
  };

  template <class BaseArgs, class Offset>
  struct offset_args
  {
      offset_args(BaseArgs base_) : base(base_) {}
      BaseArgs base;
  };

  template <int N, class BaseArgs, class Offset>
  inline PyObject* get(mpl::int_<N>, offset_args<BaseArgs,Offset> const& args_)
  {
      return get(mpl::int_<(N+Offset::value)>(), args_.base);
  }
  
  template <class BaseArgs, class Offset>
  inline unsigned arity(offset_args<BaseArgs,Offset> const& args_)
  {
      return arity(args_.base) - Offset::value;
  }

  template <class BasePolicy_ = default_call_policies>
  struct constructor_policy : BasePolicy_
  {
      constructor_policy(BasePolicy_ base) : BasePolicy_(base) {}
      
      // If the BasePolicy_ supplied a result converter it would be
      // ignored; issue an error if it's not the default.
      BOOST_MPL_ASSERT_MSG(
         (is_same<
              typename BasePolicy_::result_converter
            , default_result_converter
          >::value)
        , MAKE_CONSTRUCTOR_SUPPLIES_ITS_OWN_RESULT_CONVERTER_THAT_WOULD_OVERRIDE_YOURS
        , (typename BasePolicy_::result_converter)
      );
      typedef constructor_result_converter result_converter;
      typedef offset_args<typename BasePolicy_::argument_package, mpl::int_<1> > argument_package;
  };

  template <class InnerSignature>
  struct outer_constructor_signature
  {
      typedef typename mpl::pop_front<InnerSignature>::type inner_args;
      typedef typename mpl::push_front<inner_args,object>::type outer_args;
      typedef typename mpl::push_front<outer_args,void>::type type;
  };

  // ETI workaround
  template <>
  struct outer_constructor_signature<int>
  {
      typedef int type;
  };
  
  //
  // These helper functions for make_constructor (below) do the raw work
  // of constructing a Python object from some invokable entity. See
  // <boost/python/detail/caller.hpp> for more information about how
  // the Sig arguments is used.
  //
  // @group make_constructor_aux {
  template <class F, class CallPolicies, class Sig>
  object make_constructor_aux(
      F f                             // An object that can be invoked by detail::invoke()
    , CallPolicies const& p           // CallPolicies to use in the invocation
    , Sig const&                      // An MPL sequence of argument types expected by F
  )
  {
      typedef typename outer_constructor_signature<Sig>::type outer_signature;

      typedef constructor_policy<CallPolicies> inner_policy;
      
      return objects::function_object(
          objects::py_function(
              detail::caller<F,inner_policy,Sig>(f, inner_policy(p))
            , outer_signature()
          )
      );
  }
  
  // As above, except that it accepts argument keywords. NumKeywords
  // is used only for a compile-time assertion to make sure the user
  // doesn't pass more keywords than the function can accept. To
  // disable all checking, pass mpl::int_<0> for NumKeywords.
  template <class F, class CallPolicies, class Sig, class NumKeywords>
  object make_constructor_aux(
      F f
      , CallPolicies const& p
      , Sig const&
      , detail::keyword_range const& kw // a [begin,end) pair of iterators over keyword names
      , NumKeywords                     // An MPL integral type wrapper: the size of kw
      )
  {
      enum { arity = mpl::size<Sig>::value - 1 };
      
      typedef typename detail::error::more_keywords_than_function_arguments<
          NumKeywords::value, arity
          >::too_many_keywords assertion BOOST_ATTRIBUTE_UNUSED;
    
      typedef typename outer_constructor_signature<Sig>::type outer_signature;

      typedef constructor_policy<CallPolicies> inner_policy;
      
      return objects::function_object(
          objects::py_function(
              detail::caller<F,inner_policy,Sig>(f, inner_policy(p))
            , outer_signature()
          )
          , kw
      );
  }
  // }

  //
  //   These dispatch functions are used to discriminate between the
  //   cases when the 3rd argument is keywords or when it is a
  //   signature.
  //
  //   @group Helpers for make_constructor when called with 3 arguments. {
  //
  template <class F, class CallPolicies, class Keywords>
  object make_constructor_dispatch(F f, CallPolicies const& policies, Keywords const& kw, mpl::true_)
  {
      return detail::make_constructor_aux(
          f
        , policies
        , detail::get_signature(f)
        , kw.range()
        , mpl::int_<Keywords::size>()
      );
  }

  template <class F, class CallPolicies, class Signature>
  object make_constructor_dispatch(F f, CallPolicies const& policies, Signature const& sig, mpl::false_)
  {
      return detail::make_constructor_aux(
          f
        , policies
        , sig
      );
  }
  // }
}

//   These overloaded functions wrap a function or member function
//   pointer as a Python object, using optional CallPolicies,
//   Keywords, and/or Signature. @group {
//
template <class F>
object make_constructor(F f)
{
    return detail::make_constructor_aux(
        f,default_call_policies(), detail::get_signature(f));
}

template <class F, class CallPolicies>
object make_constructor(F f, CallPolicies const& policies)
{
    return detail::make_constructor_aux(
        f, policies, detail::get_signature(f));
}

template <class F, class CallPolicies, class KeywordsOrSignature>
object make_constructor(
    F f
  , CallPolicies const& policies
  , KeywordsOrSignature const& keywords_or_signature)
{
    typedef typename
        detail::is_reference_to_keywords<KeywordsOrSignature&>::type
        is_kw;
    
    return detail::make_constructor_dispatch(
        f
      , policies
      , keywords_or_signature
      , is_kw()
    );
}

template <class F, class CallPolicies, class Keywords, class Signature>
object make_constructor(
    F f
  , CallPolicies const& policies
  , Keywords const& kw
  , Signature const& sig
 )
{
    return detail::make_constructor_aux(
          f
        , policies
        , sig
        , kw.range()
        , mpl::int_<Keywords::size>()
      );
}
// }

}} 


#endif // MAKE_CONSTRUCTOR_DWA20011221_HPP

// Boost.Signals2 library

// Copyright Frank Mori Hess 2007-2009.
// Copyright Timmo Stange 2007.
// Copyright Douglas Gregor 2001-2004. Use, modification and
// distribution is subject to the Boost Software License, Version
// 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// For more information, see http://www.boost.org

#ifndef BOOST_SIGNALS2_PREPROCESSED_SLOT_HPP
#define BOOST_SIGNALS2_PREPROCESSED_SLOT_HPP

#include <boost/preprocessor/repetition.hpp>
#include <boost/signals2/detail/preprocessed_arg_type.hpp>
#include <boost/type_traits/function_traits.hpp>

#ifndef BOOST_SIGNALS2_SLOT_MAX_BINDING_ARGS
#define BOOST_SIGNALS2_SLOT_MAX_BINDING_ARGS 10
#endif


// template<typename Func, typename BindArgT0, typename BindArgT1, ..., typename BindArgTN-1> slotN(...
#define BOOST_SIGNALS2_SLOT_N_BINDING_CONSTRUCTOR(z, n, data) \
  template<typename Func, BOOST_SIGNALS2_PREFIXED_ARGS_TEMPLATE_DECL(n, BindArg)> \
  BOOST_SIGNALS2_SLOT_CLASS_NAME(BOOST_SIGNALS2_NUM_ARGS)( \
    const Func &func, BOOST_SIGNALS2_PREFIXED_FULL_REF_ARGS(n, const BindArg)) \
  { \
    init_slot_function(boost::bind(func, BOOST_SIGNALS2_SIGNATURE_ARG_NAMES(n))); \
  }
#define BOOST_SIGNALS2_SLOT_N_BINDING_CONSTRUCTORS \
  BOOST_PP_REPEAT_FROM_TO(1, BOOST_SIGNALS2_SLOT_MAX_BINDING_ARGS, BOOST_SIGNALS2_SLOT_N_BINDING_CONSTRUCTOR, ~)


#define BOOST_PP_ITERATION_LIMITS (0, BOOST_PP_INC(BOOST_SIGNALS2_MAX_ARGS))
#define BOOST_PP_FILENAME_1 <boost/signals2/detail/slot_template.hpp>
#include BOOST_PP_ITERATE()

#undef BOOST_SIGNALS2_SLOT_N_BINDING_CONSTRUCTOR
#undef BOOST_SIGNALS2_SLOT_N_BINDING_CONSTRUCTORS

namespace boost
{
  namespace signals2
  {
    template<typename Signature,
      typename SlotFunction = boost::function<Signature> >
    class slot: public detail::slotN<function_traits<Signature>::arity,
      Signature, SlotFunction>::type
    {
    private:
      typedef typename detail::slotN<boost::function_traits<Signature>::arity,
        Signature, SlotFunction>::type base_type;
    public:
      template<typename F>
      slot(const F& f): base_type(f)
      {}
      // bind syntactic sugar
// template<typename F, typename BindArgT0, typename BindArgT1, ..., typename BindArgTn-1> slot(...
#define BOOST_SIGNALS2_SLOT_BINDING_CONSTRUCTOR(z, n, data) \
  template<typename Func, BOOST_SIGNALS2_PREFIXED_ARGS_TEMPLATE_DECL(n, BindArg)> \
    slot(const Func &func, BOOST_SIGNALS2_PREFIXED_FULL_REF_ARGS(n, const BindArg)): \
    base_type(func, BOOST_SIGNALS2_SIGNATURE_ARG_NAMES(n)) \
  {}
      BOOST_PP_REPEAT_FROM_TO(1, BOOST_SIGNALS2_SLOT_MAX_BINDING_ARGS, BOOST_SIGNALS2_SLOT_BINDING_CONSTRUCTOR, ~)
#undef BOOST_SIGNALS2_SLOT_BINDING_CONSTRUCTOR
    };
  } // namespace signals2
}

#endif // BOOST_SIGNALS2_PREPROCESSED_SLOT_HPP

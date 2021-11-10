// Boost.Signals2 library

// Copyright Frank Mori Hess 2007-2008.
// Copyright Timmo Stange 2007.
// Copyright Douglas Gregor 2001-2004. Use, modification and
// distribution is subject to the Boost Software License, Version
// 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// For more information, see http://www.boost.org

// This file is included iteratively, and should not be protected from multiple inclusion

#ifdef BOOST_NO_CXX11_VARIADIC_TEMPLATES
#define BOOST_SIGNALS2_NUM_ARGS BOOST_PP_ITERATION()
#else
#define BOOST_SIGNALS2_NUM_ARGS 1
#endif


namespace boost
{
  namespace signals2
  {
#ifdef BOOST_NO_CXX11_VARIADIC_TEMPLATES
    template<typename Signature, typename SlotFunction> class slot;
#else
    template<typename Signature, typename SlotFunction = boost::function<Signature> >
      class slot;

#if BOOST_WORKAROUND(BOOST_MSVC, <= 1900)
    template<typename Signature, typename SlotFunction> class slot{};
#endif
#endif // BOOST_NO_CXX11_VARIADIC_TEMPLATES

    template<BOOST_SIGNALS2_SLOT_TEMPLATE_SPECIALIZATION_DECL(BOOST_SIGNALS2_NUM_ARGS)>
      class BOOST_SIGNALS2_SLOT_CLASS_NAME(BOOST_SIGNALS2_NUM_ARGS) BOOST_SIGNALS2_SLOT_TEMPLATE_SPECIALIZATION
      : public slot_base, public detail::BOOST_SIGNALS2_STD_FUNCTIONAL_BASE

    {
    public:
      template<BOOST_SIGNALS2_PREFIXED_SIGNATURE_TEMPLATE_DECL(BOOST_SIGNALS2_NUM_ARGS, Other), typename OtherSlotFunction>
      friend class BOOST_SIGNALS2_SLOT_CLASS_NAME(BOOST_SIGNALS2_NUM_ARGS);

      typedef SlotFunction slot_function_type;
      typedef R result_type;
      typedef typename mpl::identity<BOOST_SIGNALS2_SIGNATURE_FUNCTION_TYPE(BOOST_SIGNALS2_NUM_ARGS)>::type signature_type;

#ifdef BOOST_NO_CXX11_VARIADIC_TEMPLATES

// typedef Tn argn_type;
#define BOOST_SIGNALS2_MISC_STATEMENT(z, n, data) \
    typedef BOOST_PP_CAT(T, BOOST_PP_INC(n)) BOOST_PP_CAT(BOOST_PP_CAT(arg, BOOST_PP_INC(n)), _type);
          BOOST_PP_REPEAT(BOOST_SIGNALS2_NUM_ARGS, BOOST_SIGNALS2_MISC_STATEMENT, ~)
#undef BOOST_SIGNALS2_MISC_STATEMENT
#if BOOST_SIGNALS2_NUM_ARGS == 1
      typedef arg1_type argument_type;
#elif BOOST_SIGNALS2_NUM_ARGS == 2
      typedef arg1_type first_argument_type;
      typedef arg2_type second_argument_type;
#endif

      template<unsigned n> class arg : public
        detail::BOOST_SIGNALS2_PREPROCESSED_ARG_N_TYPE_CLASS_NAME(BOOST_SIGNALS2_NUM_ARGS)
        <n BOOST_SIGNALS2_PP_COMMA_IF(BOOST_SIGNALS2_NUM_ARGS)
        BOOST_SIGNALS2_ARGS_TEMPLATE_INSTANTIATION(BOOST_SIGNALS2_NUM_ARGS)>
      {};

      BOOST_STATIC_CONSTANT(int, arity = BOOST_SIGNALS2_NUM_ARGS);

#else // BOOST_NO_CXX11_VARIADIC_TEMPLATES

      template<unsigned n> class arg
      {
      public:
        typedef typename detail::variadic_arg_type<n, Args...>::type type;
      };
      BOOST_STATIC_CONSTANT(int, arity = sizeof...(Args));

#endif // BOOST_NO_CXX11_VARIADIC_TEMPLATES

      template<typename F>
      BOOST_SIGNALS2_SLOT_CLASS_NAME(BOOST_SIGNALS2_NUM_ARGS)(const F& f)
      {
        init_slot_function(f);
      }
      // copy constructors
#ifdef BOOST_NO_CXX11_VARIADIC_TEMPLATES
      template<BOOST_SIGNALS2_PREFIXED_SIGNATURE_TEMPLATE_DECL(BOOST_SIGNALS2_NUM_ARGS, Other), typename OtherSlotFunction>
      BOOST_SIGNALS2_SLOT_CLASS_NAME(BOOST_SIGNALS2_NUM_ARGS)(const BOOST_SIGNALS2_SLOT_CLASS_NAME(BOOST_SIGNALS2_NUM_ARGS)
        <BOOST_SIGNALS2_PREFIXED_SIGNATURE_TEMPLATE_INSTANTIATION(BOOST_SIGNALS2_NUM_ARGS, Other), OtherSlotFunction> &other_slot):
        slot_base(other_slot), _slot_function(other_slot._slot_function)
      {
      }
#endif
      template<typename Signature, typename OtherSlotFunction>
      BOOST_SIGNALS2_SLOT_CLASS_NAME(BOOST_SIGNALS2_NUM_ARGS)(const slot<Signature, OtherSlotFunction> &other_slot):
        slot_base(other_slot), _slot_function(other_slot._slot_function)
      {
      }
      // bind syntactic sugar
      BOOST_SIGNALS2_SLOT_N_BINDING_CONSTRUCTORS
      // invocation
      R operator()(BOOST_SIGNALS2_SIGNATURE_FULL_ARGS(BOOST_SIGNALS2_NUM_ARGS))
      {
        locked_container_type locked_objects = lock();
        return _slot_function(BOOST_SIGNALS2_SIGNATURE_ARG_NAMES(BOOST_SIGNALS2_NUM_ARGS));
      }
      R operator()(BOOST_SIGNALS2_SIGNATURE_FULL_ARGS(BOOST_SIGNALS2_NUM_ARGS)) const
      {
        locked_container_type locked_objects = lock();
        return _slot_function(BOOST_SIGNALS2_SIGNATURE_ARG_NAMES(BOOST_SIGNALS2_NUM_ARGS));
      }
      // tracking
      BOOST_SIGNALS2_SLOT_CLASS_NAME(BOOST_SIGNALS2_NUM_ARGS)& track(const weak_ptr<void> &tracked)      {
        _tracked_objects.push_back(tracked);
        return *this;
      }
      BOOST_SIGNALS2_SLOT_CLASS_NAME(BOOST_SIGNALS2_NUM_ARGS)& track(const signal_base &signal)
      {
        track_signal(signal);
        return *this;
      }
      BOOST_SIGNALS2_SLOT_CLASS_NAME(BOOST_SIGNALS2_NUM_ARGS)& track(const slot_base &slot)
      {
        tracked_container_type::const_iterator it;
        for(it = slot.tracked_objects().begin(); it != slot.tracked_objects().end(); ++it)
        {
          _tracked_objects.push_back(*it);
        }
        return *this;
      }
      template<typename ForeignWeakPtr>
      BOOST_SIGNALS2_SLOT_CLASS_NAME(BOOST_SIGNALS2_NUM_ARGS)& track_foreign(const ForeignWeakPtr &tracked,
        typename weak_ptr_traits<ForeignWeakPtr>::shared_type * /*SFINAE*/ = 0)
      {
        _tracked_objects.push_back(detail::foreign_void_weak_ptr(tracked));
        return *this;
      }
      template<typename ForeignSharedPtr>
      BOOST_SIGNALS2_SLOT_CLASS_NAME(BOOST_SIGNALS2_NUM_ARGS)& track_foreign(const ForeignSharedPtr &tracked,
        typename shared_ptr_traits<ForeignSharedPtr>::weak_type * /*SFINAE*/ = 0)
      {
        _tracked_objects.push_back
        (
          detail::foreign_void_weak_ptr
          (
            typename shared_ptr_traits<ForeignSharedPtr>::weak_type(tracked)
          )
        );
        return *this;
      }

      const slot_function_type& slot_function() const {return _slot_function;}
      slot_function_type& slot_function() {return _slot_function;}
    private:
      template<typename F>
      void init_slot_function(const F& f)
      {
        _slot_function = detail::get_invocable_slot(f, detail::tag_type(f));
        signals2::detail::tracked_objects_visitor visitor(this);
        boost::visit_each(visitor, f);
      }

      SlotFunction _slot_function;
    };

#ifdef BOOST_NO_CXX11_VARIADIC_TEMPLATES
    namespace detail
    {
      template<unsigned arity, typename Signature, typename SlotFunction>
      class slotN;
      // partial template specialization
      template<typename Signature, typename SlotFunction>
      class slotN<BOOST_SIGNALS2_NUM_ARGS, Signature, SlotFunction>
      {
      public:
        typedef BOOST_SIGNALS2_SLOT_CLASS_NAME(BOOST_SIGNALS2_NUM_ARGS)<
          BOOST_SIGNALS2_PORTABLE_SIGNATURE(BOOST_SIGNALS2_NUM_ARGS, Signature),
          SlotFunction> type;
      };
    }
#endif
  } // end namespace signals2
} // end namespace boost

#undef BOOST_SIGNALS2_NUM_ARGS

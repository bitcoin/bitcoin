// Boost.Signals library

// Copyright Douglas Gregor 2001-2004.
// Copyright Frank Mori Hess 2007. Use, modification and
// distribution is subject to the Boost Software License, Version
// 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// For more information, see http://www.boost.org

#ifndef BOOST_SIGNALS2_SIGNALS_COMMON_HPP
#define BOOST_SIGNALS2_SIGNALS_COMMON_HPP

#include <boost/mpl/bool.hpp>
#include <boost/mpl/if.hpp>
#include <boost/ref.hpp>
#include <boost/signals2/signal_base.hpp>
#include <boost/type_traits/is_base_of.hpp>

namespace boost {
  namespace signals2 {
    namespace detail {
      // Determine if the given type T is a signal
      template<typename T>
      class is_signal: public mpl::bool_<is_base_of<signal_base, T>::value>
      {};

      // A slot can be a signal, a reference to a function object, or a
      // function object.
      struct signal_tag {};
      struct reference_tag {};
      struct value_tag {};

      // Classify the given slot as a signal, a reference-to-slot, or a
      // standard slot
      template<typename S>
      class get_slot_tag {
        typedef typename mpl::if_<is_signal<S>,
          signal_tag, value_tag>::type signal_or_value;
      public:
        typedef typename mpl::if_<is_reference_wrapper<S>,
                            reference_tag,
                            signal_or_value>::type type;
      };

      // Get the slot so that it can be copied
      template<typename F>
      typename F::weak_signal_type
      get_invocable_slot(const F &signal, signal_tag)
      { return typename F::weak_signal_type(signal); }

      template<typename F>
      const F&
      get_invocable_slot(const F& f, reference_tag)
      { return f; }

      template<typename F>
      const F&
      get_invocable_slot(const F& f, value_tag)
      { return f; }

      // Determines the type of the slot - is it a signal, a reference to a
      // slot or just a normal slot.
      template<typename F>
      typename get_slot_tag<F>::type
      tag_type(const F&)
      {
        typedef typename get_slot_tag<F>::type
          the_tag_type;
        the_tag_type tag = the_tag_type();
        return tag;
      }
    } // end namespace detail
  } // end namespace signals2
} // end namespace boost

#endif // BOOST_SIGNALS2_SIGNALS_COMMON_HPP

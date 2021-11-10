/*
  A thread-safe version of Boost.Signals.

  Author: Frank Mori Hess <fmhess@users.sourceforge.net>
  Begin: 2007-01-23
*/
// Copyright Frank Mori Hess 2007-2008
// Use, modification and
// distribution is subject to the Boost Software License, Version
// 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// For more information, see http://www.boost.org

#ifndef BOOST_SIGNALS2_PREPROCESSED_SIGNAL_HPP
#define BOOST_SIGNALS2_PREPROCESSED_SIGNAL_HPP

#include <boost/config.hpp>
#include <boost/preprocessor/arithmetic.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/control/expr_if.hpp>
#include <boost/preprocessor/iteration.hpp>
#include <boost/preprocessor/repetition.hpp>
#include <boost/signals2/detail/preprocessed_arg_type.hpp>
#include <boost/type_traits/add_reference.hpp>
#include <boost/type_traits/is_void.hpp> 
#include <boost/utility/enable_if.hpp>

#define BOOST_PP_ITERATION_LIMITS (0, BOOST_SIGNALS2_MAX_ARGS)
#define BOOST_PP_FILENAME_1 <boost/signals2/detail/signal_template.hpp>
#include BOOST_PP_ITERATE()

namespace boost
{
  namespace signals2
  {
    template<typename Signature,
      typename Combiner = optional_last_value<typename boost::function_traits<Signature>::result_type>,
      typename Group = int,
      typename GroupCompare = std::less<Group>,
      typename SlotFunction = function<Signature>,
      typename ExtendedSlotFunction = typename detail::extended_signature<function_traits<Signature>::arity, Signature>::function_type,
      typename Mutex = mutex >
    class signal: public detail::signalN<function_traits<Signature>::arity,
      Signature, Combiner, Group, GroupCompare, SlotFunction, ExtendedSlotFunction, Mutex>::type
    {
    private:
      typedef typename detail::signalN<boost::function_traits<Signature>::arity,
        Signature, Combiner, Group, GroupCompare, SlotFunction, ExtendedSlotFunction, Mutex>::type base_type;
    public:
      signal(const Combiner &combiner_arg = Combiner(), const GroupCompare &group_compare = GroupCompare()):
        base_type(combiner_arg, group_compare)
      {}
#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES) && BOOST_WORKAROUND(BOOST_MSVC, < 1800)
      signal(signal && other) BOOST_NOEXCEPT: base_type(std::move(other)) {}
      signal & operator=(signal && other) BOOST_NOEXCEPT{ base_type::operator=(std::move(other)); return *this; }
#endif
    };
  }
}

#endif // BOOST_SIGNALS2_PREPROCESSED_SIGNAL_HPP

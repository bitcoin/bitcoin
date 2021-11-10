/*
  A meta function which supports using named template type parameters
  via Boost.Parameter to specify the template type parameters for
  the boost::signals2::signal class.

  Author: Frank Mori Hess <fmhess@users.sourceforge.net>
  Begin: 2009-01-22
*/
// Copyright Frank Mori Hess 2009
// Use, modification and
// distribution is subject to the Boost Software License, Version
// 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// For more information, see http://www.boost.org

#ifndef BOOST_SIGNALS2_SIGNAL_TYPE_HPP
#define BOOST_SIGNALS2_SIGNAL_TYPE_HPP

#include <boost/parameter/config.hpp>

#if BOOST_PARAMETER_MAX_ARITY < 7
#error Define BOOST_PARAMETER_MAX_ARITY as 7 or greater.
#endif

#include <boost/parameter/template_keyword.hpp>
#include <boost/parameter/parameters.hpp>
#include <boost/parameter/required.hpp>
#include <boost/parameter/optional.hpp>
#include <boost/parameter/value_type.hpp>
#include <boost/type_traits/is_function.hpp>
#include <boost/type_traits/function_traits.hpp>
#include <boost/signals2/signal.hpp>
#include <functional>

namespace boost
{
  namespace signals2
  {
    namespace keywords
    {
      BOOST_PARAMETER_TEMPLATE_KEYWORD(signature_type)
      BOOST_PARAMETER_TEMPLATE_KEYWORD(combiner_type)
      BOOST_PARAMETER_TEMPLATE_KEYWORD(group_type)
      BOOST_PARAMETER_TEMPLATE_KEYWORD(group_compare_type)
      BOOST_PARAMETER_TEMPLATE_KEYWORD(slot_function_type)
      BOOST_PARAMETER_TEMPLATE_KEYWORD(extended_slot_function_type)
      BOOST_PARAMETER_TEMPLATE_KEYWORD(mutex_type)
    } // namespace keywords

    template <
        typename A0,
        typename A1 = parameter::void_,
        typename A2 = parameter::void_,
        typename A3 = parameter::void_,
        typename A4 = parameter::void_,
        typename A5 = parameter::void_,
        typename A6 = parameter::void_
      >
    class signal_type
    {
      typedef parameter::parameters<
          parameter::required<keywords::tag::signature_type, is_function<boost::mpl::_> >,
          parameter::optional<keywords::tag::combiner_type>,
          parameter::optional<keywords::tag::group_type>,
          parameter::optional<keywords::tag::group_compare_type>,
          parameter::optional<keywords::tag::slot_function_type>,
          parameter::optional<keywords::tag::extended_slot_function_type>,
          parameter::optional<keywords::tag::mutex_type>
        > parameter_spec;

    public:
      // ArgumentPack
      typedef typename parameter_spec::bind<A0, A1, A2, A3, A4, A5, A6>::type
        args;

      typedef typename parameter::value_type<args, keywords::tag::signature_type>::type
        signature_type;

      typedef typename parameter::value_type
        <
          args,
          keywords::tag::combiner_type,
          optional_last_value
            <
              typename boost::function_traits<signature_type>::result_type
            >
        >::type combiner_type;

      typedef typename
        parameter::value_type<args, keywords::tag::group_type, int>::type group_type;

      typedef typename
        parameter::value_type<args, keywords::tag::group_compare_type, std::less<group_type> >::type
        group_compare_type;

      typedef typename
        parameter::value_type<args, keywords::tag::slot_function_type, function<signature_type> >::type
        slot_function_type;

      typedef typename
        parameter::value_type
          <
            args,
            keywords::tag::extended_slot_function_type,
            typename detail::extended_signature<function_traits<signature_type>::arity, signature_type>::function_type
          >::type
          extended_slot_function_type;

      typedef typename
        parameter::value_type<args, keywords::tag::mutex_type, mutex>::type mutex_type;

      typedef signal
        <
          signature_type,
          combiner_type,
          group_type,
          group_compare_type,
          slot_function_type,
          extended_slot_function_type,
          mutex_type
        > type;
    };
  } // namespace signals2
} // namespace boost

#endif // BOOST_SIGNALS2_SIGNAL_TYPE_HPP

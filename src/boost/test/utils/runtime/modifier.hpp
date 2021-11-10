//  (C) Copyright Gennadiy Rozental 2001.
//  Use, modification, and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
//  File        : $RCSfile$
//
//  Version     : $Revision$
//
//  Description : parameter modifiers
// ***************************************************************************

#ifndef BOOST_TEST_UTILS_RUNTIME_MODIFIER_HPP
#define BOOST_TEST_UTILS_RUNTIME_MODIFIER_HPP

// Boost.Test Runtime parameters
#include <boost/test/utils/runtime/fwd.hpp>

// Boost.Test
#include <boost/test/utils/named_params.hpp>
#include <boost/test/detail/global_typedef.hpp>

#include <boost/test/detail/suppress_warnings.hpp>


// New CLA API available only for some C++11 compilers
#if    !defined(BOOST_NO_CXX11_AUTO_DECLARATIONS) \
    && !defined(BOOST_NO_CXX11_TEMPLATE_ALIASES) \
    && !defined(BOOST_NO_CXX11_HDR_INITIALIZER_LIST) \
    && !defined(BOOST_NO_CXX11_UNIFIED_INITIALIZATION_SYNTAX)
#define BOOST_TEST_CLA_NEW_API
#endif

namespace boost {
namespace runtime {

// ************************************************************************** //
// **************         environment variable modifiers       ************** //
// ************************************************************************** //

namespace {

#ifdef BOOST_TEST_CLA_NEW_API
auto const& description     = unit_test::static_constant<nfp::typed_keyword<cstring,struct description_t>>::value;
auto const& help            = unit_test::static_constant<nfp::typed_keyword<cstring,struct help_t>>::value;
auto const& env_var         = unit_test::static_constant<nfp::typed_keyword<cstring,struct env_var_t>>::value;
auto const& end_of_params   = unit_test::static_constant<nfp::typed_keyword<cstring,struct end_of_params_t>>::value;
auto const& negation_prefix = unit_test::static_constant<nfp::typed_keyword<cstring,struct neg_prefix_t>>::value;
auto const& value_hint      = unit_test::static_constant<nfp::typed_keyword<cstring,struct value_hint_t>>::value;
auto const& optional_value  = unit_test::static_constant<nfp::keyword<struct optional_value_t>>::value;
auto const& default_value   = unit_test::static_constant<nfp::keyword<struct default_value_t>>::value;
auto const& callback        = unit_test::static_constant<nfp::keyword<struct callback_t>>::value;

template<typename EnumType>
using enum_values = unit_test::static_constant<
  nfp::typed_keyword<std::initializer_list<std::pair<const cstring,EnumType>>, struct enum_values_t>
>;

#else

nfp::typed_keyword<cstring,struct description_t> description;
nfp::typed_keyword<cstring,struct help_t> help;
nfp::typed_keyword<cstring,struct env_var_t> env_var;
nfp::typed_keyword<cstring,struct end_of_params_t> end_of_params;
nfp::typed_keyword<cstring,struct neg_prefix_t> negation_prefix;
nfp::typed_keyword<cstring,struct value_hint_t> value_hint;
nfp::keyword<struct optional_value_t> optional_value;
nfp::keyword<struct default_value_t> default_value;
nfp::keyword<struct callback_t> callback;

template<typename EnumType>
struct enum_values_list {
    typedef std::pair<cstring,EnumType> ElemT;
    typedef std::vector<ElemT> ValuesT;

    enum_values_list const&
    operator()( cstring k, EnumType v ) const
    {
        const_cast<enum_values_list*>(this)->m_values.push_back( ElemT( k, v ) );

        return *this;
    }

    operator ValuesT const&() const { return m_values; }

private:
    ValuesT m_values;
};

template<typename EnumType>
struct enum_values : unit_test::static_constant<
  nfp::typed_keyword<enum_values_list<EnumType>, struct enum_values_t> >
{
};

#endif

} // local namespace

} // namespace runtime
} // namespace boost

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_TEST_UTILS_RUNTIME_MODIFIER_HPP

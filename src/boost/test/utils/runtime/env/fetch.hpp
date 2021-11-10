//  (C) Copyright Gennadiy Rozental 2001.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
//  File        : $RCSfile$
//
//  Version     : $Revision$
//
//  Description : implements fetching absent parameter athuments from environment
// ***************************************************************************

#ifndef BOOST_TEST_UTILS_RUNTIME_ENV_FETCH_HPP
#define BOOST_TEST_UTILS_RUNTIME_ENV_FETCH_HPP

// Boost.Test Runtime parameters
#include <boost/test/utils/runtime/parameter.hpp>
#include <boost/test/utils/runtime/argument.hpp>

#include <boost/test/detail/suppress_warnings.hpp>

// C Runtime
#include <stdlib.h>

namespace boost {
namespace runtime {
namespace env {

namespace env_detail {

#ifndef UNDER_CE

#ifdef BOOST_MSVC
#pragma warning(push)
#pragma warning(disable:4996) // getenv
#endif

inline std::pair<cstring,bool>
sys_read_var( cstring var_name )
{
    using namespace std;
    char const* res = getenv( var_name.begin() );

    return std::make_pair( cstring(res), res != NULL );
}

#ifdef BOOST_MSVC
#pragma warning(pop)
#endif

#else

inline std::pair<cstring,bool>
sys_read_var( cstring var_name )
{
    return std::make_pair( cstring(), false );
}

#endif

//____________________________________________________________________________//

template<typename ReadFunc>
inline void
fetch_absent( parameters_store const& params, runtime::arguments_store& args, ReadFunc read_func )
{
    BOOST_TEST_FOREACH( parameters_store::storage_type::value_type const&, v, params.all() ) {
        basic_param_ptr param = v.second;

        if( args.has( param->p_name ) || param->p_env_var.empty() )
            continue;

        std::pair<cstring,bool> value = read_func( param->p_env_var );

        if( !value.second )
            continue;

        // Validate against unexpected empty value
        BOOST_TEST_I_ASSRT( !value.first.is_empty() || param->p_has_optional_value,
            format_error( param->p_name ) 
                << "Missing an argument value for the parameter " << param->p_name
                << " in the environment." );

        // Produce argument value
        param->produce_argument( value.first, false, args );

    }
}

//____________________________________________________________________________//

} // namespace env_detail

inline void
fetch_absent( parameters_store const& params, runtime::arguments_store& args )
{
    env_detail::fetch_absent( params, args, &env_detail::sys_read_var );
}

} // namespace env
} // namespace runtime
} // namespace boost

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_TEST_UTILS_RUNTIME_ENV_FETCH_HPP

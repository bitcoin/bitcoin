// Copyright Stefan Seefeld 2005.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef EXEC_SS20050616_HPP
# define EXEC_SS20050616_HPP

# include <boost/python/object.hpp>
# include <boost/python/str.hpp>

namespace boost 
{ 
namespace python 
{

// Evaluate python expression from str.
// global and local are the global and local scopes respectively,
// used during evaluation.
object 
BOOST_PYTHON_DECL
eval(str string, object global = object(), object local = object());

object 
BOOST_PYTHON_DECL
eval(char const *string, object global = object(), object local = object());

// Execute an individual python statement from str.
// global and local are the global and local scopes respectively,
// used during execution.
object 
BOOST_PYTHON_DECL
exec_statement(str string, object global = object(), object local = object());

object 
BOOST_PYTHON_DECL
exec_statement(char const *string, object global = object(), object local = object());

// Execute python source code from str.
// global and local are the global and local scopes respectively,
// used during execution.
object 
BOOST_PYTHON_DECL
exec(str string, object global = object(), object local = object());

object 
BOOST_PYTHON_DECL
exec(char const *string, object global = object(), object local = object());

// Execute python source code from file filename.
// global and local are the global and local scopes respectively,
// used during execution.
object 
BOOST_PYTHON_DECL
exec_file(str filename, object global = object(), object local = object());

object 
BOOST_PYTHON_DECL
exec_file(char const *filename, object global = object(), object local = object());

}
}

#endif

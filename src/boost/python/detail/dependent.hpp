// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef DEPENDENT_DWA200286_HPP
# define DEPENDENT_DWA200286_HPP

namespace boost { namespace python { namespace detail { 

// A way to turn a concrete type T into a type dependent on U. This
// keeps conforming compilers (those implementing proper 2-phase
// name lookup for templates) from complaining about incomplete
// types in situations where it would otherwise be inconvenient or
// impossible to re-order code so that all types are defined in time.

// One such use is when we must return an incomplete T from a member
// function template (which must be defined in the class body to
// keep MSVC happy).
template <class T, class U>
struct dependent
{
    typedef T type;
};

}}} // namespace boost::python::detail

#endif // DEPENDENT_DWA200286_HPP

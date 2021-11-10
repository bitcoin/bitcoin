// Copyright (C) 2007 Douglas Gregor 

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// This file contains a simplification of the "trigger" method for
// process groups. The simple trigger handles the common case where
// the handler associated with a trigger is a member function bound to
// a particular pointer.

#ifndef BOOST_PROPERTY_MAP_PARALLEL_SIMPLE_TRIGGER_HPP
#define BOOST_PROPERTY_MAP_PARALLEL_SIMPLE_TRIGGER_HPP

#include <boost/property_map/parallel/process_group.hpp>

namespace boost { namespace parallel {

namespace detail {

/**
 * INTERNAL ONLY
 *
 * The actual function object that bridges from the normal trigger
 * interface to the simplified interface. This is the equivalent of
 * bind(pmf, self, _1, _2, _3, _4), but without the compile-time
 * overhead of bind.
 */
template<typename Class, typename T, typename Result>
class simple_trigger_t 
{
public:
  simple_trigger_t(Class* self, 
                   Result (Class::*pmf)(int, int, const T&, 
                                        trigger_receive_context))
    : self(self), pmf(pmf) { }

  Result 
  operator()(int source, int tag, const T& data, 
             trigger_receive_context context) const
  {
    return (self->*pmf)(source, tag, data, context);
  }

private:
  Class* self;
  Result (Class::*pmf)(int, int, const T&, trigger_receive_context);
};

} // end namespace detail

/**
 * Simplified trigger interface that reduces the amount of code
 * required to connect a process group trigger to a handler that is
 * just a bound member function.
 *
 * INTERNAL ONLY
 */
template<typename ProcessGroup, typename Class, typename T>
inline void 
simple_trigger(ProcessGroup& pg, int tag, Class* self, 
               void (Class::*pmf)(int source, int tag, const T& data, 
                                  trigger_receive_context context), int)
{
  pg.template trigger<T>(tag, 
                         detail::simple_trigger_t<Class, T, void>(self, pmf));
}

/**
 * Simplified trigger interface that reduces the amount of code
 * required to connect a process group trigger with a reply to a
 * handler that is just a bound member function.
 *
 * INTERNAL ONLY
 */
template<typename ProcessGroup, typename Class, typename T, typename Result>
inline void 
simple_trigger(ProcessGroup& pg, int tag, Class* self, 
               Result (Class::*pmf)(int source, int tag, const T& data, 
                                    trigger_receive_context context), long)
{
  pg.template trigger_with_reply<T>
    (tag, detail::simple_trigger_t<Class, T, Result>(self, pmf));
}

/**
 * Simplified trigger interface that reduces the amount of code
 * required to connect a process group trigger to a handler that is
 * just a bound member function.
 */
template<typename ProcessGroup, typename Class, typename T, typename Result>
inline void 
simple_trigger(ProcessGroup& pg, int tag, Class* self, 
               Result (Class::*pmf)(int source, int tag, const T& data, 
                                    trigger_receive_context context))
{
        // We pass 0 (an int) to help VC++ disambiguate calls to simple_trigger 
        // with Result=void.
        simple_trigger(pg, tag, self, pmf, 0); 
}

} } // end namespace boost::parallel

namespace boost { namespace graph { namespace parallel { using boost::parallel::simple_trigger; } } }

#endif // BOOST_PROPERTY_MAP_PARALLEL_SIMPLE_TRIGGER_HPP

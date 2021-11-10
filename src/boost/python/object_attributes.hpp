// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef OBJECT_ATTRIBUTES_DWA2002615_HPP
# define OBJECT_ATTRIBUTES_DWA2002615_HPP

# include <boost/python/detail/prefix.hpp>

# include <boost/python/proxy.hpp>
# include <boost/python/object_core.hpp>
# include <boost/python/object_protocol.hpp>

namespace boost { namespace python { namespace api {

struct const_attribute_policies
{
    typedef char const* key_type;
    static object get(object const& target, char const* key);
    static object get(object const& target, object const& key);
};
  
struct attribute_policies : const_attribute_policies
{
    static object const& set(object const& target, char const* key, object const& value);
    static void del(object const&target, char const* key);
};

struct const_objattribute_policies
{
    typedef object const key_type;
    static object get(object const& target, object const& key);
};
  
struct objattribute_policies : const_objattribute_policies
{
    static object const& set(object const& target, object const& key, object const& value);
    static void del(object const&target, object const& key);
};

//
// implementation
//
template <class U>
inline object_attribute object_operators<U>::attr(char const* name)
{
    object_cref2 x = *static_cast<U*>(this);
    return object_attribute(x, name);
}

template <class U>
inline const_object_attribute object_operators<U>::attr(char const* name) const
{
    object_cref2 x = *static_cast<U const*>(this);
    return const_object_attribute(x, name);
}

template <class U>
inline object_objattribute object_operators<U>::attr(object const& name)
{
    object_cref2 x = *static_cast<U*>(this);
    return object_objattribute(x, name);
}

template <class U>
inline const_object_objattribute object_operators<U>::attr(object const& name) const
{
    object_cref2 x = *static_cast<U const*>(this);
    return const_object_objattribute(x, name);
}

inline object const_attribute_policies::get(object const& target, char const* key)
{
    return python::getattr(target, key);
}

inline object const_objattribute_policies::get(object const& target, object const& key)
{
    return python::getattr(target, key);
}

inline object const& attribute_policies::set(
    object const& target
    , char const* key
    , object const& value)
{
    python::setattr(target, key, value);
    return value;
}

inline object const& objattribute_policies::set(
    object const& target
    , object const& key
    , object const& value)
{
    python::setattr(target, key, value);
    return value;
}

inline void attribute_policies::del(
    object const& target
    , char const* key)
{
    python::delattr(target, key);
}

inline void objattribute_policies::del(
    object const& target
    , object const& key)
{
    python::delattr(target, key);
}

}}} // namespace boost::python::api

#endif // OBJECT_ATTRIBUTES_DWA2002615_HPP

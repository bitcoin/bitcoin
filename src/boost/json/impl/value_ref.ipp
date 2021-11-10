//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/json
//

#ifndef BOOST_JSON_IMPL_VALUE_REF_IPP
#define BOOST_JSON_IMPL_VALUE_REF_IPP

#include <boost/json/value_ref.hpp>
#include <boost/json/array.hpp>
#include <boost/json/value.hpp>

BOOST_JSON_NS_BEGIN

value_ref::
operator
value() const
{
    return make_value({});
}

value
value_ref::
from_init_list(
    void const* p,
    storage_ptr sp)
{
    return make_value(
        *reinterpret_cast<
            init_list const*>(p),
        std::move(sp));
}

bool
value_ref::
is_key_value_pair() const noexcept
{
    if(what_ != what::ini)
        return false;
    if(arg_.init_list_.size() != 2)
        return false;
    auto const& e =
        *arg_.init_list_.begin();
    if( e.what_ != what::str &&
        e.what_ != what::strfunc)
        return false;
    return true;
}

bool
value_ref::
maybe_object(
    std::initializer_list<
        value_ref> init) noexcept
{
    for(auto const& e : init)
        if(! e.is_key_value_pair())
            return false;
    return true;
}

string_view
value_ref::
get_string() const noexcept
{
    BOOST_ASSERT(
        what_ == what::str ||
        what_ == what::strfunc);
    if (what_ == what::strfunc)
        return *static_cast<const string*>(f_.p);
    return arg_.str_;
}

value
value_ref::
make_value(
    storage_ptr sp) const
{
    switch(what_)
    {
    default:
    case what::str:
        return string(
            arg_.str_,
            std::move(sp));

    case what::ini:
        return make_value(
            arg_.init_list_,
            std::move(sp));

    case what::func:
        return f_.f(f_.p,
            std::move(sp));
    
    case what::strfunc:
        return f_.f(f_.p,
            std::move(sp));
    
    case what::cfunc:
        return cf_.f(cf_.p,
            std::move(sp));
    }
}

value
value_ref::
make_value(
    std::initializer_list<
        value_ref> init,
    storage_ptr sp)
{
    if(maybe_object(init))
        return make_object(
            init, std::move(sp));
    return make_array(
        init, std::move(sp));
}

object
value_ref::
make_object(
    std::initializer_list<value_ref> init,
    storage_ptr sp)
{
    object obj(std::move(sp));
    obj.reserve(init.size());
    for(auto const& e : init)
        obj.emplace(
            e.arg_.init_list_.begin()[0].get_string(),
            e.arg_.init_list_.begin()[1].make_value(
                obj.storage()));
    return obj;
}

array
value_ref::
make_array(
    std::initializer_list<
        value_ref> init,
    storage_ptr sp)
{
    array arr(std::move(sp));
    arr.reserve(init.size());
    for(auto const& e : init)
        arr.emplace_back(
            e.make_value(
                arr.storage()));
    return arr;
}

void
value_ref::
write_array(
    value* dest,
    std::initializer_list<
        value_ref> init,
    storage_ptr const& sp)
{
    struct undo
    {
        value* const base;
        value* pos;
        ~undo()
        {
            if(pos)
                while(pos > base)
                    (--pos)->~value();
        }
    };
    undo u{dest, dest};
    for(auto const& e : init)
    {
        ::new(u.pos) value(
            e.make_value(sp));
        ++u.pos;
    }
    u.pos = nullptr;
}

BOOST_JSON_NS_END

#endif

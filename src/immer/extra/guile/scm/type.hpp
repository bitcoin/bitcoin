//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

#include <scm/detail/finalizer_wrapper.hpp>
#include <scm/detail/define.hpp>
#include <string>

namespace scm {
namespace detail {

template <typename T>
struct foreign_type_storage
{
    static SCM data;
};

template <typename T>
SCM foreign_type_storage<T>::data = SCM_UNSPECIFIED;

template <typename T>
struct convert_foreign_type
{
    using storage_t = foreign_type_storage<T>;
    static T& to_cpp(SCM v)
    {
        assert(storage_t::data != SCM_UNSPECIFIED &&
               "can not convert to undefined type");
        scm_assert_foreign_object_type(storage_t::data, v);
        return *(T*)scm_foreign_object_ref(v, 0);
    }

    template <typename U>
    static SCM to_scm(U&& v)
    {
        assert(storage_t::data != SCM_UNSPECIFIED &&
               "can not convert from undefined type");
        return scm_make_foreign_object_1(
            storage_t::data,
            new (scm_gc_malloc(sizeof(T), "scmpp")) T(
                std::forward<U>(v)));
    }
};

// Assume that every other type is foreign
template <typename T>
struct convert<T,
               std::enable_if_t<!std::is_fundamental<T>::value &&
                                // only value types are supported at
                                // the moment but the story might
                                // change later...
                                !std::is_pointer<T>::value>>
    : convert_foreign_type<T>
{
};

template <typename Tag, typename T, int Seq=0>
struct type_definer : move_sequence
{
    using this_t = type_definer;
    using next_t = type_definer<Tag, T, Seq + 1>;

    std::string type_name_ = {};
    scm_t_struct_finalize finalizer_ = nullptr;

    type_definer(type_definer&&) = default;

    type_definer(std::string type_name)
        : type_name_(std::move(type_name))
    {}

    ~type_definer()
    {
        if (!moved_from_) {
            using storage_t = detail::foreign_type_storage<T>;
            assert(storage_t::data == SCM_UNSPECIFIED);
            storage_t::data = scm_make_foreign_object_type(
                scm_from_utf8_symbol(("<" + type_name_ + ">").c_str()),
                scm_list_1(scm_from_utf8_symbol("data")),
                finalizer_);
        }
    }

    template <int Seq2, typename Enable=std::enable_if_t<Seq2 + 1 == Seq>>
    type_definer(type_definer<Tag, T, Seq2> r)
        : move_sequence{std::move(r)}
        , type_name_{std::move(r.type_name_)}
        , finalizer_{std::move(r.finalizer_)}
    {}

    next_t constructor() &&
    {
        define_impl<this_t>(type_name_, [] { return T{}; });
        return { std::move(*this) };
    }

    template <typename Fn>
    next_t constructor(Fn fn) &&
    {
        define_impl<this_t>(type_name_, fn);
        return { std::move(*this) };
    }

    next_t finalizer() &&
    {
        finalizer_ = (scm_t_struct_finalize) +finalizer_wrapper<Tag>(
            [] (T& x) { x.~T(); });
        return { std::move(*this) };
    }

    template <typename Fn>
    next_t finalizer(Fn fn) &&
    {
        finalizer_ = (scm_t_struct_finalize) +finalizer_wrapper<Tag>(fn);
        return { std::move(*this) };
    }

    next_t maker() &&
    {
        define_impl<this_t>("make-" + type_name_, [] { return T{}; });
        return { std::move(*this) };
    }

    template <typename Fn>
    next_t maker(Fn fn) &&
    {
        define_impl<this_t>("make-" + type_name_, fn);
        return { std::move(*this) };
    }

    template <typename Fn>
    next_t define(std::string name, Fn fn) &&
    {
        define_impl<this_t>(type_name_ + "-" + name, fn);
        return { std::move(*this) };
    }
};

} // namespace detail

template <typename Tag, typename T=Tag>
detail::type_definer<Tag, T> type(std::string type_name)
{
    return { type_name };
}

} // namespace scm

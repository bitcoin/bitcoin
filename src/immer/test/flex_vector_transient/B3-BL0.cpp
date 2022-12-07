//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#include <immer/flex_vector.hpp>
#include <immer/flex_vector_transient.hpp>
#include <immer/vector.hpp>
#include <immer/vector_transient.hpp>

template <typename T>
using test_flex_vector_t =
    immer::flex_vector<T, immer::default_memory_policy, 3u, 0u>;

template <typename T>
using test_flex_vector_transient_t =
    typename test_flex_vector_t<T>::transient_type;

template <typename T>
using test_vector_t = immer::vector<T, immer::default_memory_policy, 3u, 0u>;

#define FLEX_VECTOR_T test_flex_vector_t
#define FLEX_VECTOR_TRANSIENT_T test_flex_vector_transient_t
#define VECTOR_T test_vector_t

#include "generic.ipp"

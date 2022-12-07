//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#include <immer/vector.hpp>
#include <immer/vector_transient.hpp>

template <typename T>
using test_vector_t = immer::vector<T, immer::default_memory_policy, 3u, 0u>;

template <typename T>
using test_vector_transient_t = typename test_vector_t<T>::transient_type;

#define VECTOR_T test_vector_t
#define VECTOR_TRANSIENT_T test_vector_transient_t

#include "generic.ipp"

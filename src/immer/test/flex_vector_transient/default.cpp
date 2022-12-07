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

#define FLEX_VECTOR_T ::immer::flex_vector
#define FLEX_VECTOR_TRANSIENT_T ::immer::flex_vector_transient
#define VECTOR_T ::immer::vector
#include "generic.ipp"

//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

#include <cstdint>
#include <memory>
#include <stdexcept>

struct no_more_input : std::exception
{};

constexpr auto fuzzer_input_max_size = 1 << 16;

struct fuzzer_input
{
    const std::uint8_t* data_;
    std::size_t size_;

    const std::uint8_t* next(std::size_t size)
    {
        if (size_ < size)
            throw no_more_input{};
        auto r = data_;
        data_ += size;
        size_ -= size;
        return r;
    }

    const std::uint8_t* next(std::size_t size, std::size_t align)
    {
        auto& p = const_cast<void*&>(reinterpret_cast<const void*&>(data_));
        auto r  = std::align(align, size, p, size_);
        if (r == nullptr)
            throw no_more_input{};
        return next(size);
    }

    template <typename Fn>
    int run(Fn step)
    {
        if (size_ > fuzzer_input_max_size)
            return 0;
        try {
            while (step(*this))
                continue;
        } catch (const no_more_input&) {};
        return 0;
    }
};

template <typename T>
const T& read(fuzzer_input& fz)
{
    return *reinterpret_cast<const T*>(fz.next(sizeof(T), alignof(T)));
}

template <typename T, typename Cond>
T read(fuzzer_input& fz, Cond cond)
{
    auto x = read<T>(fz);
    while (!cond(x))
        x = read<T>(fz);
    return x;
}

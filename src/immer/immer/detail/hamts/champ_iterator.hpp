//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

#include <immer/detail/hamts/champ.hpp>
#include <immer/detail/iterator_facade.hpp>

namespace immer {
namespace detail {
namespace hamts {

template <typename T, typename Hash, typename Eq, typename MP, bits_t B>
struct champ_iterator
    : iterator_facade<champ_iterator<T, Hash, Eq, MP, B>,
                      std::forward_iterator_tag,
                      T,
                      const T&,
                      std::ptrdiff_t,
                      const T*>
{
    using tree_t = champ<T, Hash, Eq, MP, B>;
    using node_t = typename tree_t::node_t;

    struct end_t
    {};

    champ_iterator(const tree_t& v)
        : depth_{0}
    {
        if (v.root->datamap()) {
            cur_ = v.root->values();
            end_ = v.root->values() + v.root->data_count();
        } else {
            cur_ = end_ = nullptr;
        }
        path_[0] = &v.root;
        ensure_valid_();
    }

    champ_iterator(const tree_t& v, end_t)
        : cur_{nullptr}
        , end_{nullptr}
        , depth_{0}
    {
        path_[0] = &v.root;
    }

    champ_iterator(const champ_iterator& other)
        : cur_{other.cur_}
        , end_{other.end_}
        , depth_{other.depth_}
    {
        std::copy(other.path_, other.path_ + depth_ + 1, path_);
    }

private:
    friend iterator_core_access;

    T* cur_;
    T* end_;
    count_t depth_;
    node_t* const* path_[max_depth<B> + 1] = {
        nullptr,
    };

    void increment()
    {
        ++cur_;
        ensure_valid_();
    }

    bool step_down()
    {
        if (depth_ < max_depth<B>) {
            auto parent = *path_[depth_];
            assert(parent);
            if (parent->nodemap()) {
                ++depth_;
                path_[depth_] = parent->children();
                auto child    = *path_[depth_];
                assert(child);
                if (depth_ < max_depth<B>) {
                    if (child->datamap()) {
                        cur_ = child->values();
                        end_ = cur_ + child->data_count();
                    }
                } else {
                    cur_ = child->collisions();
                    end_ = cur_ + child->collision_count();
                }
                return true;
            }
        }
        return false;
    }

    bool step_right()
    {
        while (depth_ > 0) {
            auto parent = *path_[depth_ - 1];
            auto last   = parent->children() + parent->children_count();
            auto next   = path_[depth_] + 1;
            if (next < last) {
                path_[depth_] = next;
                auto child    = *path_[depth_];
                assert(child);
                if (depth_ < max_depth<B>) {
                    if (child->datamap()) {
                        cur_ = child->values();
                        end_ = cur_ + child->data_count();
                    }
                } else {
                    cur_ = child->collisions();
                    end_ = cur_ + child->collision_count();
                }
                return true;
            }
            --depth_;
        }
        return false;
    }

    void ensure_valid_()
    {
        while (cur_ == end_) {
            while (step_down())
                if (cur_ != end_)
                    return;
            if (!step_right()) {
                // end of sequence
                assert(depth_ == 0);
                cur_ = end_ = nullptr;
                return;
            }
        }
    }

    bool equal(const champ_iterator& other) const { return cur_ == other.cur_; }

    const T& dereference() const { return *cur_; }
};

} // namespace hamts
} // namespace detail
} // namespace immer

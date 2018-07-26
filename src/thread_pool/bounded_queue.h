/////////////////////////////////////////////////////////////////////
//          Copyright Yibo Zhu 2017
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
/////////////////////////////////////////////////////////////////////

#pragma once

#include "utility.h"
#include <atomic>
#include <cassert>
#include <limits>

namespace async {

struct bounded_traits {
  static constexpr bool NOEXCEPT_CHECK = false; // exception handling flag
  static constexpr size_t CachelineSize = 64;
  using sequence_type = uint64_t;
};

template <typename T, typename TRAITS = bounded_traits> class bounded_queue {
private:
  static_assert(std::is_nothrow_destructible<T>::value,
                "T must be nothrow destructible");

public:
  static constexpr size_t cacheline_size = TRAITS::CachelineSize;
  using seq_t = typename TRAITS::sequence_type;
  explicit bounded_queue(size_t size)
      : fastmodulo((size > 0 && ((size & (size - 1)) == 0))),
        bitshift(fastmodulo ? getShiftBitsCount(size) : 0),
        elements(new element[size]), mask(fastmodulo ? size - 1 : 0),
        qsize(size), enqueueIx(0), dequeueIx(0) {
    assert(qsize > 0); // any size <= 0 is illegal
  }
  bounded_queue(bounded_queue const &) = delete;
  bounded_queue(bounded_queue &&) = delete;
  bounded_queue &operator=(bounded_queue const &) = delete;
  bounded_queue &operator=(bounded_queue &&) = delete;
  ~bounded_queue() { delete[] elements; }
  size_t size() { return qsize; }

  template <typename... Args, // NON-SAFE
            typename = typename std::enable_if<
                !TRAITS::NOEXCEPT_CHECK ||
                std::is_nothrow_constructible<T, Args &&...>::value>::type>
  inline void blocking_enqueue(Args &&... args) noexcept {
    auto enqidx = enqueueIx.fetch_add(1, std::memory_order_acq_rel);
    auto &ele = elements[index(enqidx)];
    auto enq_tkt = ticket(enqidx);
    while (enq_tkt != ele.tkt.load(std::memory_order_acquire))
      continue;
    ele.construct(std::forward<Args>(args)...);
    ele.tkt.store(enq_tkt + 1, std::memory_order_release);
  }

  template <typename... Args, // SAFE-IMPL
            typename = typename std::enable_if<
                TRAITS::NOEXCEPT_CHECK &&
                !std::is_nothrow_constructible<T, Args &&...>::value>::type>
  inline bool blocking_enqueue(Args &&... args) noexcept {
    auto enqidx = enqueueIx.fetch_add(1, std::memory_order_acq_rel);
    auto &ele = elements[index(enqidx)];
    auto enq_tkt = ticket(enqidx);
    while (enq_tkt != ele.tkt.load(std::memory_order_acquire))
      continue;
    if (ele.construct(std::forward<Args>(args)...)) {
      ele.hasdata.store(true, std::memory_order_release);
      ele.tkt.store(enq_tkt + 1, std::memory_order_release);
      return true;
    } else {
      ele.hasdata.store(false, std::memory_order_release);
      ele.tkt.store(enq_tkt + 1, std::memory_order_release);
      return false;
    }
  }

  template <typename... Args, // NON-SAFE
            typename std::enable_if<
                !TRAITS::NOEXCEPT_CHECK ||
                    std::is_nothrow_constructible<T, Args &&...>::value,
                int>::type = 0>
  inline bool enqueue(Args &&... args) noexcept {
    auto enqidx = enqueueIx.load(std::memory_order_acquire);
    for (;;) {
      auto &ele = elements[index(enqidx)];
      seq_t tkt = ele.tkt.load(std::memory_order_acquire);
      seq_t enq_tkt = ticket(enqidx);
      seq_t diff = tkt - enq_tkt;
      if (diff == 0) {
        if (enqueueIx.compare_exchange_strong(enqidx, enqidx + 1,
                                              std::memory_order_release,
                                              std::memory_order_relaxed)) {
          ele.construct(std::forward<Args>(args)...);
          ele.tkt.store(enq_tkt + 1, std::memory_order_release);
          return true;
        }
      } else if (diff >= std::numeric_limits<seq_t>::max() / 2)
        return false; // queue is full
      else
        enqidx = enqueueIx.load(std::memory_order_acquire);
    }
  }

  template <typename... Args, // SAFE-IMPL
            typename std::enable_if<
                TRAITS::NOEXCEPT_CHECK &&
                    !std::is_nothrow_constructible<T, Args &&...>::value,
                int>::type = 0>
  inline bool enqueue(Args &&... args) noexcept {
    auto enqidx = enqueueIx.load(std::memory_order_relaxed);
    for (;;) {
      auto &ele = elements[index(enqidx)];
      seq_t tkt = ele.tkt.load(std::memory_order_acquire);
      seq_t enq_tkt = ticket(enqidx);
      seq_t diff = tkt - enq_tkt;
      if (diff == 0) {
        if (enqueueIx.compare_exchange_strong(enqidx, enqidx + 1,
                                              std::memory_order_release,
                                              std::memory_order_relaxed)) {
          if (ele.construct(std::forward<Args>(args)...)) {
            ele.hasdata.store(true, std::memory_order_release);
            ele.tkt.store(enq_tkt + 1, std::memory_order_release);
            return true;
          } else {
            ele.hasdata.store(false, std::memory_order_release);
            ele.tkt.store(enq_tkt + 1, std::memory_order_release);
            return false;
          }
        }
      } else if (diff >= std::numeric_limits<seq_t>::max() / 2)
        return false; // queue is full
      else
        enqidx = enqueueIx.load(std::memory_order_acquire);
    }
  }

  template <typename U = T, // NON-SAFE
            typename = typename std::enable_if<
                !TRAITS::NOEXCEPT_CHECK ||
                std::is_nothrow_constructible<U>::value>::type>
  inline void blocking_dequeue(U &data) noexcept {
    auto deqidx = dequeueIx.fetch_add(1, std::memory_order_acq_rel);
    auto &ele = elements[index(deqidx)];
    seq_t deq_tkt = ticket(deqidx) + 1;
    while (deq_tkt != ele.tkt.load(std::memory_order_acquire))
      continue;
    ele.move(data);
    ele.tkt.store(deq_tkt + 1, std::memory_order_release);
  }

  template <typename U = T, // SAFE-IMPL
            typename = typename std::enable_if<
                TRAITS::NOEXCEPT_CHECK &&
                !std::is_nothrow_constructible<U>::value>::type>
  inline bool blocking_dequeue(U &data) noexcept {
    auto deqidx = dequeueIx.fetch_add(1, std::memory_order_acq_rel);
    auto &ele = elements[index(deqidx)];
    seq_t deq_tkt = ticket(deqidx) + 1;
    while (deq_tkt != ele.tkt.load(std::memory_order_acquire))
      continue;
    if (ele.hasdata.load(std::memory_order_acquire)) {
      ele.move(data);
      ele.tkt.store(deq_tkt + 1, std::memory_order_release);
      return true;
    } else {
      ele.tkt.store(deq_tkt + 1, std::memory_order_release);
      return false;
    }
  }

  template <typename U = T, // NON-SAFE
            typename std::enable_if<!TRAITS::NOEXCEPT_CHECK ||
                                        std::is_nothrow_constructible<U>::value,
                                    int>::type = 0>
  inline bool dequeue(U &data) {

    auto deqidx = dequeueIx.load(std::memory_order_acquire);
    for (;;) {
      auto &ele = elements[index(deqidx)];
      seq_t tkt = ele.tkt.load(std::memory_order_acquire);
      seq_t deq_tkt = ticket(deqidx) + 1;
      seq_t diff = tkt - deq_tkt;
      if (diff == 0) {
        if (dequeueIx.compare_exchange_strong(deqidx, deqidx + 1,
                                              std::memory_order_acq_rel,
                                              std::memory_order_relaxed)) {
          ele.move(data);
          ele.tkt.store(deq_tkt + 1, std::memory_order_release);
          return true;
        }
      } else if (diff >= std::numeric_limits<seq_t>::max() / 2)
        return false; // queue is empty
      else {

        deqidx = dequeueIx.load(std::memory_order_acquire);
      }
    }
  }

  template <
      typename U = T, // SAFE-IMPL
      typename std::enable_if<TRAITS::NOEXCEPT_CHECK &&
                                  !std::is_nothrow_constructible<U>::value,
                              int>::type = 0>
  inline bool
  dequeue(U &data) // false could be queue is empty, or skip an invalid element
  {

    auto deqidx = dequeueIx.load(std::memory_order_acquire);
    for (;;) {
      auto &ele = elements[index(deqidx)];
      seq_t tkt = ele.tkt.load(std::memory_order_acquire);
      seq_t deq_tkt = ticket(deqidx) + 1;
      seq_t diff = tkt - deq_tkt;
      if (diff == 0) {
        if (dequeueIx.compare_exchange_strong(deqidx, deqidx + 1,
                                              std::memory_order_acq_rel,
                                              std::memory_order_relaxed)) {
          if (ele.hasdata.load(std::memory_order_acquire)) {
            ele.move(data);
            ele.tkt.store(deq_tkt + 1, std::memory_order_release);
            return true;
          } else {
            ele.tkt.store(deq_tkt + 1, std::memory_order_release);
            return false;
          }
        }
      } else if (diff >= std::numeric_limits<seq_t>::max() / 2)
        return false; // queue is empty
      else {
        deqidx = dequeueIx.load(std::memory_order_acquire);
      }
    }
  }

private:
  inline seq_t index(seq_t const seq) {
    if (fastmodulo)
      return seq & mask;
    else
      return seq >= qsize ? seq % qsize : seq;
  }

  inline seq_t ticket(seq_t const seq) {
    if (fastmodulo)
      return (seq >> bitshift) << 1;
    else
      return (seq / static_cast<seq_t>(qsize)) << 1;
  }
  //TODO& Review: replace the following with c++ concepts
  template <typename U = T, typename Enable = void> struct checkdata {};

  template <typename U>
  struct checkdata<U, typename std::enable_if<
                          !TRAITS::NOEXCEPT_CHECK ||
                          std::is_nothrow_constructible<U>::value>::type> {};

  template <typename U>
  struct checkdata<U, typename std::enable_if<
                          TRAITS::NOEXCEPT_CHECK &&
                          !std::is_nothrow_constructible<U>::value>::type> {
    checkdata() : hasdata(false) {}
    std::atomic<bool> hasdata;
  };

  struct element : public checkdata<T> {
    element() : tkt(0) {}
    ~element() {
      if (tkt & 1) // enqueue op visited
        destruct();
    }

    template <typename... Args, // NON-SAFE
              typename = typename std::enable_if<
                  !TRAITS::NOEXCEPT_CHECK ||
                  std::is_nothrow_constructible<T, Args &&...>::value>::type>
    inline void construct(Args &&... args) noexcept {
      new (&storage) T(std::forward<Args>(args)...);
    }

    template <typename... Args, // SAFE-IMPL
              typename = typename std::enable_if<
                  TRAITS::NOEXCEPT_CHECK &&
                  !std::is_nothrow_constructible<T, Args &&...>::value>::type>
    inline bool construct(Args &&... args) noexcept {
      try {
        new (&storage) T(std::forward<Args>(args)...);
      } catch (...) {
        return false;
      }
      return true;
    }

    inline void destruct() noexcept { reinterpret_cast<T *>(&storage)->~T(); }

    inline T *getptr() { return reinterpret_cast<T *>(&storage); }

    template <
        typename U = T, // NON-SAFE
        typename std::enable_if<!TRAITS::NOEXCEPT_CHECK ||
                                    std::is_nothrow_move_assignable<U>::value,
                                int>::type = 0>
    inline void move(U &data) {
      data = std::move(*getptr());
      destruct();
    }

    template <
        typename U = T, // SAFE-IMPL
        typename std::enable_if<TRAITS::NOEXCEPT_CHECK &&
                                    !std::is_nothrow_move_assignable<U>::value,
                                int>::type = 0>
    inline void move(U &data) {
      try {
        data = std::move(*getptr());
      } catch (...) {
      }
      destruct();
    }

    std::atomic<seq_t> tkt;
    typename std::aligned_storage<sizeof(T), alignof(T)>::type storage;
    std::atomic<bool> hasdata;
  };

  bool const fastmodulo;   // true if qsize is power of 2
  int const bitshift;      // used if fastmodulo is true
  element *const elements; // pointer to buffer
  size_t const mask;       // used if fastmodulo is true
  size_t const qsize;      // queue size
  alignas(cacheline_size) char cacheline_padding1[cacheline_size];
  alignas(cacheline_size) std::atomic<seq_t> enqueueIx;
  alignas(cacheline_size) char cacheline_padding2[cacheline_size];
  alignas(cacheline_size) std::atomic<seq_t> dequeueIx;
  alignas(cacheline_size) char cacheline_padding3[cacheline_size];
};
} // namespace async

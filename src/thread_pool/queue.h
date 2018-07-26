/////////////////////////////////////////////////////////////////////
//          Copyright Yibo Zhu 2017
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
/////////////////////////////////////////////////////////////////////
#pragma once
#include "utility.h"
#include <array>
#include <atomic>
#include <memory>

namespace async {
struct traits // 3-level (L3, L2, L1) depth of nested group design, total
              // indexing space is pow(2, 64-Tagbits)
{             // user can change the bits settings by providing your own TRAITS
  static constexpr uint64_t Tagbits = 24;
  static constexpr uint64_t L3bits = 10;
  static constexpr uint64_t L2bits = 10;
  static constexpr uint64_t L1bits = 12;
  static constexpr uint64_t Basebits = 8;
  static constexpr bool NOEXCEPT_CHECK = false; // exception handling flag
  static constexpr size_t CachelineSize = 64;
};

template <typename T, typename TRAITS = traits> class queue final {
public:
  static bool is_lock_free_v() {
    return std::atomic<uint64_t>{}.is_lock_free();
  }
  static constexpr size_t cacheline_size = TRAITS::CachelineSize;
  static constexpr uint64_t BaseMask = getBitmask<uint64_t>(TRAITS::Basebits);
  static constexpr uint64_t L1Mask = getBitmask<uint64_t>(TRAITS::L1bits)
                                     << TRAITS::Basebits;
  static constexpr uint64_t L2Mask = getBitmask<uint64_t>(TRAITS::L2bits)
                                     << (TRAITS::Basebits + TRAITS::L1bits);
  static constexpr uint64_t L3Mask =
      getBitmask<uint64_t>(TRAITS::L3bits)
      << (TRAITS::Basebits + TRAITS::L1bits + TRAITS::L2bits);
  static constexpr uint64_t TagMask =
      getBitmask<uint64_t>(TRAITS::Tagbits)
      << (TRAITS::Basebits + TRAITS::L1bits + TRAITS::L2bits + TRAITS::L3bits);
  static constexpr uint64_t TagShift = 64 - TRAITS::Tagbits;
  static constexpr uint64_t TagPlus1 = static_cast<uint64_t>(1) << TagShift;

public: // assert bits settings meet requirements
  static_assert(TRAITS::Tagbits + TRAITS::L3bits + TRAITS::L2bits +
                        TRAITS::L1bits + TRAITS::Basebits ==
                    64,
                "The sum of all bits settings should be 64");
  static_assert(TRAITS::Tagbits > 0 && TRAITS::L3bits > 0 &&
                    TRAITS::L2bits > 0 && TRAITS::L1bits > 0 &&
                    TRAITS::Basebits > 3,
                "All bits settings should be > 0 and Basebits must be > 3");
  static_assert(std::is_nothrow_destructible<T>::value,
                "T must be nothrow destructible");

public:
  queue() : nodeCount(3), dequeueIx(2), enqueueIx(2), spawnIx(1), recycleIx(1) {
    container.get(index(0)); // allocate initial space
  }
  queue(size_t size) // pre-allocate size
      : nodeCount(3), dequeueIx(2), enqueueIx(2), spawnIx(1), recycleIx(1) {
    container.get(index(0));

    if (size > (static_cast<uint64_t>(1) << TRAITS::Basebits)) {
      index ix;
      for (size_t i = (static_cast<uint64_t>(1) << TRAITS::Basebits); i < size;
           ++i) {
        auto &node = getNode(ix);
        recycle(ix);
      }
    }
  }

  queue(queue const &other) = delete;
  queue &operator=(queue const &other) = delete;
  queue(queue &&other) = delete;
  queue &operator=(queue &&other) = delete;

  template <typename... Args, // NON-SAFE
            typename = typename std::enable_if<
                !TRAITS::NOEXCEPT_CHECK ||
                std::is_nothrow_constructible<T, Args &&...>::value>::type>
  inline void enqueue(Args &&... args) noexcept {
    auto ix = encapsulate(std::forward<Args>(args)...);
    auto enqidx = enqueueIx.load(std::memory_order_relaxed);
    while (!enqueueIx.compare_exchange_weak(
        enqidx, ix, std::memory_order_release, std::memory_order_relaxed))
      continue;
    container[enqidx].next.store(ix, std::memory_order_release);
  }

  template <typename... Args, // SAFE-IMPL
            typename = typename std::enable_if<
                TRAITS::NOEXCEPT_CHECK &&
                !std::is_nothrow_constructible<T, Args &&...>::value>::type>
  inline bool enqueue(Args &&... args) noexcept {
    auto ix = encapsulate(std::forward<Args>(args)...);
    if (ix == 0)
      return false;
    auto enqidx = enqueueIx.load(std::memory_order_relaxed);
    while (!enqueueIx.compare_exchange_weak(
        enqidx, ix, std::memory_order_release, std::memory_order_relaxed))
      continue;
    container[enqidx].next.store(ix, std::memory_order_release);
    return true;
  }

  template <typename IT> void bulk_enqueue(IT it, size_t count) {
    index firstidx(0), preidx(0), lastidx(0);
    for (size_t i = 0; i < count; ++i) {
      lastidx = encapsulate(*it++);
      if (firstidx == 0)
        firstidx = lastidx;
      if (preidx != 0) {
        container[preidx].next.store(lastidx, std::memory_order_relaxed);
      }
      preidx = lastidx;
    }
    auto enqidx = enqueueIx.load(std::memory_order_relaxed);
    while (!enqueueIx.compare_exchange_weak(
        enqidx, lastidx, std::memory_order_release, std::memory_order_relaxed))
      continue;
    container[enqidx].next.store(firstidx, std::memory_order_release);
  }

  template <typename IT>
  size_t bulk_dequeue(IT &&it, size_t maxcount) // or IT& it to return the
  {
    size_t count(0);
    while (maxcount-- && dequeue(*it++)) {
      ++count;
    }
    return count;
  }

  template <typename U> // U could be T, or any kinds of iterators/adapters,
                        // like insert_iterator
  inline bool dequeue(U &data) noexcept // return false if queue is empty
  {
    for (;;) {
      auto deqidx = dequeueIx.load(std::memory_order_acquire);
      auto &node = container[deqidx];
      auto next = node.next.load(std::memory_order_relaxed);
      if (next == 0) {
        auto ready_for_consume =
            node.consume_ready.load(std::memory_order_relaxed);
        if (!ready_for_consume) {
          return false;
        }

        if (node.consume_ready.compare_exchange_strong(
                ready_for_consume, false, std::memory_order_release,
                std::memory_order_relaxed)) {
          node.template move<TRAITS>(data);
          return true;
        }
      } else {
        if (dequeueIx.compare_exchange_weak(deqidx, next,
                                            std::memory_order_acq_rel,
                                            std::memory_order_relaxed)) {
          auto ready_for_consume =
              node.consume_ready.load(std::memory_order_acquire);
          if (ready_for_consume &&
              node.consume_ready.compare_exchange_strong(
                  ready_for_consume, false, std::memory_order_release,
                  std::memory_order_relaxed)) {
            node.template move<TRAITS>(data);
          } else { // the node is being consumed by another thread, waiting for
                   // it finishes
            for (; !node.recycle_ready.load(std::memory_order_acquire);) {
            }
          }
          node.next.store(
              0, std::memory_order_relaxed); // reset link to avoid chain effect
          recycle(deqidx);
          if (ready_for_consume)
            return ready_for_consume;
        }
      }
    }
  }
  uint64_t getNodeCount() { return nodeCount; } // get in-use-nodes count

private:       // internal data structures
  struct index // simulate tagged pointer
  {
    index(uint64_t newval) noexcept
        : value(newval) {} // is_trivially_copyable must be true
    index() noexcept : value(0) {}
    inline operator uint64_t() const { return value; }
    std::uint64_t getVersion() { return (value & TagMask) >> TagShift; }
    inline void increTag() {
      value = (value & ~TagMask) | ((value + TagPlus1) & TagMask);
    }
    std::uint64_t value;
  };

  struct node // to store the data
  {
    node() : next(0), consume_ready(false), recycle_ready(true) {}
    ~node() noexcept {
      if (consume_ready.load(std::memory_order_relaxed)) {
        destruct();
      }
    }

    template <typename... Args, // NON-SAFE
              typename = typename std::enable_if<
                  !TRAITS::NOEXCEPT_CHECK ||
                  std::is_nothrow_constructible<T, Args &&...>::value>::type>
    inline void construct(Args &&... args) noexcept {
      new (&storage) T(std::forward<Args>(args)...);
      consume_ready.store(true, std::memory_order_release);
      recycle_ready.store(false, std::memory_order_release);
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

      consume_ready.store(true, std::memory_order_release);
      recycle_ready.store(false, std::memory_order_release);
      return true;
    }

    inline void destruct() noexcept { reinterpret_cast<T *>(&storage)->~T(); }

    template <
        typename TR, typename U, // NON-SAFE
        typename std::enable_if<!TR::NOEXCEPT_CHECK ||
                                    std::is_nothrow_move_assignable<T>::value,
                                int>::type = 0>
    inline void move(U &data) {
      data = std::move(*getptr());
      destruct();
      recycle_ready.store(true, std::memory_order_release);
    }

    template <
        typename TR, typename U, // SAFE-IMPL
        typename std::enable_if<TR::NOEXCEPT_CHECK &&
                                    !std::is_nothrow_move_assignable<T>::value,
                                int>::type = 0>
    inline void move(U &data) {
      try {
        data = std::move(*getptr());
      } catch (...) {
      }
      destruct();
      recycle_ready.store(true, std::memory_order_release);
    }
    inline T *getptr() { return reinterpret_cast<T *>(&storage); }
    std::atomic<index> next;         // link
    std::atomic<bool> consume_ready; // if true, consume ready
    std::atomic<bool> recycle_ready; // if true, recycle ready
    typename std::aligned_storage<sizeof(T), alignof(T)>::type storage; // data
  };

  struct basecontainer {
    inline node &get(index const &ix) { return operator[](ix); }
    inline node &at(index const &ix) { return operator[](ix); }
    inline node &operator[](index const &ix) { return nodes[ix & BaseMask]; }
    std::array<node, static_cast<uint64_t>(1) << TRAITS::Basebits> nodes;
  };

  template <typename SubGroup, uint64_t BitMask> struct nestedcontainer {
    static constexpr uint64_t mask = BitMask;
    static constexpr uint64_t bits = getSetBitsCount(mask);
    static constexpr uint64_t shift = getShiftBitsCount(mask);
    std::array<std::atomic<SubGroup *>, static_cast<uint64_t>(1) << bits>
        subgroups;
    nestedcontainer() {
      for (auto &gptr : subgroups) {
        gptr.store(nullptr, std::memory_order_release);
      }
    }
    ~nestedcontainer() {
      for (auto &gptr : subgroups) {
        if (gptr.load(std::memory_order_relaxed) != nullptr)
          delete gptr.load(std::memory_order_relaxed);
      }
    }

    inline node &get(index const &ix) // will trigger the new operation if
                                      // subgroup doesn't exist
    {
      auto ptr =
          subgroups[(ix & mask) >> shift].load(std::memory_order_acquire);
      if (ptr == nullptr) {
        auto newgroup = std::make_unique<SubGroup>(); // if ComExch fails,
                                                      // unique_ptr will self
                                                      // delete
        if (subgroups[(ix & mask) >> shift].compare_exchange_strong(
                ptr, newgroup.get(), std::memory_order_release,
                std::memory_order_acquire)) {
          ptr = newgroup.release();
        }
      }
      return ptr->get(ix); // recursively calling get 'til get the node
    }

    inline node &operator[](index const &ix) {
      return subgroups[(ix & mask) >> shift]
          .load(std::memory_order_relaxed)
          ->
          operator[](ix);
    }

    inline node &at(index const &ix) { // balanced performance and safety
      auto ptr =
          subgroups[(ix & mask) >> shift].load(std::memory_order_relaxed);
      if (ptr)
        return ptr->at(ix);
      else
        return get(ix);
    }
  };

  inline node &getNode(index &ix) { // return an existing or new node
    #if defined(__arm__) && (!defined(__aarch64__))
    //for ARMV7 or below
    ix.value = nodeCount.load(std::memory_order_relaxed);
    auto val = ix.value + 1;
    while(!nodeCount.compare_exchange_weak(
      ix.value, val, std::memory_order_release, std::memory_order_relaxed)) {
        val = ix.value + 1;
    }
    #else
    ix.value = nodeCount.fetch_add(static_cast<std::uint64_t>(1),
                              std::memory_order_relaxed);
    #endif
    if ((ix.value & BaseMask) == 0)
      return container.get(ix);
    else
      return container.at(ix);
  }

  template <typename... Args, // NON-SAFE
            typename std::enable_if<
                !TRAITS::NOEXCEPT_CHECK ||
                    std::is_nothrow_constructible<T, Args &&...>::value,
                int>::type = 0>
  inline index encapsulate(Args &&... args) noexcept {
    auto ix = spawn();
    auto &node = container[ix];
    node.construct(std::forward<Args>(args)...);
    node.next.store(0, std::memory_order_relaxed);
    return ix;
  }

  template <typename... Args, // SAFE-IMPL
            typename std::enable_if<
                TRAITS::NOEXCEPT_CHECK &&
                    !std::is_nothrow_constructible<T, Args &&...>::value,
                int>::type = 0>
  inline index encapsulate(Args &&... args) noexcept {
    auto ix = spawn();
    auto &node = container[ix];
    node.next.store(0, std::memory_order_relaxed);
    if (node.construct(std::forward<Args>(args)...))
      return ix;
    else {
      recycle(ix); // construction failed, recycle the node
      return index(0);
    }
  }

  inline void recycle(index const &ix) {
    auto recycle = recycleIx.load(std::memory_order_relaxed);
    while (!recycleIx.compare_exchange_weak(
        recycle, ix, std::memory_order_release, std::memory_order_relaxed))
      continue;
    container[recycle].next.store(ix, std::memory_order_release);
  }

  inline auto spawn()
#if ((defined(__clang__) || defined(__GNUC__)) && __cplusplus <= 201103L) ||   \
    (defined(_MSC_VER) && _MSC_VER < 1800)
      -> index
#endif
  {
    index ix(0);
    for (;;) {
      auto spaidx = spawnIx.load(std::memory_order_acquire);
      auto next = container[spaidx].next.load(std::memory_order_relaxed);
      if (next == 0) {
        getNode(ix);
        return ix;
      } else {
        if (spawnIx.compare_exchange_weak(spaidx, next,
                                          std::memory_order_acq_rel,
                                          std::memory_order_relaxed)) {
          if (spaidx != 0) {
            spaidx.increTag();
          }
          return spaidx;
        }
      }
    }
  }
 
  using L1container = nestedcontainer<basecontainer, L1Mask>;
  using L2container = nestedcontainer<L1container, L2Mask>;
  nestedcontainer<L2container, L3Mask> container;
  alignas(cacheline_size) char cacheline_padding1[cacheline_size];
  alignas(cacheline_size) std::atomic<uint64_t> nodeCount; // # of allocated nodes, not the #
                                                           // of elements stored in the queue
  alignas(cacheline_size) char cacheline_padding2[cacheline_size];
  alignas(cacheline_size) std::atomic<index> dequeueIx;    // dequeue pointer
  alignas(cacheline_size) char cacheline_padding3[cacheline_size];
  alignas(cacheline_size) std::atomic<index> enqueueIx;    // enqueue pointer
  alignas(cacheline_size) char cacheline_padding4[cacheline_size];
  alignas(cacheline_size) std::atomic<index> spawnIx;      // spawn pointer
  alignas(cacheline_size) char cacheline_padding5[cacheline_size];
  alignas(cacheline_size) std::atomic<index> recycleIx;    // recycle pointer
  alignas(cacheline_size) char cacheline_padding6[cacheline_size];
};
} // namespace async
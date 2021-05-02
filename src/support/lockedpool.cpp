// Copyright (c) 2016-2020 The XBit Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <support/lockedpool.h>
#include <support/cleanse.h>

#if defined(HAVE_CONFIG_H)
#include <config/xbit-config.h>
#endif

#ifdef WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <sys/mman.h> // for mmap
#include <sys/resource.h> // for getrlimit
#include <limits.h> // for PAGESIZE
#include <unistd.h> // for sysconf
#endif

#include <algorithm>
#ifdef ARENA_DEBUG
#include <iomanip>
#include <iostream>
#endif

LockedPoolManager* LockedPoolManager::_instance = nullptr;

/*******************************************************************************/
// Utilities
//
/** Align up to power of 2 */
static inline size_t align_up(size_t x, size_t align)
{
    return (x + align - 1) & ~(align - 1);
}

/*******************************************************************************/
// Implementation: Arena

Arena::Arena(void *base_in, size_t size_in, size_t alignment_in):
    base(static_cast<char*>(base_in)), end(static_cast<char*>(base_in) + size_in), alignment(alignment_in)
{
    // Start with one free chunk that covers the entire arena
    auto it = size_to_free_chunk.emplace(size_in, base);
    chunks_free.emplace(base, it);
    chunks_free_end.emplace(base + size_in, it);
}

Arena::~Arena()
{
}

void* Arena::alloc(size_t size)
{
    // Round to next multiple of alignment
    size = align_up(size, alignment);

    // Don't handle zero-sized chunks
    if (size == 0)
        return nullptr;

    // Pick a large enough free-chunk. Returns an iterator pointing to the first element that is not less than key.
    // This allocation strategy is best-fit. According to "Dynamic Storage Allocation: A Survey and Critical Review",
    // Wilson et. al. 1995, http://www.scs.stanford.edu/14wi-cs140/sched/readings/wilson.pdf, best-fit and first-fit
    // policies seem to work well in practice.
    auto size_ptr_it = size_to_free_chunk.lower_bound(size);
    if (size_ptr_it == size_to_free_chunk.end())
        return nullptr;

    // Create the used-chunk, taking its space from the end of the free-chunk
    const size_t size_remaining = size_ptr_it->first - size;
    auto allocated = chunks_used.emplace(size_ptr_it->second + size_remaining, size).first;
    chunks_free_end.erase(size_ptr_it->second + size_ptr_it->first);
    if (size_ptr_it->first == size) {
        // whole chunk is used up
        chunks_free.erase(size_ptr_it->second);
    } else {
        // still some memory left in the chunk
        auto it_remaining = size_to_free_chunk.emplace(size_remaining, size_ptr_it->second);
        chunks_free[size_ptr_it->second] = it_remaining;
        chunks_free_end.emplace(size_ptr_it->second + size_remaining, it_remaining);
    }
    size_to_free_chunk.erase(size_ptr_it);

    return reinterpret_cast<void*>(allocated->first);
}

void Arena::free(void *ptr)
{
    // Freeing the nullptr pointer is OK.
    if (ptr == nullptr) {
        return;
    }

    // Remove chunk from used map
    auto i = chunks_used.find(static_cast<char*>(ptr));
    if (i == chunks_used.end()) {
        throw std::runtime_error("Arena: invalid or double free");
    }
    std::pair<char*, size_t> freed = *i;
    chunks_used.erase(i);

    // coalesce freed with previous chunk
    auto prev = chunks_free_end.find(freed.first);
    if (prev != chunks_free_end.end()) {
        freed.first -= prev->second->first;
        freed.second += prev->second->first;
        size_to_free_chunk.erase(prev->second);
        chunks_free_end.erase(prev);
    }

    // coalesce freed with chunk after freed
    auto next = chunks_free.find(freed.first + freed.second);
    if (next != chunks_free.end()) {
        freed.second += next->second->first;
        size_to_free_chunk.erase(next->second);
        chunks_free.erase(next);
    }

    // Add/set space with coalesced free chunk
    auto it = size_to_free_chunk.emplace(freed.second, freed.first);
    chunks_free[freed.first] = it;
    chunks_free_end[freed.first + freed.second] = it;
}

Arena::Stats Arena::stats() const
{
    Arena::Stats r{ 0, 0, 0, chunks_used.size(), chunks_free.size() };
    for (const auto& chunk: chunks_used)
        r.used += chunk.second;
    for (const auto& chunk: chunks_free)
        r.free += chunk.second->first;
    r.total = r.used + r.free;
    return r;
}

#ifdef ARENA_DEBUG
static void printchunk(void* base, size_t sz, bool used) {
    std::cout <<
        "0x" << std::hex << std::setw(16) << std::setfill('0') << base <<
        " 0x" << std::hex << std::setw(16) << std::setfill('0') << sz <<
        " 0x" << used << std::endl;
}
void Arena::walk() const
{
    for (const auto& chunk: chunks_used)
        printchunk(chunk.first, chunk.second, true);
    std::cout << std::endl;
    for (const auto& chunk: chunks_free)
        printchunk(chunk.first, chunk.second->first, false);
    std::cout << std::endl;
}
#endif

/*******************************************************************************/
// Implementation: Win32LockedPageAllocator

#ifdef WIN32
/** LockedPageAllocator specialized for Windows.
 */
class Win32LockedPageAllocator: public LockedPageAllocator
{
public:
    Win32LockedPageAllocator();
    void* AllocateLocked(size_t len, bool *lockingSuccess) override;
    void FreeLocked(void* addr, size_t len) override;
    size_t GetLimit() override;
private:
    size_t page_size;
};

Win32LockedPageAllocator::Win32LockedPageAllocator()
{
    // Determine system page size in bytes
    SYSTEM_INFO sSysInfo;
    GetSystemInfo(&sSysInfo);
    page_size = sSysInfo.dwPageSize;
}
void *Win32LockedPageAllocator::AllocateLocked(size_t len, bool *lockingSuccess)
{
    len = align_up(len, page_size);
    void *addr = VirtualAlloc(nullptr, len, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (addr) {
        // VirtualLock is used to attempt to keep keying material out of swap. Note
        // that it does not provide this as a guarantee, but, in practice, memory
        // that has been VirtualLock'd almost never gets written to the pagefile
        // except in rare circumstances where memory is extremely low.
        *lockingSuccess = VirtualLock(const_cast<void*>(addr), len) != 0;
    }
    return addr;
}
void Win32LockedPageAllocator::FreeLocked(void* addr, size_t len)
{
    len = align_up(len, page_size);
    memory_cleanse(addr, len);
    VirtualUnlock(const_cast<void*>(addr), len);
}

size_t Win32LockedPageAllocator::GetLimit()
{
    // TODO is there a limit on Windows, how to get it?
    return std::numeric_limits<size_t>::max();
}
#endif

/*******************************************************************************/
// Implementation: PosixLockedPageAllocator

#ifndef WIN32
/** LockedPageAllocator specialized for OSes that don't try to be
 * special snowflakes.
 */
class PosixLockedPageAllocator: public LockedPageAllocator
{
public:
    PosixLockedPageAllocator();
    void* AllocateLocked(size_t len, bool *lockingSuccess) override;
    void FreeLocked(void* addr, size_t len) override;
    size_t GetLimit() override;
private:
    size_t page_size;
};

PosixLockedPageAllocator::PosixLockedPageAllocator()
{
    // Determine system page size in bytes
#if defined(PAGESIZE) // defined in limits.h
    page_size = PAGESIZE;
#else                   // assume some POSIX OS
    page_size = sysconf(_SC_PAGESIZE);
#endif
}

// Some systems (at least OS X) do not define MAP_ANONYMOUS yet and define
// MAP_ANON which is deprecated
#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif

void *PosixLockedPageAllocator::AllocateLocked(size_t len, bool *lockingSuccess)
{
    void *addr;
    len = align_up(len, page_size);
    addr = mmap(nullptr, len, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    if (addr == MAP_FAILED) {
        return nullptr;
    }
    if (addr) {
        *lockingSuccess = mlock(addr, len) == 0;
#if defined(MADV_DONTDUMP) // Linux
        madvise(addr, len, MADV_DONTDUMP);
#elif defined(MADV_NOCORE) // FreeBSD
        madvise(addr, len, MADV_NOCORE);
#endif
    }
    return addr;
}
void PosixLockedPageAllocator::FreeLocked(void* addr, size_t len)
{
    len = align_up(len, page_size);
    memory_cleanse(addr, len);
    munlock(addr, len);
    munmap(addr, len);
}
size_t PosixLockedPageAllocator::GetLimit()
{
#ifdef RLIMIT_MEMLOCK
    struct rlimit rlim;
    if (getrlimit(RLIMIT_MEMLOCK, &rlim) == 0) {
        if (rlim.rlim_cur != RLIM_INFINITY) {
            return rlim.rlim_cur;
        }
    }
#endif
    return std::numeric_limits<size_t>::max();
}
#endif

/*******************************************************************************/
// Implementation: LockedPool

LockedPool::LockedPool(std::unique_ptr<LockedPageAllocator> allocator_in, LockingFailed_Callback lf_cb_in):
    allocator(std::move(allocator_in)), lf_cb(lf_cb_in), cumulative_bytes_locked(0)
{
}

LockedPool::~LockedPool()
{
}
void* LockedPool::alloc(size_t size)
{
    std::lock_guard<std::mutex> lock(mutex);

    // Don't handle impossible sizes
    if (size == 0 || size > ARENA_SIZE)
        return nullptr;

    // Try allocating from each current arena
    for (auto &arena: arenas) {
        void *addr = arena.alloc(size);
        if (addr) {
            return addr;
        }
    }
    // If that fails, create a new one
    if (new_arena(ARENA_SIZE, ARENA_ALIGN)) {
        return arenas.back().alloc(size);
    }
    return nullptr;
}

void LockedPool::free(void *ptr)
{
    std::lock_guard<std::mutex> lock(mutex);
    // TODO we can do better than this linear search by keeping a map of arena
    // extents to arena, and looking up the address.
    for (auto &arena: arenas) {
        if (arena.addressInArena(ptr)) {
            arena.free(ptr);
            return;
        }
    }
    throw std::runtime_error("LockedPool: invalid address not pointing to any arena");
}

LockedPool::Stats LockedPool::stats() const
{
    std::lock_guard<std::mutex> lock(mutex);
    LockedPool::Stats r{0, 0, 0, cumulative_bytes_locked, 0, 0};
    for (const auto &arena: arenas) {
        Arena::Stats i = arena.stats();
        r.used += i.used;
        r.free += i.free;
        r.total += i.total;
        r.chunks_used += i.chunks_used;
        r.chunks_free += i.chunks_free;
    }
    return r;
}

bool LockedPool::new_arena(size_t size, size_t align)
{
    bool locked;
    // If this is the first arena, handle this specially: Cap the upper size
    // by the process limit. This makes sure that the first arena will at least
    // be locked. An exception to this is if the process limit is 0:
    // in this case no memory can be locked at all so we'll skip past this logic.
    if (arenas.empty()) {
        size_t limit = allocator->GetLimit();
        if (limit > 0) {
            size = std::min(size, limit);
        }
    }
    void *addr = allocator->AllocateLocked(size, &locked);
    if (!addr) {
        return false;
    }
    if (locked) {
        cumulative_bytes_locked += size;
    } else if (lf_cb) { // Call the locking-failed callback if locking failed
        if (!lf_cb()) { // If the callback returns false, free the memory and fail, otherwise consider the user warned and proceed.
            allocator->FreeLocked(addr, size);
            return false;
        }
    }
    arenas.emplace_back(allocator.get(), addr, size, align);
    return true;
}

LockedPool::LockedPageArena::LockedPageArena(LockedPageAllocator *allocator_in, void *base_in, size_t size_in, size_t align_in):
    Arena(base_in, size_in, align_in), base(base_in), size(size_in), allocator(allocator_in)
{
}
LockedPool::LockedPageArena::~LockedPageArena()
{
    allocator->FreeLocked(base, size);
}

/*******************************************************************************/
// Implementation: LockedPoolManager
//
LockedPoolManager::LockedPoolManager(std::unique_ptr<LockedPageAllocator> allocator_in):
    LockedPool(std::move(allocator_in), &LockedPoolManager::LockingFailed)
{
}

bool LockedPoolManager::LockingFailed()
{
    // TODO: log something but how? without including util.h
    return true;
}

void LockedPoolManager::CreateInstance()
{
    // Using a local static instance guarantees that the object is initialized
    // when it's first needed and also deinitialized after all objects that use
    // it are done with it.  I can think of one unlikely scenario where we may
    // have a static deinitialization order/problem, but the check in
    // LockedPoolManagerBase's destructor helps us detect if that ever happens.
#ifdef WIN32
    std::unique_ptr<LockedPageAllocator> allocator(new Win32LockedPageAllocator());
#else
    std::unique_ptr<LockedPageAllocator> allocator(new PosixLockedPageAllocator());
#endif
    static LockedPoolManager instance(std::move(allocator));
    LockedPoolManager::_instance = &instance;
}

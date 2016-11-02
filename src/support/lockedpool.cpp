// Copyright (c) 2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "support/lockedpool.h"
#include "support/cleanse.h"

#if defined(HAVE_CONFIG_H)
#include "config/dash-config.h"
#endif

#ifdef WIN32
#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif
#define _WIN32_WINNT 0x0501
#define WIN32_LEAN_AND_MEAN 1
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

LockedPoolManager* LockedPoolManager::_instance = NULL;
std::once_flag LockedPoolManager::init_flag;

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
    chunks.emplace(base, Chunk(size_in, false));
}

Arena::~Arena()
{
}

void* Arena::alloc(size_t size)
{
    // Round to next multiple of alignment
    size = align_up(size, alignment);

    // Don't handle zero-sized chunks, or those bigger than MAX_SIZE
    if (size == 0 || size >= Chunk::MAX_SIZE) {
        return nullptr;
    }

    for (auto& chunk: chunks) {
        if (!chunk.second.isInUse() && size <= chunk.second.getSize()) {
            char* _base = chunk.first;
            size_t leftover = chunk.second.getSize() - size;
            if (leftover > 0) { // Split chunk
                chunks.emplace(_base + size, Chunk(leftover, false));
                chunk.second.setSize(size);
            }
            chunk.second.setInUse(true);
            return reinterpret_cast<void*>(_base);
        }
    }
    return nullptr;
}

void Arena::free(void *ptr)
{
    // Freeing the NULL pointer is OK.
    if (ptr == nullptr) {
        return;
    }
    auto i = chunks.find(static_cast<char*>(ptr));
    if (i == chunks.end() || !i->second.isInUse()) {
        throw std::runtime_error("Arena: invalid or double free");
    }

    i->second.setInUse(false);

    if (i != chunks.begin()) { // Absorb into previous chunk if exists and free
        auto prev = i;
        --prev;
        if (!prev->second.isInUse()) {
            // Absorb current chunk size into previous chunk.
            prev->second.setSize(prev->second.getSize() + i->second.getSize());
            // Erase current chunk. Erasing does not invalidate current
            // iterators for a map, except for that pointing to the object
            // itself, which will be overwritten in the next statement.
            chunks.erase(i);
            // From here on, the previous chunk is our current chunk.
            i = prev;
        }
    }
    auto next = i;
    ++next;
    if (next != chunks.end()) { // Absorb next chunk if exists and free
        if (!next->second.isInUse()) {
            // Absurb next chunk size into current chunk
            i->second.setSize(i->second.getSize() + next->second.getSize());
            // Erase next chunk.
            chunks.erase(next);
        }
    }
}

Arena::Stats Arena::stats() const
{
    Arena::Stats r;
    r.used = r.free = r.total = r.chunks_used = r.chunks_free = 0;
    for (const auto& chunk: chunks) {
        if (chunk.second.isInUse()) {
            r.used += chunk.second.getSize();
            r.chunks_used += 1;
        } else {
            r.free += chunk.second.getSize();
            r.chunks_free += 1;
        }
        r.total += chunk.second.getSize();
    }
    return r;
}

#ifdef ARENA_DEBUG
void Arena::walk() const
{
    for (const auto& chunk: chunks) {
        std::cout <<
            "0x" << std::hex << std::setw(16) << std::setfill('0') << chunk.first <<
            " 0x" << std::hex << std::setw(16) << std::setfill('0') << chunk.second.getSize() <<
            " 0x" << chunk.second.isInUse() << std::endl;
    }
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
    void* AllocateLocked(size_t len, bool *lockingSuccess);
    void FreeLocked(void* addr, size_t len);
    size_t GetLimit();
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
    // TODO is there a limit on windows, how to get it?
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
    void* AllocateLocked(size_t len, bool *lockingSuccess);
    void FreeLocked(void* addr, size_t len);
    size_t GetLimit();
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
    if (addr) {
        *lockingSuccess = mlock(addr, len) == 0;
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
    LockedPool::Stats r;
    r.used = r.free = r.total = r.chunks_used = r.chunks_free = 0;
    r.locked = cumulative_bytes_locked;
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
LockedPoolManager::LockedPoolManager(std::unique_ptr<LockedPageAllocator> allocator):
    LockedPool(std::move(allocator), &LockedPoolManager::LockingFailed)
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

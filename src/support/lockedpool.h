// Copyright (c) 2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SUPPORT_LOCKEDPOOL_H
#define BITCOIN_SUPPORT_LOCKEDPOOL_H

#include <stdint.h>
#include <list>
#include <map>
#include <mutex>
#include <memory>

/**
 * OS-dependent allocation and deallocation of locked/pinned memory pages.
 * Abstract base class.
 */
class LockedPageAllocator
{
public:
    virtual ~LockedPageAllocator() {}
    /** Allocate and lock memory pages.
     * If len is not a multiple of the system page size, it is rounded up.
     * Returns 0 in case of allocation failure.
     *
     * If locking the memory pages could not be accomplished it will still
     * return the memory, however the lockingSuccess flag will be false.
     * lockingSuccess is undefined if the allocation fails.
     */
    virtual void* AllocateLocked(size_t len, bool *lockingSuccess) = 0;

    /** Unlock and free memory pages.
     * Clear the memory before unlocking.
     */
    virtual void FreeLocked(void* addr, size_t len) = 0;

    /** Get the total limit on the amount of memory that may be locked by this
     * process, in bytes. Return size_t max if there is no limit or the limit
     * is unknown. Return 0 if no memory can be locked at all.
     */
    virtual size_t GetLimit() = 0;
};

/* An arena manages a contiguous region of memory by dividing it into
 * chunks.
 */
class Arena
{
public:
    Arena(void *base, size_t size, size_t alignment);
    virtual ~Arena();

    /** Memory statistics. */
    struct Stats
    {
        size_t used;
        size_t free;
        size_t total;
        size_t chunks_used;
        size_t chunks_free;
    };

    /** Allocate size bytes from this arena.
     * Returns pointer on success, or 0 if memory is full or
     * the application tried to allocate 0 bytes.
     */
    void* alloc(size_t size);

    /** Free a previously allocated chunk of memory.
     * Freeing the zero pointer has no effect.
     * Raises std::runtime_error in case of error.
     */
    void free(void *ptr);

    /** Get arena usage statistics */
    Stats stats() const;

#ifdef ARENA_DEBUG
    void walk() const;
#endif

    /** Return whether a pointer points inside this arena.
     * This returns base <= ptr < (base+size) so only use it for (inclusive)
     * chunk starting addresses.
     */
    bool addressInArena(void *ptr) const { return ptr >= base && ptr < end; }
private:
    Arena(const Arena& other) = delete; // non construction-copyable
    Arena& operator=(const Arena&) = delete; // non copyable

    /** Map of chunk address to chunk information. This class makes use of the
     * sorted order to merge previous and next chunks during deallocation.
     */
    std::map<char*, size_t> chunks_free;
    std::map<char*, size_t> chunks_used;
    /** Base address of arena */
    char* base;
    /** End address of arena */
    char* end;
    /** Minimum chunk alignment */
    size_t alignment;
};

/** Pool for locked memory chunks.
 *
 * To avoid sensitive key data from being swapped to disk, the memory in this pool
 * is locked/pinned.
 *
 * An arena manages a contiguous region of memory. The pool starts out with one arena
 * but can grow to multiple arenas if the need arises.
 *
 * Unlike a normal C heap, the administrative structures are seperate from the managed
 * memory. This has been done as the sizes and bases of objects are not in themselves sensitive
 * information, as to conserve precious locked memory. In some operating systems
 * the amount of memory that can be locked is small.
 */
class LockedPool
{
public:
    /** Size of one arena of locked memory. This is a compromise.
     * Do not set this too low, as managing many arenas will increase
     * allocation and deallocation overhead. Setting it too high allocates
     * more locked memory from the OS than strictly necessary.
     */
    static const size_t ARENA_SIZE = 256*1024;
    /** Chunk alignment. Another compromise. Setting this too high will waste
     * memory, setting it too low will facilitate fragmentation.
     */
    static const size_t ARENA_ALIGN = 16;

    /** Callback when allocation succeeds but locking fails.
     */
    typedef bool (*LockingFailed_Callback)();

    /** Memory statistics. */
    struct Stats
    {
        size_t used;
        size_t free;
        size_t total;
        size_t locked;
        size_t chunks_used;
        size_t chunks_free;
    };

    /** Create a new LockedPool. This takes ownership of the MemoryPageLocker,
     * you can only instantiate this with LockedPool(std::move(...)).
     *
     * The second argument is an optional callback when locking a newly allocated arena failed.
     * If this callback is provided and returns false, the allocation fails (hard fail), if
     * it returns true the allocation proceeds, but it could warn.
     */
    LockedPool(std::unique_ptr<LockedPageAllocator> allocator, LockingFailed_Callback lf_cb_in = 0);
    ~LockedPool();

    /** Allocate size bytes from this arena.
     * Returns pointer on success, or 0 if memory is full or
     * the application tried to allocate 0 bytes.
     */
    void* alloc(size_t size);

    /** Free a previously allocated chunk of memory.
     * Freeing the zero pointer has no effect.
     * Raises std::runtime_error in case of error.
     */
    void free(void *ptr);

    /** Get pool usage statistics */
    Stats stats() const;
private:
    LockedPool(const LockedPool& other) = delete; // non construction-copyable
    LockedPool& operator=(const LockedPool&) = delete; // non copyable

    std::unique_ptr<LockedPageAllocator> allocator;

    /** Create an arena from locked pages */
    class LockedPageArena: public Arena
    {
    public:
        LockedPageArena(LockedPageAllocator *alloc_in, void *base_in, size_t size, size_t align);
        ~LockedPageArena();
    private:
        void *base;
        size_t size;
        LockedPageAllocator *allocator;
    };

    bool new_arena(size_t size, size_t align);

    std::list<LockedPageArena> arenas;
    LockingFailed_Callback lf_cb;
    size_t cumulative_bytes_locked;
    /** Mutex protects access to this pool's data structures, including arenas.
     */
    mutable std::mutex mutex;
};

/**
 * Singleton class to keep track of locked (ie, non-swappable) memory, for use in
 * std::allocator templates.
 *
 * Some implementations of the STL allocate memory in some constructors (i.e., see
 * MSVC's vector<T> implementation where it allocates 1 byte of memory in the allocator.)
 * Due to the unpredictable order of static initializers, we have to make sure the
 * LockedPoolManager instance exists before any other STL-based objects that use
 * secure_allocator are created. So instead of having LockedPoolManager also be
 * static-initialized, it is created on demand.
 */
class LockedPoolManager : public LockedPool
{
public:
    /** Return the current instance, or create it once */
    static LockedPoolManager& Instance()
    {
        std::call_once(LockedPoolManager::init_flag, LockedPoolManager::CreateInstance);
        return *LockedPoolManager::_instance;
    }

private:
    LockedPoolManager(std::unique_ptr<LockedPageAllocator> allocator);

    /** Create a new LockedPoolManager specialized to the OS */
    static void CreateInstance();
    /** Called when locking fails, warn the user here */
    static bool LockingFailed();

    static LockedPoolManager* _instance;
    static std::once_flag init_flag;
};

#endif // BITCOIN_SUPPORT_LOCKEDPOOL_H

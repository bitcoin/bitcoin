#ifndef BOOST_THREAD_DETAIL_THREAD_SAFETY_HPP
#define BOOST_THREAD_DETAIL_THREAD_SAFETY_HPP

#if defined(__GNUC__) && !defined(__GXX_EXPERIMENTAL_CXX0X__)
//
// This is horrible, but it seems to be the only we can shut up the
// "anonymous variadic macros were introduced in C99 [-Wvariadic-macros]"
// warning that get spewed out otherwise in non-C++11 mode.
//
#pragma GCC system_header
#endif

// See https://clang.llvm.org/docs/ThreadSafetyAnalysis.html

// Un-comment to enable Thread Safety Analysis
//#define BOOST_THREAD_ENABLE_THREAD_SAFETY_ANALYSIS

// Enable thread safety attributes only with clang.
// The attributes can be safely erased when compiling with other compilers.
#if defined (BOOST_THREAD_ENABLE_THREAD_SAFETY_ANALYSIS) && defined(__clang__) && (!defined(SWIG))
#define BOOST_THREAD_ANNOTATION_ATTRIBUTE__(x)   __attribute__((x))
#else
#define BOOST_THREAD_ANNOTATION_ATTRIBUTE__(x)   // no-op
#endif

#define BOOST_THREAD_CAPABILITY(x) \
  BOOST_THREAD_ANNOTATION_ATTRIBUTE__(capability(x))

#define BOOST_THREAD_SCOPED_CAPABILITY \
  BOOST_THREAD_ANNOTATION_ATTRIBUTE__(scoped_lockable)

#define BOOST_THREAD_GUARDED_BY(x) \
  BOOST_THREAD_ANNOTATION_ATTRIBUTE__(guarded_by(x))

#define BOOST_THREAD_PT_GUARDED_BY(x) \
  BOOST_THREAD_ANNOTATION_ATTRIBUTE__(pt_guarded_by(x))

#define BOOST_THREAD_ACQUIRED_BEFORE(...) \
  BOOST_THREAD_ANNOTATION_ATTRIBUTE__(acquired_before(__VA_ARGS__))

#define BOOST_THREAD_ACQUIRED_AFTER(...) \
  BOOST_THREAD_ANNOTATION_ATTRIBUTE__(acquired_after(__VA_ARGS__))

#define BOOST_THREAD_REQUIRES(...) \
  BOOST_THREAD_ANNOTATION_ATTRIBUTE__(requires_capability(__VA_ARGS__))

#define BOOST_THREAD_REQUIRES_SHARED(...) \
  BOOST_THREAD_ANNOTATION_ATTRIBUTE__(requires_shared_capability(__VA_ARGS__))

#define BOOST_THREAD_ACQUIRE(...) \
  BOOST_THREAD_ANNOTATION_ATTRIBUTE__(acquire_capability(__VA_ARGS__))

#define BOOST_THREAD_ACQUIRE_SHARED(...) \
  BOOST_THREAD_ANNOTATION_ATTRIBUTE__(acquire_shared_capability(__VA_ARGS__))

#define BOOST_THREAD_RELEASE(...) \
  BOOST_THREAD_ANNOTATION_ATTRIBUTE__(release_capability(__VA_ARGS__))

#define BOOST_THREAD_RELEASE_SHARED(...) \
  BOOST_THREAD_ANNOTATION_ATTRIBUTE__(release_shared_capability(__VA_ARGS__))

#define BOOST_THREAD_TRY_ACQUIRE(...) \
  BOOST_THREAD_ANNOTATION_ATTRIBUTE__(try_acquire_capability(__VA_ARGS__))

#define BOOST_THREAD_TRY_ACQUIRE_SHARED(...) \
  BOOST_THREAD_ANNOTATION_ATTRIBUTE__(try_acquire_shared_capability(__VA_ARGS__))

#define BOOST_THREAD_EXCLUDES(...) \
  BOOST_THREAD_ANNOTATION_ATTRIBUTE__(locks_excluded(__VA_ARGS__))

#define BOOST_THREAD_ASSERT_CAPABILITY(x) \
  BOOST_THREAD_ANNOTATION_ATTRIBUTE__(assert_capability(x))

#define BOOST_THREAD_ASSERT_SHARED_CAPABILITY(x) \
  BOOST_THREAD_ANNOTATION_ATTRIBUTE__(assert_shared_capability(x))

#define BOOST_THREAD_RETURN_CAPABILITY(x) \
  BOOST_THREAD_ANNOTATION_ATTRIBUTE__(lock_returned(x))

#define BOOST_THREAD_NO_THREAD_SAFETY_ANALYSIS \
  BOOST_THREAD_ANNOTATION_ATTRIBUTE__(no_thread_safety_analysis)

#if defined(__clang__) && (!defined(SWIG)) && defined(__FreeBSD__)
#if __has_attribute(no_thread_safety_analysis)
#define BOOST_THREAD_DISABLE_THREAD_SAFETY_ANALYSIS __attribute__((no_thread_safety_analysis))
#else
#define BOOST_THREAD_DISABLE_THREAD_SAFETY_ANALYSIS
#endif
#else
#define BOOST_THREAD_DISABLE_THREAD_SAFETY_ANALYSIS
#endif

#ifdef USE_LOCK_STYLE_THREAD_SAFETY_ATTRIBUTES
// The original version of thread safety analysis the following attribute
// definitions.  These use a lock-based terminology.  They are still in use
// by existing thread safety code, and will continue to be supported.

// Deprecated.
#define BOOST_THREAD_PT_GUARDED_VAR \
  BOOST_THREAD_ANNOTATION_ATTRIBUTE__(pt_guarded_var)

// Deprecated.
#define BOOST_THREAD_GUARDED_VAR \
  BOOST_THREAD_ANNOTATION_ATTRIBUTE__(guarded_var)

// Replaced by REQUIRES
#define BOOST_THREAD_EXCLUSIVE_LOCKS_REQUIRED(...) \
  BOOST_THREAD_ANNOTATION_ATTRIBUTE__(exclusive_locks_required(__VA_ARGS__))

// Replaced by REQUIRES_SHARED
#define BOOST_THREAD_SHARED_LOCKS_REQUIRED(...) \
  BOOST_THREAD_ANNOTATION_ATTRIBUTE__(shared_locks_required(__VA_ARGS__))

// Replaced by CAPABILITY
#define BOOST_THREAD_LOCKABLE \
  BOOST_THREAD_ANNOTATION_ATTRIBUTE__(lockable)

// Replaced by SCOPED_CAPABILITY
#define BOOST_THREAD_SCOPED_LOCKABLE \
  BOOST_THREAD_ANNOTATION_ATTRIBUTE__(scoped_lockable)

// Replaced by ACQUIRE
#define BOOST_THREAD_EXCLUSIVE_LOCK_FUNCTION(...) \
  BOOST_THREAD_ANNOTATION_ATTRIBUTE__(exclusive_lock_function(__VA_ARGS__))

// Replaced by ACQUIRE_SHARED
#define BOOST_THREAD_SHARED_LOCK_FUNCTION(...) \
  BOOST_THREAD_ANNOTATION_ATTRIBUTE__(shared_lock_function(__VA_ARGS__))

// Replaced by RELEASE and RELEASE_SHARED
#define BOOST_THREAD_UNLOCK_FUNCTION(...) \
  BOOST_THREAD_ANNOTATION_ATTRIBUTE__(unlock_function(__VA_ARGS__))

// Replaced by TRY_ACQUIRE
#define BOOST_THREAD_EXCLUSIVE_TRYLOCK_FUNCTION(...) \
  BOOST_THREAD_ANNOTATION_ATTRIBUTE__(exclusive_trylock_function(__VA_ARGS__))

// Replaced by TRY_ACQUIRE_SHARED
#define BOOST_THREAD_SHARED_TRYLOCK_FUNCTION(...) \
  BOOST_THREAD_ANNOTATION_ATTRIBUTE__(shared_trylock_function(__VA_ARGS__))

// Replaced by ASSERT_CAPABILITY
#define BOOST_THREAD_ASSERT_EXCLUSIVE_LOCK(...) \
  BOOST_THREAD_ANNOTATION_ATTRIBUTE__(assert_exclusive_lock(__VA_ARGS__))

// Replaced by ASSERT_SHARED_CAPABILITY
#define BOOST_THREAD_ASSERT_SHARED_LOCK(...) \
  BOOST_THREAD_ANNOTATION_ATTRIBUTE__(assert_shared_lock(__VA_ARGS__))

// Replaced by EXCLUDE_CAPABILITY.
#define BOOST_THREAD_LOCKS_EXCLUDED(...) \
  BOOST_THREAD_ANNOTATION_ATTRIBUTE__(locks_excluded(__VA_ARGS__))

// Replaced by RETURN_CAPABILITY
#define BOOST_THREAD_LOCK_RETURNED(x) \
  BOOST_THREAD_ANNOTATION_ATTRIBUTE__(lock_returned(x))

#endif  // USE_LOCK_STYLE_THREAD_SAFETY_ATTRIBUTES

#endif  // BOOST_THREAD_DETAIL_THREAD_SAFETY_HPP

#pragma once

/**
	@file
	@brief scoped array and aligned array

	@author MITSUNARI Shigeo(@herumi)
*/
#include <new>
#include <utility>
#ifdef _WIN32
	#include <malloc.h>
#else
	#include <stdlib.h>
#endif
#include <cybozu/inttype.hpp>

namespace cybozu {

inline void *AlignedMalloc(size_t size, size_t alignment)
{
#ifdef _WIN32
	return _aligned_malloc(size, alignment);
#else
	void *p;
	int ret = posix_memalign(&p, alignment, size);
	return (ret == 0) ? p : 0;
#endif
}

inline void AlignedFree(void *p)
{
#ifdef _WIN32
	if (p == 0) return;
	_aligned_free(p);
#else
	free(p);
#endif
}

template<class T>
class ScopedArray {
	T *p_;
	size_t size_;
	ScopedArray(const ScopedArray&);
	void operator=(const ScopedArray&);
public:
	explicit ScopedArray(size_t size)
		: p_(new T[size])
		, size_(size)
	{
	}
	~ScopedArray()
	{
		delete[] p_;
	}
	T& operator[](size_t idx) CYBOZU_NOEXCEPT { return p_[idx]; }
	const T& operator[](size_t idx) const CYBOZU_NOEXCEPT { return p_[idx]; }
	size_t size() const CYBOZU_NOEXCEPT { return size_; }
	bool empty() const CYBOZU_NOEXCEPT { return size_ == 0; }
	T* begin() CYBOZU_NOEXCEPT { return p_; }
	T* end() CYBOZU_NOEXCEPT { return p_ + size_; }
	const T* begin() const CYBOZU_NOEXCEPT { return p_; }
	const T* end() const CYBOZU_NOEXCEPT { return p_ + size_; }
    T* data() CYBOZU_NOEXCEPT { return p_; }
    const T* data() const CYBOZU_NOEXCEPT { return p_; }
};

/**
	T must be POD type
	16byte aligment array
*/
template<class T, size_t N = 16, bool defaultDoClear = true>
class AlignedArray {
	T *p_;
	size_t size_;
	size_t allocSize_;
	T *alloc(size_t size) const
	{
		T *p = static_cast<T*>(AlignedMalloc(size * sizeof(T), N));
		if (p == 0) throw std::bad_alloc();
		return p;
	}
	void copy(T *dst, const T *src, size_t n) const
	{
		for (size_t i = 0; i < n; i++) dst[i] = src[i];
	}
	void setZero(T *p, size_t n) const
	{
		for (size_t i = 0; i < n; i++) p[i] = 0;
	}
	/*
		alloc allocN and copy [p, p + copyN) to new p_
		don't modify size_
	*/
	void allocCopy(size_t allocN, const T *p, size_t copyN)
	{
		T *q = alloc(allocN);
		copy(q, p, copyN);
		AlignedFree(p_);
		p_ = q;
		allocSize_ = allocN;
	}
public:
	/*
		don't clear buffer with zero if doClear is false
	*/
	explicit AlignedArray(size_t size = 0, bool doClear = defaultDoClear)
		: p_(0)
		, size_(0)
		, allocSize_(0)
	{
		resize(size, doClear);
	}
	AlignedArray(const AlignedArray& rhs)
		: p_(0)
		, size_(0)
		, allocSize_(0)
	{
		*this = rhs;
	}
	AlignedArray& operator=(const AlignedArray& rhs)
	{
		if (allocSize_ < rhs.size_) {
			allocCopy(rhs.size_, rhs.p_, rhs.size_);
		} else {
			copy(p_, rhs.p_, rhs.size_);
		}
		size_ = rhs.size_;
		return *this;
	}
#if (CYBOZU_CPP_VERSION >= CYBOZU_CPP_VERSION_CPP11)
	AlignedArray(AlignedArray&& rhs) CYBOZU_NOEXCEPT
		: p_(rhs.p_)
		, size_(rhs.size_)
		, allocSize_(rhs.allocSize_)
	{
		rhs.p_ = 0;
		rhs.size_ = 0;
		rhs.allocSize_ = 0;
	}
	AlignedArray& operator=(AlignedArray&& rhs) CYBOZU_NOEXCEPT
	{
		swap(rhs);
		rhs.clear();
		return *this;
	}
#endif
	/*
		don't clear buffer with zero if doClear is false
		@note don't free if shrinked
	*/
	void resize(size_t size, bool doClear = defaultDoClear)
	{
		// shrink
		if (size <= size_) {
			size_ = size;
			return;
		}
		// realloc if necessary
		if (size > allocSize_) {
			allocCopy(size, p_, size_);
		}
		if (doClear) setZero(p_ + size_, size - size_);
		size_ = size;
	}
	void clear() // not free
	{
		size_ = 0;
	}
	~AlignedArray()
	{
		AlignedFree(p_);
	}
	void swap(AlignedArray& rhs) CYBOZU_NOEXCEPT
	{
		std::swap(p_, rhs.p_);
		std::swap(size_, rhs.size_);
		std::swap(allocSize_, rhs.allocSize_);
	}
	T& operator[](size_t idx) CYBOZU_NOEXCEPT { return p_[idx]; }
	const T& operator[](size_t idx) const CYBOZU_NOEXCEPT { return p_[idx]; }
	size_t size() const CYBOZU_NOEXCEPT { return size_; }
	bool empty() const CYBOZU_NOEXCEPT { return size_ == 0; }
	T* begin() CYBOZU_NOEXCEPT { return p_; }
	T* end() CYBOZU_NOEXCEPT { return p_ + size_; }
	const T* begin() const CYBOZU_NOEXCEPT { return p_; }
	const T* end() const CYBOZU_NOEXCEPT { return p_ + size_; }
    T* data() CYBOZU_NOEXCEPT { return p_; }
    const T* data() const CYBOZU_NOEXCEPT { return p_; }
#if (CYBOZU_CPP_VERSION >= CYBOZU_CPP_VERSION_CPP11)
	const T* cbegin() const CYBOZU_NOEXCEPT { return p_; }
	const T* cend() const CYBOZU_NOEXCEPT { return p_ + size_; }
#endif
};

} // cybozu

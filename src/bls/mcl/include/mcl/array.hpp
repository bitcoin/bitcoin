#pragma once
/**
	@file
	@brief tiny vector class
	@author MITSUNARI Shigeo(@herumi)
	@license modified new BSD license
	http://opensource.org/licenses/BSD-3-Clause
*/
#include <stdlib.h>
#include <stddef.h>
#ifndef CYBOZU_DONT_USE_EXCEPTION
#include <new>
#endif

namespace mcl {

template<class T>
class Array {
	T *p_;
	size_t n_;
	template<class U>
	void swap_(U& x, U& y) const
	{
		U t;
		t = x;
		x = y;
		y = t;
	}
public:
	Array() : p_(0), n_(0) {}
	~Array()
	{
		free(p_);
	}
#ifndef CYBOZU_DONT_USE_EXCEPTION
	Array(const Array& rhs)
		: p_(0)
		, n_(0)
	{
		if (rhs.n_ == 0) return;
		p_ = (T*)malloc(sizeof(T) * rhs.n_);
		if (p_ == 0) throw std::bad_alloc();
		n_ = rhs.n_;
		for (size_t i = 0; i < n_; i++) {
			p_[i] = rhs.p_[i];
		}
	}
	Array& operator=(const Array& rhs)
	{
		Array tmp(rhs);
		tmp.swap(*this);
		return *this;
	}
#endif
	bool resize(size_t n)
	{
		if (n <= n_) {
			n_ = n;
			if (n == 0) {
				free(p_);
				p_ = 0;
			}
			return true;
		}
		T *q = (T*)malloc(sizeof(T) * n);
		if (q == 0) return false;
		for (size_t i = 0; i < n_; i++) {
			q[i] = p_[i];
		}
		free(p_);
		p_ = q;
		n_ = n;
		return true;
	}
	bool copy(const Array<T>& rhs)
	{
		if (this == &rhs) return true;
		if (n_ < rhs.n_) {
			clear();
			if (!resize(rhs.n_)) return false;
		}
		for (size_t i = 0; i < rhs.n_; i++) {
			p_[i] = rhs.p_[i];
		}
		n_ = rhs.n_;
		return true;
	}
	void clear()
	{
		free(p_);
		p_ = 0;
		n_ = 0;
	}
	size_t size() const { return n_; }
	void swap(Array<T>& rhs)
	{
		swap_(p_, rhs.p_);
		swap_(n_, rhs.n_);
	}
	T& operator[](size_t n) { return p_[n]; }
	const T& operator[](size_t n) const { return p_[n]; }
	T* data() { return p_; }
	const T* data() const { return p_; }
};

template<class T, size_t maxSize>
class FixedArray {
	T p_[maxSize];
	size_t n_;
	FixedArray(const FixedArray&);
	void operator=(const FixedArray&);
	template<class U>
	void swap_(U& x, U& y) const
	{
		U t;
		t = x;
		x = y;
		y = t;
	}
public:
	typedef T value_type;
	FixedArray() : n_(0) {}
	bool resize(size_t n)
	{
		if (n > maxSize) return false;
		n_ = n;
		return true;
	}
	void push(bool *pb, const T& x)
	{
		if (n_ == maxSize) {
			*pb = false;
			return;
		}
		p_[n_++] = x;
		*pb = true;
	}
	bool copy(const FixedArray<T, maxSize>& rhs)
	{
		if (this == &rhs) return true;
		for (size_t i = 0; i < rhs.n_; i++) {
			p_[i] = rhs.p_[i];
		}
		n_ = rhs.n_;
		return true;
	}
	void clear()
	{
		n_ = 0;
	}
	size_t size() const { return n_; }
	void swap(FixedArray<T, maxSize>& rhs)
	{
		T *minP = p_;
		size_t minN = n_;
		T *maxP = rhs.p_;
		size_t maxN = rhs.n_;
		if (minP > maxP) {
			swap_(minP, maxP);
			swap_(minN, maxN);
		}
		for (size_t i = 0; i < minN; i++) {
			swap_(minP[i], maxP[i]);
		}
		for (size_t i = minN; i < maxN; i++) {
			minP[i] = maxP[i];
		}
		swap_(n_, rhs.n_);
	}
	T& operator[](size_t n) { return p_[n]; }
	const T& operator[](size_t n) const { return p_[n]; }
	T* data() { return p_; }
	const T* data() const { return p_; }
};

} // mcl


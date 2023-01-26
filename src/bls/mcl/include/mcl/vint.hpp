#pragma once
/**
	emulate mpz_class
*/
#ifndef CYBOZU_DONT_USE_EXCEPTION
#include <cybozu/exception.hpp>
#endif
#include <cybozu/bit_operation.hpp>
#include <cybozu/xorshift.hpp>
#include <assert.h>
#ifndef CYBOZU_DONT_USE_STRING
#include <iostream>
#endif
#include <mcl/config.hpp>
#include <mcl/array.hpp>
#include <mcl/util.hpp>
#include <mcl/randgen.hpp>
#include <mcl/conversion.hpp>

#if defined(__EMSCRIPTEN__) || defined(__wasm__)
	#define MCL_VINT_64BIT_PORTABLE
	#define MCL_VINT_FIXED_BUFFER
#endif
#ifndef MCL_MAX_BIT_SIZE
	#error "define MCL_MAX_BIT_SZIE"
#endif

namespace mcl {

namespace vint {

typedef fp::Unit Unit;

template<size_t x>
struct RoundUp {
	static const size_t UnitBitSize = sizeof(Unit) * 8;
	static const size_t N = (x + UnitBitSize - 1) / UnitBitSize;
	static const size_t bit = N * UnitBitSize;
};

template<class T>
void dump(const T *x, size_t n, const char *msg = "")
{
	const size_t is4byteUnit = sizeof(*x) == 4;
	if (msg) printf("%s ", msg);
	for (size_t i = 0; i < n; i++) {
		if (is4byteUnit) {
			printf("%08x", (uint32_t)x[n - 1 - i]);
		} else {
			printf("%016llx", (unsigned long long)x[n - 1 - i]);
		}
	}
	printf("\n");
}

inline uint64_t make64(uint32_t H, uint32_t L)
{
	return ((uint64_t)H << 32) | L;
}

inline void split64(uint32_t *H, uint32_t *L, uint64_t x)
{
	*H = uint32_t(x >> 32);
	*L = uint32_t(x);
}

/*
	[H:L] <= x * y
	@return L
*/
inline uint32_t mulUnit(uint32_t *pH, uint32_t x, uint32_t y)
{
	uint64_t t = uint64_t(x) * y;
	uint32_t L;
	split64(pH, &L, t);
	return L;
}
#if MCL_SIZEOF_UNIT == 8
inline uint64_t mulUnit(uint64_t *pH, uint64_t x, uint64_t y)
{
#ifdef MCL_VINT_64BIT_PORTABLE
	const uint64_t mask = 0xffffffff;
	uint64_t v = (x & mask) * (y & mask);
	uint64_t L = uint32_t(v);
	uint64_t H = v >> 32;
	uint64_t ad = (x & mask) * uint32_t(y >> 32);
	uint64_t bc = uint32_t(x >> 32) * (y & mask);
	H += uint32_t(ad);
	H += uint32_t(bc);
	L |= H << 32;
	H >>= 32;
	H += ad >> 32;
	H += bc >> 32;
	H += (x >> 32) * (y >> 32);
	*pH = H;
	return L;
#elif defined(_WIN64) && !defined(__INTEL_COMPILER)
	return _umul128(x, y, pH);
#else
	typedef __attribute__((mode(TI))) unsigned int uint128;
	uint128 t = uint128(x) * y;
	*pH = uint64_t(t >> 64);
	return uint64_t(t);
#endif
}
#endif

template<class T>
void divNM(T *q, size_t qn, T *r, const T *x, size_t xn, const T *y, size_t yn);

/*
	q = [H:L] / y
	r = [H:L] % y
	return q
*/
inline uint32_t divUnit(uint32_t *pr, uint32_t H, uint32_t L, uint32_t y)
{
	assert(y != 0);
	uint64_t t = make64(H, L);
	uint32_t q = uint32_t(t / y);
	*pr = uint32_t(t % y);
	return q;
}
#if MCL_SIZEOF_UNIT == 8
inline uint64_t divUnit(uint64_t *pr, uint64_t H, uint64_t L, uint64_t y)
{
	assert(y != 0);
#if defined(MCL_VINT_64BIT_PORTABLE) || (defined(_MSC_VER) && _MSC_VER < 1920)
	uint32_t px[4] = { uint32_t(L), uint32_t(L >> 32), uint32_t(H), uint32_t(H >> 32) };
	uint32_t py[2] = { uint32_t(y), uint32_t(y >> 32) };
	size_t xn = 4;
	size_t yn = 2;
	uint32_t q[4];
	uint32_t r[2];
	size_t qn = xn - yn + 1;
	divNM(q, qn, r, px, xn, py, yn);
	*pr = make64(r[1], r[0]);
	return make64(q[1], q[0]);
#elif defined(_MSC_VER)
	return _udiv128(H, L, y, pr);
#else
	typedef __attribute__((mode(TI))) unsigned int uint128;
	uint128 t = (uint128(H) << 64) | L;
	uint64_t q = uint64_t(t / y);
	*pr = uint64_t(t % y);
	return q;
#endif
}
#endif

/*
	compare x[] and y[]
	@retval positive  if x > y
	@retval 0         if x == y
	@retval negative  if x < y
*/
template<class T>
int compareNM(const T *x, size_t xn, const T *y, size_t yn)
{
	assert(xn > 0 && yn > 0);
	if (xn != yn) return xn > yn ? 1 : -1;
	for (int i = (int)xn - 1; i >= 0; i--) {
		if (x[i] != y[i]) return x[i] > y[i] ? 1 : -1;
	}
	return 0;
}

template<class T>
void clearN(T *x, size_t n)
{
	for (size_t i = 0; i < n; i++) x[i] = 0;
}

template<class T>
void copyN(T *y, const T *x, size_t n)
{
	for (size_t i = 0; i < n; i++) y[i] = x[i];
}

/*
	z[] = x[n] + y[n]
	@note return 1 if having carry
	z may be equal to x or y
*/
template<class T>
T addN(T *z, const T *x, const T *y, size_t n)
{
	T c = 0;
	for (size_t i = 0; i < n; i++) {
		T xc = x[i] + c;
		c = xc < c;
		T yi = y[i];
		xc += yi;
		c += xc < yi;
		z[i] = xc;
	}
	return c;
}

/*
	z[] = x[] + y
*/
template<class T>
T addu1(T *z, const T *x, size_t n, T y)
{
	assert(n > 0);
	T t = x[0] + y;
	z[0] = t;
	size_t i = 0;
	if (t >= y) goto EXIT_0;
	i = 1;
	for (; i < n; i++) {
		t = x[i] + 1;
		z[i] = t;
		if (t != 0) goto EXIT_0;
	}
	return 1;
EXIT_0:
	i++;
	for (; i < n; i++) {
		z[i] = x[i];
	}
	return 0;
}

/*
	x[] += y
*/
template<class T>
T addu1(T *x, size_t n, T y)
{
	assert(n > 0);
	T t = x[0] + y;
	x[0] = t;
	size_t i = 0;
	if (t >= y) return 0;
	i = 1;
	for (; i < n; i++) {
		t = x[i] + 1;
		x[i] = t;
		if (t != 0) return 0;
	}
	return 1;
}
/*
	z[zn] = x[xn] + y[yn]
	@note zn = max(xn, yn)
*/
template<class T>
T addNM(T *z, const T *x, size_t xn, const T *y, size_t yn)
{
	if (yn > xn) {
		fp::swap_(xn, yn);
		fp::swap_(x, y);
	}
	assert(xn >= yn);
	size_t max = xn;
	size_t min = yn;
	T c = vint::addN(z, x, y, min);
	if (max > min) {
		c = vint::addu1(z + min, x + min, max - min, c);
	}
	return c;
}

/*
	z[] = x[n] - y[n]
	z may be equal to x or y
*/
template<class T>
T subN(T *z, const T *x, const T *y, size_t n)
{
	assert(n > 0);
	T c = 0;
	for (size_t i = 0; i < n; i++) {
		T yi = y[i];
		yi += c;
		c = yi < c;
		T xi = x[i];
		c += xi < yi;
		z[i] = xi - yi;
	}
	return c;
}

/*
	out[] = x[n] - y
*/
template<class T>
T subu1(T *z, const T *x, size_t n, T y)
{
	assert(n > 0);
#if 0
	T t = x[0];
	z[0] = t - y;
	size_t i = 0;
	if (t >= y) goto EXIT_0;
	i = 1;
	for (; i < n; i++ ){
		t = x[i];
		z[i] = t - 1;
		if (t != 0) goto EXIT_0;
	}
	return 1;
EXIT_0:
	i++;
	for (; i < n; i++) {
		z[i] = x[i];
	}
	return 0;
#else
	T c = x[0] < y ? 1 : 0;
	z[0] = x[0] - y;
	for (size_t i = 1; i < n; i++) {
		if (x[i] < c) {
			z[i] = T(-1);
		} else {
			z[i] = x[i] - c;
			c = 0;
		}
	}
	return c;
#endif
}

/*
	z[xn] = x[xn] - y[yn]
	@note xn >= yn
*/
template<class T>
T subNM(T *z, const T *x, size_t xn, const T *y, size_t yn)
{
	assert(xn >= yn);
	T c = vint::subN(z, x, y, yn);
	if (xn > yn) {
		c = vint::subu1(z + yn, x + yn, xn - yn, c);
	}
	return c;
}

/*
	z[0..n) = x[0..n) * y
	return z[n]
	@note accept z == x
*/
template<class T>
T mulu1(T *z, const T *x, size_t n, T y)
{
	assert(n > 0);
	T H = 0;
	for (size_t i = 0; i < n; i++) {
		T t = H;
		T L = mulUnit(&H, x[i], y);
		z[i] = t + L;
		if (z[i] < t) {
			H++;
		}
	}
	return H; // z[n]
}

/*
	z[xn * yn] = x[xn] * y[ym]
*/
template<class T>
static inline void mulNM(T *z, const T *x, size_t xn, const T *y, size_t yn)
{
	assert(xn > 0 && yn > 0);
	if (yn > xn) {
		fp::swap_(yn, xn);
		fp::swap_(x, y);
	}
	assert(xn >= yn);
	if (z == x) {
		T *p = (T*)CYBOZU_ALLOCA(sizeof(T) * xn);
		copyN(p, x, xn);
		x = p;
	}
	if (z == y) {
		T *p = (T*)CYBOZU_ALLOCA(sizeof(T) * yn);
		copyN(p, y, yn);
		y = p;
	}
	z[xn] = vint::mulu1(&z[0], x, xn, y[0]);
	clearN(z + xn + 1, yn - 1);

	T *t2 = (T*)CYBOZU_ALLOCA(sizeof(T) * (xn + 1));
	for (size_t i = 1; i < yn; i++) {
		t2[xn] = vint::mulu1(&t2[0], x, xn, y[i]);
		vint::addN(&z[i], &z[i], &t2[0], xn + 1);
	}
}
/*
	out[xn * 2] = x[xn] * x[xn]
	QQQ : optimize this
*/
template<class T>
static inline void sqrN(T *y, const T *x, size_t xn)
{
	mulNM(y, x, xn, x, xn);
}

/*
	q[] = x[] / y
	@retval r = x[] % y
	accept q == x
*/
template<class T>
T divu1(T *q, const T *x, size_t n, T y)
{
	T r = 0;
	for (int i = (int)n - 1; i >= 0; i--) {
		q[i] = divUnit(&r, r, x[i], y);
	}
	return r;
}
/*
	q[] = x[] / y
	@retval r = x[] % y
*/
template<class T>
T modu1(const T *x, size_t n, T y)
{
	T r = 0;
	for (int i = (int)n - 1; i >= 0; i--) {
		divUnit(&r, r, x[i], y);
	}
	return r;
}

/*
	y[] = x[] << bit
	0 < bit < sizeof(T) * 8
	accept y == x
*/
template<class T>
T shlBit(T *y, const T *x, size_t xn, size_t bit)
{
	assert(0 < bit && bit < sizeof(T) * 8);
	assert(xn > 0);
	size_t rBit = sizeof(T) * 8 - bit;
	T keep = x[xn - 1];
	T prev = keep;
	for (size_t i = xn - 1; i > 0; i--) {
		T t = x[i - 1];
		y[i] = (prev << bit) | (t >> rBit);
		prev = t;
	}
	y[0] = prev << bit;
	return keep >> rBit;
}

/*
	y[yn] = x[xn] << bit
	yn = xn + (bit + unitBitBit - 1) / unitBitSize
	accept y == x
*/
template<class T>
void shlN(T *y, const T *x, size_t xn, size_t bit)
{
	assert(xn > 0);
	const size_t unitBitSize = sizeof(T) * 8;
	size_t q = bit / unitBitSize;
	size_t r = bit % unitBitSize;
	if (r == 0) {
		// don't use copyN(y + q, x, xn); if overlaped
		for (size_t i = 0; i < xn; i++) {
			y[q + xn - 1 - i] = x[xn - 1 - i];
		}
	} else {
		y[q + xn] = shlBit(y + q, x, xn, r);
	}
	clearN(y, q);
}

/*
	y[] = x[] >> bit
	0 < bit < sizeof(T) * 8
*/
template<class T>
void shrBit(T *y, const T *x, size_t xn, size_t bit)
{
	assert(0 < bit && bit < sizeof(T) * 8);
	assert(xn > 0);
	size_t rBit = sizeof(T) * 8 - bit;
	T prev = x[0];
	for (size_t i = 1; i < xn; i++) {
		T t = x[i];
		y[i - 1] = (prev >> bit) | (t << rBit);
		prev = t;
	}
	y[xn - 1] = prev >> bit;
}
/*
	y[yn] = x[xn] >> bit
	yn = xn - bit / unitBit
*/
template<class T>
void shrN(T *y, const T *x, size_t xn, size_t bit)
{
	assert(xn > 0);
	const size_t unitBitSize = sizeof(T) * 8;
	size_t q = bit / unitBitSize;
	size_t r = bit % unitBitSize;
	assert(xn >= q);
	if (r == 0) {
		copyN(y, x + q, xn - q);
	} else {
		shrBit(y, x + q, xn - q, r);
	}
}

template<class T>
size_t getRealSize(const T *x, size_t xn)
{
	int i = (int)xn - 1;
	for (; i > 0; i--) {
		if (x[i]) {
			return i + 1;
		}
	}
	return 1;
}

/*
	q[qn] = x[xn] / y[yn] ; qn == xn - yn + 1 if xn >= yn if q
	r[rn] = x[xn] % y[yn] ; rn = yn before getRealSize
	allow q == 0
*/
template<class T>
void divNM(T *q, size_t qn, T *r, const T *x, size_t xn, const T *y, size_t yn)
{
	assert(xn > 0 && yn > 0);
	assert(xn < yn || (q == 0 || qn == xn - yn + 1));
	assert(q != r);
	const size_t rn = yn;
	xn = getRealSize(x, xn);
	yn = getRealSize(y, yn);
	if (x == y) {
		assert(xn == yn);
	x_is_y:
		clearN(r, rn);
		if (q) {
			q[0] = 1;
			clearN(q + 1, qn - 1);
		}
		return;
	}
	if (yn > xn) {
		/*
			if y > x then q = 0 and r = x
		*/
	q_is_zero:
		copyN(r, x, xn);
		clearN(r + xn, rn - xn);
		if (q) clearN(q, qn);
		return;
	}
	if (yn == 1) {
		T t;
		if (q) {
			if (qn > xn) {
				clearN(q + xn, qn - xn);
			}
			t = divu1(q, x, xn, y[0]);
		} else {
			t = modu1(x, xn, y[0]);
		}
		r[0] = t;
		clearN(r + 1, rn - 1);
		return;
	}
	const size_t yTopBit = cybozu::bsr(y[yn - 1]);
	assert(yn >= 2);
	if (xn == yn) {
		const size_t xTopBit = cybozu::bsr(x[xn - 1]);
		if (xTopBit < yTopBit) goto q_is_zero;
		if (yTopBit == xTopBit) {
			int ret = compareNM(x, xn, y, yn);
			if (ret == 0) goto x_is_y;
			if (ret < 0) goto q_is_zero;
			if (r) {
				subN(r, x, y, yn);
			}
			if (q) {
				q[0] = 1;
				clearN(q + 1, qn - 1);
			}
			return;
		}
		assert(xTopBit > yTopBit);
		// fast reduction for larger than fullbit-3 size p
		if (yTopBit >= sizeof(T) * 8 - 4) {
			T *xx = (T*)CYBOZU_ALLOCA(sizeof(T) * xn);
			T qv = 0;
			if (yTopBit == sizeof(T) * 8 - 2) {
				copyN(xx, x, xn);
			} else {
				qv = x[xn - 1] >> (yTopBit + 1);
				mulu1(xx, y, yn, qv);
				subN(xx, x, xx, xn);
				xn = getRealSize(xx, xn);
			}
			for (;;) {
				T ret = subN(xx, xx, y, yn);
				if (ret) {
					addN(xx, xx, y, yn);
					break;
				}
				qv++;
				xn = getRealSize(xx, xn);
			}
			if (r) {
				copyN(r, xx, xn);
				clearN(r + xn, rn - xn);
			}
			if (q) {
				q[0] = qv;
				clearN(q + 1, qn - 1);
			}
			return;
		}
	}
	/*
		bitwise left shift x and y to adjust MSB of y[yn - 1] = 1
	*/
	const size_t shift = sizeof(T) * 8 - 1 - yTopBit;
	T *xx = (T*)CYBOZU_ALLOCA(sizeof(T) * (xn + 1));
	const T *yy;
	if (shift) {
		T v = shlBit(xx, x, xn, shift);
		if (v) {
			xx[xn] = v;
			xn++;
		}
		T *yBuf = (T*)CYBOZU_ALLOCA(sizeof(T) * yn);
		shlBit(yBuf, y, yn ,shift);
		yy = yBuf;
	} else {
		copyN(xx, x, xn);
		yy = y;
	}
	if (q) {
		clearN(q, qn);
	}
	assert((yy[yn - 1] >> (sizeof(T) * 8 - 1)) != 0);
	T *tt = (T*)CYBOZU_ALLOCA(sizeof(T) * (yn + 1));
	while (xn > yn) {
		size_t d = xn - yn;
		T xTop = xx[xn - 1];
		T yTop = yy[yn - 1];
		if (xTop > yTop || (compareNM(xx + d, xn - d, yy, yn) >= 0)) {
			vint::subN(xx + d, xx + d, yy, yn);
			xn = getRealSize(xx, xn);
			if (q) vint::addu1<T>(q + d, qn - d, 1);
			continue;
		}
		if (xTop == 1) {
			vint::subNM(xx + d - 1, xx + d - 1, xn - d + 1, yy, yn);
			xn = getRealSize(xx, xn);
			if (q) vint::addu1<T>(q + d - 1, qn - d + 1, 1);
			continue;
		}
		tt[yn] = vint::mulu1(tt, yy, yn, xTop);
		vint::subN(xx + d - 1, xx + d - 1, tt, yn + 1);
		xn = getRealSize(xx, xn);
		if (q) vint::addu1<T>(q + d - 1, qn - d + 1, xTop);
	}
	if (xn == yn && compareNM(xx, xn, yy, yn) >= 0) {
		subN(xx, xx, yy, yn);
		xn = getRealSize(xx, xn);
		if (q) vint::addu1<T>(q, qn, 1);
	}
	if (shift) {
		shrBit(r, xx, xn, shift);
	} else {
		copyN(r, xx, xn);
	}
	clearN(r + xn, rn - xn);
}

#ifndef MCL_VINT_FIXED_BUFFER
template<class T>
class Buffer {
	size_t allocSize_;
	T *ptr_;
public:
	typedef T Unit;
	Buffer() : allocSize_(0), ptr_(0) {}
	~Buffer()
	{
		clear();
	}
	Buffer(const Buffer& rhs)
		: allocSize_(rhs.allocSize_)
		, ptr_(0)
	{
		ptr_ = (T*)malloc(allocSize_ * sizeof(T));
		if (ptr_ == 0) throw cybozu::Exception("Buffer:malloc") << rhs.allocSize_;
		memcpy(ptr_, rhs.ptr_, allocSize_ * sizeof(T));
	}
	Buffer& operator=(const Buffer& rhs)
	{
		Buffer t(rhs);
		swap(t);
		return *this;
	}
	void swap(Buffer& rhs)
#if CYBOZU_CPP_VERSION >= CYBOZU_CPP_VERSION_CPP11
		noexcept
#endif
	{
		fp::swap_(allocSize_, rhs.allocSize_);
		fp::swap_(ptr_, rhs.ptr_);
	}
	void clear()
	{
		allocSize_ = 0;
		free(ptr_);
		ptr_ = 0;
	}

	/*
		@note extended buffer may be not cleared
	*/
	void alloc(bool *pb, size_t n)
	{
		if (n > allocSize_) {
			T *p = (T*)malloc(n * sizeof(T));
			if (p == 0) {
				*pb = false;
				return;
			}
			copyN(p, ptr_, allocSize_);
			free(ptr_);
			ptr_ = p;
			allocSize_ = n;
		}
		*pb = true;
	}
#ifndef CYBOZU_DONT_USE_EXCEPTION
	void alloc(size_t n)
	{
		bool b;
		alloc(&b, n);
		if (!b) throw cybozu::Exception("Buffer:alloc");
	}
#endif
	/*
		*this = rhs
		rhs may be destroyed
	*/
	const T& operator[](size_t n) const { return ptr_[n]; }
	T& operator[](size_t n) { return ptr_[n]; }
};
#endif

template<class T, size_t BitLen>
class FixedBuffer {
	enum {
		N = (BitLen + sizeof(T) * 8 - 1) / (sizeof(T) * 8)
	};
	size_t size_;
	T v_[N];
public:
	typedef T Unit;
	FixedBuffer()
		: size_(0)
	{
	}
	FixedBuffer(const FixedBuffer& rhs)
	{
		operator=(rhs);
	}
	FixedBuffer& operator=(const FixedBuffer& rhs)
	{
		size_ = rhs.size_;
#if defined(__GNUC__) && !defined(__EMSCRIPTEN__) && !defined(__clang__)
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
		for (size_t i = 0; i < size_; i++) {
			v_[i] = rhs.v_[i];
		}
#if defined(__GNUC__) && !defined(__EMSCRIPTEN__) && !defined(__clang__)
	#pragma GCC diagnostic pop
#endif
		return *this;
	}
	void clear() { size_ = 0; }
	void alloc(bool *pb, size_t n)
	{
		if (n > N) {
			*pb = false;
			return;
		}
		size_ = n;
		*pb = true;
	}
#ifndef CYBOZU_DONT_USE_EXCEPTION
	void alloc(size_t n)
	{
		bool b;
		alloc(&b, n);
		if (!b) throw cybozu::Exception("FixedBuffer:alloc");
	}
#endif
	void swap(FixedBuffer& rhs)
	{
		FixedBuffer *p1 = this;
		FixedBuffer *p2 = &rhs;
		if (p1->size_ < p2->size_) {
			fp::swap_(p1, p2);
		}
		assert(p1->size_ >= p2->size_);
		for (size_t i = 0; i < p2->size_; i++) {
			fp::swap_(p1->v_[i], p2->v_[i]);
		}
		for (size_t i = p2->size_; i < p1->size_; i++) {
			p2->v_[i] = p1->v_[i];
		}
		fp::swap_(p1->size_, p2->size_);
	}
	// to avoid warning of gcc
	void verify(size_t n) const
	{
		assert(n <= N);
		(void)n;
	}
	const T& operator[](size_t n) const { verify(n); return v_[n]; }
	T& operator[](size_t n) { verify(n); return v_[n]; }
};

inline void mcl_fpDbl_mod_SECP256K1(Unit *z, const Unit *x, const Unit *p)
{
	const size_t n = 32 / MCL_SIZEOF_UNIT;
#if MCL_SIZEOF_UNIT == 8
	const Unit a = (uint64_t(1) << 32) + 0x3d1;
	Unit buf[5];
	buf[4] = mulu1(buf, x + 4, 4, a); // H * a
	buf[4] += addN(buf, buf, x, 4); // t = H * a + L
	Unit x2[2];
	x2[0] = mulUnit(&x2[1], buf[4], a);
	Unit x3 = addN(buf, buf, x2, 2);
	if (x3) {
		x3 = addu1(buf + 2, buf + 2, 2, Unit(1)); // t' = H' * a + L'
		if (x3) {
			x3 = addu1(buf, buf, 4, a);
			assert(x3 == 0);
		}
	}
#else
	Unit buf[n + 2];
	// H * a = H * 0x3d1 + (H << 32)
	buf[n] = mulu1(buf, x + n, n, 0x3d1u); // H * 0x3d1
	buf[n + 1] = addN(buf + 1, buf + 1, x + n, n);
	// t = H * a + L
	Unit t = addN(buf, buf, x, n);
	addu1(buf + n, buf + n, 2, t);
	Unit x2[4];
	// x2 = buf[n:n+2] * a
	x2[2] = mulu1(x2, buf + n, 2, 0x3d1u);
	x2[3] = addN(x2 + 1, x2 + 1, buf + n, 2);
	Unit x3 = addN(buf, buf, x2, 4);
	if (x3) {
		x3 = addu1(buf + 4, buf + 4, n - 4, Unit(1));
		if (x3) {
			Unit a[2] = { 0x3d1, 1 };
			x3 = addN(buf, buf, a, 2);
			if (x3) {
				addu1(buf + 2, buf + 2, n - 2, 1u);
			}
		}
	}
#endif
	if (fp::isGreaterOrEqualArray(buf, p, n)) {
		subN(z, buf, p, n);
	} else {
		fp::copyArray(z, buf, n);
	}
}

inline void mcl_fp_mul_SECP256K1(Unit *z, const Unit *x, const Unit *y, const Unit *p)
{
	const size_t n = 32 / MCL_SIZEOF_UNIT;
	Unit xy[n * 2];
	mulNM(xy, x, n, y, n);
	mcl_fpDbl_mod_SECP256K1(z, xy, p);
}
inline void mcl_fp_sqr_SECP256K1(Unit *y, const Unit *x, const Unit *p)
{
	const size_t n = 32 / MCL_SIZEOF_UNIT;
	Unit xx[n * 2];
	sqrN(xx, x, n);
	mcl_fpDbl_mod_SECP256K1(y, xx, p);
}

} // vint

/**
	signed integer with variable length
*/
template<class _Buffer>
class VintT {
public:
	typedef _Buffer Buffer;
	typedef typename Buffer::Unit Unit;
	static const size_t unitBitSize = sizeof(Unit) * 8;
	static const int invalidVar = -2147483647 - 1; // abs(invalidVar) is not defined
private:
	Buffer buf_;
	size_t size_;
	bool isNeg_;
	void trim(size_t n)
	{
		assert(n > 0);
		int i = (int)n - 1;
		for (; i > 0; i--) {
			if (buf_[i]) {
				size_ = i + 1;
				return;
			}
		}
		size_ = 1;
		// zero
		if (buf_[0] == 0) {
			isNeg_ = false;
		}
	}
	static int ucompare(const Buffer& x, size_t xn, const Buffer& y, size_t yn)
	{
		return vint::compareNM(&x[0], xn, &y[0], yn);
	}
	static void uadd(VintT& z, const Buffer& x, size_t xn, const Buffer& y, size_t yn)
	{
		size_t zn = fp::max_(xn, yn) + 1;
		bool b;
		z.buf_.alloc(&b, zn);
		assert(b);
		if (!b) {
			z.clear();
			return;
		}
		z.buf_[zn - 1] = vint::addNM(&z.buf_[0], &x[0], xn, &y[0], yn);
		z.trim(zn);
	}
	static void uadd1(VintT& z, const Buffer& x, size_t xn, Unit y)
	{
		size_t zn = xn + 1;
		bool b;
		z.buf_.alloc(&b, zn);
		assert(b);
		if (!b) {
			z.clear();
			return;
		}
		z.buf_[zn - 1] = vint::addu1(&z.buf_[0], &x[0], xn, y);
		z.trim(zn);
	}
	static void usub1(VintT& z, const Buffer& x, size_t xn, Unit y)
	{
		size_t zn = xn;
		bool b;
		z.buf_.alloc(&b, zn);
		assert(b);
		if (!b) {
			z.clear();
			return;
		}
		Unit c = vint::subu1(&z.buf_[0], &x[0], xn, y);
		(void)c;
		assert(!c);
		z.trim(zn);
	}
	static void usub(VintT& z, const Buffer& x, size_t xn, const Buffer& y, size_t yn)
	{
		assert(xn >= yn);
		bool b;
		z.buf_.alloc(&b, xn);
		assert(b);
		if (!b) {
			z.clear();
			return;
		}
		Unit c = vint::subN(&z.buf_[0], &x[0], &y[0], yn);
		if (xn > yn) {
			c = vint::subu1(&z.buf_[yn], &x[yn], xn - yn, c);
		}
		assert(!c);
		z.trim(xn);
	}
	static void _add(VintT& z, const VintT& x, bool xNeg, const VintT& y, bool yNeg)
	{
		if ((xNeg ^ yNeg) == 0) {
			// same sign
			uadd(z, x.buf_, x.size(), y.buf_, y.size());
			z.isNeg_ = xNeg;
			return;
		}
		int r = ucompare(x.buf_, x.size(), y.buf_, y.size());
		if (r >= 0) {
			usub(z, x.buf_, x.size(), y.buf_, y.size());
			z.isNeg_ = xNeg;
		} else {
			usub(z, y.buf_, y.size(), x.buf_, x.size());
			z.isNeg_ = yNeg;
		}
	}
	static void _adds1(VintT& z, const VintT& x, int y, bool yNeg)
	{
		assert(y >= 0);
		if ((x.isNeg_ ^ yNeg) == 0) {
			// same sign
			uadd1(z, x.buf_, x.size(), y);
			z.isNeg_ = yNeg;
			return;
		}
		if (x.size() > 1 || x.buf_[0] >= (Unit)y) {
			usub1(z, x.buf_, x.size(), y);
			z.isNeg_ = x.isNeg_;
		} else {
			z = y - x.buf_[0];
			z.isNeg_ = yNeg;
		}
	}
	static void _addu1(VintT& z, const VintT& x, Unit y, bool yNeg)
	{
		if ((x.isNeg_ ^ yNeg) == 0) {
			// same sign
			uadd1(z, x.buf_, x.size(), y);
			z.isNeg_ = yNeg;
			return;
		}
		if (x.size() > 1 || x.buf_[0] >= y) {
			usub1(z, x.buf_, x.size(), y);
			z.isNeg_ = x.isNeg_;
		} else {
			z = y - x.buf_[0];
			z.isNeg_ = yNeg;
		}
	}
	/**
		@param q [out] x / y if q != 0
		@param r [out] x % y
	*/
	static void udiv(VintT* q, VintT& r, const Buffer& x, size_t xn, const Buffer& y, size_t yn)
	{
		assert(q != &r);
		if (xn < yn) {
			r.buf_ = x;
			r.trim(xn);
			if (q) q->clear();
			return;
		}
		size_t qn = xn - yn + 1;
		bool b;
		if (q) {
			q->buf_.alloc(&b, qn);
			assert(b);
			if (!b) {
				q->clear();
				r.clear();
				return;
			}
		}
		r.buf_.alloc(&b, yn);
		assert(b);
		if (!b) {
			r.clear();
			if (q) q->clear();
			return;
		}
		vint::divNM(q ? &q->buf_[0] : 0, qn, &r.buf_[0], &x[0], xn, &y[0], yn);
		if (q) {
			q->trim(qn);
		}
		r.trim(yn);
	}
	/*
		@param x [inout] x <- d
		@retval s for x = 2^s d where d is odd
	*/
	static uint32_t countTrailingZero(VintT& x)
	{
		uint32_t s = 0;
		while (x.isEven()) {
			x >>= 1;
			s++;
		}
		return s;
	}
	struct MulMod {
		const VintT *pm;
		void operator()(VintT& z, const VintT& x, const VintT& y) const
		{
			VintT::mul(z, x, y);
			z %= *pm;
		}
	};
	struct SqrMod {
		const VintT *pm;
		void operator()(VintT& y, const VintT& x) const
		{
			VintT::sqr(y, x);
			y %= *pm;
		}
	};
public:
	VintT(int x = 0)
		: size_(0)
	{
		*this = x;
	}
	VintT(Unit x)
		: size_(0)
	{
		*this = x;
	}
	VintT(const VintT& rhs)
		: buf_(rhs.buf_)
		, size_(rhs.size_)
		, isNeg_(rhs.isNeg_)
	{
	}
	VintT& operator=(int x)
	{
		assert(x != invalidVar);
		isNeg_ = x < 0;
		bool b;
		buf_.alloc(&b, 1);
		assert(b); (void)b;
		buf_[0] = fp::abs_(x);
		size_ = 1;
		return *this;
	}
	VintT& operator=(Unit x)
	{
		isNeg_ = false;
		bool b;
		buf_.alloc(&b, 1);
		assert(b); (void)b;
		buf_[0] = x;
		size_ = 1;
		return *this;
	}
	VintT& operator=(const VintT& rhs)
	{
		buf_ = rhs.buf_;
		size_ = rhs.size_;
		isNeg_ = rhs.isNeg_;
		return *this;
	}
#if CYBOZU_CPP_VERSION >= CYBOZU_CPP_VERSION_CPP11
	VintT(VintT&& rhs)
		: buf_(rhs.buf_)
		, size_(rhs.size_)
		, isNeg_(rhs.isNeg_)
	{
	}
	VintT& operator=(VintT&& rhs)
	{
		buf_ = std::move(rhs.buf_);
		size_ = rhs.size_;
		isNeg_ = rhs.isNeg_;
		return *this;
	}
#endif
	void swap(VintT& rhs)
#if CYBOZU_CPP_VERSION >= CYBOZU_CPP_VERSION_CPP11
		noexcept
#endif
	{
		fp::swap_(buf_, rhs.buf_);
		fp::swap_(size_, rhs.size_);
		fp::swap_(isNeg_, rhs.isNeg_);
	}
	void dump(const char *msg = "") const
	{
		vint::dump(&buf_[0], size_, msg);
	}
	/*
		set positive value
		@note x is treated as a little endian
	*/
	template<class S>
	void setArray(bool *pb, const S *x, size_t size)
	{
		isNeg_ = false;
		if (size == 0) {
			clear();
			*pb = true;
			return;
		}
		size_t unitSize = (sizeof(S) * size + sizeof(Unit) - 1) / sizeof(Unit);
		buf_.alloc(pb, unitSize);
		if (!*pb) return;
		bool b = fp::convertArrayAsLE(&buf_[0], unitSize, x, size);
		assert(b);
		(void)b;
		trim(unitSize);
	}
	/*
		set [0, max) randomly
	*/
	void setRand(bool *pb, const VintT& max, fp::RandGen rg = fp::RandGen())
	{
		assert(max > 0);
		if (rg.isZero()) rg = fp::RandGen::get();
		size_t n = max.size();
		buf_.alloc(pb, n);
		if (!*pb) return;
		rg.read(pb, &buf_[0], n * sizeof(buf_[0]));
		if (!*pb) return;
		trim(n);
		*this %= max;
	}
	/*
		get abs value
		buf_[0, size) = x
		buf_[size, maxSize) with zero
		@note assume little endian system
	*/
	void getArray(bool *pb, Unit *x, size_t maxSize) const
	{
		size_t n = size();
		if (n > maxSize) {
			*pb = false;
			return;
		}
		vint::copyN(x, &buf_[0], n);
		vint::clearN(x + n, maxSize - n);
		*pb = true;
	}
	void clear() { *this = 0; }
	template<class OutputStream>
	void save(bool *pb, OutputStream& os, int base = 10) const
	{
		if (isNeg_) cybozu::writeChar(pb, os, '-');
		char buf[1024];
		size_t n = mcl::fp::arrayToStr(buf, sizeof(buf), &buf_[0], size_, base, false);
		if (n == 0) {
			*pb = false;
			return;
		}
		cybozu::write(pb, os, buf + sizeof(buf) - n, n);
	}
	/*
		set buf with string terminated by '\0'
		return strlen(buf) if success else 0
	*/
	size_t getStr(char *buf, size_t bufSize, int base = 10) const
	{
		cybozu::MemoryOutputStream os(buf, bufSize);
		bool b;
		save(&b, os, base);
		const size_t n = os.getPos();
		if (!b || n == bufSize) return 0;
		buf[n] = '\0';
		return n;
	}
	/*
		return bitSize(abs(*this))
		@note return 1 if zero
	*/
	size_t getBitSize() const
	{
		if (isZero()) return 1;
		size_t n = size();
		Unit v = buf_[n - 1];
		assert(v);
		return (n - 1) * sizeof(Unit) * 8 + 1 + cybozu::bsr<Unit>(v);
	}
	// ignore sign
	bool testBit(size_t i) const
	{
		size_t q = i / unitBitSize;
		size_t r = i % unitBitSize;
		assert(q <= size());
		Unit mask = Unit(1) << r;
		return (buf_[q] & mask) != 0;
	}
	void setBit(size_t i, bool v = true)
	{
		size_t q = i / unitBitSize;
		size_t r = i % unitBitSize;
		assert(q <= size());
		bool b;
		buf_.alloc(&b, q + 1);
		assert(b);
		if (!b) {
			clear();
			return;
		}
		Unit mask = Unit(1) << r;
		if (v) {
			buf_[q] |= mask;
		} else {
			buf_[q] &= ~mask;
			trim(q + 1);
		}
	}
	/*
		@param str [in] number string
		@note "0x..."   => base = 16
		      "0b..."   => base = 2
		      otherwise => base = 10
	*/
	void setStr(bool *pb, const char *str, int base = 0)
	{
		// allow twice size of MCL_MAX_BIT_SIZE because of multiplication
		const size_t maxN = (MCL_MAX_BIT_SIZE * 2 + unitBitSize - 1) / unitBitSize;
		buf_.alloc(pb, maxN);
		if (!*pb) return;
		*pb = false;
		isNeg_ = false;
		size_t len = strlen(str);
		size_t n = fp::strToArray(&isNeg_, &buf_[0], maxN, str, len, base);
		if (n == 0) return;
		trim(n);
		*pb = true;
	}
	static int compare(const VintT& x, const VintT& y)
	{
		if (x.isNeg_ ^ y.isNeg_) {
			if (x.isZero() && y.isZero()) return 0;
			return x.isNeg_ ? -1 : 1;
		} else {
			// same sign
			int c = ucompare(x.buf_, x.size(), y.buf_, y.size());
			if (x.isNeg_) {
				return -c;
			}
			return c;
		}
	}
	static int compares1(const VintT& x, int y)
	{
		assert(y != invalidVar);
		if (x.isNeg_ ^ (y < 0)) {
			if (x.isZero() && y == 0) return 0;
			return x.isNeg_ ? -1 : 1;
		} else {
			// same sign
			Unit y0 = fp::abs_(y);
			int c = vint::compareNM(&x.buf_[0], x.size(), &y0, 1);
			if (x.isNeg_) {
				return -c;
			}
			return c;
		}
	}
	static int compareu1(const VintT& x, uint32_t y)
	{
		if (x.isNeg_) return -1;
		if (x.size() > 1) return 1;
		Unit x0 = x.buf_[0];
		return x0 > y ? 1 : x0 == y ? 0 : -1;
	}
	size_t size() const { return size_; }
	bool isZero() const { return size() == 1 && buf_[0] == 0; }
	bool isNegative() const { return !isZero() && isNeg_; }
	uint32_t getLow32bit() const { return (uint32_t)buf_[0]; }
	bool isOdd() const { return (buf_[0] & 1) == 1; }
	bool isEven() const { return !isOdd(); }
	const Unit *getUnit() const { return &buf_[0]; }
	size_t getUnitSize() const { return size_; }
	static void add(VintT& z, const VintT& x, const VintT& y)
	{
		_add(z, x, x.isNeg_, y, y.isNeg_);
	}
	static void sub(VintT& z, const VintT& x, const VintT& y)
	{
		_add(z, x, x.isNeg_, y, !y.isNeg_);
	}
	static void mul(VintT& z, const VintT& x, const VintT& y)
	{
		const size_t xn = x.size();
		const size_t yn = y.size();
		size_t zn = xn + yn;
		bool b;
		z.buf_.alloc(&b, zn);
		assert(b);
		if (!b) {
			z.clear();
			return;
		}
		vint::mulNM(&z.buf_[0], &x.buf_[0], xn, &y.buf_[0], yn);
		z.isNeg_ = x.isNeg_ ^ y.isNeg_;
		z.trim(zn);
	}
	static void sqr(VintT& y, const VintT& x)
	{
		mul(y, x, x);
	}
	static void addu1(VintT& z, const VintT& x, Unit y)
	{
		_addu1(z, x, y, false);
	}
	static void subu1(VintT& z, const VintT& x, Unit y)
	{
		_addu1(z, x, y, true);
	}
	static void mulu1(VintT& z, const VintT& x, Unit y)
	{
		size_t xn = x.size();
		size_t zn = xn + 1;
		bool b;
		z.buf_.alloc(&b, zn);
		assert(b);
		if (!b) {
			z.clear();
			return;
		}
		z.buf_[zn - 1] = vint::mulu1(&z.buf_[0], &x.buf_[0], xn, y);
		z.isNeg_ = x.isNeg_;
		z.trim(zn);
	}
	static void divu1(VintT& q, const VintT& x, Unit y)
	{
		udivModu1(&q, x, y);
	}
	static void modu1(VintT& r, const VintT& x, Unit y)
	{
		bool xNeg = x.isNeg_;
		r = divModu1(0, x, y);
		r.isNeg_ = xNeg;
	}
	static void adds1(VintT& z, const VintT& x, int y)
	{
		assert(y != invalidVar);
		_adds1(z, x, fp::abs_(y), y < 0);
	}
	static void subs1(VintT& z, const VintT& x, int y)
	{
		assert(y != invalidVar);
		_adds1(z, x, fp::abs_(y), !(y < 0));
	}
	static void muls1(VintT& z, const VintT& x, int y)
	{
		assert(y != invalidVar);
		mulu1(z, x, fp::abs_(y));
		z.isNeg_ ^= (y < 0);
	}
	/*
		@param q [out] q = x / y if q is not zero
		@param x [in]
		@param y [in] must be not zero
		return x % y
	*/
	static int divMods1(VintT *q, const VintT& x, int y)
	{
		assert(y != invalidVar);
		bool xNeg = x.isNeg_;
		bool yNeg = y < 0;
		Unit absY = fp::abs_(y);
		size_t xn = x.size();
		int r;
		if (q) {
			q->isNeg_ = xNeg ^ yNeg;
			bool b;
			q->buf_.alloc(&b, xn);
			assert(b);
			if (!b) {
				q->clear();
				return 0;
			}
			r = (int)vint::divu1(&q->buf_[0], &x.buf_[0], xn, absY);
			q->trim(xn);
		} else {
			r = (int)vint::modu1(&x.buf_[0], xn, absY);
		}
		return xNeg ? -r : r;
	}
	/*
		like C
		  13 /  5 =  2 ...  3
		  13 / -5 = -2 ...  3
		 -13 /  5 = -2 ... -3
		 -13 / -5 =  2 ... -3
	*/
	static void divMod(VintT *q, VintT& r, const VintT& x, const VintT& y)
	{
		bool xNeg = x.isNeg_;
		bool qsign = xNeg ^ y.isNeg_;
		udiv(q, r, x.buf_, x.size(), y.buf_, y.size());
		r.isNeg_ = xNeg;
		if (q) q->isNeg_ = qsign;
	}
	static void div(VintT& q, const VintT& x, const VintT& y)
	{
		VintT r;
		divMod(&q, r, x, y);
	}
	static void mod(VintT& r, const VintT& x, const VintT& y)
	{
		divMod(0, r, x, y);
	}
	static void divs1(VintT& q, const VintT& x, int y)
	{
		divMods1(&q, x, y);
	}
	static void mods1(VintT& r, const VintT& x, int y)
	{
		bool xNeg = x.isNeg_;
		r = divMods1(0, x, y);
		r.isNeg_ = xNeg;
	}
	static Unit udivModu1(VintT *q, const VintT& x, Unit y)
	{
		assert(!x.isNeg_);
		size_t xn = x.size();
		if (q) {
			bool b;
			q->buf_.alloc(&b, xn);
			assert(b);
			if (!b) {
				q->clear();
				return 0;
			}
		}
		Unit r = vint::divu1(q ? &q->buf_[0] : 0, &x.buf_[0], xn, y);
		if (q) {
			q->trim(xn);
			q->isNeg_ = false;
		}
		return r;
	}
	/*
		like Python
		 13 /  5 =  2 ...  3
		 13 / -5 = -3 ... -2
		-13 /  5 = -3 ...  2
		-13 / -5 =  2 ... -3
	*/
	static void quotRem(VintT *q, VintT& r, const VintT& x, const VintT& y)
	{
		assert(q != &r);
		VintT yy = y;
		bool yNeg = y.isNeg_;
		bool qsign = x.isNeg_ ^ yNeg;
		udiv(q, r, x.buf_, x.size(), yy.buf_, yy.size());
		r.isNeg_ = yNeg;
		if (q) q->isNeg_ = qsign;
		if (!r.isZero() && qsign) {
			if (q) {
				uadd1(*q, q->buf_, q->size(), 1);
			}
			usub(r, yy.buf_, yy.size(), r.buf_, r.size());
		}
	}
	template<class InputStream>
	void load(bool *pb, InputStream& is, int ioMode)
	{
		*pb = false;
		char buf[1024];
		size_t n = fp::local::loadWord(buf, sizeof(buf), is);
		if (n == 0) return;
		const size_t maxN = 384 / (sizeof(MCL_SIZEOF_UNIT) * 8);
		buf_.alloc(pb, maxN);
		if (!*pb) return;
		isNeg_ = false;
		n = fp::strToArray(&isNeg_, &buf_[0], maxN, buf, n, ioMode);
		if (n == 0) return;
		trim(n);
		*pb = true;
	}
	// logical left shift (copy sign)
	static void shl(VintT& y, const VintT& x, size_t shiftBit)
	{
		size_t xn = x.size();
		size_t yn = xn + (shiftBit + unitBitSize - 1) / unitBitSize;
		bool b;
		y.buf_.alloc(&b, yn);
		assert(b);
		if (!b) {
			y.clear();
			return;
		}
		vint::shlN(&y.buf_[0], &x.buf_[0], xn, shiftBit);
		y.isNeg_ = x.isNeg_;
		y.trim(yn);
	}
	// logical right shift (copy sign)
	static void shr(VintT& y, const VintT& x, size_t shiftBit)
	{
		size_t xn = x.size();
		if (xn * unitBitSize <= shiftBit) {
			y.clear();
			return;
		}
		size_t yn = xn - shiftBit / unitBitSize;
		bool b;
		y.buf_.alloc(&b, yn);
		assert(b);
		if (!b) {
			y.clear();
			return;
		}
		vint::shrN(&y.buf_[0], &x.buf_[0], xn, shiftBit);
		y.isNeg_ = x.isNeg_;
		y.trim(yn);
	}
	static void neg(VintT& y, const VintT& x)
	{
		if (&y != &x) { y = x; }
		y.isNeg_ = !x.isNeg_;
	}
	static void abs(VintT& y, const VintT& x)
	{
		if (&y != &x) { y = x; }
		y.isNeg_ = false;
	}
	static VintT abs(const VintT& x)
	{
		VintT y = x;
		abs(y, x);
		return y;
	}
	// accept only non-negative value
	static void orBit(VintT& z, const VintT& x, const VintT& y)
	{
		assert(!x.isNeg_ && !y.isNeg_);
		const VintT *px = &x, *py = &y;
		if (x.size() < y.size()) {
			fp::swap_(px, py);
		}
		size_t xn = px->size();
		size_t yn = py->size();
		assert(xn >= yn);
		bool b;
		z.buf_.alloc(&b, xn);
		assert(b);
		if (!b) {
			z.clear();
		}
		for (size_t i = 0; i < yn; i++) {
			z.buf_[i] = x.buf_[i] | y.buf_[i];
		}
		vint::copyN(&z.buf_[0] + yn, &px->buf_[0] + yn, xn - yn);
		z.trim(xn);
	}
	static void andBit(VintT& z, const VintT& x, const VintT& y)
	{
		assert(!x.isNeg_ && !y.isNeg_);
		const VintT *px = &x, *py = &y;
		if (x.size() < y.size()) {
			fp::swap_(px, py);
		}
		size_t yn = py->size();
		assert(px->size() >= yn);
		bool b;
		z.buf_.alloc(&b, yn);
		assert(b);
		if (!b) {
			z.clear();
			return;
		}
		for (size_t i = 0; i < yn; i++) {
			z.buf_[i] = x.buf_[i] & y.buf_[i];
		}
		z.trim(yn);
	}
	static void orBitu1(VintT& z, const VintT& x, Unit y)
	{
		assert(!x.isNeg_);
		z = x;
		z.buf_[0] |= y;
	}
	static void andBitu1(VintT& z, const VintT& x, Unit y)
	{
		assert(!x.isNeg_);
		bool b;
		z.buf_.alloc(&b, 1);
		assert(b); (void)b;
		z.buf_[0] = x.buf_[0] & y;
		z.size_ = 1;
		z.isNeg_ = false;
	}
	/*
		REMARK y >= 0;
	*/
	static void pow(VintT& z, const VintT& x, const VintT& y)
	{
		assert(!y.isNeg_);
		const VintT xx = x;
		z = 1;
		mcl::fp::powGeneric(z, xx, &y.buf_[0], y.size(), mul, sqr, (void (*)(VintT&, const VintT&))0);
	}
	/*
		REMARK y >= 0;
	*/
	static void pow(VintT& z, const VintT& x, int64_t y)
	{
		assert(y >= 0);
		const VintT xx = x;
		z = 1;
#if MCL_SIZEOF_UNIT == 8
		Unit ua = fp::abs_(y);
		mcl::fp::powGeneric(z, xx, &ua, 1, mul, sqr, (void (*)(VintT&, const VintT&))0);
#else
		uint64_t ua = fp::abs_(y);
		Unit u[2] = { uint32_t(ua), uint32_t(ua >> 32) };
		size_t un = u[1] ? 2 : 1;
		mcl::fp::powGeneric(z, xx, u, un, mul, sqr, (void (*)(VintT&, const VintT&))0);
#endif
	}
	/*
		z = x ^ y mod m
		REMARK y >= 0;
	*/
	static void powMod(VintT& z, const VintT& x, const VintT& y, const VintT& m)
	{
		assert(!y.isNeg_);
		VintT zz;
		MulMod mulMod;
		SqrMod sqrMod;
		mulMod.pm = &m;
		sqrMod.pm = &m;
		zz = 1;
		mcl::fp::powGeneric(zz, x, &y.buf_[0], y.size(), mulMod, sqrMod, (void (*)(VintT&, const VintT&))0);
		z.swap(zz);
	}
	/*
		inverse mod
		y = 1/x mod m
		REMARK x != 0 and m != 0;
	*/
	static void invMod(VintT& y, const VintT& x, const VintT& m)
	{
		assert(!x.isZero() && !m.isZero());
#if 0
		VintT u = x;
		VintT v = m;
		VintT x1 = 1, x2 = 0;
		VintT t;
		while (u != 1 && v != 1) {
			while (u.isEven()) {
				u >>= 1;
				if (x1.isOdd()) {
					x1 += m;
				}
				x1 >>= 1;
			}
			while (v.isEven()) {
				v >>= 1;
				if (x2.isOdd()) {
					x2 += m;
				}
				x2 >>= 1;
			}
			if (u >= v) {
				u -= v;
				x1 -= x2;
				if (x1 < 0) {
					x1 += m;
				}
			} else {
				v -= u;
				x2 -= x1;
				if (x2 < 0) {
					x2 += m;
				}
			}
		}
		if (u == 1) {
			y = x1;
		} else {
			y = x2;
		}
#else
		if (x == 1) {
			y = 1;
			return;
		}
		VintT a = 1;
		VintT t;
		VintT q;
		divMod(&q, t, m, x);
		VintT s = x;
		VintT b = -q;

		for (;;) {
			divMod(&q, s, s, t);
			if (s.isZero()) {
				if (b.isNeg_) {
					b += m;
				}
				y = b;
				return;
			}
			a -= b * q;

			divMod(&q, t, t, s);
			if (t.isZero()) {
				if (a.isNeg_) {
					a += m;
				}
				y = a;
				return;
			}
			b -= a * q;
		}
#endif
	}
	/*
		Miller-Rabin
	*/
	static bool isPrime(bool *pb, const VintT& n, int tryNum = 32)
	{
		*pb = true;
		if (n <= 1) return false;
		if (n == 2 || n == 3) return true;
		if (n.isEven()) return false;
		cybozu::XorShift rg;
		const VintT nm1 = n - 1;
		VintT d = nm1;
		uint32_t r = countTrailingZero(d);
		// n - 1 = 2^r d
		VintT a, x;
		for (int i = 0; i < tryNum; i++) {
			a.setRand(pb, n - 3, rg);
			if (!*pb) return false;
			a += 2; // a in [2, n - 2]
			powMod(x, a, d, n);
			if (x == 1 || x == nm1) {
				continue;
			}
			for (uint32_t j = 1; j < r; j++) {
				sqr(x, x);
				x %= n;
				if (x == 1) return false;
				if (x == nm1) goto NEXT_LOOP;
			}
			return false;
		NEXT_LOOP:;
		}
		return true;
	}
	bool isPrime(bool *pb, int tryNum = 32) const
	{
		return isPrime(pb, *this, tryNum);
	}
	static void gcd(VintT& z, VintT x, VintT y)
	{
		VintT t;
		for (;;) {
			if (y.isZero()) {
				z = x;
				return;
			}
			t = x;
			x = y;
			mod(y, t, y);
		}
	}
	static VintT gcd(const VintT& x, const VintT& y)
	{
		VintT z;
		gcd(z, x, y);
		return z;
	}
	static void lcm(VintT& z, const VintT& x, const VintT& y)
	{
		VintT c;
		gcd(c, x, y);
		div(c, x, c);
		mul(z, c, y);
	}
	static VintT lcm(const VintT& x, const VintT& y)
	{
		VintT z;
		lcm(z, x, y);
		return z;
	}
	/*
		 1 if m is quadratic residue modulo n (i.e., there exists an x s.t. x^2 = m mod n)
		 0 if m = 0 mod n
		-1 otherwise
		@note return legendre_symbol(m, p) for m and odd prime p
	*/
	static int jacobi(VintT m, VintT n)
	{
		assert(n.isOdd());
		if (n == 1) return 1;
		if (m < 0 || m > n) {
			quotRem(0, m, m, n); // m = m mod n
		}
		if (m.isZero()) return 0;
		if (m == 1) return 1;
		if (gcd(m, n) != 1) return 0;

		int j = 1;
		VintT t;
		goto START;
		while (m != 1) {
			if ((m.getLow32bit() % 4) == 3 && (n.getLow32bit() % 4) == 3) {
				j = -j;
			}
			mod(t, n, m);
			n = m;
			m = t;
		START:
			int s = countTrailingZero(m);
			uint32_t nmod8 = n.getLow32bit() % 8;
			if ((s % 2) && (nmod8 == 3 || nmod8 == 5)) {
				j = -j;
			}
		}
		return j;
	}
#ifndef CYBOZU_DONT_USE_STRING
	explicit VintT(const std::string& str)
		: size_(0)
	{
		setStr(str);
	}
	void getStr(std::string& s, int base = 10) const
	{
		s.clear();
		cybozu::StringOutputStream os(s);
		save(os, base);
	}
	std::string getStr(int base = 10) const
	{
		std::string s;
		getStr(s, base);
		return s;
	}
	inline friend std::ostream& operator<<(std::ostream& os, const VintT& x)
	{
		return os << x.getStr(os.flags() & std::ios_base::hex ? 16 : 10);
	}
	inline friend std::istream& operator>>(std::istream& is, VintT& x)
	{
		x.load(is);
		return is;
	}
#endif
#ifndef CYBOZU_DONT_USE_EXCEPTION
	void setStr(const std::string& str, int base = 0)
	{
		bool b;
		setStr(&b, str.c_str(), base);
		if (!b) throw cybozu::Exception("Vint:setStr") << str;
	}
	void setRand(const VintT& max, fp::RandGen rg = fp::RandGen())
	{
		bool b;
		setRand(&b, max, rg);
		if (!b) throw cybozu::Exception("Vint:setRand");
	}
	void getArray(Unit *x, size_t maxSize) const
	{
		bool b;
		getArray(&b, x, maxSize);
		if (!b) throw cybozu::Exception("Vint:getArray");
	}
	template<class InputStream>
	void load(InputStream& is, int ioMode = 0)
	{
		bool b;
		load(&b, is, ioMode);
		if (!b) throw cybozu::Exception("Vint:load");
	}
	template<class OutputStream>
	void save(OutputStream& os, int base = 10) const
	{
		bool b;
		save(&b, os, base);
		if (!b) throw cybozu::Exception("Vint:save");
	}
	static bool isPrime(const VintT& n, int tryNum = 32)
	{
		bool b;
		bool ret = isPrime(&b, n, tryNum);
		if (!b) throw cybozu::Exception("Vint:isPrime");
		return ret;
	}
	bool isPrime(int tryNum = 32) const
	{
		bool b;
		bool ret = isPrime(&b, *this, tryNum);
		if (!b) throw cybozu::Exception("Vint:isPrime");
		return ret;
	}
	template<class S>
	void setArray(const S *x, size_t size)
	{
		bool b;
		setArray(&b, x, size);
		if (!b) throw cybozu::Exception("Vint:setArray");
	}
#endif
	VintT& operator++() { adds1(*this, *this, 1); return *this; }
	VintT& operator--() { subs1(*this, *this, 1); return *this; }
	VintT operator++(int) { VintT c = *this; adds1(*this, *this, 1); return c; }
	VintT operator--(int) { VintT c = *this; subs1(*this, *this, 1); return c; }
	friend bool operator<(const VintT& x, const VintT& y) { return compare(x, y) < 0; }
	friend bool operator>=(const VintT& x, const VintT& y) { return !operator<(x, y); }
	friend bool operator>(const VintT& x, const VintT& y) { return compare(x, y) > 0; }
	friend bool operator<=(const VintT& x, const VintT& y) { return !operator>(x, y); }
	friend bool operator==(const VintT& x, const VintT& y) { return compare(x, y) == 0; }
	friend bool operator!=(const VintT& x, const VintT& y) { return !operator==(x, y); }

	friend bool operator<(const VintT& x, int y) { return compares1(x, y) < 0; }
	friend bool operator>=(const VintT& x, int y) { return !operator<(x, y); }
	friend bool operator>(const VintT& x, int y) { return compares1(x, y) > 0; }
	friend bool operator<=(const VintT& x, int y) { return !operator>(x, y); }
	friend bool operator==(const VintT& x, int y) { return compares1(x, y) == 0; }
	friend bool operator!=(const VintT& x, int y) { return !operator==(x, y); }

	friend bool operator<(const VintT& x, uint32_t y) { return compareu1(x, y) < 0; }
	friend bool operator>=(const VintT& x, uint32_t y) { return !operator<(x, y); }
	friend bool operator>(const VintT& x, uint32_t y) { return compareu1(x, y) > 0; }
	friend bool operator<=(const VintT& x, uint32_t y) { return !operator>(x, y); }
	friend bool operator==(const VintT& x, uint32_t y) { return compareu1(x, y) == 0; }
	friend bool operator!=(const VintT& x, uint32_t y) { return !operator==(x, y); }

	VintT& operator+=(const VintT& rhs) { add(*this, *this, rhs); return *this; }
	VintT& operator-=(const VintT& rhs) { sub(*this, *this, rhs); return *this; }
	VintT& operator*=(const VintT& rhs) { mul(*this, *this, rhs); return *this; }
	VintT& operator/=(const VintT& rhs) { div(*this, *this, rhs); return *this; }
	VintT& operator%=(const VintT& rhs) { mod(*this, *this, rhs); return *this; }
	VintT& operator&=(const VintT& rhs) { andBit(*this, *this, rhs); return *this; }
	VintT& operator|=(const VintT& rhs) { orBit(*this, *this, rhs); return *this; }

	VintT& operator+=(int rhs) { adds1(*this, *this, rhs); return *this; }
	VintT& operator-=(int rhs) { subs1(*this, *this, rhs); return *this; }
	VintT& operator*=(int rhs) { muls1(*this, *this, rhs); return *this; }
	VintT& operator/=(int rhs) { divs1(*this, *this, rhs); return *this; }
	VintT& operator%=(int rhs) { mods1(*this, *this, rhs); return *this; }
	VintT& operator+=(Unit rhs) { addu1(*this, *this, rhs); return *this; }
	VintT& operator-=(Unit rhs) { subu1(*this, *this, rhs); return *this; }
	VintT& operator*=(Unit rhs) { mulu1(*this, *this, rhs); return *this; }
	VintT& operator/=(Unit rhs) { divu1(*this, *this, rhs); return *this; }
	VintT& operator%=(Unit rhs) { modu1(*this, *this, rhs); return *this; }

	VintT& operator&=(Unit rhs) { andBitu1(*this, *this, rhs); return *this; }
	VintT& operator|=(Unit rhs) { orBitu1(*this, *this, rhs); return *this; }

	friend VintT operator+(const VintT& a, const VintT& b) { VintT c; add(c, a, b); return c; }
	friend VintT operator-(const VintT& a, const VintT& b) { VintT c; sub(c, a, b); return c; }
	friend VintT operator*(const VintT& a, const VintT& b) { VintT c; mul(c, a, b); return c; }
	friend VintT operator/(const VintT& a, const VintT& b) { VintT c; div(c, a, b); return c; }
	friend VintT operator%(const VintT& a, const VintT& b) { VintT c; mod(c, a, b); return c; }
	friend VintT operator&(const VintT& a, const VintT& b) { VintT c; andBit(c, a, b); return c; }
	friend VintT operator|(const VintT& a, const VintT& b) { VintT c; orBit(c, a, b); return c; }

	friend VintT operator+(const VintT& a, int b) { VintT c; adds1(c, a, b); return c; }
	friend VintT operator-(const VintT& a, int b) { VintT c; subs1(c, a, b); return c; }
	friend VintT operator*(const VintT& a, int b) { VintT c; muls1(c, a, b); return c; }
	friend VintT operator/(const VintT& a, int b) { VintT c; divs1(c, a, b); return c; }
	friend VintT operator%(const VintT& a, int b) { VintT c; mods1(c, a, b); return c; }
	friend VintT operator+(const VintT& a, Unit b) { VintT c; addu1(c, a, b); return c; }
	friend VintT operator-(const VintT& a, Unit b) { VintT c; subu1(c, a, b); return c; }
	friend VintT operator*(const VintT& a, Unit b) { VintT c; mulu1(c, a, b); return c; }
	friend VintT operator/(const VintT& a, Unit b) { VintT c; divu1(c, a, b); return c; }
	friend VintT operator%(const VintT& a, Unit b) { VintT c; modu1(c, a, b); return c; }

	friend VintT operator&(const VintT& a, Unit b) { VintT c; andBitu1(c, a, b); return c; }
	friend VintT operator|(const VintT& a, Unit b) { VintT c; orBitu1(c, a, b); return c; }

	VintT operator-() const { VintT c; neg(c, *this); return c; }
	VintT& operator<<=(size_t n) { shl(*this, *this, n); return *this; }
	VintT& operator>>=(size_t n) { shr(*this, *this, n); return *this; }
	VintT operator<<(size_t n) const { VintT c = *this; c <<= n; return c; }
	VintT operator>>(size_t n) const { VintT c = *this; c >>= n; return c; }
};

#ifdef MCL_VINT_FIXED_BUFFER
typedef VintT<vint::FixedBuffer<mcl::vint::Unit, vint::RoundUp<MCL_MAX_BIT_SIZE>::bit * 2> > Vint;
#else
typedef VintT<vint::Buffer<mcl::vint::Unit> > Vint;
#endif

} // mcl

//typedef mcl::Vint mpz_class;

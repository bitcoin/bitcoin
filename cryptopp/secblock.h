// secblock.h - written and placed in the public domain by Wei Dai

#ifndef CRYPTOPP_SECBLOCK_H
#define CRYPTOPP_SECBLOCK_H

#include "config.h"
#include "misc.h"
#include <assert.h>

#if defined(CRYPTOPP_MEMALIGN_AVAILABLE) || defined(CRYPTOPP_MM_MALLOC_AVAILABLE) || defined(QNX)
	#include <malloc.h>
#else
	#include <stdlib.h>
#endif

NAMESPACE_BEGIN(CryptoPP)

// ************** secure memory allocation ***************

template<class T>
class AllocatorBase
{
public:
	typedef T value_type;
	typedef size_t size_type;
#ifdef CRYPTOPP_MSVCRT6
	typedef ptrdiff_t difference_type;
#else
	typedef std::ptrdiff_t difference_type;
#endif
	typedef T * pointer;
	typedef const T * const_pointer;
	typedef T & reference;
	typedef const T & const_reference;

	pointer address(reference r) const {return (&r);}
	const_pointer address(const_reference r) const {return (&r); }
	void construct(pointer p, const T& val) {new (p) T(val);}
	void destroy(pointer p) {p->~T();}
	size_type max_size() const {return ~size_type(0)/sizeof(T);}	// switch to std::numeric_limits<T>::max later

protected:
	static void CheckSize(size_t n)
	{
		if (n > ~size_t(0) / sizeof(T))
			throw InvalidArgument("AllocatorBase: requested size would cause integer overflow");
	}
};

#define CRYPTOPP_INHERIT_ALLOCATOR_TYPES	\
typedef typename AllocatorBase<T>::value_type value_type;\
typedef typename AllocatorBase<T>::size_type size_type;\
typedef typename AllocatorBase<T>::difference_type difference_type;\
typedef typename AllocatorBase<T>::pointer pointer;\
typedef typename AllocatorBase<T>::const_pointer const_pointer;\
typedef typename AllocatorBase<T>::reference reference;\
typedef typename AllocatorBase<T>::const_reference const_reference;

#if defined(_MSC_VER) && (_MSC_VER < 1300)
// this pragma causes an internal compiler error if placed immediately before std::swap(a, b)
#pragma warning(push)
#pragma warning(disable: 4700)	// VC60 workaround: don't know how to get rid of this warning
#endif

template <class T, class A>
typename A::pointer StandardReallocate(A& a, T *p, typename A::size_type oldSize, typename A::size_type newSize, bool preserve)
{
	if (oldSize == newSize)
		return p;

	if (preserve)
	{
		typename A::pointer newPointer = a.allocate(newSize, NULL);
		memcpy_s(newPointer, sizeof(T)*newSize, p, sizeof(T)*STDMIN(oldSize, newSize));
		a.deallocate(p, oldSize);
		return newPointer;
	}
	else
	{
		a.deallocate(p, oldSize);
		return a.allocate(newSize, NULL);
	}
}

#if defined(_MSC_VER) && (_MSC_VER < 1300)
#pragma warning(pop)
#endif

template <class T, bool T_Align16 = false>
class AllocatorWithCleanup : public AllocatorBase<T>
{
public:
	CRYPTOPP_INHERIT_ALLOCATOR_TYPES

	pointer allocate(size_type n, const void * = NULL)
	{
		CheckSize(n);
		if (n == 0)
			return NULL;

		if (CRYPTOPP_BOOL_ALIGN16_ENABLED && T_Align16 && n*sizeof(T) >= 16)
		{
			byte *p;
		#ifdef CRYPTOPP_MM_MALLOC_AVAILABLE
			while (!(p = (byte *)_mm_malloc(sizeof(T)*n, 16)))
		#elif defined(CRYPTOPP_MEMALIGN_AVAILABLE)
			while (!(p = (byte *)memalign(16, sizeof(T)*n)))
		#elif defined(CRYPTOPP_MALLOC_ALIGNMENT_IS_16)
			while (!(p = (byte *)malloc(sizeof(T)*n)))
		#else
			while (!(p = (byte *)malloc(sizeof(T)*n + 16)))
		#endif
				CallNewHandler();

		#ifdef CRYPTOPP_NO_ALIGNED_ALLOC
			size_t adjustment = 16-((size_t)p%16);
			p += adjustment;
			p[-1] = (byte)adjustment;
		#endif

			assert(IsAlignedOn(p, 16));
			return (pointer)p;
		}

		pointer p;
		while (!(p = (pointer)malloc(sizeof(T)*n)))
			CallNewHandler();
		return p;
	}

	void deallocate(void *p, size_type n)
	{
		memset_z(p, 0, n*sizeof(T));

		if (CRYPTOPP_BOOL_ALIGN16_ENABLED && T_Align16 && n*sizeof(T) >= 16)
		{
		#ifdef CRYPTOPP_MM_MALLOC_AVAILABLE
			_mm_free(p);
		#elif defined(CRYPTOPP_NO_ALIGNED_ALLOC)
			p = (byte *)p - ((byte *)p)[-1];
			free(p);
		#else
			free(p);
		#endif
			return;
		}

		free(p);
	}

	pointer reallocate(T *p, size_type oldSize, size_type newSize, bool preserve)
	{
		return StandardReallocate(*this, p, oldSize, newSize, preserve);
	}

	// VS.NET STL enforces the policy of "All STL-compliant allocators have to provide a
	// template class member called rebind".
    template <class U> struct rebind { typedef AllocatorWithCleanup<U, T_Align16> other; };
#if _MSC_VER >= 1500
	AllocatorWithCleanup() {}
	template <class U, bool A> AllocatorWithCleanup(const AllocatorWithCleanup<U, A> &) {}
#endif
};

CRYPTOPP_DLL_TEMPLATE_CLASS AllocatorWithCleanup<byte>;
CRYPTOPP_DLL_TEMPLATE_CLASS AllocatorWithCleanup<word16>;
CRYPTOPP_DLL_TEMPLATE_CLASS AllocatorWithCleanup<word32>;
CRYPTOPP_DLL_TEMPLATE_CLASS AllocatorWithCleanup<word64>;
#if CRYPTOPP_BOOL_X86
CRYPTOPP_DLL_TEMPLATE_CLASS AllocatorWithCleanup<word, true>;	// for Integer
#endif

template <class T>
class NullAllocator : public AllocatorBase<T>
{
public:
	CRYPTOPP_INHERIT_ALLOCATOR_TYPES

	pointer allocate(size_type n, const void * = NULL)
	{
		assert(false);
		return NULL;
	}

	void deallocate(void *p, size_type n)
	{
		assert(false);
	}

	size_type max_size() const {return 0;}
};

// This allocator can't be used with standard collections because
// they require that all objects of the same allocator type are equivalent.
// So this is for use with SecBlock only.
template <class T, size_t S, class A = NullAllocator<T>, bool T_Align16 = false>
class FixedSizeAllocatorWithCleanup : public AllocatorBase<T>
{
public:
	CRYPTOPP_INHERIT_ALLOCATOR_TYPES

	FixedSizeAllocatorWithCleanup() : m_allocated(false) {}

	pointer allocate(size_type n)
	{
		assert(IsAlignedOn(m_array, 8));

		if (n <= S && !m_allocated)
		{
			m_allocated = true;
			return GetAlignedArray();
		}
		else
			return m_fallbackAllocator.allocate(n);
	}

	pointer allocate(size_type n, const void *hint)
	{
		if (n <= S && !m_allocated)
		{
			m_allocated = true;
			return GetAlignedArray();
		}
		else
			return m_fallbackAllocator.allocate(n, hint);
	}

	void deallocate(void *p, size_type n)
	{
		if (p == GetAlignedArray())
		{
			assert(n <= S);
			assert(m_allocated);
			m_allocated = false;
			memset(p, 0, n*sizeof(T));
		}
		else
			m_fallbackAllocator.deallocate(p, n);
	}

	pointer reallocate(pointer p, size_type oldSize, size_type newSize, bool preserve)
	{
		if (p == GetAlignedArray() && newSize <= S)
		{
			assert(oldSize <= S);
			if (oldSize > newSize)
				memset(p + newSize, 0, (oldSize-newSize)*sizeof(T));
			return p;
		}

		pointer newPointer = allocate(newSize, NULL);
		if (preserve)
			memcpy(newPointer, p, sizeof(T)*STDMIN(oldSize, newSize));
		deallocate(p, oldSize);
		return newPointer;
	}

	size_type max_size() const {return STDMAX(m_fallbackAllocator.max_size(), S);}

private:
#ifdef __BORLANDC__
	T* GetAlignedArray() {return m_array;}
	T m_array[S];
#else
	T* GetAlignedArray() {return (CRYPTOPP_BOOL_ALIGN16_ENABLED && T_Align16) ? (T*)(((byte *)m_array) + (0-(size_t)m_array)%16) : m_array;}
	CRYPTOPP_ALIGN_DATA(8) T m_array[(CRYPTOPP_BOOL_ALIGN16_ENABLED && T_Align16) ? S+8/sizeof(T) : S];
#endif
	A m_fallbackAllocator;
	bool m_allocated;
};

//! a block of memory allocated using A
template <class T, class A = AllocatorWithCleanup<T> >
class SecBlock
{
public:
	typedef typename A::value_type value_type;
	typedef typename A::pointer iterator;
	typedef typename A::const_pointer const_iterator;
	typedef typename A::size_type size_type;

	explicit SecBlock(size_type size=0)
		: m_size(size) {m_ptr = m_alloc.allocate(size, NULL);}
	SecBlock(const SecBlock<T, A> &t)
		: m_size(t.m_size) {m_ptr = m_alloc.allocate(m_size, NULL); memcpy_s(m_ptr, m_size*sizeof(T), t.m_ptr, m_size*sizeof(T));}
	SecBlock(const T *t, size_type len)
		: m_size(len)
	{
		m_ptr = m_alloc.allocate(len, NULL);
		if (t == NULL)
			memset_z(m_ptr, 0, len*sizeof(T));
		else
			memcpy(m_ptr, t, len*sizeof(T));
	}

	~SecBlock()
		{m_alloc.deallocate(m_ptr, m_size);}

#ifdef __BORLANDC__
	operator T *() const
		{return (T*)m_ptr;}
#else
	operator const void *() const
		{return m_ptr;}
	operator void *()
		{return m_ptr;}

	operator const T *() const
		{return m_ptr;}
	operator T *()
		{return m_ptr;}
#endif

//	T *operator +(size_type offset)
//		{return m_ptr+offset;}

//	const T *operator +(size_type offset) const
//		{return m_ptr+offset;}

//	T& operator[](size_type index)
//		{assert(index >= 0 && index < m_size); return m_ptr[index];}

//	const T& operator[](size_type index) const
//		{assert(index >= 0 && index < m_size); return m_ptr[index];}

	iterator begin()
		{return m_ptr;}
	const_iterator begin() const
		{return m_ptr;}
	iterator end()
		{return m_ptr+m_size;}
	const_iterator end() const
		{return m_ptr+m_size;}

	typename A::pointer data() {return m_ptr;}
	typename A::const_pointer data() const {return m_ptr;}

	size_type size() const {return m_size;}
	bool empty() const {return m_size == 0;}

	byte * BytePtr() {return (byte *)m_ptr;}
	const byte * BytePtr() const {return (const byte *)m_ptr;}
	size_type SizeInBytes() const {return m_size*sizeof(T);}

	//! set contents and size
	void Assign(const T *t, size_type len)
	{
		New(len);
		memcpy_s(m_ptr, m_size*sizeof(T), t, len*sizeof(T));
	}

	//! copy contents and size from another SecBlock
	void Assign(const SecBlock<T, A> &t)
	{
		New(t.m_size);
		memcpy_s(m_ptr, m_size*sizeof(T), t.m_ptr, m_size*sizeof(T));
	}

	SecBlock<T, A>& operator=(const SecBlock<T, A> &t)
	{
		Assign(t);
		return *this;
	}

	// append to this object
	SecBlock<T, A>& operator+=(const SecBlock<T, A> &t)
	{
		size_type oldSize = m_size;
		Grow(m_size+t.m_size);
		memcpy_s(m_ptr+oldSize, m_size*sizeof(T), t.m_ptr, t.m_size*sizeof(T));
		return *this;
	}

	// append operator
	SecBlock<T, A> operator+(const SecBlock<T, A> &t)
	{
		SecBlock<T, A> result(m_size+t.m_size);
		memcpy_s(result.m_ptr, result.m_size*sizeof(T), m_ptr, m_size*sizeof(T));
		memcpy_s(result.m_ptr+m_size, t.m_size*sizeof(T), t.m_ptr, t.m_size*sizeof(T));
		return result;
	}

	bool operator==(const SecBlock<T, A> &t) const
	{
		return m_size == t.m_size && VerifyBufsEqual(m_ptr, t.m_ptr, m_size*sizeof(T));
	}

	bool operator!=(const SecBlock<T, A> &t) const
	{
		return !operator==(t);
	}

	//! change size, without preserving contents
	void New(size_type newSize)
	{
		m_ptr = m_alloc.reallocate(m_ptr, m_size, newSize, false);
		m_size = newSize;
	}

	//! change size and set contents to 0
	void CleanNew(size_type newSize)
	{
		New(newSize);
		memset_z(m_ptr, 0, m_size*sizeof(T));
	}

	//! change size only if newSize > current size. contents are preserved
	void Grow(size_type newSize)
	{
		if (newSize > m_size)
		{
			m_ptr = m_alloc.reallocate(m_ptr, m_size, newSize, true);
			m_size = newSize;
		}
	}

	//! change size only if newSize > current size. contents are preserved and additional area is set to 0
	void CleanGrow(size_type newSize)
	{
		if (newSize > m_size)
		{
			m_ptr = m_alloc.reallocate(m_ptr, m_size, newSize, true);
			memset(m_ptr+m_size, 0, (newSize-m_size)*sizeof(T));
			m_size = newSize;
		}
	}

	//! change size and preserve contents
	void resize(size_type newSize)
	{
		m_ptr = m_alloc.reallocate(m_ptr, m_size, newSize, true);
		m_size = newSize;
	}

	//! swap contents and size with another SecBlock
	void swap(SecBlock<T, A> &b)
	{
		std::swap(m_alloc, b.m_alloc);
		std::swap(m_size, b.m_size);
		std::swap(m_ptr, b.m_ptr);
	}

//private:
	A m_alloc;
	size_type m_size;
	T *m_ptr;
};

typedef SecBlock<byte> SecByteBlock;
typedef SecBlock<byte, AllocatorWithCleanup<byte, true> > AlignedSecByteBlock;
typedef SecBlock<word> SecWordBlock;

//! a SecBlock with fixed size, allocated statically
template <class T, unsigned int S, class A = FixedSizeAllocatorWithCleanup<T, S> >
class FixedSizeSecBlock : public SecBlock<T, A>
{
public:
	explicit FixedSizeSecBlock() : SecBlock<T, A>(S) {}
};

template <class T, unsigned int S, bool T_Align16 = true>
class FixedSizeAlignedSecBlock : public FixedSizeSecBlock<T, S, FixedSizeAllocatorWithCleanup<T, S, NullAllocator<T>, T_Align16> >
{
};

//! a SecBlock that preallocates size S statically, and uses the heap when this size is exceeded
template <class T, unsigned int S, class A = FixedSizeAllocatorWithCleanup<T, S, AllocatorWithCleanup<T> > >
class SecBlockWithHint : public SecBlock<T, A>
{
public:
	explicit SecBlockWithHint(size_t size) : SecBlock<T, A>(size) {}
};

template<class T, bool A, class U, bool B>
inline bool operator==(const CryptoPP::AllocatorWithCleanup<T, A>&, const CryptoPP::AllocatorWithCleanup<U, B>&) {return (true);}
template<class T, bool A, class U, bool B>
inline bool operator!=(const CryptoPP::AllocatorWithCleanup<T, A>&, const CryptoPP::AllocatorWithCleanup<U, B>&) {return (false);}

NAMESPACE_END

NAMESPACE_BEGIN(std)
template <class T, class A>
inline void swap(CryptoPP::SecBlock<T, A> &a, CryptoPP::SecBlock<T, A> &b)
{
	a.swap(b);
}

#if defined(_STLP_DONT_SUPPORT_REBIND_MEMBER_TEMPLATE) || (defined(_STLPORT_VERSION) && !defined(_STLP_MEMBER_TEMPLATE_CLASSES))
// working for STLport 5.1.3 and MSVC 6 SP5
template <class _Tp1, class _Tp2>
inline CryptoPP::AllocatorWithCleanup<_Tp2>&
__stl_alloc_rebind(CryptoPP::AllocatorWithCleanup<_Tp1>& __a, const _Tp2*)
{
	return (CryptoPP::AllocatorWithCleanup<_Tp2>&)(__a);
}
#endif

NAMESPACE_END

#endif

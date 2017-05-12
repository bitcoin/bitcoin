// secblock.h - written and placed in the public domain by Wei Dai

//! \file secblock.h
//! \brief Classes and functions for secure memory allocations.

#ifndef CRYPTOPP_SECBLOCK_H
#define CRYPTOPP_SECBLOCK_H

#include "config.h"
#include "stdcpp.h"
#include "misc.h"

#if CRYPTOPP_MSC_VERSION
# pragma warning(push)
# pragma warning(disable: 4700)
# if (CRYPTOPP_MSC_VERSION >= 1400)
#  pragma warning(disable: 6386)
# endif
#endif

NAMESPACE_BEGIN(CryptoPP)

// ************** secure memory allocation ***************

//! \class AllocatorBase
//! \brief Base class for all allocators used by SecBlock
//! \tparam T the class or type
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
	void destroy(pointer p) {CRYPTOPP_UNUSED(p); p->~T();}

	//! \brief Returns the maximum number of elements the allocator can provide
	//! \returns the maximum number of elements the allocator can provide
	//! \details Internally, preprocessor macros are used rather than std::numeric_limits
	//!   because the latter is \a not a \a constexpr. Some compilers, like Clang, do not
	//!   optimize it well under all circumstances. Compilers like GCC, ICC and MSVC appear
	//!   to optimize it well in either form.
	CRYPTOPP_CONSTEXPR size_type max_size() const {return (SIZE_MAX/sizeof(T));}

#if defined(CRYPTOPP_CXX11_VARIADIC_TEMPLATES) || defined(CRYPTOPP_DOXYGEN_PROCESSING)

	//! \brief Constructs a new U using variadic arguments
	//! \tparam U the type to be forwarded
	//! \tparam Args the arguments to be forwarded
	//! \param ptr pointer to type U
	//! \param args variadic arguments
	//! \details This is a C++11 feature. It is available when CRYPTOPP_CXX11_VARIADIC_TEMPLATES
	//!   is defined. The define is controlled by compiler versions detected in config.h.
    template<typename U, typename... Args>
    void construct(U* ptr, Args&&... args) {::new ((void*)ptr) U(std::forward<Args>(args)...);}

	//! \brief Destroys an U constructed with variadic arguments
	//! \tparam U the type to be forwarded
	//! \details This is a C++11 feature. It is available when CRYPTOPP_CXX11_VARIADIC_TEMPLATES
	//!   is defined. The define is controlled by compiler versions detected in config.h.
    template<typename U>
    void destroy(U* ptr) {if(ptr) ptr->~U();}

#endif

protected:

	//! \brief Verifies the allocator can satisfy a request based on size
	//! \param size the size of the allocation, in elements
	//! \throws InvalidArgument
	//! \details CheckSize verifies the number of elements requested is valid.
	//! \details If size is greater than max_size(), then InvalidArgument is thrown.
	//!   The library throws InvalidArgument if the size is too large to satisfy.
	//! \details Internally, preprocessor macros are used rather than std::numeric_limits
	//!   because the latter is \a not a \a constexpr. Some compilers, like Clang, do not
	//!   optimize it well under all circumstances. Compilers like GCC, ICC and MSVC appear
	//!   to optimize it well in either form.
	//! \note size is the count of elements, and not the number of bytes
	static void CheckSize(size_t size)
	{
		// C++ throws std::bad_alloc (C++03) or std::bad_array_new_length (C++11) here.
		if (size > (SIZE_MAX/sizeof(T)))
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

//! \brief Reallocation function
//! \tparam T the class or type
//! \tparam A the class or type's allocator
//! \param alloc the allocator
//! \param oldPtr the previous allocation
//! \param oldSize the size of the previous allocation
//! \param newSize the new, requested size
//! \param preserve flag that indicates if the old allocation should be preserved
//! \note oldSize and newSize are the count of elements, and not the
//!   number of bytes.
template <class T, class A>
typename A::pointer StandardReallocate(A& alloc, T *oldPtr, typename A::size_type oldSize, typename A::size_type newSize, bool preserve)
{
	CRYPTOPP_ASSERT((oldPtr && oldSize) || !(oldPtr || oldSize));
	if (oldSize == newSize)
		return oldPtr;

	if (preserve)
	{
		typename A::pointer newPointer = alloc.allocate(newSize, NULL);
		const size_t copySize = STDMIN(oldSize, newSize) * sizeof(T);

		if (oldPtr && newPointer) {memcpy_s(newPointer, copySize, oldPtr, copySize);}
		alloc.deallocate(oldPtr, oldSize);
		return newPointer;
	}
	else
	{
		alloc.deallocate(oldPtr, oldSize);
		return alloc.allocate(newSize, NULL);
	}
}

//! \class AllocatorWithCleanup
//! \brief Allocates a block of memory with cleanup
//! \tparam T class or type
//! \tparam T_Align16 boolean that determines whether allocations should be aligned on 16-byte boundaries
//! \details If T_Align16 is true, then AllocatorWithCleanup calls AlignedAllocate()
//!    for memory allocations. If T_Align16 is false, then AllocatorWithCleanup() calls
//!    UnalignedAllocate() for memory allocations.
//! \details Template parameter T_Align16 is effectively controlled by cryptlib.h and mirrors
//!    CRYPTOPP_BOOL_ALIGN16. CRYPTOPP_BOOL_ALIGN16 is often used as the template parameter.
template <class T, bool T_Align16 = false>
class AllocatorWithCleanup : public AllocatorBase<T>
{
public:
	CRYPTOPP_INHERIT_ALLOCATOR_TYPES

	//! \brief Allocates a block of memory
	//! \param ptr the size of the allocation
	//! \param size the size of the allocation, in elements
	//! \returns a memory block
	//! \throws InvalidArgument
	//! \details allocate() first checks the size of the request. If it is non-0
	//!   and less than max_size(), then an attempt is made to fulfill the request using either
	//!   AlignedAllocate() or UnalignedAllocate().
	//! \details AlignedAllocate() is used if T_Align16 is true.
	//!   UnalignedAllocate() used if T_Align16 is false.
	//! \details This is the C++ *Placement New* operator. ptr is not used, and the function
	//!   CRYPTOPP_ASSERTs in Debug builds if ptr is non-NULL.
	//! \sa CallNewHandler() for the methods used to recover from a failed
	//!   allocation attempt.
	//! \note size is the count of elements, and not the number of bytes
	pointer allocate(size_type size, const void *ptr = NULL)
	{
		CRYPTOPP_UNUSED(ptr); CRYPTOPP_ASSERT(ptr == NULL);
		this->CheckSize(size);
		if (size == 0)
			return NULL;

#if CRYPTOPP_BOOL_ALIGN16
		// TODO: should this need the test 'size*sizeof(T) >= 16'?
		if (T_Align16 && size*sizeof(T) >= 16)
			return (pointer)AlignedAllocate(size*sizeof(T));
#endif

		return (pointer)UnalignedAllocate(size*sizeof(T));
	}

	//! \brief Deallocates a block of memory
	//! \param ptr the pointer for the allocation
	//! \param size the size of the allocation, in elements
	//! \details Internally, SecureWipeArray() is called before deallocating the memory.
	//!   Once the memory block is wiped or zeroized, AlignedDeallocate() or
	//!   UnalignedDeallocate() is called.
	//! \details AlignedDeallocate() is used if T_Align16 is true.
	//!   UnalignedDeallocate() used if T_Align16 is false.
	void deallocate(void *ptr, size_type size)
	{
		CRYPTOPP_ASSERT((ptr && size) || !(ptr || size));
		SecureWipeArray((pointer)ptr, size);

#if CRYPTOPP_BOOL_ALIGN16
		if (T_Align16 && size*sizeof(T) >= 16)
			return AlignedDeallocate(ptr);
#endif

		UnalignedDeallocate(ptr);
	}

	//! \brief Reallocates a block of memory
	//! \param oldPtr the previous allocation
	//! \param oldSize the size of the previous allocation
	//! \param newSize the new, requested size
	//! \param preserve flag that indicates if the old allocation should be preserved
	//! \returns pointer to the new memory block
	//! \details Internally, reallocate() calls StandardReallocate().
	//! \details If preserve is true, then index 0 is used to begin copying the
	//!   old memory block to the new one. If the block grows, then the old array
	//!   is copied in its entirety. If the block shrinks, then only newSize
	//!   elements are copied from the old block to the new one.
	//! \note oldSize and newSize are the count of elements, and not the
	//!   number of bytes.
	pointer reallocate(T *oldPtr, size_type oldSize, size_type newSize, bool preserve)
	{
		CRYPTOPP_ASSERT((oldPtr && oldSize) || !(oldPtr || oldSize));
		return StandardReallocate(*this, oldPtr, oldSize, newSize, preserve);
	}

	//! \brief Template class memeber Rebind
	//! \tparam T allocated class or type
	//! \tparam T_Align16 boolean that determines whether allocations should be aligned on 16-byte boundaries
	//! \tparam U bound class or type
	//! \details Rebind allows a container class to allocate a different type of object
	//!   to store elements. For example, a std::list will allocate std::list_node to
	//!   store elements in the list.
	//! \details VS.NET STL enforces the policy of "All STL-compliant allocators
	//!   have to provide a template class member called rebind".
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
#if defined(CRYPTOPP_WORD128_AVAILABLE)
CRYPTOPP_DLL_TEMPLATE_CLASS AllocatorWithCleanup<word128, true>; // for Integer
#endif
#if CRYPTOPP_BOOL_X86
CRYPTOPP_DLL_TEMPLATE_CLASS AllocatorWithCleanup<word, true>;	 // for Integer
#endif

//! \class NullAllocator
//! \brief NULL allocator
//! \tparam T class or type
//! \details A NullAllocator is useful for fixed-size, stack based allocations
//!   (i.e., static arrays used by FixedSizeAllocatorWithCleanup).
//! \details A NullAllocator always returns 0 for max_size(), and always returns
//!   NULL for allocation requests. Though the allocator does not allocate at
//!   runtime, it does perform a secure wipe or zeroization during cleanup.
template <class T>
class NullAllocator : public AllocatorBase<T>
{
public:
	//LCOV_EXCL_START
	CRYPTOPP_INHERIT_ALLOCATOR_TYPES

	// TODO: should this return NULL or throw bad_alloc? Non-Windows C++ standard
	// libraries always throw. And late mode Windows throws. Early model Windows
	// (circa VC++ 6.0) returned NULL.
	pointer allocate(size_type n, const void* unused = NULL)
	{
		CRYPTOPP_UNUSED(n); CRYPTOPP_UNUSED(unused);
		CRYPTOPP_ASSERT(false); return NULL;
	}

	void deallocate(void *p, size_type n)
	{
		CRYPTOPP_UNUSED(p); CRYPTOPP_UNUSED(n);
		CRYPTOPP_ASSERT(false);
	}

	CRYPTOPP_CONSTEXPR size_type max_size() const {return 0;}
	//LCOV_EXCL_STOP
};

//! \class FixedSizeAllocatorWithCleanup
//! \brief Static secure memory block with cleanup
//! \tparam T class or type
//! \tparam S fixed-size of the stack-based memory block, in elements
//! \tparam A AllocatorBase derived class for allocation and cleanup
//! \details FixedSizeAllocatorWithCleanup provides a fixed-size, stack-
//!    based allocation at compile time. The class can grow its memory
//!    block at runtime if a suitable allocator is available. If size
//!    grows beyond S and a suitable allocator is available, then the
//!    statically allocated array is obsoleted.
//! \note This allocator can't be used with standard collections because
//!   they require that all objects of the same allocator type are equivalent.
template <class T, size_t S, class A = NullAllocator<T>, bool T_Align16 = false>
class FixedSizeAllocatorWithCleanup : public AllocatorBase<T>
{
public:
	CRYPTOPP_INHERIT_ALLOCATOR_TYPES

	//! \brief Constructs a FixedSizeAllocatorWithCleanup
	FixedSizeAllocatorWithCleanup() : m_allocated(false) {}

	//! \brief Allocates a block of memory
	//! \param size size of the memory block, in elements
	//! \details FixedSizeAllocatorWithCleanup provides a fixed-size, stack-based
	//!   allocation at compile time. If size is less than or equal to
	//!   <tt>S</tt>, then a pointer to the static array is returned.
	//! \details The class can grow its memory block at runtime if a suitable
	//!   allocator is available. If size grows beyond S and a suitable
	//!   allocator is available, then the statically allocated array is
	//!   obsoleted. If a suitable allocator is \a not available, as with a
	//!   NullAllocator, then the function returns NULL and a runtime error
	//!   eventually occurs.
	//! \sa reallocate(), SecBlockWithHint
	pointer allocate(size_type size)
	{
		CRYPTOPP_ASSERT(IsAlignedOn(m_array, 8));

		if (size <= S && !m_allocated)
		{
			m_allocated = true;
			return GetAlignedArray();
		}
		else
			return m_fallbackAllocator.allocate(size);
	}

	//! \brief Allocates a block of memory
	//! \param size size of the memory block, in elements
	//! \param hint an unused hint
	//! \details FixedSizeAllocatorWithCleanup provides a fixed-size, stack-
	//!   based allocation at compile time. If size is less than or equal to
	//!   S, then a pointer to the static array is returned.
	//! \details The class can grow its memory block at runtime if a suitable
	//!   allocator is available. If size grows beyond S and a suitable
	//!   allocator is available, then the statically allocated array is
	//!   obsoleted. If a suitable allocator is \a not available, as with a
	//!   NullAllocator, then the function returns NULL and a runtime error
	//!   eventually occurs.
	//! \sa reallocate(), SecBlockWithHint
	pointer allocate(size_type size, const void *hint)
	{
		if (size <= S && !m_allocated)
		{
			m_allocated = true;
			return GetAlignedArray();
		}
		else
			return m_fallbackAllocator.allocate(size, hint);
	}

	//! \brief Deallocates a block of memory
	//! \param ptr a pointer to the memory block to deallocate
	//! \param size size of the memory block, in elements
	//! \details The memory block is wiped or zeroized before deallocation.
	//!   If the statically allocated memory block is active, then no
	//!   additional actions are taken after the wipe.
	//! \details If a dynamic memory block is active, then the pointer and
	//!   size are passed to the allocator for deallocation.
	void deallocate(void *ptr, size_type size)
	{
		if (ptr == GetAlignedArray())
		{
			CRYPTOPP_ASSERT(size <= S);
			CRYPTOPP_ASSERT(m_allocated);
			m_allocated = false;
			SecureWipeArray((pointer)ptr, size);
		}
		else
			m_fallbackAllocator.deallocate(ptr, size);
	}

	//! \brief Reallocates a block of memory
	//! \param oldPtr the previous allocation
	//! \param oldSize the size of the previous allocation
	//! \param newSize the new, requested size
	//! \param preserve flag that indicates if the old allocation should be preserved
	//! \returns pointer to the new memory block
	//! \details FixedSizeAllocatorWithCleanup provides a fixed-size, stack-
	//!   based allocation at compile time. If size is less than or equal to
	//!   S, then a pointer to the static array is returned.
	//! \details The class can grow its memory block at runtime if a suitable
	//!   allocator is available. If size grows beyond S and a suitable
	//!   allocator is available, then the statically allocated array is
	//!   obsoleted. If a suitable allocator is \a not available, as with a
	//!   NullAllocator, then the function returns NULL and a runtime error
	//!   eventually occurs.
	//! \note size is the count of elements, and not the number of bytes.
	//! \sa reallocate(), SecBlockWithHint
	pointer reallocate(pointer oldPtr, size_type oldSize, size_type newSize, bool preserve)
	{
		if (oldPtr == GetAlignedArray() && newSize <= S)
		{
			CRYPTOPP_ASSERT(oldSize <= S);
			if (oldSize > newSize)
				SecureWipeArray(oldPtr+newSize, oldSize-newSize);
			return oldPtr;
		}

		pointer newPointer = allocate(newSize, NULL);
		if (preserve && newSize)
		{
			const size_t copySize = STDMIN(oldSize, newSize);
			memcpy_s(newPointer, sizeof(T)*newSize, oldPtr, sizeof(T)*copySize);
		}
		deallocate(oldPtr, oldSize);
		return newPointer;
	}

	CRYPTOPP_CONSTEXPR size_type max_size() const {return STDMAX(m_fallbackAllocator.max_size(), S);}

private:

#ifdef __BORLANDC__
	T* GetAlignedArray() {return m_array;}
	T m_array[S];
#else
	T* GetAlignedArray() {return (CRYPTOPP_BOOL_ALIGN16 && T_Align16) ? (T*)(void *)(((byte *)m_array) + (0-(size_t)m_array)%16) : m_array;}
	CRYPTOPP_ALIGN_DATA(8) T m_array[(CRYPTOPP_BOOL_ALIGN16 && T_Align16) ? S+8/sizeof(T) : S];
#endif

	A m_fallbackAllocator;
	bool m_allocated;
};

//! \class SecBlock
//! \brief Secure memory block with allocator and cleanup
//! \tparam T a class or type
//! \tparam A AllocatorWithCleanup derived class for allocation and cleanup
template <class T, class A = AllocatorWithCleanup<T> >
class SecBlock
{
public:
	typedef typename A::value_type value_type;
	typedef typename A::pointer iterator;
	typedef typename A::const_pointer const_iterator;
	typedef typename A::size_type size_type;

	//! \brief Construct a SecBlock with space for size elements.
	//! \param size the size of the allocation, in elements
	//! \throws std::bad_alloc
	//! \details The elements are not initialized.
	//! \note size is the count of elements, and not the number of bytes
	explicit SecBlock(size_type size=0)
		: m_size(size), m_ptr(m_alloc.allocate(size, NULL)) { }

	//! \brief Copy construct a SecBlock from another SecBlock
	//! \param t the other SecBlock
	//! \throws std::bad_alloc
	SecBlock(const SecBlock<T, A> &t)
		: m_size(t.m_size), m_ptr(m_alloc.allocate(t.m_size, NULL)) {
			CRYPTOPP_ASSERT((!t.m_ptr && !m_size) || (t.m_ptr && m_size));
			if (t.m_ptr) {memcpy_s(m_ptr, m_size*sizeof(T), t.m_ptr, t.m_size*sizeof(T));}
		}

	//! \brief Construct a SecBlock from an array of elements.
	//! \param ptr a pointer to an array of T
	//! \param len the number of elements in the memory block
	//! \throws std::bad_alloc
	//! \details If <tt>ptr!=NULL</tt> and <tt>len!=0</tt>, then the block is initialized from the pointer ptr.
	//!    If <tt>ptr==NULL</tt> and <tt>len!=0</tt>, then the block is initialized to 0.
	//!    Otherwise, the block is empty and \a not initialized.
	//! \note size is the count of elements, and not the number of bytes
	SecBlock(const T *ptr, size_type len)
		: m_size(len), m_ptr(m_alloc.allocate(len, NULL)) {
			CRYPTOPP_ASSERT((!m_ptr && !m_size) || (m_ptr && m_size));
			if (ptr && m_ptr)
				memcpy_s(m_ptr, m_size*sizeof(T), ptr, len*sizeof(T));
			else if (m_size)
				memset(m_ptr, 0, m_size*sizeof(T));
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

	//! \brief Provides an iterator pointing to the first element in the memory block
	//! \returns iterator pointing to the first element in the memory block
	iterator begin()
		{return m_ptr;}
	//! \brief Provides a constant iterator pointing to the first element in the memory block
	//! \returns constant iterator pointing to the first element in the memory block
	const_iterator begin() const
		{return m_ptr;}
	//! \brief Provides an iterator pointing beyond the last element in the memory block
	//! \returns iterator pointing beyond the last element in the memory block
	iterator end()
		{return m_ptr+m_size;}
	//! \brief Provides a constant iterator pointing beyond the last element in the memory block
	//! \returns constant iterator pointing beyond the last element in the memory block
	const_iterator end() const
		{return m_ptr+m_size;}

	//! \brief Provides a pointer to the first element in the memory block
	//! \returns pointer to the first element in the memory block
	typename A::pointer data() {return m_ptr;}
	//! \brief Provides a pointer to the first element in the memory block
	//! \returns constant pointer to the first element in the memory block
	typename A::const_pointer data() const {return m_ptr;}

	//! \brief Provides the count of elements in the SecBlock
	//! \returns number of elements in the memory block
	//! \note the return value is the count of elements, and not the number of bytes
	size_type size() const {return m_size;}
	//! \brief Determines if the SecBlock is empty
	//! \returns true if number of elements in the memory block is 0, false otherwise
	bool empty() const {return m_size == 0;}

	//! \brief Provides a byte pointer to the first element in the memory block
	//! \returns byte pointer to the first element in the memory block
	byte * BytePtr() {return (byte *)m_ptr;}
	//! \brief Return a byte pointer to the first element in the memory block
	//! \returns constant byte pointer to the first element in the memory block
	const byte * BytePtr() const {return (const byte *)m_ptr;}
	//! \brief Provides the number of bytes in the SecBlock
	//! \return the number of bytes in the memory block
	//! \note the return value is the number of bytes, and not count of elements.
	size_type SizeInBytes() const {return m_size*sizeof(T);}

	//! \brief Set contents and size from an array
	//! \param ptr a pointer to an array of T
	//! \param len the number of elements in the memory block
	//! \details If the memory block is reduced in size, then the reclaimed memory is set to 0.
	void Assign(const T *ptr, size_type len)
	{
		New(len);
		if (m_ptr && ptr && len)
			{memcpy_s(m_ptr, m_size*sizeof(T), ptr, len*sizeof(T));}
	}

	//! \brief Copy contents from another SecBlock
	//! \param t the other SecBlock
	//! \details Assign checks for self assignment.
	//! \details If the memory block is reduced in size, then the reclaimed memory is set to 0.
	void Assign(const SecBlock<T, A> &t)
	{
		if (this != &t)
		{
			New(t.m_size);
			if (m_ptr && t.m_ptr && t.m_size)
				{memcpy_s(m_ptr, m_size*sizeof(T), t, t.m_size*sizeof(T));}
		}
	}

	//! \brief Assign contents from another SecBlock
	//! \param t the other SecBlock
	//! \details Internally, operator=() calls Assign().
	//! \details If the memory block is reduced in size, then the reclaimed memory is set to 0.
	SecBlock<T, A>& operator=(const SecBlock<T, A> &t)
	{
		// Assign guards for self-assignment
		Assign(t);
		return *this;
	}

	//! \brief Append contents from another SecBlock
	//! \param t the other SecBlock
	//! \details Internally, this SecBlock calls Grow and then appends t.
	SecBlock<T, A>& operator+=(const SecBlock<T, A> &t)
	{
		CRYPTOPP_ASSERT((!t.m_ptr && !t.m_size) || (t.m_ptr && t.m_size));

		if(t.m_size)
		{
			const size_type oldSize = m_size;
			if(this != &t)  // s += t
			{
				Grow(m_size+t.m_size);
				memcpy_s(m_ptr+oldSize, (m_size-oldSize)*sizeof(T), t.m_ptr, t.m_size*sizeof(T));
			}
			else            // t += t
			{
				Grow(m_size*2);
				memcpy_s(m_ptr+oldSize, (m_size-oldSize)*sizeof(T), m_ptr, oldSize*sizeof(T));
			}
		}
		return *this;
	}

	//! \brief Construct a SecBlock from this and another SecBlock
	//! \param t the other SecBlock
	//! \returns a newly constructed SecBlock that is a conacentation of this and t
	//! \details Internally, a new SecBlock is created from this and a concatenation of t.
	SecBlock<T, A> operator+(const SecBlock<T, A> &t)
	{
		CRYPTOPP_ASSERT((!m_ptr && !m_size) || (m_ptr && m_size));
		CRYPTOPP_ASSERT((!t.m_ptr && !t.m_size) || (t.m_ptr && t.m_size));
		if(!t.m_size) return SecBlock(*this);

		SecBlock<T, A> result(m_size+t.m_size);
		if(m_size) {memcpy_s(result.m_ptr, result.m_size*sizeof(T), m_ptr, m_size*sizeof(T));}
		memcpy_s(result.m_ptr+m_size, (result.m_size-m_size)*sizeof(T), t.m_ptr, t.m_size*sizeof(T));
		return result;
	}

	//! \brief Bitwise compare two SecBlocks
	//! \param t the other SecBlock
	//! \returns true if the size and bits are equal, false otherwise
	//! \details Uses a constant time compare if the arrays are equal size. The constant time
	//!    compare is VerifyBufsEqual() found in misc.h.
	//! \sa operator!=()
	bool operator==(const SecBlock<T, A> &t) const
	{
		return m_size == t.m_size &&
			VerifyBufsEqual(reinterpret_cast<const byte*>(m_ptr), reinterpret_cast<const byte*>(t.m_ptr), m_size*sizeof(T));
	}

	//! \brief Bitwise compare two SecBlocks
	//! \param t the other SecBlock
	//! \returns true if the size and bits are equal, false otherwise
	//! \details Uses a constant time compare if the arrays are equal size. The constant time
	//!    compare is VerifyBufsEqual() found in misc.h.
	//! \details Internally, operator!=() returns the inverse of operator==().
	//! \sa operator==()
	bool operator!=(const SecBlock<T, A> &t) const
	{
		return !operator==(t);
	}

	//! \brief Change size without preserving contents
	//! \param newSize the new size of the memory block
	//! \details Old content is \a not preserved. If the memory block is reduced in size,
	//!    then the reclaimed memory is set to 0. If the memory block grows in size, then
	//!    the new memory is \a not initialized.
	//! \details Internally, this SecBlock calls reallocate().
	//! \sa New(), CleanNew(), Grow(), CleanGrow(), resize()
	void New(size_type newSize)
	{
		m_ptr = m_alloc.reallocate(m_ptr, m_size, newSize, false);
		m_size = newSize;
	}

	//! \brief Change size without preserving contents
	//! \param newSize the new size of the memory block
	//! \details Old content is \a not preserved. If the memory block is reduced in size,
	//!    then the reclaimed content is set to 0. If the memory block grows in size, then
	//!    the new memory is initialized to 0.
	//! \details Internally, this SecBlock calls New().
	//! \sa New(), CleanNew(), Grow(), CleanGrow(), resize()
	void CleanNew(size_type newSize)
	{
		New(newSize);
		if (m_ptr) {memset_z(m_ptr, 0, m_size*sizeof(T));}
	}

	//! \brief Change size and preserve contents
	//! \param newSize the new size of the memory block
	//! \details Old content is preserved. New content is not initialized.
	//! \details Internally, this SecBlock calls reallocate() when size must increase. If the
	//!    size does not increase, then Grow() does not take action. If the size must
	//!    change, then use resize().
	//! \sa New(), CleanNew(), Grow(), CleanGrow(), resize()
	void Grow(size_type newSize)
	{
		if (newSize > m_size)
		{
			m_ptr = m_alloc.reallocate(m_ptr, m_size, newSize, true);
			m_size = newSize;
		}
	}

	//! \brief Change size and preserve contents
	//! \param newSize the new size of the memory block
	//! \details Old content is preserved. New content is initialized to 0.
	//! \details Internally, this SecBlock calls reallocate() when size must increase. If the
	//!    size does not increase, then CleanGrow() does not take action. If the size must
	//!    change, then use resize().
	//! \sa New(), CleanNew(), Grow(), CleanGrow(), resize()
	void CleanGrow(size_type newSize)
	{
		if (newSize > m_size)
		{
			m_ptr = m_alloc.reallocate(m_ptr, m_size, newSize, true);
			memset_z(m_ptr+m_size, 0, (newSize-m_size)*sizeof(T));
			m_size = newSize;
		}
	}

	//! \brief Change size and preserve contents
	//! \param newSize the new size of the memory block
	//! \details Old content is preserved. If the memory block grows in size, then
	//!    new memory is \a not initialized.
	//! \details Internally, this SecBlock calls reallocate().
	//! \sa New(), CleanNew(), Grow(), CleanGrow(), resize()
	void resize(size_type newSize)
	{
		m_ptr = m_alloc.reallocate(m_ptr, m_size, newSize, true);
		m_size = newSize;
	}

	//! \brief Swap contents with another SecBlock
	//! \param b the other SecBlock
	//! \details Internally, std::swap() is called on m_alloc, m_size and m_ptr.
	void swap(SecBlock<T, A> &b)
	{
		// Swap must occur on the allocator in case its FixedSize that spilled into the heap.
		std::swap(m_alloc, b.m_alloc);
		std::swap(m_size, b.m_size);
		std::swap(m_ptr, b.m_ptr);
	}

// protected:
	A m_alloc;
	size_type m_size;
	T *m_ptr;
};

#ifdef CRYPTOPP_DOXYGEN_PROCESSING
//! \class SecByteBlock
//! \brief \ref SecBlock "SecBlock<byte>" typedef.
class SecByteBlock : public SecBlock<byte> {};
//! \class SecWordBlock
//! \brief \ref SecBlock "SecBlock<word>" typedef.
class SecWordBlock : public SecBlock<word> {};
//! \class AlignedSecByteBlock
//! \brief SecBlock using \ref AllocatorWithCleanup "AllocatorWithCleanup<byte, true>" typedef
class AlignedSecByteBlock : public SecBlock<byte, AllocatorWithCleanup<byte, true> > {};
#else
typedef SecBlock<byte> SecByteBlock;
typedef SecBlock<word> SecWordBlock;
typedef SecBlock<byte, AllocatorWithCleanup<byte, true> > AlignedSecByteBlock;
#endif

// No need for move semantics on derived class *if* the class does not add any
//   data members; see http://stackoverflow.com/q/31755703, and Rule of {0|3|5}.

//! \class FixedSizeSecBlock
//! \brief Fixed size stack-based SecBlock
//! \tparam T class or type
//! \tparam S fixed-size of the stack-based memory block, in elements
//! \tparam A AllocatorBase derived class for allocation and cleanup
template <class T, unsigned int S, class A = FixedSizeAllocatorWithCleanup<T, S> >
class FixedSizeSecBlock : public SecBlock<T, A>
{
public:
	//! \brief Construct a FixedSizeSecBlock
	explicit FixedSizeSecBlock() : SecBlock<T, A>(S) {}
};

//! \class FixedSizeAlignedSecBlock
//! \brief Fixed size stack-based SecBlock with 16-byte alignment
//! \tparam T class or type
//! \tparam S fixed-size of the stack-based memory block, in elements
//! \tparam A AllocatorBase derived class for allocation and cleanup
template <class T, unsigned int S, bool T_Align16 = true>
class FixedSizeAlignedSecBlock : public FixedSizeSecBlock<T, S, FixedSizeAllocatorWithCleanup<T, S, NullAllocator<T>, T_Align16> >
{
};

//! \class SecBlockWithHint
//! \brief Stack-based SecBlock that grows into the heap
//! \tparam T class or type
//! \tparam S fixed-size of the stack-based memory block, in elements
//! \tparam A AllocatorBase derived class for allocation and cleanup
template <class T, unsigned int S, class A = FixedSizeAllocatorWithCleanup<T, S, AllocatorWithCleanup<T> > >
class SecBlockWithHint : public SecBlock<T, A>
{
public:
	//! construct a SecBlockWithHint with a count of elements
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

#if CRYPTOPP_MSC_VERSION
# pragma warning(pop)
#endif

#endif

// smartptr.h - written and placed in the public domain by Wei Dai

//! \file
//! \headerfile smartptr.h
//! \brief Classes for automatic resource management

#ifndef CRYPTOPP_SMARTPTR_H
#define CRYPTOPP_SMARTPTR_H

#include "config.h"
#include "stdcpp.h"

NAMESPACE_BEGIN(CryptoPP)

//! \class simple_ptr
//! \brief Manages resources for a single object
//! \tparam T class or type
//! \details \p simple_ptr is used frequently in the library to manage resources and
//!   ensure cleanup under the RAII pattern (Resource Acquisition Is Initialization).
template <class T> class simple_ptr
{
public:
	simple_ptr(T *p = NULL) : m_p(p) {}
	~simple_ptr()
	{
		delete m_p;
		*((volatile T**)&m_p) = NULL;
	}

	T *m_p;
};

//! \class member_ptr
//! \brief Pointer that overloads operator→
//! \tparam T class or type
//! \details member_ptr is used frequently in the library to avoid the issues related to
//!   std::auto_ptr in C++11 (deprecated) and std::unique_ptr in C++03 (non-existent).
//! \bug <a href="http://github.com/weidai11/cryptopp/issues/48">Issue 48: "Use of auto_ptr causes dirty compile under C++11"</a>
template <class T> class member_ptr
{
public:
	explicit member_ptr(T *p = NULL) : m_p(p) {}

	~member_ptr();

	const T& operator*() const { return *m_p; }
	T& operator*() { return *m_p; }

	const T* operator->() const { return m_p; }
	T* operator->() { return m_p; }

	const T* get() const { return m_p; }
	T* get() { return m_p; }

	T* release()
	{
		T *old_p = m_p;
		*((volatile T**)&m_p) = NULL;
		return old_p;
	} 

	void reset(T *p = 0);

protected:
	member_ptr(const member_ptr<T>& rhs);		// copy not allowed
	void operator=(const member_ptr<T>& rhs);	// assignment not allowed

	T *m_p;
};

template <class T> member_ptr<T>::~member_ptr() {delete m_p;}
template <class T> void member_ptr<T>::reset(T *p) {delete m_p; m_p = p;}

// ********************************************************

//! \class value_ptr
//! \brief Value pointer 
//! \tparam T class or type
template<class T> class value_ptr : public member_ptr<T>
{
public:
	value_ptr(const T &obj) : member_ptr<T>(new T(obj)) {}
	value_ptr(T *p = NULL) : member_ptr<T>(p) {}
	value_ptr(const value_ptr<T>& rhs)
		: member_ptr<T>(rhs.m_p ? new T(*rhs.m_p) : NULL) {}

	value_ptr<T>& operator=(const value_ptr<T>& rhs);
	bool operator==(const value_ptr<T>& rhs)
	{
		return (!this->m_p && !rhs.m_p) || (this->m_p && rhs.m_p && *this->m_p == *rhs.m_p);
	}
};

template <class T> value_ptr<T>& value_ptr<T>::operator=(const value_ptr<T>& rhs)
{
	T *old_p = this->m_p;
	this->m_p = rhs.m_p ? new T(*rhs.m_p) : NULL;
	delete old_p;
	return *this;
}

// ********************************************************

//! \class clonable_ptr
//! \brief A pointer which can be copied and cloned
//! \tparam T class or type
//! \details \p T should adhere to the \p Clonable interface
template<class T> class clonable_ptr : public member_ptr<T>
{
public:
	clonable_ptr(const T &obj) : member_ptr<T>(obj.Clone()) {}
	clonable_ptr(T *p = NULL) : member_ptr<T>(p) {}
	clonable_ptr(const clonable_ptr<T>& rhs)
		: member_ptr<T>(rhs.m_p ? rhs.m_p->Clone() : NULL) {}

	clonable_ptr<T>& operator=(const clonable_ptr<T>& rhs);
};

template <class T> clonable_ptr<T>& clonable_ptr<T>::operator=(const clonable_ptr<T>& rhs)
{
	T *old_p = this->m_p;
	this->m_p = rhs.m_p ? rhs.m_p->Clone() : NULL;
	delete old_p;
	return *this;
}

// ********************************************************

//! \class counted_ptr
//! \brief Reference counted pointer
//! \tparam T class or type
//! \details users should declare \p m_referenceCount as <tt>std::atomic<unsigned></tt>
//!   (or similar) under C++ 11
template<class T> class counted_ptr
{
public:
	explicit counted_ptr(T *p = 0);
	counted_ptr(const T &r) : m_p(0) {attach(r);}
	counted_ptr(const counted_ptr<T>& rhs);

	~counted_ptr();

	const T& operator*() const { return *m_p; }
	T& operator*() { return *m_p; }

	const T* operator->() const { return m_p; }
	T* operator->() { return get(); }

	const T* get() const { return m_p; }
	T* get();

	void attach(const T &p);

	counted_ptr<T> & operator=(const counted_ptr<T>& rhs);

private:
	T *m_p;
};

template <class T> counted_ptr<T>::counted_ptr(T *p)
	: m_p(p) 
{
	if (m_p)
		m_p->m_referenceCount = 1;
}

template <class T> counted_ptr<T>::counted_ptr(const counted_ptr<T>& rhs)
	: m_p(rhs.m_p)
{
	if (m_p)
		m_p->m_referenceCount++;
}

template <class T> counted_ptr<T>::~counted_ptr()
{
	if (m_p && --m_p->m_referenceCount == 0)
		delete m_p;
}

template <class T> void counted_ptr<T>::attach(const T &r)
{
	if (m_p && --m_p->m_referenceCount == 0)
		delete m_p;
	if (r.m_referenceCount == 0)
	{
		m_p = r.clone();
		m_p->m_referenceCount = 1;
	}
	else
	{
		m_p = const_cast<T *>(&r);
		m_p->m_referenceCount++;
	}
}

template <class T> T* counted_ptr<T>::get()
{
	if (m_p && m_p->m_referenceCount > 1)
	{
		T *temp = m_p->clone();
		m_p->m_referenceCount--;
		m_p = temp;
		m_p->m_referenceCount = 1;
	}
	return m_p;
}

template <class T> counted_ptr<T> & counted_ptr<T>::operator=(const counted_ptr<T>& rhs)
{
	if (m_p != rhs.m_p)
	{
		if (m_p && --m_p->m_referenceCount == 0)
			delete m_p;
		m_p = rhs.m_p;
		if (m_p)
			m_p->m_referenceCount++;
	}
	return *this;
}

// ********************************************************

//! \class vector_ptr
//! \brief Manages resources for an array of objects
//! \tparam T class or type
//! \details \p vector_ptr is used frequently in the library to avoid large stack allocations,
//!   and manage resources and ensure cleanup under the RAII pattern (Resource Acquisition
//!   Is Initialization).
template <class T> class vector_ptr
{
public:
	//! Construct an arry of \p T
	//! \param size the size of the array, in elements
	//! \details If \p T is a Plain Old Dataype (POD), then the array is uninitialized.
	vector_ptr(size_t size=0)
		: m_size(size), m_ptr(new T[m_size]) {}
	~vector_ptr()
		{delete [] m_ptr;}

	T& operator[](size_t index)
		{assert(m_size && index<this->m_size); return this->m_ptr[index];}
	const T& operator[](size_t index) const
		{assert(m_size && index<this->m_size); return this->m_ptr[index];}

	size_t size() const {return this->m_size;}
	void resize(size_t newSize)
	{
		T *newPtr = new T[newSize];
		for (size_t i=0; i<this->m_size && i<newSize; i++)
			newPtr[i] = m_ptr[i];
		delete [] this->m_ptr;
		this->m_size = newSize;
		this->m_ptr = newPtr;
	}
	
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

private:
	vector_ptr(const vector_ptr<T> &c);	// copy not allowed
	void operator=(const vector_ptr<T> &x);		// assignment not allowed

	size_t m_size;
	T *m_ptr;
};

// ********************************************************

//! \class vector_member_ptrs
//! \brief Manages resources for an array of objects
//! \tparam T class or type
template <class T> class vector_member_ptrs
{
public:
	//! Construct an arry of \p T
	//! \param size the size of the array, in elements
	//! \details If \p T is a Plain Old Dataype (POD), then the array is uninitialized.
	vector_member_ptrs(size_t size=0)
		: m_size(size), m_ptr(new member_ptr<T>[size]) {}
	~vector_member_ptrs()
		{delete [] this->m_ptr;}

	member_ptr<T>& operator[](size_t index)
		{assert(index<this->m_size); return this->m_ptr[index];}
	const member_ptr<T>& operator[](size_t index) const
		{assert(index<this->m_size); return this->m_ptr[index];}

	size_t size() const {return this->m_size;}
	void resize(size_t newSize)
	{
		member_ptr<T> *newPtr = new member_ptr<T>[newSize];
		for (size_t i=0; i<this->m_size && i<newSize; i++)
			newPtr[i].reset(this->m_ptr[i].release());
		delete [] this->m_ptr;
		this->m_size = newSize;
		this->m_ptr = newPtr;
	}

private:
	vector_member_ptrs(const vector_member_ptrs<T> &c);	// copy not allowed
	void operator=(const vector_member_ptrs<T> &x);		// assignment not allowed

	size_t m_size;
	member_ptr<T> *m_ptr;
};

NAMESPACE_END

#endif

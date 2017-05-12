// algparam.h - written and placed in the public domain by Wei Dai

//! \file
//! \headerfile algparam.h
//! \brief Classes for working with NameValuePairs


#ifndef CRYPTOPP_ALGPARAM_H
#define CRYPTOPP_ALGPARAM_H

#include "config.h"
#include "cryptlib.h"

// TODO: fix 6011 when the API/ABI can change
#if (CRYPTOPP_MSC_VERSION >= 1400)
# pragma warning(push)
# pragma warning(disable: 6011 28193)
#endif

#include "smartptr.h"
#include "secblock.h"
#include "integer.h"
#include "misc.h"

NAMESPACE_BEGIN(CryptoPP)

//! \class ConstByteArrayParameter
//! \brief Used to pass byte array input as part of a NameValuePairs object
class ConstByteArrayParameter
{
public:
	//! \brief Construct a ConstByteArrayParameter
	//! \param data a C-String
	//! \param deepCopy flag indicating whether the data should be copied
	//! \details The deepCopy option is used when the NameValuePairs object can't
	//!   keep a copy of the data available
	ConstByteArrayParameter(const char *data = NULL, bool deepCopy = false)
		: m_deepCopy(false), m_data(NULL), m_size(0)
	{
		Assign((const byte *)data, data ? strlen(data) : 0, deepCopy);
	}

	//! \brief Construct a ConstByteArrayParameter
	//! \param data a memory buffer
	//! \param size the length of the memory buffer
	//! \param deepCopy flag indicating whether the data should be copied
	//! \details The deepCopy option is used when the NameValuePairs object can't
	//!   keep a copy of the data available
	ConstByteArrayParameter(const byte *data, size_t size, bool deepCopy = false)
		: m_deepCopy(false), m_data(NULL), m_size(0)
	{
		Assign(data, size, deepCopy);
	}

	//! \brief Construct a ConstByteArrayParameter
	//! \tparam T a std::basic_string<char> class
	//! \param string a std::basic_string<char> class
	//! \param deepCopy flag indicating whether the data should be copied
	//! \details The deepCopy option is used when the NameValuePairs object can't
	//!   keep a copy of the data available
	template <class T> ConstByteArrayParameter(const T &string, bool deepCopy = false)
		: m_deepCopy(false), m_data(NULL), m_size(0)
	{
		CRYPTOPP_COMPILE_ASSERT(sizeof(CPP_TYPENAME T::value_type) == 1);
		Assign((const byte *)string.data(), string.size(), deepCopy);
	}

	//! \brief Assign contents from a memory buffer
	//! \param data a memory buffer
	//! \param size the length of the memory buffer
	//! \param deepCopy flag indicating whether the data should be copied
	//! \details The deepCopy option is used when the NameValuePairs object can't
	//!   keep a copy of the data available
	void Assign(const byte *data, size_t size, bool deepCopy)
	{
		// This fires, which means: no data with a size, or data with no size.
		// CRYPTOPP_ASSERT((data && size) || !(data || size));
		if (deepCopy)
			m_block.Assign(data, size);
		else
		{
			m_data = data;
			m_size = size;
		}
		m_deepCopy = deepCopy;
	}

	//! \brief Pointer to the first byte in the memory block
	const byte *begin() const {return m_deepCopy ? m_block.begin() : m_data;}
	//! \brief Pointer beyond the last byte in the memory block
	const byte *end() const {return m_deepCopy ? m_block.end() : m_data + m_size;}
	//! \brief Length of the memory block
	size_t size() const {return m_deepCopy ? m_block.size() : m_size;}

private:
	bool m_deepCopy;
	const byte *m_data;
	size_t m_size;
	SecByteBlock m_block;
};

//! \class ByteArrayParameter
//! \brief Used to pass byte array input as part of a NameValuePairs object
class ByteArrayParameter
{
public:
	//! \brief Construct a ByteArrayParameter
	//! \param data a memory buffer
	//! \param size the length of the memory buffer
	ByteArrayParameter(byte *data = NULL, unsigned int size = 0)
		: m_data(data), m_size(size) {}

	//! \brief Construct a ByteArrayParameter
	//! \param block a SecByteBlock
	ByteArrayParameter(SecByteBlock &block)
		: m_data(block.begin()), m_size(block.size()) {}

	//! \brief Pointer to the first byte in the memory block
	byte *begin() const {return m_data;}
	//! \brief Pointer beyond the last byte in the memory block
	byte *end() const {return m_data + m_size;}
	//! \brief Length of the memory block
	size_t size() const {return m_size;}

private:
	byte *m_data;
	size_t m_size;
};

//! \class CombinedNameValuePairs
//! \brief Combines two sets of NameValuePairs
//! \details CombinedNameValuePairs allows you to provide two sets of of NameValuePairs.
//!   If a name is not found in the first set, then the second set is searched for the
//!   name and value pair. The second set of NameValuePairs often provides default values.
class CRYPTOPP_DLL CombinedNameValuePairs : public NameValuePairs
{
public:
	//! \brief Construct a CombinedNameValuePairs
	//! \param pairs1 reference to the first set of NameValuePairs
	//! \param pairs2 reference to the second set of NameValuePairs
	CombinedNameValuePairs(const NameValuePairs &pairs1, const NameValuePairs &pairs2)
		: m_pairs1(pairs1), m_pairs2(pairs2) {}

	bool GetVoidValue(const char *name, const std::type_info &valueType, void *pValue) const;

private:
	const NameValuePairs &m_pairs1, &m_pairs2;
};

#ifndef CRYPTOPP_DOXYGEN_PROCESSING
template <class T, class BASE>
class GetValueHelperClass
{
public:
	GetValueHelperClass(const T *pObject, const char *name, const std::type_info &valueType, void *pValue, const NameValuePairs *searchFirst)
		: m_pObject(pObject), m_name(name), m_valueType(&valueType), m_pValue(pValue), m_found(false), m_getValueNames(false)
	{
		if (strcmp(m_name, "ValueNames") == 0)
		{
			m_found = m_getValueNames = true;
			NameValuePairs::ThrowIfTypeMismatch(m_name, typeid(std::string), *m_valueType);
			if (searchFirst)
				searchFirst->GetVoidValue(m_name, valueType, pValue);
			if (typeid(T) != typeid(BASE))
				pObject->BASE::GetVoidValue(m_name, valueType, pValue);
			((*reinterpret_cast<std::string *>(m_pValue) += "ThisPointer:") += typeid(T).name()) += ';';
		}

		if (!m_found && strncmp(m_name, "ThisPointer:", 12) == 0 && strcmp(m_name+12, typeid(T).name()) == 0)
		{
			NameValuePairs::ThrowIfTypeMismatch(m_name, typeid(T *), *m_valueType);
			*reinterpret_cast<const T **>(pValue) = pObject;
			m_found = true;
			return;
		}

		if (!m_found && searchFirst)
			m_found = searchFirst->GetVoidValue(m_name, valueType, pValue);

		if (!m_found && typeid(T) != typeid(BASE))
			m_found = pObject->BASE::GetVoidValue(m_name, valueType, pValue);
	}

	operator bool() const {return m_found;}

	template <class R>
	GetValueHelperClass<T,BASE> & operator()(const char *name, const R & (T::*pm)() const)
	{
		if (m_getValueNames)
			(*reinterpret_cast<std::string *>(m_pValue) += name) += ";";
		if (!m_found && strcmp(name, m_name) == 0)
		{
			NameValuePairs::ThrowIfTypeMismatch(name, typeid(R), *m_valueType);
			*reinterpret_cast<R *>(m_pValue) = (m_pObject->*pm)();
			m_found = true;
		}
		return *this;
	}

	GetValueHelperClass<T,BASE> &Assignable()
	{
#ifndef __INTEL_COMPILER	// ICL 9.1 workaround: Intel compiler copies the vTable pointer for some reason
		if (m_getValueNames)
			((*reinterpret_cast<std::string *>(m_pValue) += "ThisObject:") += typeid(T).name()) += ';';
		if (!m_found && strncmp(m_name, "ThisObject:", 11) == 0 && strcmp(m_name+11, typeid(T).name()) == 0)
		{
			NameValuePairs::ThrowIfTypeMismatch(m_name, typeid(T), *m_valueType);
			*reinterpret_cast<T *>(m_pValue) = *m_pObject;
			m_found = true;
		}
#endif
		return *this;
	}

private:
	const T *m_pObject;
	const char *m_name;
	const std::type_info *m_valueType;
	void *m_pValue;
	bool m_found, m_getValueNames;
};

template <class BASE, class T>
GetValueHelperClass<T, BASE> GetValueHelper(const T *pObject, const char *name, const std::type_info &valueType, void *pValue, const NameValuePairs *searchFirst=NULL, BASE *dummy=NULL)
{
	CRYPTOPP_UNUSED(dummy);
	return GetValueHelperClass<T, BASE>(pObject, name, valueType, pValue, searchFirst);
}

template <class T>
GetValueHelperClass<T, T> GetValueHelper(const T *pObject, const char *name, const std::type_info &valueType, void *pValue, const NameValuePairs *searchFirst=NULL)
{
	return GetValueHelperClass<T, T>(pObject, name, valueType, pValue, searchFirst);
}

// ********************************************************

// VC60 workaround
#if defined(_MSC_VER) && (_MSC_VER < 1300)
template <class R>
R Hack_DefaultValueFromConstReferenceType(const R &)
{
	return R();
}

template <class R>
bool Hack_GetValueIntoConstReference(const NameValuePairs &source, const char *name, const R &value)
{
	return source.GetValue(name, const_cast<R &>(value));
}

template <class T, class BASE>
class AssignFromHelperClass
{
public:
	AssignFromHelperClass(T *pObject, const NameValuePairs &source)
		: m_pObject(pObject), m_source(source), m_done(false)
	{
		if (source.GetThisObject(*pObject))
			m_done = true;
		else if (typeid(BASE) != typeid(T))
			pObject->BASE::AssignFrom(source);
	}

	template <class R>
	AssignFromHelperClass & operator()(const char *name, void (T::*pm)(R))	// VC60 workaround: "const R &" here causes compiler error
	{
		if (!m_done)
		{
			R value = Hack_DefaultValueFromConstReferenceType(reinterpret_cast<R>(*(int *)NULL));
			if (!Hack_GetValueIntoConstReference(m_source, name, value))
				throw InvalidArgument(std::string(typeid(T).name()) + ": Missing required parameter '" + name + "'");
			(m_pObject->*pm)(value);
		}
		return *this;
	}

	template <class R, class S>
	AssignFromHelperClass & operator()(const char *name1, const char *name2, void (T::*pm)(R, S))	// VC60 workaround: "const R &" here causes compiler error
	{
		if (!m_done)
		{
			R value1 = Hack_DefaultValueFromConstReferenceType(reinterpret_cast<R>(*(int *)NULL));
			if (!Hack_GetValueIntoConstReference(m_source, name1, value1))
				throw InvalidArgument(std::string(typeid(T).name()) + ": Missing required parameter '" + name1 + "'");
			S value2 = Hack_DefaultValueFromConstReferenceType(reinterpret_cast<S>(*(int *)NULL));
			if (!Hack_GetValueIntoConstReference(m_source, name2, value2))
				throw InvalidArgument(std::string(typeid(T).name()) + ": Missing required parameter '" + name2 + "'");
			(m_pObject->*pm)(value1, value2);
		}
		return *this;
	}

private:
	T *m_pObject;
	const NameValuePairs &m_source;
	bool m_done;
};
#else
template <class T, class BASE>
class AssignFromHelperClass
{
public:
	AssignFromHelperClass(T *pObject, const NameValuePairs &source)
		: m_pObject(pObject), m_source(source), m_done(false)
	{
		if (source.GetThisObject(*pObject))
			m_done = true;
		else if (typeid(BASE) != typeid(T))
			pObject->BASE::AssignFrom(source);
	}

	template <class R>
	AssignFromHelperClass & operator()(const char *name, void (T::*pm)(const R&))
	{
		if (!m_done)
		{
			R value;
			if (!m_source.GetValue(name, value))
				throw InvalidArgument(std::string(typeid(T).name()) + ": Missing required parameter '" + name + "'");
			(m_pObject->*pm)(value);
		}
		return *this;
	}

	template <class R, class S>
	AssignFromHelperClass & operator()(const char *name1, const char *name2, void (T::*pm)(const R&, const S&))
	{
		if (!m_done)
		{
			R value1;
			if (!m_source.GetValue(name1, value1))
				throw InvalidArgument(std::string(typeid(T).name()) + ": Missing required parameter '" + name1 + "'");
			S value2;
			if (!m_source.GetValue(name2, value2))
				throw InvalidArgument(std::string(typeid(T).name()) + ": Missing required parameter '" + name2 + "'");
			(m_pObject->*pm)(value1, value2);
		}
		return *this;
	}

private:
	T *m_pObject;
	const NameValuePairs &m_source;
	bool m_done;
};
#endif

template <class BASE, class T>
AssignFromHelperClass<T, BASE> AssignFromHelper(T *pObject, const NameValuePairs &source, BASE *dummy=NULL)
{
	CRYPTOPP_UNUSED(dummy);
	return AssignFromHelperClass<T, BASE>(pObject, source);
}

template <class T>
AssignFromHelperClass<T, T> AssignFromHelper(T *pObject, const NameValuePairs &source)
{
	return AssignFromHelperClass<T, T>(pObject, source);
}

#endif // CRYPTOPP_DOXYGEN_PROCESSING

// ********************************************************

// to allow the linker to discard Integer code if not needed.
typedef bool (CRYPTOPP_API * PAssignIntToInteger)(const std::type_info &valueType, void *pInteger, const void *pInt);
CRYPTOPP_DLL extern PAssignIntToInteger g_pAssignIntToInteger;

CRYPTOPP_DLL const std::type_info & CRYPTOPP_API IntegerTypeId();

//! \class AlgorithmParametersBase
//! \brief Base class for AlgorithmParameters
class CRYPTOPP_DLL AlgorithmParametersBase
{
public:
	//! \class ParameterNotUsed
	//! \brief Exception thrown when an AlgorithmParameter is unused
	class ParameterNotUsed : public Exception
	{
	public:
		ParameterNotUsed(const char *name) : Exception(OTHER_ERROR, std::string("AlgorithmParametersBase: parameter \"") + name + "\" not used") {}
	};

	// this is actually a move, not a copy
	AlgorithmParametersBase(const AlgorithmParametersBase &x)
		: m_name(x.m_name), m_throwIfNotUsed(x.m_throwIfNotUsed), m_used(x.m_used)
	{
		m_next.reset(const_cast<AlgorithmParametersBase &>(x).m_next.release());
		x.m_used = true;
	}

	//! \brief Construct a AlgorithmParametersBase
	//! \param name the parameter name
	//! \param throwIfNotUsed flags indicating whether an exception should be thrown
	//! \details If throwIfNotUsed is true, then a ParameterNotUsed exception
	//!   will be thrown in the destructor if the parameter is not not retrieved.
	AlgorithmParametersBase(const char *name, bool throwIfNotUsed)
		: m_name(name), m_throwIfNotUsed(throwIfNotUsed), m_used(false) {}

	virtual ~AlgorithmParametersBase() CRYPTOPP_THROW
	{
#ifdef CRYPTOPP_UNCAUGHT_EXCEPTION_AVAILABLE
		if (!std::uncaught_exception())
#else
		try
#endif
		{
			if (m_throwIfNotUsed && !m_used)
				throw ParameterNotUsed(m_name);
		}
#ifndef CRYPTOPP_UNCAUGHT_EXCEPTION_AVAILABLE
		catch(const Exception&)
		{
		}
#endif
	}

	bool GetVoidValue(const char *name, const std::type_info &valueType, void *pValue) const;

protected:
	friend class AlgorithmParameters;
	void operator=(const AlgorithmParametersBase& rhs);	// assignment not allowed, declare this for VC60

	virtual void AssignValue(const char *name, const std::type_info &valueType, void *pValue) const =0;
	virtual void MoveInto(void *p) const =0;	// not really const

	const char *m_name;
	bool m_throwIfNotUsed;
	mutable bool m_used;
	member_ptr<AlgorithmParametersBase> m_next;
};

//! \class AlgorithmParametersTemplate
//! \brief Template base class for AlgorithmParameters
//! \tparam T the class or type
template <class T>
class AlgorithmParametersTemplate : public AlgorithmParametersBase
{
public:
	//! \brief Construct an AlgorithmParametersTemplate
	//! \param name the name of the value
	//! \param value a reference to the value
	//! \param throwIfNotUsed flags indicating whether an exception should be thrown
	//! \details If throwIfNotUsed is true, then a ParameterNotUsed exception
	//!   will be thrown in the destructor if the parameter is not not retrieved.
	AlgorithmParametersTemplate(const char *name, const T &value, bool throwIfNotUsed)
		: AlgorithmParametersBase(name, throwIfNotUsed), m_value(value)
	{
	}

	void AssignValue(const char *name, const std::type_info &valueType, void *pValue) const
	{
		// special case for retrieving an Integer parameter when an int was passed in
		if (!(g_pAssignIntToInteger != NULL && typeid(T) == typeid(int) && g_pAssignIntToInteger(valueType, pValue, &m_value)))
		{
			NameValuePairs::ThrowIfTypeMismatch(name, typeid(T), valueType);
			*reinterpret_cast<T *>(pValue) = m_value;
		}
	}

#if defined(DEBUG_NEW) && (_MSC_VER >= 1300)
# pragma push_macro("new")
# undef new
#endif

	void MoveInto(void *buffer) const
	{
		AlgorithmParametersTemplate<T>* p = new(buffer) AlgorithmParametersTemplate<T>(*this);
		CRYPTOPP_UNUSED(p);	// silence warning
	}

#if defined(DEBUG_NEW) && (_MSC_VER >= 1300)
# pragma pop_macro("new")
#endif

protected:
	T m_value;
};

CRYPTOPP_DLL_TEMPLATE_CLASS AlgorithmParametersTemplate<bool>;
CRYPTOPP_DLL_TEMPLATE_CLASS AlgorithmParametersTemplate<int>;
CRYPTOPP_DLL_TEMPLATE_CLASS AlgorithmParametersTemplate<ConstByteArrayParameter>;

//! \class AlgorithmParameters
//! \brief An object that implements NameValuePairs
//! \tparam T the class or type
//! \param name the name of the object or value to retrieve
//! \param value reference to a variable that receives the value
//! \param throwIfNotUsed if true, the object will throw an exception if the value is not accessed
//! \note throwIfNotUsed is ignored if using a compiler that does not support std::uncaught_exception(),
//!   such as MSVC 7.0 and earlier.
//! \note A NameValuePairs object containing an arbitrary number of name value pairs may be constructed by
//!   repeatedly using operator() on the object returned by MakeParameters, for example:
//!   <pre>
//!     AlgorithmParameters parameters = MakeParameters(name1, value1)(name2, value2)(name3, value3);
//!   </pre>
class CRYPTOPP_DLL AlgorithmParameters : public NameValuePairs
{
public:
	AlgorithmParameters();

#ifdef __BORLANDC__
	template <class T>
	AlgorithmParameters(const char *name, const T &value, bool throwIfNotUsed=true)
		: m_next(new AlgorithmParametersTemplate<T>(name, value, throwIfNotUsed))
		, m_defaultThrowIfNotUsed(throwIfNotUsed)
	{
	}
#endif

	AlgorithmParameters(const AlgorithmParameters &x);

	AlgorithmParameters & operator=(const AlgorithmParameters &x);

	//! \tparam T the class or type
	//! \param name the name of the object or value to retrieve
	//! \param value reference to a variable that receives the value
	//! \param throwIfNotUsed if true, the object will throw an exception if the value is not accessed
	template <class T>
	AlgorithmParameters & operator()(const char *name, const T &value, bool throwIfNotUsed)
	{
		member_ptr<AlgorithmParametersBase> p(new AlgorithmParametersTemplate<T>(name, value, throwIfNotUsed));
		p->m_next.reset(m_next.release());
		m_next.reset(p.release());
		m_defaultThrowIfNotUsed = throwIfNotUsed;
		return *this;
	}

	//! \brief Appends a NameValuePair to a collection of NameValuePairs
	//! \tparam T the class or type
	//! \param name the name of the object or value to retrieve
	//! \param value reference to a variable that receives the value
	template <class T>
	AlgorithmParameters & operator()(const char *name, const T &value)
	{
		return operator()(name, value, m_defaultThrowIfNotUsed);
	}

	bool GetVoidValue(const char *name, const std::type_info &valueType, void *pValue) const;

protected:
	member_ptr<AlgorithmParametersBase> m_next;
	bool m_defaultThrowIfNotUsed;
};

//! \brief Create an object that implements NameValuePairs
//! \tparam T the class or type
//! \param name the name of the object or value to retrieve
//! \param value reference to a variable that receives the value
//! \param throwIfNotUsed if true, the object will throw an exception if the value is not accessed
//! \note throwIfNotUsed is ignored if using a compiler that does not support std::uncaught_exception(),
//!   such as MSVC 7.0 and earlier.
//! \note A NameValuePairs object containing an arbitrary number of name value pairs may be constructed by
//!   repeatedly using \p operator() on the object returned by \p MakeParameters, for example:
//!   <pre>
//!     AlgorithmParameters parameters = MakeParameters(name1, value1)(name2, value2)(name3, value3);
//!   </pre>
#ifdef __BORLANDC__
typedef AlgorithmParameters MakeParameters;
#else
template <class T>
AlgorithmParameters MakeParameters(const char *name, const T &value, bool throwIfNotUsed = true)
{
	return AlgorithmParameters()(name, value, throwIfNotUsed);
}
#endif

#define CRYPTOPP_GET_FUNCTION_ENTRY(name)		(Name::name(), &ThisClass::Get##name)
#define CRYPTOPP_SET_FUNCTION_ENTRY(name)		(Name::name(), &ThisClass::Set##name)
#define CRYPTOPP_SET_FUNCTION_ENTRY2(name1, name2)	(Name::name1(), Name::name2(), &ThisClass::Set##name1##And##name2)

// TODO: fix 6011 when the API/ABI can change
#if (CRYPTOPP_MSC_VERSION >= 1400)
# pragma warning(pop)
#endif

NAMESPACE_END

#endif

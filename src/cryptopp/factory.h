#ifndef CRYPTOPP_OBJFACT_H
#define CRYPTOPP_OBJFACT_H

#include "cryptlib.h"
#include "misc.h"
#include "stdcpp.h"

NAMESPACE_BEGIN(CryptoPP)

//! _
template <class AbstractClass>
class ObjectFactory
{
public:
	virtual ~ObjectFactory () {}
	virtual AbstractClass * CreateObject() const =0;
};

//! _
template <class AbstractClass, class ConcreteClass>
class DefaultObjectFactory : public ObjectFactory<AbstractClass>
{
public:
	AbstractClass * CreateObject() const
	{
		return new ConcreteClass;
	}
};

//! _
template <class AbstractClass, int instance=0>
class ObjectFactoryRegistry
{
public:
	class FactoryNotFound : public Exception
	{
	public:
		FactoryNotFound(const char *name) : Exception(OTHER_ERROR, std::string("ObjectFactoryRegistry: could not find factory for algorithm ") + name)  {}
	};

	~ObjectFactoryRegistry()
	{
		for (CPP_TYPENAME Map::iterator i = m_map.begin(); i != m_map.end(); ++i)
		{
			delete (ObjectFactory<AbstractClass> *)i->second;
			i->second = NULL;
		}
	}

	void RegisterFactory(const std::string &name, ObjectFactory<AbstractClass> *factory)
	{
		m_map[name] = factory;
	}

	const ObjectFactory<AbstractClass> * GetFactory(const char *name) const
	{
		CPP_TYPENAME Map::const_iterator i = m_map.find(name);
		return i == m_map.end() ? NULL : (ObjectFactory<AbstractClass> *)i->second;
	}

	AbstractClass *CreateObject(const char *name) const
	{
		const ObjectFactory<AbstractClass> *factory = GetFactory(name);
		if (!factory)
			throw FactoryNotFound(name);
		return factory->CreateObject();
	}

	// Return a vector containing the factory names. This is easier than returning an iterator.
	// from Andrew Pitonyak
	std::vector<std::string> GetFactoryNames() const
	{
		std::vector<std::string> names;
		CPP_TYPENAME Map::const_iterator iter;
		for (iter = m_map.begin(); iter != m_map.end(); ++iter)
			names.push_back(iter->first);
		return names;
	}

	CRYPTOPP_NOINLINE static ObjectFactoryRegistry<AbstractClass, instance> & Registry(CRYPTOPP_NOINLINE_DOTDOTDOT);

private:
	// use void * instead of ObjectFactory<AbstractClass> * to save code size
	typedef std::map<std::string, void *> Map;
	Map m_map;
};

template <class AbstractClass, int instance>
ObjectFactoryRegistry<AbstractClass, instance> & ObjectFactoryRegistry<AbstractClass, instance>::Registry(CRYPTOPP_NOINLINE_DOTDOTDOT)
{
	static ObjectFactoryRegistry<AbstractClass, instance> s_registry;
	return s_registry;
}

template <class AbstractClass, class ConcreteClass, int instance = 0>
struct RegisterDefaultFactoryFor {
RegisterDefaultFactoryFor(const char *name=NULL)
{
	// BCB2006 workaround
	std::string n = name ? std::string(name) : std::string(ConcreteClass::StaticAlgorithmName());
	ObjectFactoryRegistry<AbstractClass, instance>::Registry().
		RegisterFactory(n, new DefaultObjectFactory<AbstractClass, ConcreteClass>);
}};

template <class SchemeClass>
void RegisterAsymmetricCipherDefaultFactories(const char *name=NULL, SchemeClass *dummy=NULL)
{
	CRYPTOPP_UNUSED(dummy);
	RegisterDefaultFactoryFor<PK_Encryptor, CPP_TYPENAME SchemeClass::Encryptor>((const char *)name);
	RegisterDefaultFactoryFor<PK_Decryptor, CPP_TYPENAME SchemeClass::Decryptor>((const char *)name);
}

template <class SchemeClass>
void RegisterSignatureSchemeDefaultFactories(const char *name=NULL, SchemeClass *dummy=NULL)
{
	CRYPTOPP_UNUSED(dummy);
	RegisterDefaultFactoryFor<PK_Signer, CPP_TYPENAME SchemeClass::Signer>((const char *)name);
	RegisterDefaultFactoryFor<PK_Verifier, CPP_TYPENAME SchemeClass::Verifier>((const char *)name);
}

template <class SchemeClass>
void RegisterSymmetricCipherDefaultFactories(const char *name=NULL, SchemeClass *dummy=NULL)
{
	CRYPTOPP_UNUSED(dummy);
	RegisterDefaultFactoryFor<SymmetricCipher, CPP_TYPENAME SchemeClass::Encryption, ENCRYPTION>((const char *)name);
	RegisterDefaultFactoryFor<SymmetricCipher, CPP_TYPENAME SchemeClass::Decryption, DECRYPTION>((const char *)name);
}

template <class SchemeClass>
void RegisterAuthenticatedSymmetricCipherDefaultFactories(const char *name=NULL, SchemeClass *dummy=NULL)
{
	CRYPTOPP_UNUSED(dummy);
	RegisterDefaultFactoryFor<AuthenticatedSymmetricCipher, CPP_TYPENAME SchemeClass::Encryption, ENCRYPTION>((const char *)name);
	RegisterDefaultFactoryFor<AuthenticatedSymmetricCipher, CPP_TYPENAME SchemeClass::Decryption, DECRYPTION>((const char *)name);
}

NAMESPACE_END

#endif

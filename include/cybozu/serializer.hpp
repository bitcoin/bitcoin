#pragma once
/**
	@file
	@brief serializer for vector, list, map and so on

	@author MITSUNARI Shigeo(@herumi)
*/
#include <assert.h>
#include <cybozu/stream.hpp>

#ifdef _MSC_VER
	#pragma warning(push)
	#pragma warning(disable : 4127)
#endif

//#define CYBOZU_SERIALIZER_FIXED_SIZE_INTEGER

namespace cybozu {

namespace serializer_local {

template<class T>
union ci {
	T i;
	uint8_t c[sizeof(T)];
};

template<class S, void (S::*)(size_t)>
struct HasMemFunc { };

template<class T>
void dispatch_reserve(T& t, size_t size, int, HasMemFunc<T, &T::reserve>* = 0)
{
	t.reserve(size);
}

template<class T>
void dispatch_reserve(T&, size_t, int*)
{
}

template<class T>
void reserve_if_exists(T& t, size_t size)
{
	dispatch_reserve(t, size, 0);
}

} // serializer_local

template<class InputStream, class T>
void loadRange(T *p, size_t num, InputStream& is)
{
	cybozu::read(p, num * sizeof(T), is);
}

template<class OutputStream, class T>
void saveRange(OutputStream& os, const T *p, size_t num)
{
	cybozu::write(os, p, num * sizeof(T));
}

template<class InputStream, class T>
void loadPod(T& x, InputStream& is)
{
	serializer_local::ci<T> ci;
	loadRange(ci.c, sizeof(ci.c), is);
	x = ci.i;
}

template<class OutputStream, class T>
void savePod(OutputStream& os, const T& x)
{
	serializer_local::ci<T> ci;
	ci.i = x;
	saveRange(os, ci.c, sizeof(ci.c));
}

template<class InputStream, class T>
void load(T& x, InputStream& is)
{
	x.load(is);
}

template<class OutputStream, class T>
void save(OutputStream& os, const T& x)
{
	x.save(os);
}

#define CYBOZU_SERIALIZER_MAKE_SERIALIZER_F(type) \
template<class InputStream>void load(type& x, InputStream& is) { loadPod(x, is); } \
template<class OutputStream>void save(OutputStream& os, type x) { savePod(os, x); }

CYBOZU_SERIALIZER_MAKE_SERIALIZER_F(bool)
CYBOZU_SERIALIZER_MAKE_SERIALIZER_F(char)
CYBOZU_SERIALIZER_MAKE_SERIALIZER_F(short)
CYBOZU_SERIALIZER_MAKE_SERIALIZER_F(unsigned char)
CYBOZU_SERIALIZER_MAKE_SERIALIZER_F(unsigned short)
CYBOZU_SERIALIZER_MAKE_SERIALIZER_F(wchar_t)

CYBOZU_SERIALIZER_MAKE_SERIALIZER_F(float)
CYBOZU_SERIALIZER_MAKE_SERIALIZER_F(double)

#ifdef CYBOZU_SERIALIZER_FIXED_SIZE_INTEGER

#define CYBOZU_SERIALIZER_MAKE_INT_SERIALIZER(type) CYBOZU_SERIALIZER_MAKE_SERIALIZER_F(type)

#else

namespace serializer_local {

template<class S, class T>
bool isRecoverable(T x)
{
	return T(S(x)) == x;
}
/*
	data structure H:D of integer x
	H:header(1byte)
	0x80 ; D = 1 byte zero ext
	0x81 ; D = 2 byte zero ext
	0x82 ; D = 4 byte zero ext
	0x83 ; D = 8 byte zero ext
	0x84 ; D = 1 byte signed ext
	0x85 ; D = 2 byte signed ext
	0x86 ; D = 4 byte signed ext
	0x87 ; D = 8 byte signed ext
	other; x = signed H, D = none
*/
template<class OutputStream, class T>
void saveVariableInt(OutputStream& os, const T& x)
{
	if (isRecoverable<int8_t>(x)) {
		uint8_t u8 = uint8_t(x);
		if (unsigned(u8 - 0x80) <= 7) {
			savePod(os, uint8_t(0x84));
		}
		savePod(os, u8);
	} else if (isRecoverable<uint8_t>(x)) {
		savePod(os, uint8_t(0x80));
		savePod(os, uint8_t(x));
	} else if (isRecoverable<uint16_t>(x) || isRecoverable<int16_t>(x)) {
		savePod(os, uint8_t(isRecoverable<uint16_t>(x) ? 0x81 : 0x85));
		savePod(os, uint16_t(x));
	} else if (isRecoverable<uint32_t>(x) || isRecoverable<int32_t>(x)) {
		savePod(os, uint8_t(isRecoverable<uint32_t>(x) ? 0x82 : 0x86));
		savePod(os, uint32_t(x));
	} else {
		assert(sizeof(T) == 8);
		savePod(os, uint8_t(0x83));
		savePod(os, uint64_t(x));
	}
}

template<class InputStream, class T>
void loadVariableInt(T& x, InputStream& is)
{
	uint8_t h;
	loadPod(h, is);
	if (h == 0x80) {
		uint8_t v;
		loadPod(v, is);
		x = v;
	} else if (h == 0x81) {
		uint16_t v;
		loadPod(v, is);
		x = v;
	} else if (h == 0x82) {
		uint32_t v;
		loadPod(v, is);
		x = v;
	} else if (h == 0x83) {
		if (sizeof(T) == 4) throw cybozu::Exception("loadVariableInt:bad header") << h;
		uint64_t v;
		loadPod(v, is);
		x = static_cast<T>(v);
	} else if (h == 0x84) {
		int8_t v;
		loadPod(v, is);
		x = v;
	} else if (h == 0x85) {
		int16_t v;
		loadPod(v, is);
		x = v;
	} else if (h == 0x86) {
		int32_t v;
		loadPod(v, is);
		x = v;
	} else if (h == 0x87) {
		if (sizeof(T) == 4) throw cybozu::Exception("loadVariableInt:bad header") << h;
		int64_t v;
		loadPod(v, is);
		x = static_cast<T>(v);
	} else {
		x = static_cast<int8_t>(h);
	}
}

} // serializer_local

#define CYBOZU_SERIALIZER_MAKE_INT_SERIALIZER(type) \
template<class InputStream>void load(type& x, InputStream& is) { serializer_local::loadVariableInt(x, is); } \
template<class OutputStream>void save(OutputStream& os, type x) { serializer_local::saveVariableInt(os, x); }

#endif

CYBOZU_SERIALIZER_MAKE_INT_SERIALIZER(int)
CYBOZU_SERIALIZER_MAKE_INT_SERIALIZER(long)
CYBOZU_SERIALIZER_MAKE_INT_SERIALIZER(long long)
CYBOZU_SERIALIZER_MAKE_INT_SERIALIZER(unsigned int)
CYBOZU_SERIALIZER_MAKE_INT_SERIALIZER(unsigned long)
CYBOZU_SERIALIZER_MAKE_INT_SERIALIZER(unsigned long long)

#undef CYBOZU_SERIALIZER_MAKE_INT_SERIALIZER
#undef CYBOZU_SERIALIZER_MAKE_UNT_SERIALIZER
#undef CYBOZU_SERIALIZER_MAKE_SERIALIZER_F
#undef CYBOZU_SERIALIZER_MAKE_SERIALIZER_V

// only for std::vector<POD>
template<class V, class InputStream>
void loadPodVec(V& v, InputStream& is)
{
	size_t size;
	load(size, is);
	v.resize(size);
	if (size > 0) loadRange(&v[0], size, is);
}

// only for std::vector<POD>
template<class V, class OutputStream>
void savePodVec(OutputStream& os, const V& v)
{
	save(os, v.size());
	if (!v.empty()) saveRange(os, &v[0], v.size());
}

template<class InputStream>
void load(std::string& str, InputStream& is)
{
	loadPodVec(str, is);
}

template<class OutputStream>
void save(OutputStream& os, const std::string& str)
{
	savePodVec(os, str);
}

template<class OutputStream>
void save(OutputStream& os, const char *x)
{
	const size_t len = strlen(x);
	save(os, len);
	if (len > 0) saveRange(os, x, len);
}


// for vector, list
template<class InputStream, class T, class Alloc, template<class T_, class Alloc_>class Container>
void load(Container<T, Alloc>& x, InputStream& is)
{
	size_t size;
	load(size, is);
	serializer_local::reserve_if_exists(x, size);
	for (size_t i = 0; i < size; i++) {
		x.push_back(T());
		T& t = x.back();
		load(t, is);
	}
}

template<class OutputStream, class T, class Alloc, template<class T_, class Alloc_>class Container>
void save(OutputStream& os, const Container<T, Alloc>& x)
{
	typedef Container<T, Alloc> V;
	save(os, x.size());
	for (typename V::const_iterator i = x.begin(), end = x.end(); i != end; ++i) {
		save(os, *i);
	}
}

// for set
template<class InputStream, class K, class Pred, class Alloc, template<class K_, class Pred_, class Alloc_>class Container>
void load(Container<K, Pred, Alloc>& x, InputStream& is)
{
	size_t size;
	load(size, is);
	for (size_t i = 0; i < size; i++) {
		K t;
		load(t, is);
		x.insert(t);
	}
}

template<class OutputStream, class K, class Pred, class Alloc, template<class K_, class Pred_, class Alloc_>class Container>
void save(OutputStream& os, const Container<K, Pred, Alloc>& x)
{
	typedef Container<K, Pred, Alloc> Set;
	save(os, x.size());
	for (typename Set::const_iterator i = x.begin(), end = x.end(); i != end; ++i) {
		save(os, *i);
	}
}

// for map
template<class InputStream, class K, class V, class Pred, class Alloc, template<class K_, class V_, class Pred_, class Alloc_>class Container>
void load(Container<K, V, Pred, Alloc>& x, InputStream& is)
{
	typedef Container<K, V, Pred, Alloc> Map;
	size_t size;
	load(size, is);
	for (size_t i = 0; i < size; i++) {
		std::pair<typename Map::key_type, typename Map::mapped_type> vt;
		load(vt.first, is);
		load(vt.second, is);
		x.insert(vt);
	}
}

template<class OutputStream, class K, class V, class Pred, class Alloc, template<class K_, class V_, class Pred_, class Alloc_>class Container>
void save(OutputStream& os, const Container<K, V, Pred, Alloc>& x)
{
	typedef Container<K, V, Pred, Alloc> Map;
	save(os, x.size());
	for (typename Map::const_iterator i = x.begin(), end = x.end(); i != end; ++i) {
		save(os, i->first);
		save(os, i->second);
	}
}

// unordered_map
template<class InputStream, class K, class V, class Hash, class Pred, class Alloc, template<class K_, class V_, class Hash_, class Pred_, class Alloc_>class Container>
void load(Container<K, V, Hash, Pred, Alloc>& x, InputStream& is)
{
	typedef Container<K, V, Hash, Pred, Alloc> Map;
	size_t size;
	load(size, is);
//	x.reserve(size); // tr1::unordered_map may not have reserve
	cybozu::serializer_local::reserve_if_exists(x, size);
	for (size_t i = 0; i < size; i++) {
		std::pair<typename Map::key_type, typename Map::mapped_type> vt;
		load(vt.first, is);
		load(vt.second, is);
		x.insert(vt);
	}
}

template<class OutputStream, class K, class V, class Hash, class Pred, class Alloc, template<class K_, class V_, class Hash_, class Pred_, class Alloc_>class Container>
void save(OutputStream& os, const Container<K, V, Hash, Pred, Alloc>& x)
{
	typedef Container<K, V, Hash, Pred, Alloc> Map;
	save(os, x.size());
	for (typename Map::const_iterator i = x.begin(), end = x.end(); i != end; ++i) {
		save(os, i->first);
		save(os, i->second);
	}
}

} // cybozu

#ifdef _MSC_VER
	#pragma warning(pop)
#endif

#ifndef SYSCOIN_NEVM_VECTOR_REF_H
#define SYSCOIN_NEVM_VECTOR_REF_H

#include <cstring>
#include <cassert>
#include <type_traits>
#include <vector>
#include <string>
#include <atomic>

#ifdef __INTEL_COMPILER
#pragma warning(disable:597) //will not be called for implicit or explicit conversions
#endif

namespace dev
{

/**
 * A modifiable reference to an existing object or vector in memory.
 */
template <class _T>
class vector_ref
{
public:
	using value_type = _T;
	using element_type = _T;
	using mutable_value_type = typename std::conditional<std::is_const<_T>::value, typename std::remove_const<_T>::type, _T>::type;

	static_assert(std::is_pod<value_type>::value, "vector_ref can only be used with PODs due to its low-level treatment of data.");

	vector_ref(): m_data(nullptr), m_count(0) {}
	/// Creates a new vector_ref to point to @a _count elements starting at @a _data.
	vector_ref(_T* _data, size_t _count): m_data(_data), m_count(_count) {}
	/// Creates a new vector_ref pointing to the data part of a string (given as pointer).
	vector_ref(typename std::conditional<std::is_const<_T>::value, std::string const*, std::string*>::type _data): m_data(reinterpret_cast<_T*>(_data->data())), m_count(_data->size() / sizeof(_T)) {}
	/// Creates a new vector_ref pointing to the data part of a vector (given as pointer).
	vector_ref(typename std::conditional<std::is_const<_T>::value, std::vector<typename std::remove_const<_T>::type> const*, std::vector<_T>*>::type _data): m_data(_data->data()), m_count(_data->size()) {}
	/// Creates a new vector_ref pointing to the data part of a string (given as reference).
	vector_ref(typename std::conditional<std::is_const<_T>::value, std::string const&, std::string&>::type _data): m_data(reinterpret_cast<_T*>(_data.data())), m_count(_data.size() / sizeof(_T)) {}
#if DEV_LDB
	vector_ref(ldb::Slice const& _s): m_data(reinterpret_cast<_T*>(_s.data())), m_count(_s.size() / sizeof(_T)) {}
#endif
	explicit operator bool() const { return m_data && m_count; }

	bool contentsEqual(std::vector<mutable_value_type> const& _c) const { if (!m_data || m_count == 0) return _c.empty(); else return _c.size() == m_count && !memcmp(_c.data(), m_data, m_count * sizeof(_T)); }
	std::vector<mutable_value_type> toVector() const { return std::vector<mutable_value_type>(m_data, m_data + m_count); }
	std::vector<unsigned char> toBytes() const { return std::vector<unsigned char>(reinterpret_cast<unsigned char const*>(m_data), reinterpret_cast<unsigned char const*>(m_data) + m_count * sizeof(_T)); }
	std::string toString() const { return std::string((char const*)m_data, ((char const*)m_data) + m_count * sizeof(_T)); }

	template <class _T2> explicit operator vector_ref<_T2>() const { assert(m_count * sizeof(_T) / sizeof(_T2) * sizeof(_T2) / sizeof(_T) == m_count); return vector_ref<_T2>(reinterpret_cast<_T2*>(m_data), m_count * sizeof(_T) / sizeof(_T2)); }
	operator vector_ref<_T const>() const { return vector_ref<_T const>(m_data, m_count); }

	_T* data() const { return m_data; }
	/// @returns the number of elements referenced (not necessarily number of bytes).
	size_t count() const { return m_count; }
	/// @returns the number of elements referenced (not necessarily number of bytes).
	size_t size() const { return m_count; }
	bool empty() const { return !m_count; }
	/// @returns a new vector_ref pointing at the next chunk of @a size() elements.
	vector_ref<_T> next() const { if (!m_data) return *this; else return vector_ref<_T>(m_data + m_count, m_count); }
	/// @returns a new vector_ref which is a shifted and shortened view of the original data.
	/// If this goes out of bounds in any way, returns an empty vector_ref.
	/// If @a _count is ~size_t(0), extends the view to the end of the data.
	vector_ref<_T> cropped(size_t _begin, size_t _count) const { if (m_data && _begin <= m_count && _count <= m_count && _begin + _count <= m_count) return vector_ref<_T>(m_data + _begin, _count == ~size_t(0) ? m_count - _begin : _count); else return vector_ref<_T>(); }
	/// @returns a new vector_ref which is a shifted view of the original data (not going beyond it).
	vector_ref<_T> cropped(size_t _begin) const { if (m_data && _begin <= m_count) return vector_ref<_T>(m_data + _begin, m_count - _begin); else return vector_ref<_T>(); }
	void retarget(_T* _d, size_t _s) { m_data = _d; m_count = _s; }
	void retarget(std::vector<_T> const& _t) { m_data = _t.data(); m_count = _t.size(); }
	template <class T> bool overlapsWith(vector_ref<T> _t) const { void const* f1 = data(); void const* t1 = data() + size(); void const* f2 = _t.data(); void const* t2 = _t.data() + _t.size(); return f1 < t2 && t1 > f2; }
	/// Copies the contents of this vector_ref to the contents of @a _t, up to the max size of @a _t.
	void copyTo(vector_ref<typename std::remove_const<_T>::type> _t) const { if (overlapsWith(_t)) memmove(_t.data(), m_data, std::min(_t.size(), m_count) * sizeof(_T)); else memcpy(_t.data(), m_data, std::min(_t.size(), m_count) * sizeof(_T)); }
	/// Copies the contents of this vector_ref to the contents of @a _t, and zeros further trailing elements in @a _t.
	void populate(vector_ref<typename std::remove_const<_T>::type> _t) const { copyTo(_t); memset(_t.data() + m_count, 0, std::max(_t.size(), m_count) - m_count); }
	/// Securely overwrite the memory.
	/// @note adapted from OpenSSL's implementation.
	void cleanse()
	{
		static std::atomic<unsigned char> s_cleanseCounter{0u};
		uint8_t* p = (uint8_t*)begin();
		size_t const len = (uint8_t*)end() - p;
		size_t loop = len;
		size_t count = s_cleanseCounter;
		while (loop--)
		{
			*(p++) = (uint8_t)count;
			count += (17 + ((size_t)p & 0xf));
		}
		p = (uint8_t*)memchr((uint8_t*)begin(), (uint8_t)count, len);
		if (p)
			count += (63 + (size_t)p);
		s_cleanseCounter = (uint8_t)count;
		memset((uint8_t*)begin(), 0, len);
	}

	_T* begin() { return m_data; }
	_T* end() { return m_data + m_count; }
	_T const* begin() const { return m_data; }
	_T const* end() const { return m_data + m_count; }

	_T& operator[](size_t _i) { assert(m_data); assert(_i < m_count); return m_data[_i]; }
	_T const& operator[](size_t _i) const { assert(m_data); assert(_i < m_count); return m_data[_i]; }

	bool operator==(vector_ref<_T> const& _cmp) const { return m_data == _cmp.m_data && m_count == _cmp.m_count; }
	bool operator!=(vector_ref<_T> const& _cmp) const { return !operator==(_cmp); }

#if DEV_LDB
	operator ldb::Slice() const { return ldb::Slice((char const*)m_data, m_count * sizeof(_T)); }
#endif

	void reset() { m_data = nullptr; m_count = 0; }

private:
	_T* m_data;
	size_t m_count;
};

template<class _T> vector_ref<_T const> ref(_T const& _t) { return vector_ref<_T const>(&_t, 1); }
template<class _T> vector_ref<_T> ref(_T& _t) { return vector_ref<_T>(&_t, 1); }
template<class _T> vector_ref<_T const> ref(std::vector<_T> const& _t) { return vector_ref<_T const>(&_t); }
template<class _T> vector_ref<_T> ref(std::vector<_T>& _t) { return vector_ref<_T>(&_t); }

}
#endif // SYSCOIN_NEVM_VECTOR_REF_H
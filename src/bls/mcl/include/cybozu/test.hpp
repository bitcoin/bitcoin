#pragma once
/**
	@file
	@brief unit test class

	@author MITSUNARI Shigeo(@herumi)
*/

#include <stdio.h>
#include <string.h>
#include <string>
#include <list>
#include <iostream>
#include <utility>
#if defined(_MSC_VER) && (MSC_VER <= 1500)
	#include <cybozu/inttype.hpp>
#else
	#include <stdint.h>
#endif

namespace cybozu { namespace test {

class AutoRun {
	typedef void (*Func)();
	typedef std::list<std::pair<const char*, Func> > UnitTestList;
public:
	AutoRun()
		: init_(0)
		, term_(0)
		, okCount_(0)
		, ngCount_(0)
		, exceptionCount_(0)
	{
	}
	void setup(Func init, Func term)
	{
		init_ = init;
		term_ = term;
	}
	void append(const char *name, Func func)
	{
		list_.push_back(std::make_pair(name, func));
	}
	void set(bool isOK)
	{
		if (isOK) {
			okCount_++;
		} else {
			ngCount_++;
		}
	}
	std::string getBaseName(const std::string& name) const
	{
#ifdef _WIN32
		const char sep = '\\';
#else
		const char sep = '/';
#endif
		size_t pos = name.find_last_of(sep);
		std::string ret = name.substr(pos + 1);
		pos = ret.find('.');
		return ret.substr(0, pos);
	}
	int run(int, char *argv[])
	{
		std::string msg;
		try {
			if (init_) init_();
			for (UnitTestList::const_iterator i = list_.begin(), ie = list_.end(); i != ie; ++i) {
				std::cout << "ctest:module=" << i->first << std::endl;
				try {
					(i->second)();
				} catch (std::exception& e) {
					exceptionCount_++;
					std::cout << "ctest:  " << i->first << " is stopped by exception " << e.what() << std::endl;
				} catch (...) {
					exceptionCount_++;
					std::cout << "ctest:  " << i->first << " is stopped by unknown exception" << std::endl;
				}
			}
			if (term_) term_();
		} catch (std::exception& e) {
			msg = std::string("ctest:err:") + e.what();
		} catch (...) {
			msg = "ctest:err: catch unknown exception";
		}
		fflush(stdout);
		if (msg.empty()) {
			int err = ngCount_ + exceptionCount_;
			int total = okCount_ + err;
			std::cout << "ctest:name=" << getBaseName(*argv)
					  << ", module=" << list_.size()
					  << ", total=" << total
					  << ", ok=" << okCount_
					  << ", ng=" << ngCount_
					  << ", exception=" << exceptionCount_ << std::endl;
			return err > 0 ? 1 : 0;
		} else {
			std::cout << msg << std::endl;
			return 1;
		}
	}
	static inline AutoRun& getInstance()
	{
		static AutoRun instance;
		return instance;
	}
private:
	Func init_;
	Func term_;
	int okCount_;
	int ngCount_;
	int exceptionCount_;
	UnitTestList list_;
};

static AutoRun& autoRun = AutoRun::getInstance();

inline void test(bool ret, const std::string& msg, const std::string& param, const char *file, int line)
{
	autoRun.set(ret);
	if (!ret) {
		printf("%s(%d):ctest:%s(%s);\n", file, line, msg.c_str(), param.c_str());
	}
}

template<typename T, typename U>
bool isEqual(const T& lhs, const U& rhs)
{
	return lhs == rhs;
}

// avoid warning of comparision of integers of different signs
inline bool isEqual(size_t lhs, int rhs)
{
	return lhs == size_t(rhs);
}
inline bool isEqual(int lhs, size_t rhs)
{
	return size_t(lhs) == rhs;
}
inline bool isEqual(const char *lhs, const char *rhs)
{
	return strcmp(lhs, rhs) == 0;
}
inline bool isEqual(char *lhs, const char *rhs)
{
	return strcmp(lhs, rhs) == 0;
}
inline bool isEqual(const char *lhs, char *rhs)
{
	return strcmp(lhs, rhs) == 0;
}
inline bool isEqual(char *lhs, char *rhs)
{
	return strcmp(lhs, rhs) == 0;
}
// avoid to compare float directly
inline bool isEqual(float lhs, float rhs)
{
	union fi {
		float f;
		uint32_t i;
	} lfi, rfi;
	lfi.f = lhs;
	rfi.f = rhs;
	return lfi.i == rfi.i;
}
// avoid to compare double directly
inline bool isEqual(double lhs, double rhs)
{
	union di {
		double d;
		uint64_t i;
	} ldi, rdi;
	ldi.d = lhs;
	rdi.d = rhs;
	return ldi.i == rdi.i;
}

} } // cybozu::test

#ifndef CYBOZU_TEST_DISABLE_AUTO_RUN
int main(int argc, char *argv[])
{
	return cybozu::test::autoRun.run(argc, argv);
}
#endif

/**
	alert if !x
	@param x [in]
*/
#define CYBOZU_TEST_ASSERT(x) cybozu::test::test(!!(x), "CYBOZU_TEST_ASSERT", #x, __FILE__, __LINE__)

/**
	alert if x != y
	@param x [in]
	@param y [in]
*/
#define CYBOZU_TEST_EQUAL(x, y) { \
	bool _cybozu_eq = cybozu::test::isEqual(x, y); \
	cybozu::test::test(_cybozu_eq, "CYBOZU_TEST_EQUAL", #x ", " #y, __FILE__, __LINE__); \
	if (!_cybozu_eq) { \
		std::cout << "ctest:  lhs=" << (x) << std::endl; \
		std::cout << "ctest:  rhs=" << (y) << std::endl; \
	} \
}
/**
	alert if fabs(x, y) >= eps
	@param x [in]
	@param y [in]
*/
#define CYBOZU_TEST_NEAR(x, y, eps) { \
	bool _cybozu_isNear = fabs((x) - (y)) < eps; \
	cybozu::test::test(_cybozu_isNear, "CYBOZU_TEST_NEAR", #x ", " #y, __FILE__, __LINE__); \
	if (!_cybozu_isNear) { \
		std::cout << "ctest:  lhs=" << (x) << std::endl; \
		std::cout << "ctest:  rhs=" << (y) << std::endl; \
	} \
}

#define CYBOZU_TEST_EQUAL_POINTER(x, y) { \
	bool _cybozu_eq = x == y; \
	cybozu::test::test(_cybozu_eq, "CYBOZU_TEST_EQUAL_POINTER", #x ", " #y, __FILE__, __LINE__); \
	if (!_cybozu_eq) { \
		std::cout << "ctest:  lhs=" << static_cast<const void*>(x) << std::endl; \
		std::cout << "ctest:  rhs=" << static_cast<const void*>(y) << std::endl; \
	} \
}
/**
	alert if x[] != y[]
	@param x [in]
	@param y [in]
	@param n [in]
*/
#define CYBOZU_TEST_EQUAL_ARRAY(x, y, n) { \
	for (size_t _cybozu_test_i = 0, _cybozu_ie = (size_t)(n); _cybozu_test_i < _cybozu_ie; _cybozu_test_i++) { \
		bool _cybozu_eq = cybozu::test::isEqual((x)[_cybozu_test_i], (y)[_cybozu_test_i]); \
		cybozu::test::test(_cybozu_eq, "CYBOZU_TEST_EQUAL_ARRAY", #x ", " #y ", " #n, __FILE__, __LINE__); \
		if (!_cybozu_eq) { \
			std::cout << "ctest:  i=" << _cybozu_test_i << std::endl; \
			std::cout << "ctest:  lhs=" << (x)[_cybozu_test_i] << std::endl; \
			std::cout << "ctest:  rhs=" << (y)[_cybozu_test_i] << std::endl; \
		} \
	} \
}

/**
	always alert
	@param msg [in]
*/
#define CYBOZU_TEST_FAIL(msg) cybozu::test::test(false, "CYBOZU_TEST_FAIL", msg, __FILE__, __LINE__)

/**
	verify message in exception
*/
#define CYBOZU_TEST_EXCEPTION_MESSAGE(statement, Exception, msg) \
{ \
	int _cybozu_ret = 0; \
	std::string _cybozu_errMsg; \
	try { \
		statement; \
		_cybozu_ret = 1; \
	} catch (const Exception& _cybozu_e) { \
		_cybozu_errMsg = _cybozu_e.what(); \
		if (_cybozu_errMsg.find(msg) == std::string::npos) { \
			_cybozu_ret = 2; \
		} \
	} catch (...) { \
		_cybozu_ret = 3; \
	} \
	if (_cybozu_ret) { \
		cybozu::test::test(false, "CYBOZU_TEST_EXCEPTION_MESSAGE", #statement ", " #Exception ", " #msg, __FILE__, __LINE__); \
		if (_cybozu_ret == 1) { \
			std::cout << "ctest:  no exception" << std::endl; \
		} else if (_cybozu_ret == 2) { \
			std::cout << "ctest:  bad exception msg:" << _cybozu_errMsg << std::endl; \
		} else { \
			std::cout << "ctest:  unexpected exception" << std::endl; \
		} \
	} else { \
		cybozu::test::autoRun.set(true); \
	} \
}

#define CYBOZU_TEST_EXCEPTION(statement, Exception) \
{ \
	int _cybozu_ret = 0; \
	try { \
		statement; \
		_cybozu_ret = 1; \
	} catch (const Exception&) { \
	} catch (...) { \
		_cybozu_ret = 2; \
	} \
	if (_cybozu_ret) { \
		cybozu::test::test(false, "CYBOZU_TEST_EXCEPTION", #statement ", " #Exception, __FILE__, __LINE__); \
		if (_cybozu_ret == 1) { \
			std::cout << "ctest:  no exception" << std::endl; \
		} else { \
			std::cout << "ctest:  unexpected exception" << std::endl; \
		} \
	} else { \
		cybozu::test::autoRun.set(true); \
	} \
}

/**
	verify statement does not throw
*/
#define CYBOZU_TEST_NO_EXCEPTION(statement) \
try { \
	statement; \
	cybozu::test::autoRun.set(true); \
} catch (...) { \
	cybozu::test::test(false, "CYBOZU_TEST_NO_EXCEPTION", #statement, __FILE__, __LINE__); \
}

/**
	append auto unit test
	@param name [in] module name
*/
#define CYBOZU_TEST_AUTO(name) \
void cybozu_test_ ## name(); \
struct cybozu_test_local_ ## name { \
	cybozu_test_local_ ## name() \
	{ \
		cybozu::test::autoRun.append(#name, cybozu_test_ ## name); \
	} \
} cybozu_test_local_instance_ ## name; \
void cybozu_test_ ## name()

/**
	append auto unit test with fixture
	@param name [in] module name
*/
#define CYBOZU_TEST_AUTO_WITH_FIXTURE(name, Fixture) \
void cybozu_test_ ## name(); \
void cybozu_test_real_ ## name() \
{ \
	Fixture f; \
	cybozu_test_ ## name(); \
} \
struct cybozu_test_local_ ## name { \
	cybozu_test_local_ ## name() \
	{ \
		cybozu::test::autoRun.append(#name, cybozu_test_real_ ## name); \
	} \
} cybozu_test_local_instance_ ## name; \
void cybozu_test_ ## name()

/**
	setup fixture
	@param Fixture [in] class name of fixture
	@note cstr of Fixture is called before test and dstr of Fixture is called after test
*/
#define CYBOZU_TEST_SETUP_FIXTURE(Fixture) \
Fixture *cybozu_test_local_fixture; \
void cybozu_test_local_init() \
{ \
	cybozu_test_local_fixture = new Fixture(); \
} \
void cybozu_test_local_term() \
{ \
	delete cybozu_test_local_fixture; \
} \
struct cybozu_test_local_fixture_setup_ { \
	cybozu_test_local_fixture_setup_() \
	{ \
		cybozu::test::autoRun.setup(cybozu_test_local_init, cybozu_test_local_term); \
	} \
} cybozu_test_local_fixture_setup_instance_;

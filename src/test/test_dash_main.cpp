// Copyright (c) 2011-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#define BOOST_TEST_MODULE Bitcoin Test Suite

#include "net.h"
#include "stacktraces.h"

#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_monitor.hpp>

std::unique_ptr<CConnman> g_connman;

void Shutdown(void* parg)
{
  exit(EXIT_SUCCESS);
}

void StartShutdown()
{
  exit(EXIT_SUCCESS);
}

bool ShutdownRequested()
{
  return false;
}

template<typename T>
void translate_exception(const T &e)
{
    std::cerr << GetPrettyExceptionStr(std::current_exception()) << std::endl;
    throw;
}

template<typename T>
void register_exception_translator()
{
    boost::unit_test::unit_test_monitor.register_exception_translator<T>(&translate_exception<T>);
}

struct ExceptionInitializer {
    ExceptionInitializer()
    {
        RegisterPrettyTerminateHander();
        RegisterPrettySignalHandlers();

        register_exception_translator<std::exception>();
        register_exception_translator<std::string>();
        register_exception_translator<const char*>();
    }
    ~ExceptionInitializer()
    {
    }
};

BOOST_GLOBAL_FIXTURE(ExceptionInitializer);

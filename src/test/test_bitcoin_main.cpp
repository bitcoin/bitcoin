// Copyright (c) 2011-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#define BOOST_TEST_MODULE Bitcoin Test Suite

#include <net.h>
#include <validation_layer.h>

#include <memory>

#include <boost/test/unit_test.hpp>

std::unique_ptr<CConnman> g_connman;
std::unique_ptr<ValidationLayer> g_validation_layer;
std::unique_ptr<ValidationLayer> g_mempool_layer;

[[noreturn]] void Shutdown(void* parg)
{
  std::exit(EXIT_SUCCESS);
}

[[noreturn]] void StartShutdown()
{
  std::exit(EXIT_SUCCESS);
}

bool ShutdownRequested()
{
  return false;
}

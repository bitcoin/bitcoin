/**********************************************************************
 * Copyright (c) 2014 Pieter Wuille                                   *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <secp256k1.h>
#include <iostream>
#include <iomanip>
#include <sys/time.h>
#include "secp256k1/src/util.h"
#include "thread_pool/thread_pool.hpp"
#include "thread_pool/thread_pool_options.hpp"
#include <future>
#include <functional>
#include <util/time.h>

const int ITERATIONS = 10000;

static double gettimedouble(void) {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_usec * 0.000001 + tv.tv_sec;
}
tp::ThreadPool *threadpool = NULL;
tp::ThreadPoolOptions options;
void print_number(double x) {
  double y = x;
  int c = 0;
  if (y < 0.0) {
    y = -y;
  }
  while (y > 0 && y < 100.0) {
    y *= 10.0;
    c++;
  }
  printf("%.*f", c, x);
}

void run_benchmark(char *name, void(*benchmark)(void*, int), void(*setup)(void*), void(*teardown)(void*), void* data) {
  int count = 10;
  for (int i = 1; i <= count; i += 1) {
    // printf("starting #%d %s for %d iterations\n", i, name, ITERATIONS);
    if (setup != NULL) {
      setup(data);
    }

    double begin = gettimedouble();
    benchmark(data, i);
    double end = gettimedouble();
    double totalUS = (end - begin) * 1000000.0;
    double totalMS = (end - begin) * 1000.0;

    if (teardown != NULL) {
      teardown(data);
    }

    double avgUS = totalUS / (ITERATIONS*i);

    printf("#%d %s (x%d): total ", i, name, ITERATIONS*i);
    print_number(totalMS);
    printf("ms / avg ");
    print_number(avgUS);
    printf("us\n");
  }
}

typedef struct {
  secp256k1_context *ctx;
  unsigned char msg[32];
  unsigned char key[32];
  unsigned char sig[72];
  size_t siglen;
  unsigned char pubkey[33];
  size_t pubkeylen;
} benchmark_verify_t;

static void benchmark_verify(void* arg, int count) {
  benchmark_verify_t* data = (benchmark_verify_t*)arg;
  unsigned char sigData[72];
  int siglen = data->siglen;
  for (int i = 0; i <= ITERATIONS*count; i++) {
    secp256k1_pubkey pubkey;
    secp256k1_ecdsa_signature sig;
	
    memcpy(sigData, data->sig, sizeof(data->sig));
    sigData[siglen - 1] ^= (i & 0xFF);
    sigData[siglen - 2] ^= ((i >> 8) & 0xFF);
    sigData[siglen - 3] ^= ((i >> 16) & 0xFF);
    CHECK(secp256k1_ec_pubkey_parse(data->ctx, &pubkey, data->pubkey, data->pubkeylen) == 1);
    CHECK(secp256k1_ecdsa_signature_parse_der(data->ctx, &sig, sigData, siglen) == 1);
    CHECK(secp256k1_ecdsa_verify(data->ctx, &sig, data->msg, &pubkey) == (i == 0));
    sigData[siglen - 1] ^= (i & 0xFF);
    sigData[siglen - 2] ^= ((i >> 8) & 0xFF);
    sigData[siglen - 3] ^= ((i >> 16) & 0xFF);
  }
}
static void benchmark_verify_parallel(void* arg, int count) {  
  threadpool = new tp::ThreadPool(options);

  int index = 0;
  std::vector<std::future<void>> workers;
  while (index <= ITERATIONS*count) {
    benchmark_verify_t* data = (benchmark_verify_t*)arg;
    // define a task for the worker to process
    std::packaged_task<void()> task([data, index]() {
      unsigned char sigData[72];
      std::copy(data->sig, data->sig + sizeof(data->sig), sigData);

      int siglen = data->siglen;      
      secp256k1_pubkey pubkey;
      secp256k1_ecdsa_signature sig;
    
      sigData[siglen - 1] ^= (index & 0xFF);
      sigData[siglen - 2] ^= ((index >> 8) & 0xFF);
      sigData[siglen - 3] ^= ((index >> 16) & 0xFF);
      CHECK(secp256k1_ec_pubkey_parse(data->ctx, &pubkey, data->pubkey, data->pubkeylen) == 1);
      CHECK(secp256k1_ecdsa_signature_parse_der(data->ctx, &sig, sigData, siglen) == 1);
      CHECK(secp256k1_ecdsa_verify(data->ctx, &sig, data->msg, &pubkey) == (index == 0));
      sigData[siglen - 1] ^= (index & 0xFF);
      sigData[siglen - 2] ^= ((index >> 8) & 0xFF);
      sigData[siglen - 3] ^= ((index >> 16) & 0xFF);
    });
    
    std::future<void> future = task.get_future();
    workers.push_back(std::move(future));

    // retry if the threadpool queue is full and return error if we can't post
    bool isThreadPosted = false;
    while (!isThreadPosted) {
      isThreadPosted = threadpool->tryPost(task);
      if (isThreadPosted) {
        index += 1;
        if (index >= ITERATIONS) break;
      } else {
		  std::this_thread::sleep_for(std::chrono::milliseconds(0));
      }
    }
  }

  //wait for responses
  for (auto& future : workers) { 
    future.get();
  }

  delete threadpool;
}
int main(int argc, char* argv[])
{
  options.setQueueSize(65536);

  printf("cores: %d threads: %zu queue: %zu\n", std::thread::hardware_concurrency(), options.threadCount(), options.queueSize());

  int i;
  secp256k1_pubkey pubkey;
  secp256k1_ecdsa_signature sig;
  benchmark_verify_t data;

  data.ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);

  for (i = 0; i < 32; i++) {
    data.msg[i] = 1 + i;
  }
  for (i = 0; i < 32; i++) {
    data.key[i] = 33 + i;
  }
  data.siglen = 72;
  CHECK(secp256k1_ecdsa_sign(data.ctx, &sig, data.msg, data.key, NULL, NULL));
  CHECK(secp256k1_ecdsa_signature_serialize_der(data.ctx, data.sig, &data.siglen, &sig));
  CHECK(secp256k1_ec_pubkey_create(data.ctx, &pubkey, data.key));
  data.pubkeylen = 33;
  CHECK(secp256k1_ec_pubkey_serialize(data.ctx, data.pubkey, &data.pubkeylen, &pubkey, SECP256K1_EC_COMPRESSED) == 1);

  run_benchmark((char *) "ecdsa_verify", benchmark_verify, NULL, NULL, &data);
  run_benchmark((char *) "ecdsa_verify_parallel", benchmark_verify_parallel, NULL, NULL, &data);
  
  secp256k1_context_destroy(data.ctx);
  return 0;
}

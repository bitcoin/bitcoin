// bench.h - written and placed in the public domain by Wei Dai

#ifndef CRYPTOPP_BENCH_H
#define CRYPTOPP_BENCH_H

#include "cryptlib.h"

extern const double CLOCK_TICKS_PER_SECOND;

void BenchmarkAll(double t, double hertz);
void BenchmarkAll2(double t, double hertz);

#endif

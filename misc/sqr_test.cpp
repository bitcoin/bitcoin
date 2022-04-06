#include <stdint.h>
#include <mcl/gmp_util.hpp>
#include <iostream>

#include "sqr.h"


void dump(const uint32_t *x, size_t N)
{
	for (size_t i = 0; i < N; i++) {
		if (i > 0) printf("_");
		printf("%08x", x[N - 1 - i]);
	}
	printf("\n");
}

int main()
	try
{
	const size_t N = 8;
	mpz_class mx("0x66");
	uint32_t x[N+N], xx[N*2];
	std::cout << std::hex;
	for (int i = 0; i < 100; i++) {
		mcl::gmp::getArray(x, CYBOZU_NUM_OF_ARRAY(x), mx);
		sqrPre(xx, x);
		mpz_class y;
		mcl::gmp::setArray(y, xx, N*2);
		mpz_class mxx = mx * mx;
		printf("x =0x"); dump(x, N);
		if (y != mxx) {
			puts("err");
			uint32_t ok[N*2];
			mcl::gmp::getArray(ok, N*2, mxx);
			printf("x =0x"); dump(x, N);
			printf("xx=0x"); dump(xx, N*2);
			printf("ok=0x"); dump(ok, N*2);
			return 1;
		}
		mx = ((mxx>>3)+7) % (mpz_class(1) << (N * 32));
	}
	puts("ok");
} catch (std::exception& e) {
	printf("err %s\n", e.what());
	return 1;
}

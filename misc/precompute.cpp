#include <mcl/bn256.hpp>
#include <iostream>

using namespace mcl::bn;

int main()
{
	initPairing(mcl::BN254);
	G2 Q;
	mapToG2(Q, 1);
	std::vector<Fp6> Qcoeff;
	precomputeG2(Qcoeff, Q);
	puts("#if MCL_SIZEOF_UNIT == 8");
	puts("static const uint64_t QcoeffTblBN254[][6][4] = {");
	for (size_t i = 0; i < Qcoeff.size(); i++) {
		const Fp6& x6 = Qcoeff[i];
		puts("\t{");
		for (size_t j = 0; j < 6; j++) {
			printf("\t\t{");
			const Fp& x = x6.getFp0()[j];
			for (size_t k = 0; k < 4; k++) {
				printf("0x%016llxull,", (unsigned long long)x.getUnit()[k]);
			}
			puts("},");
		}
		puts("\t},");
	}
	puts("};");
	puts("#endif");
}

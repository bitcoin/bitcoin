#include <bls/bls384_256.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

void simpleSample()
{
	blsSecretKey sec;
	blsSecretKeySetDecStr(&sec, "13", 2);
	blsPublicKey pub;
	blsGetPublicKey(&pub, &sec);
	blsSignature sig;
	const char *msg = "abc";
	size_t msgSize = 3;
	blsSign(&sig, &sec, msg, msgSize);
	printf("verify correct message %d\n", blsVerify(&sig, &pub, msg, msgSize));
	printf("verify wrong message %d\n", blsVerify(&sig, &pub, "xyz", msgSize));
}

void k_of_nSample()
{
#define N 5 // you can increase
#define K 3 // fixed
	blsPublicKey mpk;
	blsId ids[N];
	blsSecretKey secs[N];
	blsPublicKey pubs[N];
	blsSignature sigs[N];

	const char *msg = "abc";
	const size_t msgSize = strlen(msg);

	// All ids must be non-zero and different from each other.
	for (int i = 0; i < N; i++) {
		blsIdSetInt(&ids[i], i + 1);
	}

	/*
		A trusted third party distributes N secret keys.
		If you want to avoid it, then see DKG (distributed key generation),
		which is out of the scope of this library.
	*/
	{
		blsSecretKey msk[K];
		for (int i = 0; i < K; i++) {
			blsSecretKeySetByCSPRNG(&msk[i]);
		}
		// share secret key
		for (int i = 0; i < N; i++) {
			blsSecretKeyShare(&secs[i], msk, K, &ids[i]);
		}

		// get master public key
		blsGetPublicKey(&mpk, &msk[0]);

		// each user gets their own public key
		for (int i = 0; i < N; i++) {
			blsGetPublicKey(&pubs[i], &secs[i]);
		}
	}

	// each user signs the message
	for (int i = 0; i < N; i++) {
		blsSign(&sigs[i], &secs[i], msg, msgSize);
	}

	// The master signature can be recovered from any K subset of N sigs.
	{
		assert(K == 3);
		blsSignature subSigs[K];
		blsId subIds[K];
		for (int i = 0; i < N; i++) {
			subSigs[0] = sigs[i];
			subIds[0] = ids[i];
			for (int j = i + 1; j < N; j++) {
				subSigs[1] = sigs[j];
				subIds[1] = ids[j];
				for (int k = j + 1; k < N; k++) {
					subSigs[2] = sigs[k];
					subIds[2] = ids[k];
					// recover sig from subSigs[K] and subIds[K]
					blsSignature sig;
					blsSignatureRecover(&sig, subSigs, subIds, K);
					if (!blsVerify(&sig, &mpk, msg, msgSize)) {
						printf("ERR can't recover i=%d, j=%d, k=%d\n", i, j, k);
						return;
					}
				}
			}
		}
		puts("recover test1 is ok");
	}

	// any K-1 of N sigs can't recover
	{
		assert(K == 3);
		blsSignature subSigs[K - 1];
		blsId subIds[K - 1];
		for (int i = 0; i < N; i++) {
			subSigs[0] = sigs[i];
			subIds[0] = ids[i];
			for (int j = i + 1; j < N; j++) {
				subSigs[1] = sigs[j];
				subIds[1] = ids[j];
				// can't recover sig from subSigs[K-1] and subIds[K-1]
				blsSignature sig;
				blsSignatureRecover(&sig, subSigs, subIds, K - 1);
				if (blsVerify(&sig, &mpk, msg, msgSize)) {
					printf("ERR verify must always fail. i=%d, j=%d\n", i, j);
					return;
				}
			}
		}
		puts("recover test2 is ok");
	}
#undef K
#undef N
}

int main()
{
#ifdef BLS_ETH
	puts("BLS_ETH mode");
#else
	puts("no BLS_ETH mode");
#endif
	int r = blsInit(MCL_BLS12_381, MCLBN_COMPILED_TIME_VAR);
	if (r != 0) {
		printf("err blsInit %d\n", r);
		return 1;
	}
#ifdef BLS_ETH
	r = blsSetETHmode(BLS_ETH_MODE_DRAFT_07);
	if (r != 0) {
		printf("err blsSetETHmode %d\n", r);
		return 1;
	}
#endif
	simpleSample();
	k_of_nSample();
	return 0;
}

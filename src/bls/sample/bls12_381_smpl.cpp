#include <bls/bls384_256.h>
#include <string.h>
#include <stdio.h>

int main()
{
	blsSecretKey sec;
	blsPublicKey pub;
	blsSignature sig;
	const char *msg = "abc";
	const size_t msgSize = strlen(msg);
	int ret = blsInit(MCL_BLS12_381, MCLBN_COMPILED_TIME_VAR);
	if (ret) {
		printf("err %d\n", ret);
		return 1;
	}
	blsSecretKeySetByCSPRNG(&sec);
	blsGetPublicKey(&pub, &sec);
	blsSign(&sig, &sec, msg, msgSize);
	printf("verify %d\n", blsVerify(&sig, &pub, msg, msgSize));
}

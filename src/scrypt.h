#ifndef SCRYPT_H
#define SCRYPT_H

#ifdef __cplusplus
extern "C" {
#endif

const int SCRYPT_SCRATCHPAD_SIZE = 131072 + 63;

void scrypt_1024_1_1_256_sp(const char *input, char *output, char *scratchpad);
void scrypt_1024_1_1_256(const char *input, char *output);

#ifdef __cplusplus
}
#endif

#endif

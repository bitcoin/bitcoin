
/* BASE64 libraries used internally - should not need to be packaged */
#include <stddef.h>
#define b64_encode_len(A) ((A+2)/3 * 4 + 1)
#define b64_decode_len(A) (A / 4 * 3 + 2)

int	libscrypt_b64_encode(unsigned char const *src, size_t srclength, 
        /*@out@*/ char *target, size_t targetsize);
int	libscrypt_b64_decode(char const *src, /*@out@*/ unsigned char *target, 
        size_t targetsize);

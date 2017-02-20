/* DO NOT EDIT: automatically built by dist/s_include. */
#ifndef _hmac_ext_h_
#define _hmac_ext_h_

#if defined(__cplusplus)
extern "C" {
#endif

void __db_chksum __P((void *, u_int8_t *, size_t, u_int8_t *, u_int8_t *));
void __db_derive_mac __P((u_int8_t *, size_t, u_int8_t *));
int __db_check_chksum __P((ENV *, void *, DB_CIPHER *, u_int8_t *, void *, size_t, int));
void __db_SHA1Transform __P((u_int32_t *, unsigned char *));
void __db_SHA1Init __P((SHA1_CTX *));
void __db_SHA1Update __P((SHA1_CTX *, unsigned char *, size_t));
void __db_SHA1Final __P((unsigned char *, SHA1_CTX *));

#if defined(__cplusplus)
}
#endif
#endif /* !_hmac_ext_h_ */

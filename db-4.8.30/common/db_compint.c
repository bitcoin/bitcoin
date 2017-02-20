/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996-2009 Oracle.  All rights reserved.
 */

#include "db_config.h"

#include "db_int.h"

#ifdef HAVE_COMPRESSION

/*
 * Integer compression
 *
 *  First byte | Next | Maximum
 *  byte       | bytes| value
 * ------------+------+---------------------------------------------------------
 * [0 xxxxxxx] | 0    | 2^7 - 1
 * [10 xxxxxx] | 1    | 2^14 + 2^7 - 1
 * [110 xxxxx] | 2    | 2^21 + 2^14 + 2^7 - 1
 * [1110 xxxx] | 3    | 2^28 + 2^21 + 2^14 + 2^7 - 1
 * [11110 xxx] | 4    | 2^35 + 2^28 + 2^21 + 2^14 + 2^7 - 1
 * [11111 000] | 5    | 2^40 + 2^35 + 2^28 + 2^21 + 2^14 + 2^7 - 1
 * [11111 001] | 6    | 2^48 + 2^40 + 2^35 + 2^28 + 2^21 + 2^14 + 2^7 - 1
 * [11111 010] | 7    | 2^56 + 2^48 + 2^40 + 2^35 + 2^28 + 2^21 + 2^14 + 2^7 - 1
 * [11111 011] | 8    | 2^64 + 2^56 + 2^48 + 2^40 + 2^35 + 2^28 + 2^21 + 2^14 +
 *	       |      |	2^7 - 1
 *
 * NOTE: this compression algorithm depends
 * on big-endian order, so swap if necessary.
 *
 */

#define	CMP_INT_1BYTE_MAX 0x7F
#define	CMP_INT_2BYTE_MAX 0x407F
#define	CMP_INT_3BYTE_MAX 0x20407F
#define	CMP_INT_4BYTE_MAX 0x1020407F

#if defined(_MSC_VER) && _MSC_VER < 1300
#define	CMP_INT_5BYTE_MAX 0x081020407Fi64
#define	CMP_INT_6BYTE_MAX 0x01081020407Fi64
#define	CMP_INT_7BYTE_MAX 0x0101081020407Fi64
#define	CMP_INT_8BYTE_MAX 0x010101081020407Fi64
#else
#define	CMP_INT_5BYTE_MAX 0x081020407FLL
#define	CMP_INT_6BYTE_MAX 0x01081020407FLL
#define	CMP_INT_7BYTE_MAX 0x0101081020407FLL
#define	CMP_INT_8BYTE_MAX 0x010101081020407FLL
#endif

#define	CMP_INT_2BYTE_VAL 0x80
#define	CMP_INT_3BYTE_VAL 0xC0
#define	CMP_INT_4BYTE_VAL 0xE0
#define	CMP_INT_5BYTE_VAL 0xF0
#define	CMP_INT_6BYTE_VAL 0xF8
#define	CMP_INT_7BYTE_VAL 0xF9
#define	CMP_INT_8BYTE_VAL 0xFA
#define	CMP_INT_9BYTE_VAL 0xFB
/* CMP_INT_SPARE_VAL is defined in db_int.h */

#define	CMP_INT_2BYTE_MASK 0x3F
#define	CMP_INT_3BYTE_MASK 0x1F
#define	CMP_INT_4BYTE_MASK 0x0F
#define	CMP_INT_5BYTE_MASK 0x07

static const u_int8_t __db_marshaled_int_size[] = {
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,

	0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
	0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
	0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
	0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
	0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
	0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
	0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
	0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,

	0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
	0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
	0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
	0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,

	0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
	0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,

	0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
	0x06, 0x07, 0x08, 0x09, 0xFF, 0xFF, 0xFF, 0xFF
};

/*
 * __db_compress_count_int --
 *	Return the number of bytes that the compressed version
 *	of the argument will occupy.
 *
 * PUBLIC: u_int32_t __db_compress_count_int __P((u_int64_t));
 */
u_int32_t
__db_compress_count_int(i)
	u_int64_t i;
{
	if (i <= CMP_INT_1BYTE_MAX)
		return 1;
	else if (i <= CMP_INT_2BYTE_MAX)
		return 2;
	else if (i <= CMP_INT_3BYTE_MAX)
		return 3;
	else if (i <= CMP_INT_4BYTE_MAX)
		return 4;
	else if (i <= CMP_INT_5BYTE_MAX)
		return 5;
	else if (i <= CMP_INT_6BYTE_MAX)
		return 6;
	else if (i <= CMP_INT_7BYTE_MAX)
		return 7;
	else if (i <= CMP_INT_8BYTE_MAX)
		return 8;
	else
		return 9;
}

/*
 * __db_compress_int --
 *	Compresses the integer into the buffer, returning the number of
 *	bytes occupied.
 *
 * PUBLIC: int __db_compress_int __P((u_int8_t *, u_int64_t));
 */
int
__db_compress_int(buf, i)
	u_int8_t *buf;
	u_int64_t i;
{
	if (i <= CMP_INT_1BYTE_MAX) {
		/* no swapping for one byte value */
		buf[0] = (u_int8_t)i;
		return 1;
	} else {
		u_int8_t *p = (u_int8_t*)&i;
		if (i <= CMP_INT_2BYTE_MAX) {
			i -= CMP_INT_1BYTE_MAX + 1;
			if (__db_isbigendian() != 0) {
				buf[0] = p[6] | CMP_INT_2BYTE_VAL;
				buf[1] = p[7];
			} else {
				buf[0] = p[1] | CMP_INT_2BYTE_VAL;
				buf[1] = p[0];
			}
			return 2;
		} else if (i <= CMP_INT_3BYTE_MAX) {
			i -= CMP_INT_2BYTE_MAX + 1;
			if (__db_isbigendian() != 0) {
				buf[0] = p[5] | CMP_INT_3BYTE_VAL;
				buf[1] = p[6];
				buf[2] = p[7];
			} else {
				buf[0] = p[2] | CMP_INT_3BYTE_VAL;
				buf[1] = p[1];
				buf[2] = p[0];
			}
			return 3;
		} else if (i <= CMP_INT_4BYTE_MAX) {
			i -= CMP_INT_3BYTE_MAX + 1;
			if (__db_isbigendian() != 0) {
				buf[0] = p[4] | CMP_INT_4BYTE_VAL;
				buf[1] = p[5];
				buf[2] = p[6];
				buf[3] = p[7];
			} else {
				buf[0] = p[3] | CMP_INT_4BYTE_VAL;
				buf[1] = p[2];
				buf[2] = p[1];
				buf[3] = p[0];
			}
			return 4;
		} else if (i <= CMP_INT_5BYTE_MAX) {
			i -= CMP_INT_4BYTE_MAX + 1;
			if (__db_isbigendian() != 0) {
				buf[0] = p[3] | CMP_INT_5BYTE_VAL;
				buf[1] = p[4];
				buf[2] = p[5];
				buf[3] = p[6];
				buf[4] = p[7];
			} else {
				buf[0] = p[4] | CMP_INT_5BYTE_VAL;
				buf[1] = p[3];
				buf[2] = p[2];
				buf[3] = p[1];
				buf[4] = p[0];
			}
			return 5;
		} else if (i <= CMP_INT_6BYTE_MAX) {
			i -= CMP_INT_5BYTE_MAX + 1;
			if (__db_isbigendian() != 0) {
				buf[0] = CMP_INT_6BYTE_VAL;
				buf[1] = p[3];
				buf[2] = p[4];
				buf[3] = p[5];
				buf[4] = p[6];
				buf[5] = p[7];
			} else {
				buf[0] = CMP_INT_6BYTE_VAL;
				buf[1] = p[4];
				buf[2] = p[3];
				buf[3] = p[2];
				buf[4] = p[1];
				buf[5] = p[0];
			}
			return 6;
		} else if (i <= CMP_INT_7BYTE_MAX) {
			i -= CMP_INT_6BYTE_MAX + 1;
			if (__db_isbigendian() != 0) {
				buf[0] = CMP_INT_7BYTE_VAL;
				buf[1] = p[2];
				buf[2] = p[3];
				buf[3] = p[4];
				buf[4] = p[5];
				buf[5] = p[6];
				buf[6] = p[7];
			} else {
				buf[0] = CMP_INT_7BYTE_VAL;
				buf[1] = p[5];
				buf[2] = p[4];
				buf[3] = p[3];
				buf[4] = p[2];
				buf[5] = p[1];
				buf[6] = p[0];
			}
			return 7;
		} else if (i <= CMP_INT_8BYTE_MAX) {
			i -= CMP_INT_7BYTE_MAX + 1;
			if (__db_isbigendian() != 0) {
				buf[0] = CMP_INT_8BYTE_VAL;
				buf[1] = p[1];
				buf[2] = p[2];
				buf[3] = p[3];
				buf[4] = p[4];
				buf[5] = p[5];
				buf[6] = p[6];
				buf[7] = p[7];
			} else {
				buf[0] = CMP_INT_8BYTE_VAL;
				buf[1] = p[6];
				buf[2] = p[5];
				buf[3] = p[4];
				buf[4] = p[3];
				buf[5] = p[2];
				buf[6] = p[1];
				buf[7] = p[0];
			}
			return 8;
		} else {
			i -= CMP_INT_8BYTE_MAX + 1;
			if (__db_isbigendian() != 0) {
				buf[0] = CMP_INT_9BYTE_VAL;
				buf[1] = p[0];
				buf[2] = p[1];
				buf[3] = p[2];
				buf[4] = p[3];
				buf[5] = p[4];
				buf[6] = p[5];
				buf[7] = p[6];
				buf[8] = p[7];
			} else {
				buf[0] = CMP_INT_9BYTE_VAL;
				buf[1] = p[7];
				buf[2] = p[6];
				buf[3] = p[5];
				buf[4] = p[4];
				buf[5] = p[3];
				buf[6] = p[2];
				buf[7] = p[1];
				buf[8] = p[0];
			}
			return 9;
		}
	}
}

/*
 * __db_decompress_count_int --
 *	Return the number of bytes occupied by the compressed
 *	integer pointed to by buf.
 *
 * PUBLIC: u_int32_t __db_decompress_count_int __P((const u_int8_t *));
 */
u_int32_t
__db_decompress_count_int(buf)
	const u_int8_t *buf;
{
	return __db_marshaled_int_size[*buf];
}

/*
 * __db_decompress_int --
 *	Decompresses the compressed integer pointer to by buf into i,
 *	returning the number of bytes read.
 *
 * PUBLIC: int __db_decompress_int __P((const u_int8_t *, u_int64_t *));
 */
int
__db_decompress_int(buf, i)
	const u_int8_t *buf;
	u_int64_t *i;
{
	int len;
	u_int64_t tmp;
	u_int8_t *p;
	u_int8_t c;

	tmp = 0;
	p = (u_int8_t*)&tmp;
	c = buf[0];
	len = __db_marshaled_int_size[c];

	switch (len) {
	case 1:
		*i = c;
		return 1;
	case 2:
		if (__db_isbigendian() != 0) {
			p[6] = (c & CMP_INT_2BYTE_MASK);
			p[7] = buf[1];
		} else {
			p[1] = (c & CMP_INT_2BYTE_MASK);
			p[0] = buf[1];
		}
		tmp += CMP_INT_1BYTE_MAX + 1;
		break;
	case 3:
		if (__db_isbigendian() != 0) {
			p[5] = (c & CMP_INT_3BYTE_MASK);
			p[6] = buf[1];
			p[7] = buf[2];
		} else {
			p[2] = (c & CMP_INT_3BYTE_MASK);
			p[1] = buf[1];
			p[0] = buf[2];
		}
		tmp += CMP_INT_2BYTE_MAX + 1;
		break;
	case 4:
		if (__db_isbigendian() != 0) {
			p[4] = (c & CMP_INT_4BYTE_MASK);
			p[5] = buf[1];
			p[6] = buf[2];
			p[7] = buf[3];
		} else {
			p[3] = (c & CMP_INT_4BYTE_MASK);
			p[2] = buf[1];
			p[1] = buf[2];
			p[0] = buf[3];
		}
		tmp += CMP_INT_3BYTE_MAX + 1;
		break;
	case 5:
		if (__db_isbigendian() != 0) {
			p[3] = (c & CMP_INT_5BYTE_MASK);
			p[4] = buf[1];
			p[5] = buf[2];
			p[6] = buf[3];
			p[7] = buf[4];
		} else {
			p[4] = (c & CMP_INT_5BYTE_MASK);
			p[3] = buf[1];
			p[2] = buf[2];
			p[1] = buf[3];
			p[0] = buf[4];
		}
		tmp += CMP_INT_4BYTE_MAX + 1;
		break;
	case 6:
		if (__db_isbigendian() != 0) {
			p[3] = buf[1];
			p[4] = buf[2];
			p[5] = buf[3];
			p[6] = buf[4];
			p[7] = buf[5];
		} else {
			p[4] = buf[1];
			p[3] = buf[2];
			p[2] = buf[3];
			p[1] = buf[4];
			p[0] = buf[5];
		}
		tmp += CMP_INT_5BYTE_MAX + 1;
		break;
	case 7:
		if (__db_isbigendian() != 0) {
			p[2] = buf[1];
			p[3] = buf[2];
			p[4] = buf[3];
			p[5] = buf[4];
			p[6] = buf[5];
			p[7] = buf[6];
		} else {
			p[5] = buf[1];
			p[4] = buf[2];
			p[3] = buf[3];
			p[2] = buf[4];
			p[1] = buf[5];
			p[0] = buf[6];
		}
		tmp += CMP_INT_6BYTE_MAX + 1;
		break;
	case 8:
		if (__db_isbigendian() != 0) {
			p[1] = buf[1];
			p[2] = buf[2];
			p[3] = buf[3];
			p[4] = buf[4];
			p[5] = buf[5];
			p[6] = buf[6];
			p[7] = buf[7];
		} else {
			p[6] = buf[1];
			p[5] = buf[2];
			p[4] = buf[3];
			p[3] = buf[4];
			p[2] = buf[5];
			p[1] = buf[6];
			p[0] = buf[7];
		}
		tmp += CMP_INT_7BYTE_MAX + 1;
		break;
	case 9:
		if (__db_isbigendian() != 0) {
			p[0] = buf[1];
			p[1] = buf[2];
			p[2] = buf[3];
			p[3] = buf[4];
			p[4] = buf[5];
			p[5] = buf[6];
			p[6] = buf[7];
			p[7] = buf[8];
		} else {
			p[7] = buf[1];
			p[6] = buf[2];
			p[5] = buf[3];
			p[4] = buf[4];
			p[3] = buf[5];
			p[2] = buf[6];
			p[1] = buf[7];
			p[0] = buf[8];
		}
		tmp += CMP_INT_8BYTE_MAX + 1;
		break;
	default:
		break;
	}

	*i = tmp;
	return len;
}

/*
 * __db_decompress_int32 --
 *	Decompresses the compressed 32 bit integer pointer to by buf into i,
 *	returning the number of bytes read.
 *
 * PUBLIC: int __db_decompress_int32 __P((const u_int8_t *, u_int32_t *));
 */
int
__db_decompress_int32(buf, i)
	const u_int8_t *buf;
	u_int32_t *i;
{
	int len;
	u_int32_t tmp;
	u_int8_t *p;
	u_int8_t c;

	tmp = 0;
	p = (u_int8_t*)&tmp;
	c = buf[0];
	len = __db_marshaled_int_size[c];

	switch (len) {
	case 1:
		*i = c;
		return 1;
	case 2:
		if (__db_isbigendian() != 0) {
			p[2] = (c & CMP_INT_2BYTE_MASK);
			p[3] = buf[1];
		} else {
			p[1] = (c & CMP_INT_2BYTE_MASK);
			p[0] = buf[1];
		}
		tmp += CMP_INT_1BYTE_MAX + 1;
		break;
	case 3:
		if (__db_isbigendian() != 0) {
			p[1] = (c & CMP_INT_3BYTE_MASK);
			p[2] = buf[1];
			p[3] = buf[2];
		} else {
			p[2] = (c & CMP_INT_3BYTE_MASK);
			p[1] = buf[1];
			p[0] = buf[2];
		}
		tmp += CMP_INT_2BYTE_MAX + 1;
		break;
	case 4:
		if (__db_isbigendian() != 0) {
			p[0] = (c & CMP_INT_4BYTE_MASK);
			p[1] = buf[1];
			p[2] = buf[2];
			p[3] = buf[3];
		} else {
			p[3] = (c & CMP_INT_4BYTE_MASK);
			p[2] = buf[1];
			p[1] = buf[2];
			p[0] = buf[3];
		}
		tmp += CMP_INT_3BYTE_MAX + 1;
		break;
	case 5:
		if (__db_isbigendian() != 0) {
			p[0] = buf[1];
			p[1] = buf[2];
			p[2] = buf[3];
			p[3] = buf[4];
		} else {
			p[3] = buf[1];
			p[2] = buf[2];
			p[1] = buf[3];
			p[0] = buf[4];
		}
		tmp += CMP_INT_4BYTE_MAX + 1;
		break;
	default:
		break;
	}

	*i = tmp;
	return len;
}

#endif

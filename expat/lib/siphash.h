/* ==========================================================================
 * siphash.h - SipHash-2-4 in a single header file
 * --------------------------------------------------------------------------
 * Derived by William Ahern from the reference implementation[1] published[2]
 * by Jean-Philippe Aumasson and Daniel J. Berstein.
 * Minimal changes by Sebastian Pipping and Victor Stinner on top, see below.
 * Licensed under the CC0 Public Domain Dedication license.
 *
 * 1. https://www.131002.net/siphash/siphash24.c
 * 2. https://www.131002.net/siphash/
 * --------------------------------------------------------------------------
 * HISTORY:
 *
 * 2017-07-05  (Sebastian Pipping)
 *   - Address C89 issues:
 *     - Resolve use of uint64_t and ULL integer constants by moving to
 *       a pair of uint32_t's
 *   - Add const qualifiers at two places
 *   - Ensure <=80 characters line length (assuming tab width 4)
 *
 * 2017-06-23  (Victor Stinner)
 *   - Address Win64 compile warnings
 *
 * 2017-06-18  (Sebastian Pipping)
 *   - Clarify license note in the header
 *   - Address C89 issues:
 *     - Stop using inline keyword (and let compiler decide)
 *     - Replace _Bool by int
 *     - Turn macro siphash24 into a function
 *     - Address invalid conversion (void pointer) by explicit cast
 *   - Address lack of stdint.h for Visual Studio 2003 to 2008
 *   - Always expose sip24_valid (for self-tests)
 *
 * 2012-11-04 - Born.  (William Ahern)
 * --------------------------------------------------------------------------
 * USAGE:
 *
 * SipHash-2-4 takes as input two 64-bit words as the key, some number of
 * message bytes, and outputs a 64-bit word as the message digest. This
 * implementation employs two data structures: a struct sipkey for
 * representing the key, and a struct siphash for representing the hash
 * state.
 *
 * For converting a 16-byte unsigned char array to a key, use either the
 * macro sip_keyof or the routine sip_tokey. The former instantiates a
 * compound literal key, while the latter requires a key object as a
 * parameter.
 *
 * 	unsigned char secret[16];
 * 	arc4random_buf(secret, sizeof secret);
 * 	struct sipkey *key = sip_keyof(secret);
 *
 * For hashing a message, use either the convenience macro siphash24 or the
 * routines sip24_init, sip24_update, and sip24_final.
 *
 * 	struct siphash state;
 * 	void *msg;
 * 	size_t len;
 * 	split_uint64_t hash;
 *
 * 	sip24_init(&state, key);
 * 	sip24_update(&state, msg, len);
 * 	hash = sip24_final(&state);
 *
 * or
 *
 * 	hash = siphash24(msg, len, key);
 *
 * To convert the 64-bit hash value to a canonical 8-byte little-endian
 * binary representation, use either the macro sip_binof or the routine
 * sip_tobin. The former instantiates and returns a compound literal array,
 * while the latter requires an array object as a parameter.
 * --------------------------------------------------------------------------
 * NOTES:
 *
 * o Neither sip_keyof, sip_binof, nor siphash24 will work with compilers
 *   lacking compound literal support. Instead, you must use the lower-level
 *   interfaces which take as parameters the temporary state objects.
 *
 * o Uppercase macros may evaluate parameters more than once. Lowercase
 *   macros should not exhibit any such side effects.
 * ==========================================================================
 */
#ifndef SIPHASH_H
#define SIPHASH_H

#include <stddef.h> /* size_t */

#if defined(_WIN32) && defined(_MSC_VER) && (_MSC_VER < 1600)
  /* For vs2003/7.1 up to vs2008/9.0; _MSC_VER 1600 is vs2010/10.0 */
  typedef unsigned __int8   uint8_t;
  typedef unsigned __int32 uint32_t;
#else
 #include <stdint.h> /* uint32_t uint8_t */
#endif


typedef struct _split_uint64_t {
#if defined(WORDS_BIGENDIAN)
	uint32_t high;
	uint32_t low;
#else
	uint32_t low;
	uint32_t high;
#endif  /* defined(WORDS_BIGENDIAN) */
} split_uint64_t;


static split_uint64_t sui64(uint32_t high, uint32_t low) {
	split_uint64_t res;
	res.high = high;
	res.low = low;
	return res;
}

static split_uint64_t sui64_from_char(char c) {
	split_uint64_t res;
	res.high = 0;
	res.low = (unsigned char)c;
	return res;
}

static split_uint64_t sui64_bitwise_or(
		const split_uint64_t a,
		const split_uint64_t b) {
	split_uint64_t res;
	res.high = a.high | b.high;
	res.low = a.low | b.low;
	return res;
}

static split_uint64_t sui64_add(const split_uint64_t a,
		const split_uint64_t b) {
	split_uint64_t res;
	res.high = a.high + b.high + (a.low > (0xffffffffU - b.low));
	res.low = a.low + b.low;
	return res;
}

static split_uint64_t sui64_bitwise_xor(const split_uint64_t a,
		const split_uint64_t b) {
	split_uint64_t res;
	res.high = a.high ^ b.high;
	res.low = a.low ^ b.low;
	return res;
}

static int sui64_equal(const split_uint64_t a, const split_uint64_t b) {
	return ((a.high == b.high) && (a.low == b.low)) ? 1 : 0;
}

static split_uint64_t sui64_shift_left(const split_uint64_t v, size_t bits) {
	if (bits == 0) {
		return v;
	}

	{
		split_uint64_t res;
		if (bits >= 64) {
			res.high = 0;
			res.low = 0;
		} else if (bits < 32) {
			res.high = (v.high << bits) | (v.low >> (32 - bits));
			res.low = v.low << bits;
		} else {
			res.high = v.low << (bits - 32);
			res.low = 0;
		}
		return res;
	}
}

static split_uint64_t sui64_rotate_left(const split_uint64_t v, size_t bits) {
	bits &= 0x3fU;
	if (bits == 0) {
		return v;
	}

	{
		split_uint64_t res;
		if (bits < 32) {
			res.high = (v.high << bits) | (v.low >> (32 - bits));
			res.low = (v.low << bits) | (v.high >> (32 - bits));
		} else if (bits == 32) {
			res.high = v.low;
			res.low = v.high;
		} else {
			res.high = (v.low << (bits - 32)) | (v.high >> (64 - bits));
			res.low = (v.high << (bits - 32)) | (v.low >> (64 - bits));
		}
		return res;
	}
}

static uint32_t sui64_combine(const split_uint64_t v) {
	return v.high ^ v.low;
}


#define SIP_U32TO8_LE(p, v) \
	(p)[0] = (uint8_t)((v) >>  0); (p)[1] = (uint8_t)((v) >>  8); \
	(p)[2] = (uint8_t)((v) >> 16); (p)[3] = (uint8_t)((v) >> 24);

#define SIP_U64TO8_LE(p, v) \
	SIP_U32TO8_LE((p) + 0, v.low); \
	SIP_U32TO8_LE((p) + 4, v.high);

#define SIP_U8TO64_LE(p) \
	sui64( \
		((uint32_t)((p)[4]) << (32 - 32)) | \
		((uint32_t)((p)[5]) << (40 - 32)) | \
		((uint32_t)((p)[6]) << (48 - 32)) | \
		((uint32_t)((p)[7]) << (56 - 32)), \
		((uint32_t)((p)[0]) <<  0) | \
		((uint32_t)((p)[1]) <<  8) | \
		((uint32_t)((p)[2]) << 16) | \
		((uint32_t)((p)[3]) << 24) \
	)


#define SIPHASH_INITIALIZER \
	{ {0, 0}, {0, 0}, {0, 0}, {0, 0}, { 0 }, 0, {0, 0} }

struct siphash {
	split_uint64_t v0, v1, v2, v3;

	unsigned char buf[8], *p;
	split_uint64_t c;
}; /* struct siphash */


#define SIP_KEYLEN 16

struct sipkey {
	split_uint64_t k[2];
}; /* struct sipkey */

#define sip_keyof(k) sip_tokey(&(struct sipkey){ { 0 } }, (k))

static struct sipkey *sip_tokey(struct sipkey *key, const void *src) {
	key->k[0] = SIP_U8TO64_LE((const unsigned char *)src);
	key->k[1] = SIP_U8TO64_LE((const unsigned char *)src + 8);
	return key;
} /* sip_tokey() */


#define sip_binof(v) sip_tobin((unsigned char[8]){ 0 }, (v))

static void *sip_tobin(void *dst, split_uint64_t u64) {
	SIP_U64TO8_LE((unsigned char *)dst, u64);
	return dst;
} /* sip_tobin() */


static void sip_round(struct siphash *H, const int rounds) {
	int i;

	for (i = 0; i < rounds; i++) {
		H->v0 = sui64_add(H->v0, H->v1);
		H->v1 = sui64_rotate_left(H->v1, 13);
		H->v1 = sui64_bitwise_xor(H->v1, H->v0);
		H->v0 = sui64_rotate_left(H->v0, 32);

		H->v2 = sui64_add(H->v2, H->v3);
		H->v3 = sui64_rotate_left(H->v3, 16);
		H->v3 = sui64_bitwise_xor(H->v3, H->v2);

		H->v0 = sui64_add(H->v0, H->v3);
		H->v3 = sui64_rotate_left(H->v3, 21);
		H->v3 = sui64_bitwise_xor(H->v3, H->v0);

		H->v2 = sui64_add(H->v2, H->v1);
		H->v1 = sui64_rotate_left(H->v1, 17);
		H->v1 = sui64_bitwise_xor(H->v1, H->v2);
		H->v2 = sui64_rotate_left(H->v2, 32);
	}
} /* sip_round() */


static struct siphash *sip24_init(struct siphash *H,
		const struct sipkey *key) {
	H->v0 = sui64_bitwise_xor(sui64(0x736f6d65U, 0x70736575U), key->k[0]);
	H->v1 = sui64_bitwise_xor(sui64(0x646f7261U, 0x6e646f6dU), key->k[1]);
	H->v2 = sui64_bitwise_xor(sui64(0x6c796765U, 0x6e657261U), key->k[0]);
	H->v3 = sui64_bitwise_xor(sui64(0x74656462U, 0x79746573U), key->k[1]);

	H->p = H->buf;
	H->c = sui64_from_char(0);

	return H;
} /* sip24_init() */


#define sip_endof(a) (&(a)[sizeof (a) / sizeof *(a)])

static struct siphash *sip24_update(struct siphash *H, const void *src,
		size_t len) {
	const unsigned char *p = (const unsigned char *)src, *pe = p + len;
	split_uint64_t m;

	do {
		while (p < pe && H->p < sip_endof(H->buf))
			*H->p++ = *p++;

		if (H->p < sip_endof(H->buf))
			break;

		m = SIP_U8TO64_LE(H->buf);
		H->v3 = sui64_bitwise_xor(H->v3, m);
		sip_round(H, 2);
		H->v0 = sui64_bitwise_xor(H->v0, m);

		H->p = H->buf;
		H->c = sui64_add(H->c, sui64_from_char(8));
	} while (p < pe);

	return H;
} /* sip24_update() */


static split_uint64_t sip24_final(struct siphash *H) {
	const char left = (char)(H->p - H->buf);
	split_uint64_t b = sui64_shift_left(
			sui64_add(H->c, sui64_from_char(left)), 56);

	switch (left) {
	case 7: b = sui64_bitwise_or(b,
			sui64_shift_left(sui64_from_char(H->buf[6]), 48));
	case 6: b = sui64_bitwise_or(b,
			sui64_shift_left(sui64_from_char(H->buf[5]), 40));
	case 5: b = sui64_bitwise_or(b,
			sui64_shift_left(sui64_from_char(H->buf[4]), 32));
	case 4: b = sui64_bitwise_or(b,
			sui64_shift_left(sui64_from_char(H->buf[3]), 24));
	case 3: b = sui64_bitwise_or(b,
			sui64_shift_left(sui64_from_char(H->buf[2]), 16));
	case 2: b = sui64_bitwise_or(b,
			sui64_shift_left(sui64_from_char(H->buf[1]), 8));
	case 1: b = sui64_bitwise_or(b,
			sui64_shift_left(sui64_from_char(H->buf[0]), 0));
	case 0: break;
	}

	H->v3 = sui64_bitwise_xor(H->v3, b);
	sip_round(H, 2);
	H->v0 = sui64_bitwise_xor(H->v0, b);
	H->v2 = sui64_bitwise_xor(H->v2, sui64_from_char((char)0xff));
	sip_round(H, 4);

	return sui64_bitwise_xor(
		sui64_bitwise_xor(H->v0, H->v1),
		sui64_bitwise_xor(H->v2, H->v3)
	);
} /* sip24_final() */


static split_uint64_t siphash24(const void *src, size_t len,
		const struct sipkey *key) {
	struct siphash state = SIPHASH_INITIALIZER;
	return sip24_final(sip24_update(sip24_init(&state, key), src, len));
} /* siphash24() */


/*
 * SipHash-2-4 output with
 * k = 00 01 02 ...
 * and
 * in = (empty string)
 * in = 00 (1 byte)
 * in = 00 01 (2 bytes)
 * in = 00 01 02 (3 bytes)
 * ...
 * in = 00 01 02 ... 3e (63 bytes)
 */
static int sip24_valid(void) {
	static const unsigned char vectors[64][8] = {
		{ 0x31, 0x0e, 0x0e, 0xdd, 0x47, 0xdb, 0x6f, 0x72, },
		{ 0xfd, 0x67, 0xdc, 0x93, 0xc5, 0x39, 0xf8, 0x74, },
		{ 0x5a, 0x4f, 0xa9, 0xd9, 0x09, 0x80, 0x6c, 0x0d, },
		{ 0x2d, 0x7e, 0xfb, 0xd7, 0x96, 0x66, 0x67, 0x85, },
		{ 0xb7, 0x87, 0x71, 0x27, 0xe0, 0x94, 0x27, 0xcf, },
		{ 0x8d, 0xa6, 0x99, 0xcd, 0x64, 0x55, 0x76, 0x18, },
		{ 0xce, 0xe3, 0xfe, 0x58, 0x6e, 0x46, 0xc9, 0xcb, },
		{ 0x37, 0xd1, 0x01, 0x8b, 0xf5, 0x00, 0x02, 0xab, },
		{ 0x62, 0x24, 0x93, 0x9a, 0x79, 0xf5, 0xf5, 0x93, },
		{ 0xb0, 0xe4, 0xa9, 0x0b, 0xdf, 0x82, 0x00, 0x9e, },
		{ 0xf3, 0xb9, 0xdd, 0x94, 0xc5, 0xbb, 0x5d, 0x7a, },
		{ 0xa7, 0xad, 0x6b, 0x22, 0x46, 0x2f, 0xb3, 0xf4, },
		{ 0xfb, 0xe5, 0x0e, 0x86, 0xbc, 0x8f, 0x1e, 0x75, },
		{ 0x90, 0x3d, 0x84, 0xc0, 0x27, 0x56, 0xea, 0x14, },
		{ 0xee, 0xf2, 0x7a, 0x8e, 0x90, 0xca, 0x23, 0xf7, },
		{ 0xe5, 0x45, 0xbe, 0x49, 0x61, 0xca, 0x29, 0xa1, },
		{ 0xdb, 0x9b, 0xc2, 0x57, 0x7f, 0xcc, 0x2a, 0x3f, },
		{ 0x94, 0x47, 0xbe, 0x2c, 0xf5, 0xe9, 0x9a, 0x69, },
		{ 0x9c, 0xd3, 0x8d, 0x96, 0xf0, 0xb3, 0xc1, 0x4b, },
		{ 0xbd, 0x61, 0x79, 0xa7, 0x1d, 0xc9, 0x6d, 0xbb, },
		{ 0x98, 0xee, 0xa2, 0x1a, 0xf2, 0x5c, 0xd6, 0xbe, },
		{ 0xc7, 0x67, 0x3b, 0x2e, 0xb0, 0xcb, 0xf2, 0xd0, },
		{ 0x88, 0x3e, 0xa3, 0xe3, 0x95, 0x67, 0x53, 0x93, },
		{ 0xc8, 0xce, 0x5c, 0xcd, 0x8c, 0x03, 0x0c, 0xa8, },
		{ 0x94, 0xaf, 0x49, 0xf6, 0xc6, 0x50, 0xad, 0xb8, },
		{ 0xea, 0xb8, 0x85, 0x8a, 0xde, 0x92, 0xe1, 0xbc, },
		{ 0xf3, 0x15, 0xbb, 0x5b, 0xb8, 0x35, 0xd8, 0x17, },
		{ 0xad, 0xcf, 0x6b, 0x07, 0x63, 0x61, 0x2e, 0x2f, },
		{ 0xa5, 0xc9, 0x1d, 0xa7, 0xac, 0xaa, 0x4d, 0xde, },
		{ 0x71, 0x65, 0x95, 0x87, 0x66, 0x50, 0xa2, 0xa6, },
		{ 0x28, 0xef, 0x49, 0x5c, 0x53, 0xa3, 0x87, 0xad, },
		{ 0x42, 0xc3, 0x41, 0xd8, 0xfa, 0x92, 0xd8, 0x32, },
		{ 0xce, 0x7c, 0xf2, 0x72, 0x2f, 0x51, 0x27, 0x71, },
		{ 0xe3, 0x78, 0x59, 0xf9, 0x46, 0x23, 0xf3, 0xa7, },
		{ 0x38, 0x12, 0x05, 0xbb, 0x1a, 0xb0, 0xe0, 0x12, },
		{ 0xae, 0x97, 0xa1, 0x0f, 0xd4, 0x34, 0xe0, 0x15, },
		{ 0xb4, 0xa3, 0x15, 0x08, 0xbe, 0xff, 0x4d, 0x31, },
		{ 0x81, 0x39, 0x62, 0x29, 0xf0, 0x90, 0x79, 0x02, },
		{ 0x4d, 0x0c, 0xf4, 0x9e, 0xe5, 0xd4, 0xdc, 0xca, },
		{ 0x5c, 0x73, 0x33, 0x6a, 0x76, 0xd8, 0xbf, 0x9a, },
		{ 0xd0, 0xa7, 0x04, 0x53, 0x6b, 0xa9, 0x3e, 0x0e, },
		{ 0x92, 0x59, 0x58, 0xfc, 0xd6, 0x42, 0x0c, 0xad, },
		{ 0xa9, 0x15, 0xc2, 0x9b, 0xc8, 0x06, 0x73, 0x18, },
		{ 0x95, 0x2b, 0x79, 0xf3, 0xbc, 0x0a, 0xa6, 0xd4, },
		{ 0xf2, 0x1d, 0xf2, 0xe4, 0x1d, 0x45, 0x35, 0xf9, },
		{ 0x87, 0x57, 0x75, 0x19, 0x04, 0x8f, 0x53, 0xa9, },
		{ 0x10, 0xa5, 0x6c, 0xf5, 0xdf, 0xcd, 0x9a, 0xdb, },
		{ 0xeb, 0x75, 0x09, 0x5c, 0xcd, 0x98, 0x6c, 0xd0, },
		{ 0x51, 0xa9, 0xcb, 0x9e, 0xcb, 0xa3, 0x12, 0xe6, },
		{ 0x96, 0xaf, 0xad, 0xfc, 0x2c, 0xe6, 0x66, 0xc7, },
		{ 0x72, 0xfe, 0x52, 0x97, 0x5a, 0x43, 0x64, 0xee, },
		{ 0x5a, 0x16, 0x45, 0xb2, 0x76, 0xd5, 0x92, 0xa1, },
		{ 0xb2, 0x74, 0xcb, 0x8e, 0xbf, 0x87, 0x87, 0x0a, },
		{ 0x6f, 0x9b, 0xb4, 0x20, 0x3d, 0xe7, 0xb3, 0x81, },
		{ 0xea, 0xec, 0xb2, 0xa3, 0x0b, 0x22, 0xa8, 0x7f, },
		{ 0x99, 0x24, 0xa4, 0x3c, 0xc1, 0x31, 0x57, 0x24, },
		{ 0xbd, 0x83, 0x8d, 0x3a, 0xaf, 0xbf, 0x8d, 0xb7, },
		{ 0x0b, 0x1a, 0x2a, 0x32, 0x65, 0xd5, 0x1a, 0xea, },
		{ 0x13, 0x50, 0x79, 0xa3, 0x23, 0x1c, 0xe6, 0x60, },
		{ 0x93, 0x2b, 0x28, 0x46, 0xe4, 0xd7, 0x06, 0x66, },
		{ 0xe1, 0x91, 0x5f, 0x5c, 0xb1, 0xec, 0xa4, 0x6c, },
		{ 0xf3, 0x25, 0x96, 0x5c, 0xa1, 0x6d, 0x62, 0x9f, },
		{ 0x57, 0x5f, 0xf2, 0x8e, 0x60, 0x38, 0x1b, 0xe5, },
		{ 0x72, 0x45, 0x06, 0xeb, 0x4c, 0x32, 0x8a, 0x95, }
	};
	unsigned char in[64];
	struct sipkey k;
	size_t i;

	sip_tokey(&k, "\000\001\002\003\004\005\006\007\010\011"
			"\012\013\014\015\016\017");

	for (i = 0; i < sizeof in; ++i) {
		in[i] = (unsigned char)i;

		if (! sui64_equal(siphash24(in, i, &k), SIP_U8TO64_LE(vectors[i])))
			return 0;
	}

	return 1;
} /* sip24_valid() */


#if SIPHASH_MAIN

#include <stdio.h>

int main(void) {
	const int ok = sip24_valid();

	if (ok)
		puts("OK");
	else
		puts("FAIL");

	return !ok;
} /* main() */

#endif /* SIPHASH_MAIN */


#endif /* SIPHASH_H */

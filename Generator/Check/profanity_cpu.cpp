
#include <CL/cl.h>
#include <CL/opencl.h>
#include "profanity_cpu.hpp"


#include "precomp.hpp"

cl_ulong rotate64(cl_ulong x, cl_int i)
{
	if (i < 0) {
		printf("fff\n");
	}
	cl_ulong a = x << i;
	cl_ulong b = x >> (64 - i);
	return a | b;
}

cl_uint rotate32(cl_uint x, cl_int i)
{
	if (i < 0) {
		printf("rotate32\n");
	}
	cl_uint a = x << i;
	cl_uint b = x >> (32 - i);
	return a | b;
}



cl_uint mul_hi(cl_uint a, cl_uint b)
{
	cl_ulong res = (cl_ulong)a * (cl_ulong)b;
	res = res >> 32;
	return (cl_uint)res;
}

typedef union {
	cl_uchar b[200];
	cl_ulong q[25];
	cl_uint d[50];
} ethhash;

#define TH_ELT(t, c0, c1, c2, c3, c4, d0, d1, d2, d3, d4) \
{ \
    t = rotate64((cl_ulong)(d0 ^ d1 ^ d2 ^ d3 ^ d4), (cl_ulong)1) ^ (c0 ^ c1 ^ c2 ^ c3 ^ c4); \
}

#define THETA(s00, s01, s02, s03, s04, \
              s10, s11, s12, s13, s14, \
              s20, s21, s22, s23, s24, \
              s30, s31, s32, s33, s34, \
              s40, s41, s42, s43, s44) \
{ \
    TH_ELT(t0, s40, s41, s42, s43, s44, s10, s11, s12, s13, s14); \
    TH_ELT(t1, s00, s01, s02, s03, s04, s20, s21, s22, s23, s24); \
    TH_ELT(t2, s10, s11, s12, s13, s14, s30, s31, s32, s33, s34); \
    TH_ELT(t3, s20, s21, s22, s23, s24, s40, s41, s42, s43, s44); \
    TH_ELT(t4, s30, s31, s32, s33, s34, s00, s01, s02, s03, s04); \
    s00 ^= t0; s01 ^= t0; s02 ^= t0; s03 ^= t0; s04 ^= t0; \
    s10 ^= t1; s11 ^= t1; s12 ^= t1; s13 ^= t1; s14 ^= t1; \
    s20 ^= t2; s21 ^= t2; s22 ^= t2; s23 ^= t2; s24 ^= t2; \
    s30 ^= t3; s31 ^= t3; s32 ^= t3; s33 ^= t3; s34 ^= t3; \
    s40 ^= t4; s41 ^= t4; s42 ^= t4; s43 ^= t4; s44 ^= t4; \
}

#define RHOPI(s00, s01, s02, s03, s04, \
              s10, s11, s12, s13, s14, \
              s20, s21, s22, s23, s24, \
              s30, s31, s32, s33, s34, \
              s40, s41, s42, s43, s44) \
{ \
	t0  = rotate64(s10, (cl_ulong) 1);  \
	s10 = rotate64(s11, (cl_ulong)44); \
	s11 = rotate64(s41, (cl_ulong)20); \
	s41 = rotate64(s24, (cl_ulong)61); \
	s24 = rotate64(s42, (cl_ulong)39); \
	s42 = rotate64(s04, (cl_ulong)18); \
	s04 = rotate64(s20, (cl_ulong)62); \
	s20 = rotate64(s22, (cl_ulong)43); \
	s22 = rotate64(s32, (cl_ulong)25); \
	s32 = rotate64(s43, (cl_ulong) 8); \
	s43 = rotate64(s34, (cl_ulong)56); \
	s34 = rotate64(s03, (cl_ulong)41); \
	s03 = rotate64(s40, (cl_ulong)27); \
	s40 = rotate64(s44, (cl_ulong)14); \
	s44 = rotate64(s14, (cl_ulong) 2); \
	s14 = rotate64(s31, (cl_ulong)55); \
	s31 = rotate64(s13, (cl_ulong)45); \
	s13 = rotate64(s01, (cl_ulong)36); \
	s01 = rotate64(s30, (cl_ulong)28); \
	s30 = rotate64(s33, (cl_ulong)21); \
	s33 = rotate64(s23, (cl_ulong)15); \
	s23 = rotate64(s12, (cl_ulong)10); \
	s12 = rotate64(s21, (cl_ulong) 6); \
	s21 = rotate64(s02, (cl_ulong) 3); \
	s02 = t0; \
}

#define KHI(s00, s01, s02, s03, s04, \
            s10, s11, s12, s13, s14, \
            s20, s21, s22, s23, s24, \
            s30, s31, s32, s33, s34, \
            s40, s41, s42, s43, s44) \
{ \
    t0 = s00 ^ (~s10 &  s20); \
    t1 = s10 ^ (~s20 &  s30); \
    t2 = s20 ^ (~s30 &  s40); \
    t3 = s30 ^ (~s40 &  s00); \
    t4 = s40 ^ (~s00 &  s10); \
    s00 = t0; s10 = t1; s20 = t2; s30 = t3; s40 = t4; \
    \
    t0 = s01 ^ (~s11 &  s21); \
    t1 = s11 ^ (~s21 &  s31); \
    t2 = s21 ^ (~s31 &  s41); \
    t3 = s31 ^ (~s41 &  s01); \
    t4 = s41 ^ (~s01 &  s11); \
    s01 = t0; s11 = t1; s21 = t2; s31 = t3; s41 = t4; \
    \
    t0 = s02 ^ (~s12 &  s22); \
    t1 = s12 ^ (~s22 &  s32); \
    t2 = s22 ^ (~s32 &  s42); \
    t3 = s32 ^ (~s42 &  s02); \
    t4 = s42 ^ (~s02 &  s12); \
    s02 = t0; s12 = t1; s22 = t2; s32 = t3; s42 = t4; \
    \
    t0 = s03 ^ (~s13 &  s23); \
    t1 = s13 ^ (~s23 &  s33); \
    t2 = s23 ^ (~s33 &  s43); \
    t3 = s33 ^ (~s43 &  s03); \
    t4 = s43 ^ (~s03 &  s13); \
    s03 = t0; s13 = t1; s23 = t2; s33 = t3; s43 = t4; \
    \
    t0 = s04 ^ (~s14 &  s24); \
    t1 = s14 ^ (~s24 &  s34); \
    t2 = s24 ^ (~s34 &  s44); \
    t3 = s34 ^ (~s44 &  s04); \
    t4 = s44 ^ (~s04 &  s14); \
    s04 = t0; s14 = t1; s24 = t2; s34 = t3; s44 = t4; \
}

#define IOTA(s00, r) { s00 ^= r; }

cl_ulong keccakf_rndc[24] = {
	0x0000000000000001, 0x0000000000008082, 0x800000000000808a,
	0x8000000080008000, 0x000000000000808b, 0x0000000080000001,
	0x8000000080008081, 0x8000000000008009, 0x000000000000008a,
	0x0000000000000088, 0x0000000080008009, 0x000000008000000a,
	0x000000008000808b, 0x800000000000008b, 0x8000000000008089,
	0x8000000000008003, 0x8000000000008002, 0x8000000000000080,
	0x000000000000800a, 0x800000008000000a, 0x8000000080008081,
	0x8000000000008080, 0x0000000080000001, 0x8000000080008008
};

// Barely a bottleneck. No need to tinker more.
void sha3_keccakf(ethhash* const h)
{
	cl_ulong* const st = (cl_ulong* const)&h->q;
	h->d[33] ^= 0x80000000;
	cl_ulong t0, t1, t2, t3, t4;

	// Unrolling and removing PI stage gave negligable performance on GTX 1070.
	for (int i = 0; i < 24; ++i) {
		THETA(st[0], st[5], st[10], st[15], st[20], st[1], st[6], st[11], st[16], st[21], st[2], st[7], st[12], st[17], st[22], st[3], st[8], st[13], st[18], st[23], st[4], st[9], st[14], st[19], st[24]);
		RHOPI(st[0], st[5], st[10], st[15], st[20], st[1], st[6], st[11], st[16], st[21], st[2], st[7], st[12], st[17], st[22], st[3], st[8], st[13], st[18], st[23], st[4], st[9], st[14], st[19], st[24]);
		KHI(st[0], st[5], st[10], st[15], st[20], st[1], st[6], st[11], st[16], st[21], st[2], st[7], st[12], st[17], st[22], st[3], st[8], st[13], st[18], st[23], st[4], st[9], st[14], st[19], st[24]);
		IOTA(st[0], keccakf_rndc[i]);
	}

}


/* ------------------------------------------------------------------------ */
/* Multiprecision functions                                                 */
/* ------------------------------------------------------------------------ */

#define bswap32(n) (rotate32(n & 0x00FF00FF, 24U)|(rotate32(n, 8U) & 0x00FF00FF))



// mod              = 0xfffffffffffffffffffffffffffffffffffffffffffffffffffffffefffffc2f
const mp_number mod = { {0xfffffc2f, 0xfffffffe, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff} };

// tripleNegativeGx = 0x92c4cc831269ccfaff1ed83e946adeeaf82c096e76958573f2287becbb17b196
const mp_number tripleNegativeGx = { {0xbb17b196, 0xf2287bec, 0x76958573, 0xf82c096e, 0x946adeea, 0xff1ed83e, 0x1269ccfa, 0x92c4cc83 } };

// doubleNegativeGy = 0x6f8a4b11b2b8773544b60807e3ddeeae05d0976eb2f557ccc7705edf09de52bf
const mp_number doubleNegativeGy = { {0x09de52bf, 0xc7705edf, 0xb2f557cc, 0x05d0976e, 0xe3ddeeae, 0x44b60807, 0xb2b87735, 0x6f8a4b11} };

// negativeGy       = 0xb7c52588d95c3b9aa25b0403f1eef75702e84bb7597aabe663b82f6f04ef2777
const mp_number negativeGy = { {0x04ef2777, 0x63b82f6f, 0x597aabe6, 0x02e84bb7, 0xf1eef757, 0xa25b0403, 0xd95c3b9a, 0xb7c52588 } };


const point negativeG = { { 0x16f81798, 0x59f2815b, 0x2dce28d9, 0x029bfcdb, 0xce870b07, 0x55a06295, 0xf9dcbbac, 0x79be667e },{0x04ef2777, 0x63b82f6f, 0x597aabe6, 0x02e84bb7, 0xf1eef757, 0xa25b0403, 0xd95c3b9a, 0xb7c52588 } };

// Multiprecision subtraction. Underflow signalled via return value.
mp_word mp_sub(mp_number* const r, const mp_number* const a, const mp_number* const b) {
	mp_word t, c = 0;

	for (mp_word i = 0; i < MP_NWORDS; ++i) {
		t = a->d[i] - b->d[i] - c;
		c = t > a->d[i] ? 1 : (t == a->d[i] ? c : 0);

		r->d[i] = t;
	}

	return c;
}

// Multiprecision subtraction of the modulus saved in mod. Underflow signalled via return value.
mp_word mp_sub_mod(mp_number* const r) {
	mp_number mod = { {0xfffffc2f, 0xfffffffe, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff} };

	mp_word t, c = 0;

	for (mp_word i = 0; i < MP_NWORDS; ++i) {
		t = r->d[i] - mod.d[i] - c;
		c = t > r->d[i] ? 1 : (t == r->d[i] ? c : 0);

		r->d[i] = t;
	}

	return c;
}

// Multiprecision subtraction modulo M, M = mod.
// This function is often also used for additions by subtracting a negative number. I've chosen
// to do this because:
//   1. It's easier to re-use an already existing function
//   2. A modular addition would have more overhead since it has to determine if the result of
//      the addition (r) is in the gap M <= r < 2^256. This overhead doesn't exist in a
//      subtraction. We immediately know at the end of a subtraction if we had underflow
//      or not by inspecting the carry value. M refers to the modulus saved in variable mod.
void mp_mod_sub(mp_number* const r, const mp_number* const a, const mp_number* const b) {
	mp_word i, t, c = 0;

	for (i = 0; i < MP_NWORDS; ++i) {
		t = a->d[i] - b->d[i] - c;
		c = t < a->d[i] ? 0 : (t == a->d[i] ? c : 1);

		r->d[i] = t;
	}

	if (c) {
		c = 0;
		for (i = 0; i < MP_NWORDS; ++i) {
			r->d[i] += mod.d[i] + c;
			c = r->d[i] < mod.d[i] ? 1 : (r->d[i] == mod.d[i] ? c : 0);
		}
	}
}

// Multiprecision subtraction modulo M from a constant number.
// I made this in the belief that using constant address space instead of private address space for any
// constant numbers would lead to increase in performance. Judges are still out on this one.
void mp_mod_sub_const(mp_number* const r, const mp_number* const a, const mp_number* const b) {
	mp_word i, t, c = 0;

	for (i = 0; i < MP_NWORDS; ++i) {
		t = a->d[i] - b->d[i] - c;
		c = t < a->d[i] ? 0 : (t == a->d[i] ? c : 1);

		r->d[i] = t;
	}

	if (c) {
		c = 0;
		for (i = 0; i < MP_NWORDS; ++i) {
			r->d[i] += mod.d[i] + c;
			c = r->d[i] < mod.d[i] ? 1 : (r->d[i] == mod.d[i] ? c : 0);
		}
	}
}

// Multiprecision subtraction modulo M of G_x from a number.
// Specialization of mp_mod_sub in hope of performance gain.
void mp_mod_sub_gx(mp_number* const r, const mp_number* const a) {
	mp_word i, t, c = 0;

	t = a->d[0] - 0x16f81798; c = t < a->d[0] ? 0 : (t == a->d[0] ? c : 1); r->d[0] = t;
	t = a->d[1] - 0x59f2815b - c; c = t < a->d[1] ? 0 : (t == a->d[1] ? c : 1); r->d[1] = t;
	t = a->d[2] - 0x2dce28d9 - c; c = t < a->d[2] ? 0 : (t == a->d[2] ? c : 1); r->d[2] = t;
	t = a->d[3] - 0x029bfcdb - c; c = t < a->d[3] ? 0 : (t == a->d[3] ? c : 1); r->d[3] = t;
	t = a->d[4] - 0xce870b07 - c; c = t < a->d[4] ? 0 : (t == a->d[4] ? c : 1); r->d[4] = t;
	t = a->d[5] - 0x55a06295 - c; c = t < a->d[5] ? 0 : (t == a->d[5] ? c : 1); r->d[5] = t;
	t = a->d[6] - 0xf9dcbbac - c; c = t < a->d[6] ? 0 : (t == a->d[6] ? c : 1); r->d[6] = t;
	t = a->d[7] - 0x79be667e - c; c = t < a->d[7] ? 0 : (t == a->d[7] ? c : 1); r->d[7] = t;

	if (c) {
		c = 0;
		for (i = 0; i < MP_NWORDS; ++i) {
			r->d[i] += mod.d[i] + c;
			c = r->d[i] < mod.d[i] ? 1 : (r->d[i] == mod.d[i] ? c : 0);
		}
	}
}

// Multiprecision subtraction modulo M of G_y from a number.
// Specialization of mp_mod_sub in hope of performance gain.
void mp_mod_sub_gy(mp_number* const r, const mp_number* const a) {
	mp_word i, t, c = 0;

	t = a->d[0] - 0xfb10d4b8; c = t < a->d[0] ? 0 : (t == a->d[0] ? c : 1); r->d[0] = t;
	t = a->d[1] - 0x9c47d08f - c; c = t < a->d[1] ? 0 : (t == a->d[1] ? c : 1); r->d[1] = t;
	t = a->d[2] - 0xa6855419 - c; c = t < a->d[2] ? 0 : (t == a->d[2] ? c : 1); r->d[2] = t;
	t = a->d[3] - 0xfd17b448 - c; c = t < a->d[3] ? 0 : (t == a->d[3] ? c : 1); r->d[3] = t;
	t = a->d[4] - 0x0e1108a8 - c; c = t < a->d[4] ? 0 : (t == a->d[4] ? c : 1); r->d[4] = t;
	t = a->d[5] - 0x5da4fbfc - c; c = t < a->d[5] ? 0 : (t == a->d[5] ? c : 1); r->d[5] = t;
	t = a->d[6] - 0x26a3c465 - c; c = t < a->d[6] ? 0 : (t == a->d[6] ? c : 1); r->d[6] = t;
	t = a->d[7] - 0x483ada77 - c; c = t < a->d[7] ? 0 : (t == a->d[7] ? c : 1); r->d[7] = t;

	if (c) {
		c = 0;
		for (i = 0; i < MP_NWORDS; ++i) {
			r->d[i] += mod.d[i] + c;
			c = r->d[i] < mod.d[i] ? 1 : (r->d[i] == mod.d[i] ? c : 0);
		}
	}
}

// Multiprecision addition. Overflow signalled via return value.
mp_word mp_add(mp_number* const r, const mp_number* const a) {
	mp_word c = 0;

	for (mp_word i = 0; i < MP_NWORDS; ++i) {
		r->d[i] += a->d[i] + c;
		c = r->d[i] < a->d[i] ? 1 : (r->d[i] == a->d[i] ? c : 0);
	}

	return c;
}

// Multiprecision addition of the modulus saved in mod. Overflow signalled via return value.
mp_word mp_add_mod(mp_number* const r) {
	mp_word c = 0;

	for (mp_word i = 0; i < MP_NWORDS; ++i) {
		r->d[i] += mod.d[i] + c;
		c = r->d[i] < mod.d[i] ? 1 : (r->d[i] == mod.d[i] ? c : 0);
	}

	return c;
}

// Multiprecision addition of two numbers with one extra word each. Overflow signalled via return value.
mp_word mp_add_more(mp_number* const r, mp_word* const extraR, const mp_number* const a, const mp_word* const extraA) {
	const mp_word c = mp_add(r, a);
	*extraR += *extraA + c;
	return *extraR < *extraA ? 1 : (*extraR == *extraA ? c : 0);
}

// Multiprecision greater than or equal (>=) operator
mp_word mp_gte(const mp_number* const a, const mp_number* const b) {
	mp_word l = 0, g = 0;

	for (mp_word i = 0; i < MP_NWORDS; ++i) {
		if (a->d[i] < b->d[i]) l |= (1 << i);
		if (a->d[i] > b->d[i]) g |= (1 << i);
	}

	return g >= l;
}

// Bit shifts a number with an extra word to the right one step
void mp_shr_extra(mp_number* const r, mp_word* const e) {
	r->d[0] = (r->d[1] << 31) | (r->d[0] >> 1);
	r->d[1] = (r->d[2] << 31) | (r->d[1] >> 1);
	r->d[2] = (r->d[3] << 31) | (r->d[2] >> 1);
	r->d[3] = (r->d[4] << 31) | (r->d[3] >> 1);
	r->d[4] = (r->d[5] << 31) | (r->d[4] >> 1);
	r->d[5] = (r->d[6] << 31) | (r->d[5] >> 1);
	r->d[6] = (r->d[7] << 31) | (r->d[6] >> 1);
	r->d[7] = (*e << 31) | (r->d[7] >> 1);
	*e >>= 1;
}

// Bit shifts a number to the right one step
void mp_shr(mp_number* const r) {
	r->d[0] = (r->d[1] << 31) | (r->d[0] >> 1);
	r->d[1] = (r->d[2] << 31) | (r->d[1] >> 1);
	r->d[2] = (r->d[3] << 31) | (r->d[2] >> 1);
	r->d[3] = (r->d[4] << 31) | (r->d[3] >> 1);
	r->d[4] = (r->d[5] << 31) | (r->d[4] >> 1);
	r->d[5] = (r->d[6] << 31) | (r->d[5] >> 1);
	r->d[6] = (r->d[7] << 31) | (r->d[6] >> 1);
	r->d[7] >>= 1;
}

// Multiplies a number with a word and adds it to an existing number with an extra word, overflow of the extra word is signalled in return value
// This is a special function only used for modular multiplication
mp_word mp_mul_word_add_extra(mp_number* const r, const mp_number* const a, const mp_word w, mp_word* const extra) {
	mp_word cM = 0; // Carry for multiplication
	mp_word cA = 0; // Carry for addition
	mp_word tM = 0; // Temporary storage for multiplication

	for (mp_word i = 0; i < MP_NWORDS; ++i) {
		tM = (a->d[i] * w + cM);
		cM = mul_hi(a->d[i], w) + (tM < cM);

		r->d[i] += tM + cA;
		cA = r->d[i] < tM ? 1 : (r->d[i] == tM ? cA : 0);
	}

	*extra += cM + cA;
	return *extra < cM ? 1 : (*extra == cM ? cA : 0);
}

// Multiplies a number with a word, potentially adds modhigher to it, and then subtracts it from en existing number, no extra words, no overflow
// This is a special function only used for modular multiplication
void mp_mul_mod_word_sub(mp_number* const r, const mp_word w, const bool withModHigher) {
	// Having these numbers declared here instead of using the global values in address space seems to lead
	// to better optimizations by the compiler on my GTX 1070.
	mp_number mod = { { 0xfffffc2f, 0xfffffffe, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff} };
	mp_number modhigher = { {0x00000000, 0xfffffc2f, 0xfffffffe, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff} };

	mp_word cM = 0; // Carry for multiplication
	mp_word cS = 0; // Carry for subtraction
	mp_word tS = 0; // Temporary storage for subtraction
	mp_word tM = 0; // Temporary storage for multiplication
	mp_word cA = 0; // Carry for addition of modhigher

	for (mp_word i = 0; i < MP_NWORDS; ++i) {
		tM = (mod.d[i] * w + cM);
		cM = mul_hi(mod.d[i], w) + (tM < cM);

		tM += (withModHigher ? modhigher.d[i] : 0) + cA;
		cA = tM < (withModHigher ? modhigher.d[i] : 0) ? 1 : (tM == (withModHigher ? modhigher.d[i] : 0) ? cA : 0);

		tS = r->d[i] - tM - cS;
		cS = tS > r->d[i] ? 1 : (tS == r->d[i] ? cS : 0);

		r->d[i] = tS;
	}
}

// Modular multiplication. Based on Algorithm 3 (and a series of hunches) from this article:
// https://www.esat.kuleuven.be/cosic/publications/article-1191.pdf
// When I first implemented it I never encountered a situation where the additional end steps
// of adding or subtracting the modulo was necessary. Maybe it's not for the particular modulo
// used in secp256k1, maybe the overflow bit can be skipped in to avoid 8 subtractions and
// trade it for the final steps? Maybe the final steps are necessary but seldom needed?
// I have no idea, for the time being I'll leave it like this, also see the comments at the
// beginning of this document under the title "Cutting corners".
void mp_mod_mul(mp_number* const r, const mp_number* const X, const mp_number* const Y) {
	mp_number Z = { {0} };
	mp_word extraWord;

	for (int i = MP_NWORDS - 1; i >= 0; --i) {
		// Z = Z * 2^32
		extraWord = Z.d[7]; Z.d[7] = Z.d[6]; Z.d[6] = Z.d[5]; Z.d[5] = Z.d[4]; Z.d[4] = Z.d[3]; Z.d[3] = Z.d[2]; Z.d[2] = Z.d[1]; Z.d[1] = Z.d[0]; Z.d[0] = 0;

		// Z = Z + X * Y_i
		bool overflow = mp_mul_word_add_extra(&Z, X, Y->d[i], &extraWord);

		// Z = Z - qM
		mp_mul_mod_word_sub(&Z, extraWord, overflow);
	}

	*r = Z;
}

// Modular inversion of a number. 
void mp_mod_inverse(mp_number* const r) {
	mp_number A = { { 1 } };
	mp_number C = { { 0 } };
	mp_number v = mod;

	mp_word extraA = 0;
	mp_word extraC = 0;

	while (r->d[0] || r->d[1] || r->d[2] || r->d[3] || r->d[4] || r->d[5] || r->d[6] || r->d[7]) {
		while (!(r->d[0] & 1)) {
			mp_shr(r);
			if (A.d[0] & 1) {
				extraA += mp_add_mod(&A);
			}

			mp_shr_extra(&A, &extraA);
		}

		while (!(v.d[0] & 1)) {
			mp_shr(&v);
			if (C.d[0] & 1) {
				extraC += mp_add_mod(&C);
			}

			mp_shr_extra(&C, &extraC);
		}

		if (mp_gte(r, &v)) {
			mp_sub(r, r, &v);
			mp_add_more(&A, &extraA, &C, &extraC);
		}
		else {
			mp_sub(&v, &v, r);
			mp_add_more(&C, &extraC, &A, &extraA);
		}
	}

	while (extraC) {
		extraC -= mp_sub_mod(&C);
	}

	v = mod;
	mp_sub(r, &v, &C);
}

void printMpNumber(const char* title, mp_number* x) {
	printf("%s: %.8x%.8x%.8x%.8x%.8x%.8x%.8x%.8x\n",
		title,
		x->d[7],
		x->d[6],
		x->d[5],
		x->d[4],
		x->d[3],
		x->d[2],
		x->d[1],
		x->d[0]);
}

// Elliptical point addition
// Does not handle points sharing X coordinate, this is a deliberate design choice.
// For more information on this choice see the beginning of this file.
void point_add(point* const r, point* const p, point* const o) {
	mp_number tmp;
	mp_number newX;
	mp_number newY;

	mp_mod_sub(&tmp, &o->x, &p->x);

	mp_mod_inverse(&tmp);

	mp_mod_sub(&newX, &o->y, &p->y);
	//printMpNumber("MpNumber CPU: ", &newX);
	mp_mod_mul(&tmp, &tmp, &newX);

	mp_mod_mul(&newX, &tmp, &tmp);
	mp_mod_sub(&newX, &newX, &p->x);
	mp_mod_sub(&newX, &newX, &o->x);

	mp_mod_sub(&newY, &p->x, &newX);
	mp_mod_mul(&newY, &newY, &tmp);
	mp_mod_sub(&newY, &newY, &p->y);

	r->x = newX;
	r->y = newY;
}

void printPoint(point* const p) {

	printf("Point CPU: %.8x%.8x%.8x%.8x%.8x%.8x%.8x%.8x%.8x%.8x%.8x%.8x%.8x%.8x%.8x%.8x\n",
		p->x.d[7],
		p->x.d[6],
		p->x.d[5],
		p->x.d[4],
		p->x.d[3],
		p->x.d[2],
		p->x.d[1],
		p->x.d[0],
		p->y.d[7],
		p->y.d[6],
		p->y.d[5],
		p->y.d[4],
		p->y.d[3],
		p->y.d[2],
		p->y.d[1],
		p->y.d[0]);
}

void printPrivateKey(private_key* privKey) {
	printf("Private Key GPU: %.16llx%.16llx%.16llx%.16llx\n",
		privKey->key.s3,
		privKey->key.s2,
		privKey->key.s1,
		privKey->key.s0);
}


void profanity_init_seed(const point* const precomp, point* const p, bool* const pIsFirst, const size_t precompOffset, const cl_ulong seed) {
	point o;

	for (cl_uchar i = 0; i < 8; ++i) {
		const cl_uchar shift = i * 8;
		const cl_uchar byte = (seed >> shift) & 0xFF;

		if (byte) {
			o = precomp[precompOffset + i * 255 + byte - 1];
			if (*pIsFirst) {
				*p = o;
				*pIsFirst = false;
			}
			else {
				point_add(p, p, &o);
			}
		}
	}
}


void printAddress(const char* title, mp_number* x) {
	printf("%s: %.8x%.8x%.8x%.8x%.8x\n",
		title,
		x->d[4],
		x->d[3],
		x->d[2],
		x->d[1],
		x->d[0]);
}

void printUlong4(const char* title, cl_ulong4* x) {
	printf("%s: %.16llx%.16llx%.16llx%.16llx\n",
		title,
		x->w,
		x->z,
		x->y,
		x->x);
}
void printUlong(const char* title, cl_ulong x) {
	printf("%s: %.16llx\n",
		title,
		x);
}
void printPoint(const char* title, point* p) {
	printf("%s", title); printMpNumber("X  ", &p->x);
	printf("%s", title); printMpNumber("Y  ", &p->y);
}




void mul_G(const point* const precomp, point* const p, const size_t precompOffset, const cl_ulong k) {
	point o;
	bool bIsFirst = true;
	for (cl_uchar i = 0; i < 8; ++i) {
		const cl_uchar shift = i * 8;
		const cl_uchar byte = (k >> shift) & 0xFF;

		if (byte) {
			o = precomp[precompOffset + i * 255 + byte - 1];
			if (bIsFirst) {
				*p = o;
				bIsFirst = false;
			}
			else {
				point_add(p, p, &o);
			}
		}
	}
}

const point G = { { 0x16f81798, 0x59f2815b, 0x2dce28d9, 0x029bfcdb, 0xce870b07, 0x55a06295, 0xf9dcbbac, 0x79be667e },{ 0xfb10d4b8, 0x9c47d08f, 0xa6855419, 0xfd17b448, 0x0e1108a8, 0x5da4fbfc, 0x26a3c465, 0x483ada77 } };
const point G2 = { { 0x5c709ee5, 0xabac09b9, 0x8cef3ca7, 0x5c778e4b, 0x95c07cd8, 0x3045406e, 0x41ed7d6d, 0xc6047f94 },{ 0x50cfe52a, 0x236431a9, 0x3266d0e1, 0xf7f63265, 0x466ceaee, 0xa3c58419, 0xa63dc339, 0x1ae168fe } };
const point POS_G2192 = { { 0x2120e2b3, 0x7f3b58fa, 0x7f47f9aa, 0x7a58fdce, 0x4ce6e521, 0xe7be4ae3, 0x1f51bdba, 0xeaa649f2 },{ 0xba5ad93d, 0xd47a5305, 0xf13f7e59, 0x01a6b965, 0x9879aa5a, 0xc69a80f8, 0x5bbbb03a, 0xbe3279ed } };
const point NEG_G2192 = { { 0x45a522f2, 0x2b85acf9, 0x0ec081a6, 0xfe59469a, 0x678655a5, 0x39657f07, 0xa4444fc5, 0x41cd8612 },{ 0x2120e2b3, 0x7f3b58fa, 0x7f47f9aa, 0x7a58fdce, 0x4ce6e521, 0xe7be4ae3, 0x1f51bdba, 0xeaa649f2 } };



void gen_public_key(const point* const precomp, private_key* const pPrivateKey, point* pResult) {
	bool bIsFirst = true;
	// Calculate G^k where k = seed.wzyx (in other words, find the point indicated by the private key represented in seed)
	profanity_init_seed(precomp, (point*)pResult, &bIsFirst, 8 * 255 * 0, pPrivateKey->key.x);
	profanity_init_seed(precomp, (point*)pResult, &bIsFirst, 8 * 255 * 1, pPrivateKey->key.y);
	profanity_init_seed(precomp, (point*)pResult, &bIsFirst, 8 * 255 * 2, pPrivateKey->key.z);
	profanity_init_seed(precomp, (point*)pResult, &bIsFirst, 8 * 255 * 3, pPrivateKey->key.w);
}

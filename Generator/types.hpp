#ifndef HPP_TYPES
#define HPP_TYPES

/* The structs declared in this file should have size/alignment hints
 * to ensure that their representation is identical to that in OpenCL.
 */
#if defined(__APPLE__) || defined(__MACOSX)
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif
#include "defines.h"


#define MP_NWORDS 8

typedef cl_uint mp_word;

//#pragma pack (push, 4)
typedef struct {
	mp_word d[MP_NWORDS];
} mp_number;

typedef struct {
    mp_number x;
    mp_number y;
} point;
//#pragma pack(pop)
//#pragma pack (push, 1)
//#pragma pack(pop)
//#pragma pack (push, 4)
typedef struct {
	mp_number addr;
	mp_number public_key[2];
} hash;

typedef struct {
	mp_number hash;
} inverce;

//GEN ALL KEYS
typedef struct {
	cl_ulong8 key;
} public_key;

typedef struct {
	cl_ulong4 key;
} private_key;

typedef struct {
	cl_uint found;
	cl_uint foundId;
	mp_number addr;
	public_key pub_key;
} result;

//typedef struct {
//	private_key priv_key;
//	public_key pub_key;
//} result_wallet;


typedef struct {
	cl_ulong round;
	cl_uint found;
	cl_uint foundId;
	mp_number addr;
	public_key pub_key;
} result_crack;
//#pragma pack(pop)

typedef struct {
	private_key priv_key;
	public_key pub_key;
	mp_number addr;
} result_profanity;


#endif /* HPP_TYPES */
#ifndef HPP_PROFANITY_CPU
#define HPP_PROFANITY_CPU

#include <string>
#include <CL/cl.h>
#include "types.hpp"
void gen_public_key(const point* const precomp, private_key* const pPrivateKey, point* pResult);
#endif /* HPP_PROFANITY_CPU */

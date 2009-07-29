#include "misc.h"
#include <stdio.h>
#include <sys/types.h>

#ifndef UTIL_H_INCLUDED
#define UTIL_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif


#define ROUNDUP64B(x) (((u_int64_t)(x) + 64 - 1) & ~(64 - 1))

u_int16_t be16 (const u_int8_t *p);
u_int32_t be32 (const u_int8_t *p);
u_int64_t be64 (const u_int8_t *p);

u_int32_t get_dol_size (const u_int8_t *header);

#ifdef __cplusplus
}
#endif

#endif

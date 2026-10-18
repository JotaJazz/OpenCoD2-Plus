#ifndef CLEAN_PC_CGAME_MP_CG_LOCAL_H
#define CLEAN_PC_CGAME_MP_CG_LOCAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "common_types.h"

extern cg_t cgArray[1];
/* `cg` is a const pointer to the single cgArray element, so `cg->field` folds into one
 * absolute access. Deliberately a plain identifier and NOT a macro: the functions that
 * take a `cg_t *cg` PARAMETER must still be able to shadow it under normal C scoping. */
static cg_t * const cg = &cgArray[0];
/* same treatment as `cg` above; the shared cgs storage is defined in bss.c. */
extern unsigned char cgsArray[];
static cgs_t * const cgs = (cgs_t *)cgsArray;
extern centity_t *cg_entities;

#ifdef __cplusplus
}
#endif
#endif

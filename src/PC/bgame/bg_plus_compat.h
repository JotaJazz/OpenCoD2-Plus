/* What the engine's bgame needs to follow CoD2rev-Plus's code (src/bgame)
 * line by line: its constants (bg_plus_consts.h), the inline helpers of its
 * com_math.h and the engine functions it calls under the same names. */
#ifndef COD2_BG_PLUS_COMPAT_H
#define COD2_BG_PLUS_COMPAT_H

#include <math.h>
#include <stdlib.h>
#include "bg_plus_consts.h"

#ifndef assert
#define assert(x) ((void)0)
#endif
#define assertMsg(x, ...) ((void)0)

#define Square( x ) ( ( x ) * ( x ) )
#define ANGLE2SHORT( x )  ( (int)( ( x ) * 65536 / 360 ) & 65535 )
#define SHORT2ANGLE( x )  ( ( x ) * ( 360.0 / 65536 ) )
#define SURF_START_BIT 20
#define SURF_TYPEINDEX( x ) ( ( x >> ( SURF_START_BIT ) ) & 0x1f )
#define ARRAY_COUNT( x ) ( sizeof( x ) / sizeof( x[0] ) )
#ifndef qtrue
#define qtrue 1
#define qfalse 0
#endif
#define PITCH 0
#define YAW 1
#define ROLL 2

static inline float DotProduct(const vec_t *a, const vec_t *b) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; }
static inline float Dot2Product(const vec_t *a, const vec_t *b) { return a[0] * b[0] + a[1] * b[1]; }
static inline void VectorCopy(const vec_t *a, vec_t *b) { b[0] = a[0]; b[1] = a[1]; b[2] = a[2]; }
static inline void Vector2Copy(const vec_t *a, vec_t *b) { b[0] = a[0]; b[1] = a[1]; }
static inline void VectorAdd(const vec_t *a, const vec_t *b, vec_t *c) { c[0] = a[0] + b[0]; c[1] = a[1] + b[1]; c[2] = a[2] + b[2]; }
static inline void Vector2Add(const vec_t *a, const vec_t *b, vec_t *c) { c[0] = a[0] + b[0]; c[1] = a[1] + b[1]; }
static inline void VectorSubtract(const vec_t *a, const vec_t *b, vec_t *c) { c[0] = a[0] - b[0]; c[1] = a[1] - b[1]; c[2] = a[2] - b[2]; }
static inline void Vector2Subtract(const vec_t *a, const vec_t *b, vec_t *c) { c[0] = a[0] - b[0]; c[1] = a[1] - b[1]; }
static inline void VectorScale(const vec_t *v, const float s, vec_t *o) { o[0] = s * v[0]; o[1] = s * v[1]; o[2] = s * v[2]; }
static inline void Vec2Scale(const vec_t *v, const float s, vec_t *o) { o[0] = s * v[0]; o[1] = s * v[1]; }
static inline void VectorMA(const vec_t *v, const float s, const vec_t *b, vec_t *o) { o[0] = s * b[0] + v[0]; o[1] = s * b[1] + v[1]; o[2] = s * b[2] + v[2]; }
static inline void VectorMA2(const vec_t *v, const float s, const vec_t *b, vec_t *o) { o[0] = s * b[0] + v[0]; o[1] = s * b[1] + v[1]; }
static inline void VectorClear(vec_t *v) { v[0] = 0; v[1] = 0; v[2] = 0; }
static inline void VectorSet(vec_t *v, const float x, const float y, const float z) { v[0] = x; v[1] = y; v[2] = z; }
static inline void VectorNegate(const vec_t *a, vec_t *b) { b[0] = -a[0]; b[1] = -a[1]; b[2] = -a[2]; }
static inline float I_fsel(const float x, const float y, const float z) { return x < 0.0 ? z : y; }
static inline float I_fmax(const float x, const float y) { return I_fsel(x - y, x, y); }
static inline float I_fmin(const float x, const float y) { return I_fsel(y - x, x, y); }
static inline float I_fclamp(const float val, const float min, const float max) { return I_fmax(min, I_fmin(val, max)); }
static inline int I_sel(const int x, const int y, const int z) { return x < 0.0 ? z : y; }
static inline int I_max(const int x, const int y) { return I_sel(x - y, x, y); }
static inline int I_min(const int x, const int y) { return I_sel(y - x, x, y); }
static inline int I_clamp(const int val, const int min, const int max) { return I_max(min, I_min(val, max)); }
static inline float I_fabs(const float value) { return fabs(value); }
static inline float I_sqrt(const float value) { return sqrt(value); }
static inline int FloatSign(const float x) { return -2 * (x < 0.0f) + 1; }
static inline float VectorLength(const vec_t *v) { return I_sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]); }
static inline float VectorLengthSquared(const vec_t *v) { return v[0] * v[0] + v[1] * v[1] + v[2] * v[2]; }
static inline float Vec2LengthSq(const vec_t *v) { return v[0] * v[0] + v[1] * v[1]; }
static inline void Vec3Lerp(const vec_t *start, const vec_t *end, float fraction, vec_t *endpos)
{
    endpos[0] = (end[0] - start[0]) * fraction + start[0];
    endpos[1] = (end[1] - start[1]) * fraction + start[1];
    endpos[2] = (end[2] - start[2]) * fraction + start[2];
}

static inline bool Com_BitCheck(const int array[], int bitNum) { return (array[bitNum >> 5] >> (bitNum & 31)) & 1; }
static inline void Com_BitSet(int array[], int bitNum) { array[bitNum >> 5] |= 1 << (bitNum & 31); }
static inline void Com_BitClear(int array[], int bitNum) { array[bitNum >> 5] &= ~(1 << (bitNum & 31)); }

extern const float AngleSubtract(const float a1, const float a2);
extern float DiffTrack(float tgt, float cur, float rate, float deltaTime);
extern const float AngleDelta(const float angle1, const float angle2);
extern const float AngleNormalize180(const float angle);
extern const float AngleNormalize180Accurate(float angle);
extern const float AngleNormalize360Accurate(float angle);
extern void AngleVectors(const vec_t *angles, vec_t *forward, vec_t *right, vec_t *up);
extern const vec_t Vec3Normalize(vec_t *v);
extern const vec_t Vec2Normalize(vec_t *v);
extern const vec_t Vec3NormalizeTo(const vec_t *v, vec_t *out);
extern void Vec3Cross(const vec_t *v0, const vec_t *v1, vec_t *cross);
extern const float vectoyaw(const vec_t *vec);
extern void vectoangles(const vec_t *value1, vec_t *angles);
extern void ProjectPointOnPlane(const vec_t *p, const vec_t *normal, vec_t *dst);
extern float PitchForYawOnNormal(const float fYaw, const vec_t *normal);
extern void AddLeanToPosition(vec_t *position, const float fViewYaw, const float fLeanFrac, const float fViewRoll, const float fLeanDist);
extern float UnGetLeanFraction(const float fFrac);
extern void Sys_SnapVector(vec_t *v);
extern void Com_Printf(const char *fmt, ...);
extern void Com_DPrintf(const char *fmt, ...);
extern char *va(const char *format, ...);

#endif

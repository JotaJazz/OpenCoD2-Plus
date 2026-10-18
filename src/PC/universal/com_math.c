#include "common_types.h"
#include "imports.h"
#include "bytematch.h"

#if !COD2_PLUS_SERVER  /* defined by src/universal/com_math.cpp */
vec3_t bytedirs[162] = { -0.525731027f, 0.0f, 0.850651026f, -0.442862988f, 0.238856003f, 0.864188015f, -0.295242012f, 0.0f, 0.955422997f, -0.309017003f, 0.5f, 0.809017003f, -0.162459999f, 0.26286599f, 0.951056004f, 0.0f, 0.0f, 1.0f, 0.0f, 0.850651026f, 0.525731027f, -0.147621006f, 0.71656698f, 0.681717992f, 0.147621006f, 0.71656698f, 0.681717992f, 0.0f, 0.525731027f, 0.850651026f, 0.309017003f, 0.5f, 0.809017003f, 0.525731027f, 0.0f, 0.850651026f, 0.295242012f, 0.0f, 0.955422997f, 0.442862988f, 0.238856003f, 0.864188015f, 0.162459999f, 0.26286599f, 0.951056004f, -0.681717992f, 0.147621006f, 0.71656698f, -0.809017003f, 0.309017003f, 0.5f, -0.587785006f, 0.425325006f, 0.688190997f, -0.850651026f, 0.525731027f, 0.0f, -0.864188015f, 0.442862988f, 0.238856003f, -0.71656698f, 0.681717992f, 0.147621006f, -0.688190997f, 0.587785006f, 0.425325006f, -0.5f, 0.809017003f, 0.309017003f, -0.238856003f, 0.864188015f, 0.442862988f, -0.425325006f, 0.688190997f, 0.587785006f, -0.71656698f, 0.681717992f, -0.147621006f, -0.5f, 0.809017003f, -0.309017003f, -0.525731027f, 0.850651026f, 0.0f, 0.0f, 0.850651026f, -0.525731027f, -0.238856003f, 0.864188015f, -0.442862988f, 0.0f, 0.955422997f, -0.295242012f, -0.26286599f, 0.951056004f, -0.162459999f, 0.0f, 1.0f, 0.0f, 0.0f, 0.955422997f, 0.295242012f, -0.26286599f, 0.951056004f, 0.162459999f, 0.238856003f, 0.864188015f, 0.442862988f, 0.26286599f, 0.951056004f, 0.162459999f, 0.5f, 0.809017003f, 0.309017003f, 0.238856003f, 0.864188015f, -0.442862988f, 0.26286599f, 0.951056004f, -0.162459999f, 0.5f, 0.809017003f, -0.309017003f, 0.850651026f, 0.525731027f, 0.0f, 0.71656698f, 0.681717992f, 0.147621006f, 0.71656698f, 0.681717992f, -0.147621006f, 0.525731027f, 0.850651026f, 0.0f, 0.425325006f, 0.688190997f, 0.587785006f, 0.864188015f, 0.442862988f, 0.238856003f, 0.688190997f, 0.587785006f, 0.425325006f, 0.809017003f, 0.309017003f, 0.5f, 0.681717992f, 0.147621006f, 0.71656698f, 0.587785006f, 0.425325006f, 0.688190997f, 0.955422997f, 0.295242012f, 0.0f, 1.0f, 0.0f, 0.0f, 0.951056004f, 0.162459999f, 0.26286599f, 0.850651026f, -0.525731027f, 0.0f, 0.955422997f, -0.295242012f, 0.0f, 0.864188015f, -0.442862988f, 0.238856003f, 0.951056004f, -0.162459999f, 0.26286599f, 0.809017003f, -0.309017003f, 0.5f, 0.681717992f, -0.147621006f, 0.71656698f, 0.850651026f, 0.0f, 0.525731027f, 0.864188015f, 0.442862988f, -0.238856003f, 0.809017003f, 0.309017003f, -0.5f, 0.951056004f, 0.162459999f, -0.26286599f, 0.525731027f, 0.0f, -0.850651026f, 0.681717992f, 0.147621006f, -0.71656698f, 0.681717992f, -0.147621006f, -0.71656698f, 0.850651026f, 0.0f, -0.525731027f, 0.809017003f, -0.309017003f, -0.5f, 0.864188015f, -0.442862988f, -0.238856003f, 0.951056004f, -0.162459999f, -0.26286599f, 0.147621006f, 0.71656698f, -0.681717992f, 0.309017003f, 0.5f, -0.809017003f, 0.425325006f, 0.688190997f, -0.587785006f, 0.442862988f, 0.238856003f, -0.864188015f, 0.587785006f, 0.425325006f, -0.688190997f, 0.688190997f, 0.587785006f, -0.425325006f, -0.147621006f, 0.71656698f, -0.681717992f, -0.309017003f, 0.5f, -0.809017003f, 0.0f, 0.525731027f, -0.850651026f, -0.525731027f, 0.0f, -0.850651026f, -0.442862988f, 0.238856003f, -0.864188015f, -0.295242012f, 0.0f, -0.955422997f, -0.162459999f, 0.26286599f, -0.951056004f, 0.0f, 0.0f, -1.0f, 0.295242012f, 0.0f, -0.955422997f, 0.162459999f, 0.26286599f, -0.951056004f, -0.442862988f, -0.238856003f, -0.864188015f, -0.309017003f, -0.5f, -0.809017003f, -0.162459999f, -0.26286599f, -0.951056004f, 0.0f, -0.850651026f, -0.525731027f, -0.147621006f, -0.71656698f, -0.681717992f, 0.147621006f, -0.71656698f, -0.681717992f, 0.0f, -0.525731027f, -0.850651026f, 0.309017003f, -0.5f, -0.809017003f, 0.442862988f, -0.238856003f, -0.864188015f, 0.162459999f, -0.26286599f, -0.951056004f, 0.238856003f, -0.864188015f, -0.442862988f, 0.5f, -0.809017003f, -0.309017003f, 0.425325006f, -0.688190997f, -0.587785006f, 0.71656698f, -0.681717992f, -0.147621006f, 0.688190997f, -0.587785006f, -0.425325006f, 0.587785006f, -0.425325006f, -0.688190997f, 0.0f, -0.955422997f, -0.295242012f, 0.0f, -1.0f, 0.0f, 0.26286599f, -0.951056004f, -0.162459999f, 0.0f, -0.850651026f, 0.525731027f, 0.0f, -0.955422997f, 0.295242012f, 0.238856003f, -0.864188015f, 0.442862988f, 0.26286599f, -0.951056004f, 0.162459999f, 0.5f, -0.809017003f, 0.309017003f, 0.71656698f, -0.681717992f, 0.147621006f, 0.525731027f, -0.850651026f, 0.0f, -0.238856003f, -0.864188015f, -0.442862988f, -0.5f, -0.809017003f, -0.309017003f, -0.26286599f, -0.951056004f, -0.162459999f, -0.850651026f, -0.525731027f, 0.0f, -0.71656698f, -0.681717992f, -0.147621006f, -0.71656698f, -0.681717992f, 0.147621006f, -0.525731027f, -0.850651026f, 0.0f, -0.5f, -0.809017003f, 0.309017003f, -0.238856003f, -0.864188015f, 0.442862988f, -0.26286599f, -0.951056004f, 0.162459999f, -0.864188015f, -0.442862988f, 0.238856003f, -0.809017003f, -0.309017003f, 0.5f, -0.688190997f, -0.587785006f, 0.425325006f, -0.681717992f, -0.147621006f, 0.71656698f, -0.442862988f, -0.238856003f, 0.864188015f, -0.587785006f, -0.425325006f, 0.688190997f, -0.309017003f, -0.5f, 0.809017003f, -0.147621006f, -0.71656698f, 0.681717992f, -0.425325006f, -0.688190997f, 0.587785006f, -0.162459999f, -0.26286599f, 0.951056004f, 0.442862988f, -0.238856003f, 0.864188015f, 0.162459999f, -0.26286599f, 0.951056004f, 0.309017003f, -0.5f, 0.809017003f, 0.147621006f, -0.71656698f, 0.681717992f, 0.0f, -0.525731027f, 0.850651026f, 0.425325006f, -0.688190997f, 0.587785006f, 0.587785006f, -0.425325006f, 0.688190997f, 0.688190997f, -0.587785006f, 0.425325006f, -0.955422997f, 0.295242012f, 0.0f, -0.951056004f, 0.162459999f, 0.26286599f, -1.0f, 0.0f, 0.0f, -0.850651026f, 0.0f, 0.525731027f, -0.955422997f, -0.295242012f, 0.0f, -0.951056004f, -0.162459999f, 0.26286599f, -0.864188015f, 0.442862988f, -0.238856003f, -0.951056004f, 0.162459999f, -0.26286599f, -0.809017003f, 0.309017003f, -0.5f, -0.864188015f, -0.442862988f, -0.238856003f, -0.951056004f, -0.162459999f, -0.26286599f, -0.809017003f, -0.309017003f, -0.5f, -0.681717992f, 0.147621006f, -0.71656698f, -0.681717992f, -0.147621006f, -0.71656698f, -0.850651026f, 0.0f, -0.525731027f, -0.688190997f, 0.587785006f, -0.425325006f, -0.587785006f, 0.425325006f, -0.688190997f, -0.425325006f, 0.688190997f, -0.587785006f, -0.425325006f, -0.688190997f, -0.587785006f, -0.587785006f, -0.425325006f, -0.688190997f, -0.688190997f, -0.587785006f, -0.425325006f };
#endif

extern float sinf(float);
extern float cosf(float);
extern float acosf(float);
extern float asinf(float);
extern float atanf(float);
extern double atan2(double, double);
extern double tan(double);
extern double log(double);
extern double sqrt(double);
extern int rand(void);

#define qtrue 1
#define qfalse 0

extern vec3_t bytedirs[162];
extern vec2_t vec2_origin;
extern vec3_t vec3_origin;
extern const float identityMatrix44[4][4];
__attribute__((used)) UInt32 holdrand[2] = {
    0x89abcdef,
    0,
};
#define holdrand (*(unsigned long *)holdrand)

const qboolean VecNCompareCustomEpsilon(const vec_t *v0, const vec_t *v1, vec_t epsilon, int coordCount);
float randomf(void);
float GraphGetValueFromFraction(const int knotCount, vec2_t *knots, const float fraction);
const float Q_acos(const float c);
const signed char ClampChar(const int i);
const vec_t Vec2DistanceSq(const vec_t *p1, const vec_t *p2);
void Vec3Cross(const vec_t *v0, const vec_t *v1, vec_t *cross);
void Vec3RotateTranspose(const vec_t *in, vec3_t *matrix, vec_t *out);
void MatrixMultiply(const float (*in1)[3], const float (*in2)[3], float (*out)[3]);
const float vectoyaw(const vec_t *vec);
const float vectosignedyaw(const vec_t *vec);
float PointToBoxDistSq(const vec_t *pt, const vec_t *mins, const vec_t *maxs);
void MatrixIdentity44(float (*out)[4]);
void MatrixSet44(float (*out)[4], const vec_t *origin, vec3_t *axis, vec_t scale);
void MatrixMultiply43(const float (*in1)[3], const float (*in2)[3], float (*out)[3]);
void MatrixMultiply44(const float (*in1)[4], const float (*in2)[4], float (*out)[4]);
void MatrixTranspose(const float (*in)[3], float (*out)[3]);
void MatrixTranspose44(const float *in, float *out);
void MatrixInverse(const float (*in)[3], float (*out)[3]);
void MatrixTransformVector(const float *in1, const float (*in2)[3], float *out);
void MatrixInverse44(const float *mat, float *dst);
void MatrixTransformVector44(const float *vec, const float (*mat)[4], float *out);
void MatrixTransposeTransformVector(const float *in1, const float (*in2)[3], float *out);
void MatrixTransformVector43(const float *in1, const float (*in2)[3], float *out);
void QuatMultiply(const float *in1, const float *in2, float *out);
void QuatToAxis(const vec_t *quat, vec3_t *axis);
void ConvertQuatToMat(const DObjAnimMat *mat, float axis[3][3]);
float RotationToYaw(const vec_t *rot);
void InfinitePerspectiveMatrix(float (*mtx)[4], float fov_x, float fov_y, float zNear);
void MatrixForViewer(float (*mtx)[4], const vec_t *origin, vec3_t *axis);
const float AngleMod(const float a);
const float LerpAngle(const float from, const float to, const float frac);
const float AngleSubtract(const float a1, const float a2);
const float AngleNormalize360(const float angle);
const float AngleNormalize360Accurate(float angle);
const float AngleNormalize180Accurate(float angle);
void AddPointToBounds(const vec_t *v, vec_t *mins, vec_t *maxs);
void ExpandBounds(const vec_t *addedmins, const vec_t *addedmaxs, vec_t *mins, vec_t *maxs);
void AxisTransformVector(vec3_t *axes, const vec_t x, const vec_t y, const vec_t z, vec_t *out);
void ProjectPointOnPlane(const vec_t *p, const vec_t *normal, vec_t *dst);
int BoxOnPlaneSide(const vec_t *emins, const vec_t *emaxs, const cplane_s *p);
void Rand_Init(int seed);
float flrand(float min, float max);
int irand(int min, int max);
void MatrixTransformVectorQuatTrans(const vec_t *in, const DObjAnimMat *mat, vec_t *out);
float DiffTrack(float tgt, float cur, float rate, float deltaTime);
const vec_t RadiusFromBounds2D(const vec_t *mins, const vec_t *maxs);
void ByteToDir(const int b, vec_t *dir);
void AxisCopy(vec3_t *in, vec3_t *out);
const vec_t Vec3DistanceSq(const vec_t *p1, const vec_t *p2);
void MatrixTransposeTransformVector43(const float *in1, const float (*in2)[3], float *out);
void ExpandBoundsToWidth(vec_t *mins, vec_t *maxs);
void ShrinkBoundsToHeight(vec_t *mins, vec_t *maxs);
qboolean BoxDistSqrdExceeds(const vec_t *absmin, const vec_t *absmax, const vec_t *org, const float fogOpaqueDistSqrd);
const byte DirToByte(const vec_t *dir);
void GetRotatedBounds(vec3_t *baseBounds, const vec_t *origin, vec3_t *axis, vec3_t *rotatedBounds);
const vec_t ColorNormalize(const vec_t *in, vec_t *out);
void ClearBounds(vec_t *mins, vec_t *maxs);
const vec_t Vec3Normalize(vec_t *v);
const vec_t Vec2Normalize(vec_t *v);
const float vectopitch(const vec_t *vec);
void vectoangles(const vec_t *vec, vec_t *angles);
const vec_t RadiusFromBounds(const vec_t *mins, const vec_t *maxs);
void AnglesSubtract(const vec_t *v1, const vec_t *v2, vec_t *v3);
const vec_t Vec3NormalizeTo(const vec_t *v, vec_t *out);
int Vec3MajorAxis(const vec_t *dir);
const float AngleNormalize180(const float angle);
const float AngleDelta(const float angle1, const float angle2);
const vec_t Vec3Distance(const vec_t *v1, const vec_t *v2);
float DiffTrackAngle(float tgt, float cur, float rate, float deltaTime, float f, float granularity, float epsilon);
float crandom(void);
void GaussianRandom(float *f0, float *f1);
void MatrixInverseOrthogonal43(const float (*in)[3], float (*out)[3]);
qboolean WindingContainsCoplanarPoint(vec3_t *verts, int vertCount, const vec_t *normal, const vec_t *point);
void MakeNormalVectors(const vec_t *forward, vec_t *right, vec_t *up);
void PerpendicularVector(const vec_t *src, vec_t *dst);
float PitchForYawOnNormal(const float fYaw, const vec_t *normal);
void YawToAxis(float yaw, vec3_t *axis);
void RotatePointAroundVector(vec_t *dst, const vec_t *dir, const vec_t *point, const float degrees);
void AngleVectors(const vec_t *angles, vec_t *forward, vec_t *right, vec_t *up);
void AnglesToAxis(const vec_t *angles, vec3_t *axis);
void YawVectors(const vec_t yaw, vec_t *forward, vec_t *right);
void VectorAngleMultiply(vec_t *vec, float angle);
void AngleAxisToQuat(float angle, const vec_t *axis, vec_t *quat);
void AxisToAngles(vec3_t *axis, vec_t *angles);

const qboolean VecNCompareCustomEpsilon(const vec_t *v0, const vec_t *v1, vec_t epsilon, int coordCount)
{
    int i;
    for (i = 0; i < coordCount; i++) {
        float d = v0[i] - v1[i];
        if (d * d > epsilon * epsilon)
            return qfalse;
    }
    return qtrue;
}

float randomf(void)
{
    return (float)rand() * 4.656612873077393e-10f;
}

float GraphGetValueFromFraction(const int knotCount, vec2_t *knots, const float fraction)
{
    int i;
    if (knotCount <= 1)
        return -1.0f;
    for (i = 1; i < knotCount; i++) {
        if (knots[i][0] >= fraction) {
            float t = (fraction - knots[i - 1][0]) / (knots[i][0] - knots[i - 1][0]);
            return knots[i - 1][1] + t * (knots[i][1] - knots[i - 1][1]);
        }
    }
    return -1.0f;
}

const float Q_acos(const float c)
{
    float result = acosf(c);
    if ((double)result > 3.141592653589793 || (double)result < -3.141592653589793)
        return 3.1415927410125732f;
    return result;
}

const signed char ClampChar(const int i)
{
    if (i < -128)
        return -128;
    if (i > 127)
        return 127;
    return (signed char)i;
}

const vec_t Vec2DistanceSq(const vec_t *p1, const vec_t *p2)
{
    float dx = p2[0] - p1[0];
    float dy = p2[1] - p1[1];
    return dx * dx + dy * dy;
}

static inline __attribute__((always_inline)) void Vec3Cross_core(const vec_t *v0, const vec_t *v1, vec_t *cross)
{
    cross[0] = v0[1] * v1[2] - v0[2] * v1[1];
    cross[1] = v0[2] * v1[0] - v0[0] * v1[2];
    cross[2] = v0[0] * v1[1] - v0[1] * v1[0];
}

void Vec3Cross(const vec_t *v0, const vec_t *v1, vec_t *cross)
{
    Vec3Cross_core(v0, v1, cross);
}

void Vec3RotateTranspose(const vec_t *in, vec3_t *matrix, vec_t *out)
{

    out[0] = in[0] * matrix[0][0] + in[1] * matrix[1][0] + in[2] * matrix[2][0];
    out[1] = in[0] * matrix[0][1] + in[1] * matrix[1][1] + in[2] * matrix[2][1];
    out[2] = in[0] * matrix[0][2] + in[1] * matrix[1][2] + in[2] * matrix[2][2];
}

void MatrixMultiply(const float (*in1)[3], const float (*in2)[3], float (*out)[3])
{
    out[0][0] = in1[0][0] * in2[0][0] + in1[0][1] * in2[1][0] + in1[0][2] * in2[2][0];
    out[0][1] = in1[0][0] * in2[0][1] + in1[0][1] * in2[1][1] + in1[0][2] * in2[2][1];
    out[0][2] = in1[0][0] * in2[0][2] + in1[0][1] * in2[1][2] + in1[0][2] * in2[2][2];
    out[1][0] = in1[1][0] * in2[0][0] + in1[1][1] * in2[1][0] + in1[1][2] * in2[2][0];
    out[1][1] = in1[1][0] * in2[0][1] + in1[1][1] * in2[1][1] + in1[1][2] * in2[2][1];
    out[1][2] = in1[1][0] * in2[0][2] + in1[1][1] * in2[1][2] + in1[1][2] * in2[2][2];
    out[2][0] = in1[2][0] * in2[0][0] + in1[2][1] * in2[1][0] + in1[2][2] * in2[2][0];
    out[2][1] = in1[2][0] * in2[0][1] + in1[2][1] * in2[1][1] + in1[2][2] * in2[2][1];
    out[2][2] = in1[2][0] * in2[0][2] + in1[2][1] * in2[1][2] + in1[2][2] * in2[2][2];
}

const float vectoyaw(const vec_t *vec)
{
    if (vec[1] == 0.0f && vec[0] == 0.0f)
        return 0.0f;
    float yaw = (float)(atan2((double)vec[1], (double)vec[0]) * 180.0 / 3.141592653589793);
    if (yaw < 0.0f)
        yaw += 360.0f;
    return yaw;
}

const float vectosignedyaw(const vec_t *vec)
{
    if (vec[1] == 0.0f && vec[0] == 0.0f)
        return 0.0f;
    return (float)(atan2((double)vec[1], (double)vec[0]) * 180.0 / 3.141592653589793);
}

float PointToBoxDistSq(const vec_t *pt, const vec_t *mins, const vec_t *maxs)
{
    float distSq = 0.0f;
    int i;
    for (i = 0; i < 3; i++) {
        float d;
        d = mins[i] - pt[i];
        if (d > 0.0f) {
            distSq += d * d;
            continue;
        }
        d = pt[i] - maxs[i];
        if (d > 0.0f)
            distSq += d * d;
    }
    return distSq;
}

void MatrixIdentity44(float (*out)[4])
{
    int i, j;
    for (i = 0; i < 4; i++)
        for (j = 0; j < 4; j++)
            out[i][j] = identityMatrix44[i][j];
}

void MatrixSet44(float (*out)[4], const vec_t *origin, vec3_t *axis, vec_t scale)
{
    out[0][0] = axis[0][0] * scale;
    out[0][1] = axis[0][1] * scale;
    out[0][2] = axis[0][2] * scale;
    out[0][3] = 0.0f;
    out[1][0] = axis[1][0] * scale;
    out[1][1] = axis[1][1] * scale;
    out[1][2] = axis[1][2] * scale;
    out[1][3] = 0.0f;
    out[2][0] = axis[2][0] * scale;
    out[2][1] = axis[2][1] * scale;
    out[2][2] = axis[2][2] * scale;
    out[2][3] = 0.0f;
    out[3][0] = origin[0];
    out[3][1] = origin[1];
    out[3][2] = origin[2];
    out[3][3] = 1.0f;
}

void MatrixMultiply43(const float (*in1)[3], const float (*in2)[3], float (*out)[3])
{
    out[0][0] = in1[0][0] * in2[0][0] + in1[0][1] * in2[1][0] + in1[0][2] * in2[2][0];
    out[0][1] = in1[0][0] * in2[0][1] + in1[0][1] * in2[1][1] + in1[0][2] * in2[2][1];
    out[0][2] = in1[0][0] * in2[0][2] + in1[0][1] * in2[1][2] + in1[0][2] * in2[2][2];
    out[1][0] = in1[1][0] * in2[0][0] + in1[1][1] * in2[1][0] + in1[1][2] * in2[2][0];
    out[1][1] = in1[1][0] * in2[0][1] + in1[1][1] * in2[1][1] + in1[1][2] * in2[2][1];
    out[1][2] = in1[1][0] * in2[0][2] + in1[1][1] * in2[1][2] + in1[1][2] * in2[2][2];
    out[2][0] = in1[2][0] * in2[0][0] + in1[2][1] * in2[1][0] + in1[2][2] * in2[2][0];
    out[2][1] = in1[2][0] * in2[0][1] + in1[2][1] * in2[1][1] + in1[2][2] * in2[2][1];
    out[2][2] = in1[2][0] * in2[0][2] + in1[2][1] * in2[1][2] + in1[2][2] * in2[2][2];
    out[3][0] = in1[3][0] * in2[0][0] + in1[3][1] * in2[1][0] + in1[3][2] * in2[2][0] + in2[3][0];
    out[3][1] = in1[3][0] * in2[0][1] + in1[3][1] * in2[1][1] + in1[3][2] * in2[2][1] + in2[3][1];
    out[3][2] = in1[3][0] * in2[0][2] + in1[3][1] * in2[1][2] + in1[3][2] * in2[2][2] + in2[3][2];
}

void MatrixMultiply44(const float (*in1)[4], const float (*in2)[4], float (*out)[4])
{
    int i, j;
    for (i = 0; i < 4; i++)
        for (j = 0; j < 4; j++)
            out[i][j] = in1[i][0] * in2[0][j] + in1[i][1] * in2[1][j] +
                        in1[i][2] * in2[2][j] + in1[i][3] * in2[3][j];
}

static inline __attribute__((always_inline)) void MatrixTranspose_core(const float (*in)[3], float (*out)[3])
{
    out[0][0] = in[0][0];
    out[0][1] = in[1][0];
    out[0][2] = in[2][0];
    out[1][0] = in[0][1];
    out[1][1] = in[1][1];
    out[1][2] = in[2][1];
    out[2][0] = in[0][2];
    out[2][1] = in[1][2];
    out[2][2] = in[2][2];
}

void MatrixTranspose(const float (*in)[3], float (*out)[3])
{
    MatrixTranspose_core(in, out);
}

void MatrixTranspose44(const float *in, float *out)
{
    int i, j;
    for (i = 0; i < 4; i++)
        for (j = 0; j < 4; j++)
            out[i * 4 + j] = in[j * 4 + i];
}

void MatrixInverse(const float (*in)[3], float (*out)[3])
{
    float det;
    float invDet;

    det = in[0][0] * (in[2][2] * in[1][1] - in[2][1] * in[1][2]) - in[1][0] * (in[2][2] * in[0][1] - in[2][1] * in[0][2]) + in[2][0] * (in[1][2] * in[0][1] - in[1][1] * in[0][2]);

    invDet = 1.0f / det;

    out[0][0] = (in[2][2] * in[1][1] - in[2][1] * in[1][2]) * invDet;
    out[0][1] = -(in[2][2] * in[0][1] - in[2][1] * in[0][2]) * invDet;
    out[0][2] = (in[1][2] * in[0][1] - in[1][1] * in[0][2]) * invDet;
    out[1][0] = -(in[2][2] * in[1][0] - in[2][0] * in[1][2]) * invDet;
    out[1][1] = (in[2][2] * in[0][0] - in[2][0] * in[0][2]) * invDet;
    out[1][2] = -(in[1][2] * in[0][0] - in[1][0] * in[0][2]) * invDet;
    out[2][0] = (in[2][1] * in[1][0] - in[2][0] * in[1][1]) * invDet;
    out[2][1] = -(in[2][1] * in[0][0] - in[2][0] * in[0][1]) * invDet;
    out[2][2] = (in[1][1] * in[0][0] - in[1][0] * in[0][1]) * invDet;
}

static inline __attribute__((always_inline)) void MatrixTransformVector_core(const float *in1, const float (*in2)[3], float *out)
{
    out[0] = in1[0] * in2[0][0] + in1[1] * in2[1][0] + in1[2] * in2[2][0];
    out[1] = in1[0] * in2[0][1] + in1[1] * in2[1][1] + in1[2] * in2[2][1];
    out[2] = in1[0] * in2[0][2] + in1[1] * in2[1][2] + in1[2] * in2[2][2];
}

void MatrixTransformVector(const float *in1, const float (*in2)[3], float *out)
{
    MatrixTransformVector_core(in1, in2, out);
}

void MatrixInverse44(const float *mat, float *dst)
{
    float tmp[12];
    float src[16];
    float det;
    int i;

    for (i = 0; i < 4; i++) {
        src[i] = mat[i * 4];
        src[i + 4] = mat[i * 4 + 1];
        src[i + 8] = mat[i * 4 + 2];
        src[i + 12] = mat[i * 4 + 3];
    }

    tmp[0] = src[10] * src[15];
    tmp[1] = src[11] * src[14];
    tmp[2] = src[9] * src[15];
    tmp[3] = src[11] * src[13];
    tmp[4] = src[9] * src[14];
    tmp[5] = src[10] * src[13];
    tmp[6] = src[8] * src[15];
    tmp[7] = src[11] * src[12];
    tmp[8] = src[8] * src[14];
    tmp[9] = src[10] * src[12];
    tmp[10] = src[8] * src[13];
    tmp[11] = src[9] * src[12];

    dst[0] = tmp[0] * src[5] + tmp[3] * src[6] + tmp[4] * src[7] - tmp[1] * src[5] - tmp[2] * src[6] - tmp[5] * src[7];
    dst[1] = tmp[1] * src[4] + tmp[6] * src[6] + tmp[9] * src[7] - tmp[0] * src[4] - tmp[7] * src[6] - tmp[8] * src[7];
    dst[2] = tmp[2] * src[4] + tmp[7] * src[5] + tmp[10] * src[7] - tmp[3] * src[4] - tmp[6] * src[5] - tmp[11] * src[7];
    dst[3] = tmp[5] * src[4] + tmp[8] * src[5] + tmp[11] * src[6] - tmp[4] * src[4] - tmp[9] * src[5] - tmp[10] * src[6];
    dst[4] = tmp[1] * src[1] + tmp[2] * src[2] + tmp[5] * src[3] - tmp[0] * src[1] - tmp[3] * src[2] - tmp[4] * src[3];
    dst[5] = tmp[0] * src[0] + tmp[7] * src[2] + tmp[8] * src[3] - tmp[1] * src[0] - tmp[6] * src[2] - tmp[9] * src[3];
    dst[6] = tmp[3] * src[0] + tmp[6] * src[1] + tmp[11] * src[3] - tmp[2] * src[0] - tmp[7] * src[1] - tmp[10] * src[3];
    dst[7] = tmp[4] * src[0] + tmp[9] * src[1] + tmp[10] * src[2] - tmp[5] * src[0] - tmp[8] * src[1] - tmp[11] * src[2];

    tmp[0] = src[2] * src[7];
    tmp[1] = src[3] * src[6];
    tmp[2] = src[1] * src[7];
    tmp[3] = src[3] * src[5];
    tmp[4] = src[1] * src[6];
    tmp[5] = src[2] * src[5];
    tmp[6] = src[0] * src[7];
    tmp[7] = src[3] * src[4];
    tmp[8] = src[0] * src[6];
    tmp[9] = src[2] * src[4];
    tmp[10] = src[0] * src[5];
    tmp[11] = src[1] * src[4];

    dst[8] = tmp[0] * src[13] + tmp[3] * src[14] + tmp[4] * src[15] - tmp[1] * src[13] - tmp[2] * src[14] - tmp[5] * src[15];
    dst[9] = tmp[1] * src[12] + tmp[6] * src[14] + tmp[9] * src[15] - tmp[0] * src[12] - tmp[7] * src[14] - tmp[8] * src[15];
    dst[10] = tmp[2] * src[12] + tmp[7] * src[13] + tmp[10] * src[15] - tmp[3] * src[12] - tmp[6] * src[13] - tmp[11] * src[15];
    dst[11] = tmp[5] * src[12] + tmp[8] * src[13] + tmp[11] * src[14] - tmp[4] * src[12] - tmp[9] * src[13] - tmp[10] * src[14];
    dst[12] = tmp[2] * src[10] + tmp[5] * src[11] + tmp[1] * src[9] - tmp[4] * src[11] - tmp[0] * src[9] - tmp[3] * src[10];
    dst[13] = tmp[8] * src[11] + tmp[0] * src[8] + tmp[7] * src[10] - tmp[6] * src[10] - tmp[9] * src[11] - tmp[1] * src[8];
    dst[14] = tmp[6] * src[9] + tmp[11] * src[11] + tmp[3] * src[8] - tmp[10] * src[11] - tmp[2] * src[8] - tmp[7] * src[9];
    dst[15] = tmp[10] * src[10] + tmp[4] * src[8] + tmp[9] * src[9] - tmp[8] * src[9] - tmp[11] * src[10] - tmp[5] * src[8];

    det = src[0] * dst[0] + src[1] * dst[1] + src[2] * dst[2] + src[3] * dst[3];
    det = 1.0f / det;

    for (i = 0; i < 16; i++)
        dst[i] *= det;
}

void MatrixTransformVector44(const float *vec, const float (*mat)[4], float *out)
{
    out[0] = vec[0] * mat[0][0] + vec[1] * mat[1][0] + vec[2] * mat[2][0] + vec[3] * mat[3][0];
    out[1] = vec[0] * mat[0][1] + vec[1] * mat[1][1] + vec[2] * mat[2][1] + vec[3] * mat[3][1];
    out[2] = vec[0] * mat[0][2] + vec[1] * mat[1][2] + vec[2] * mat[2][2] + vec[3] * mat[3][2];
    out[3] = vec[0] * mat[0][3] + vec[1] * mat[1][3] + vec[2] * mat[2][3] + vec[3] * mat[3][3];
}

void MatrixTransposeTransformVector(const float *in1, const float (*in2)[3], float *out)
{
    out[0] = in1[0] * in2[0][0] + in1[1] * in2[0][1] + in1[2] * in2[0][2];
    out[1] = in1[0] * in2[1][0] + in1[1] * in2[1][1] + in1[2] * in2[1][2];
    out[2] = in1[0] * in2[2][0] + in1[1] * in2[2][1] + in1[2] * in2[2][2];
}

void MatrixTransformVector43(const float *in1, const float (*in2)[3], float *out)
{
    out[0] = in1[0] * in2[0][0] + in1[1] * in2[1][0] + in1[2] * in2[2][0] + in2[3][0];
    out[1] = in1[0] * in2[0][1] + in1[1] * in2[1][1] + in1[2] * in2[2][1] + in2[3][1];
    out[2] = in1[0] * in2[0][2] + in1[1] * in2[1][2] + in1[2] * in2[2][2] + in2[3][2];
}

void QuatMultiply(const float *in1, const float *in2, float *out)
{
    out[0] = in1[0] * in2[3] + in1[3] * in2[0] + in1[2] * in2[1] - in1[1] * in2[2];
    out[1] = in1[1] * in2[3] - in1[2] * in2[0] + in1[3] * in2[1] + in1[0] * in2[2];
    out[2] = in1[2] * in2[3] + in1[1] * in2[0] - in1[0] * in2[1] + in1[3] * in2[2];
    out[3] = in1[3] * in2[3] - in1[0] * in2[0] - in1[1] * in2[1] - in1[2] * in2[2];
}

void QuatToAxis(const vec_t *quat, vec3_t *axis)
{
    float x = quat[0], y = quat[1], z = quat[2], w = quat[3];
    float s = 2.0f / (w * w + (x * x + y * y + z * z));
    float xx = x * x * s, yy = y * y * s, zz = z * z * s;
    float sx = x * s, sy = y * s, sz = z * s;
    float xy = y * sx, xz = z * sx, xw = w * sx;
    float yz = z * sy, yw = w * sy;
    float zw = w * sz;
    axis[0][0] = 1.0f - (yy + zz);
    axis[0][1] = xy + zw;
    axis[0][2] = xz - yw;
    axis[1][0] = xy - zw;
    axis[1][1] = 1.0f - (zz + xx);
    axis[1][2] = xw + yz;
    axis[2][0] = yw + xz;
    axis[2][1] = yz - xw;
    axis[2][2] = 1.0f - (xx + yy);
}

void ConvertQuatToMat(const DObjAnimMat *mat, float axis[3][3])
{
    float sx = mat->transWeight * mat->quat[0];
    float sy = mat->transWeight * mat->quat[1];
    float sz = mat->transWeight * mat->quat[2];
    float xx = sx * mat->quat[0];
    float xy = sx * mat->quat[1];
    float xz = sx * mat->quat[2];
    float xw = sx * mat->quat[3];
    float yy = sy * mat->quat[1];
    float yz = sy * mat->quat[2];
    float yw = sy * mat->quat[3];
    float zz = sz * mat->quat[2];
    float zw = sz * mat->quat[3];

    axis[0][0] = 1.0f - (yy + zz);
    axis[0][1] = xy + zw;
    axis[0][2] = xz - yw;
    axis[1][0] = xy - zw;
    axis[1][1] = 1.0f - (xx + zz);
    axis[1][2] = yz + xw;
    axis[2][0] = xz + yw;
    axis[2][1] = yz - xw;
    axis[2][2] = 1.0f - (xx + yy);
}

float RotationToYaw(const vec_t *rot)
{
    float x = rot[0];
    float xsq = x * x;
    float y = rot[1];
    float s = 2.0f / (xsq + y * y);
    float xx = xsq * s;
    float xy = x * y * s;
    return (float)(atan2((double)xy, (double)(1.0f - xx)) * 57.29577951308232);
}

void InfinitePerspectiveMatrix(float (*mtx)[4], float fov_x, float fov_y, float zNear)
{
    float c;
    __builtin_memset(mtx, 0, 64);
    mtx[0][0] = (float)(tan((double)(90.0f - fov_x * 0.5f) * 0.017453292519943295) * 0.9995002746582031);
    mtx[1][1] = (float)(tan((double)(90.0f - fov_y * 0.5f) * 0.017453292519943295) * 0.9995002746582031);
    c = 0.9995002746582031f;
    mtx[2][2] = c;
    mtx[2][3] = 1.0f;
    mtx[3][2] = c * -zNear;
}

void MatrixForViewer(float (*mtx)[4], const vec_t *origin, vec3_t *axis)
{
    mtx[0][0] = -axis[1][0];
    mtx[1][0] = -axis[1][1];
    mtx[2][0] = -axis[1][2];
    mtx[3][0] = origin[0] * axis[1][0] + origin[1] * axis[1][1] + origin[2] * axis[1][2];
    mtx[0][1] = axis[2][0];
    mtx[1][1] = axis[2][1];
    mtx[2][1] = axis[2][2];
    mtx[3][1] = -(origin[0] * axis[2][0] + origin[1] * axis[2][1] + origin[2] * axis[2][2]);
    mtx[0][2] = axis[0][0];
    mtx[1][2] = axis[0][1];
    mtx[2][2] = axis[0][2];
    mtx[3][2] = -(origin[0] * axis[0][0] + origin[1] * axis[0][1] + origin[2] * axis[0][2]);
    mtx[0][3] = 0.0f;
    mtx[1][3] = 0.0f;
    mtx[2][3] = 0.0f;
    mtx[3][3] = 1.0f;
}

const float AngleMod(const float a)
{
    return (float)((int)(a * 182.04444885253906f) & 0xffff) * 0.0054931640625f;
}

const float LerpAngle(const float from, const float to, const float frac)
{
    float a = to - from;
    if (a > 180.0f)
        a -= 360.0f;
    if (a < -180.0f)
        a += 360.0f;
    return from + frac * a;
}

static inline __attribute__((always_inline)) float AngleSubtract_core(const float a1, const float a2)
{
    float a = a1 - a2;
    while (a > 180.0f)
        a -= 360.0f;
    while (a < -180.0f)
        a += 360.0f;
    return a;
}

const float AngleSubtract(const float a1, const float a2)
{
    return AngleSubtract_core(a1, a2);
}

#define ANGLE_NORMALIZE_360(a) \
    ((float)((int)((a) * 182.04444885253906f) & 0xffff) * 0.0054931640625f)

const float AngleNormalize360(const float angle)
{
    return ANGLE_NORMALIZE_360(angle);
}

const float AngleNormalize360Accurate(float angle)
{
    while (angle < 0.0f)
        angle += 360.0f;
    while (angle >= 360.0f)
        angle -= 360.0f;
    return angle;
}

const float AngleNormalize180Accurate(float angle)
{
    while (angle <= -180.0f)
        angle += 360.0f;
    while (angle > 180.0f)
        angle -= 360.0f;
    return angle;
}

void AddPointToBounds(const vec_t *v, vec_t *mins, vec_t *maxs)
{
    int i;
    for (i = 0; i < 3; i++) {
        if (v[i] < mins[i])
            mins[i] = v[i];
        if (v[i] > maxs[i])
            maxs[i] = v[i];
    }
}

void ExpandBounds(const vec_t *addedmins, const vec_t *addedmaxs, vec_t *mins, vec_t *maxs)
{
    int i;
    for (i = 0; i < 3; i++) {
        if (addedmins[i] < mins[i])
            mins[i] = addedmins[i];
        if (addedmaxs[i] > maxs[i])
            maxs[i] = addedmaxs[i];
    }
}

void AxisTransformVector(vec3_t *axes, const vec_t x, const vec_t y, const vec_t z, vec_t *out)
{
    out[0] = x * axes[0][0] + y * axes[1][0] + z * axes[2][0];
    out[1] = x * axes[0][1] + y * axes[1][1] + z * axes[2][1];
    out[2] = x * axes[0][2] + y * axes[1][2] + z * axes[2][2];
}

void ProjectPointOnPlane(const vec_t *p, const vec_t *normal, vec_t *dst)
{
    float d = -(normal[0] * p[0] + normal[1] * p[1] + normal[2] * p[2]);
    dst[0] = p[0] + d * normal[0];
    dst[1] = p[1] + d * normal[1];
    dst[2] = p[2] + d * normal[2];
}

int BoxOnPlaneSide(const vec_t *emins, const vec_t *emaxs, const cplane_s *p)
{
    float dist1, dist2;
    int sides;

    if (p->type < 3) {
        if (emins[p->type] >= p->dist)
            return 1;
        if (emaxs[p->type] <= p->dist)
            return 2;
        return 3;
    }

    switch (p->signbits) {
    case 0:
        dist1 = p->normal[0] * emaxs[0] + p->normal[1] * emaxs[1] + p->normal[2] * emaxs[2];
        dist2 = p->normal[0] * emins[0] + p->normal[1] * emins[1] + p->normal[2] * emins[2];
        break;
    case 1:
        dist1 = p->normal[0] * emins[0] + p->normal[1] * emaxs[1] + p->normal[2] * emaxs[2];
        dist2 = p->normal[0] * emaxs[0] + p->normal[1] * emins[1] + p->normal[2] * emins[2];
        break;
    case 2:
        dist1 = p->normal[0] * emaxs[0] + p->normal[1] * emins[1] + p->normal[2] * emaxs[2];
        dist2 = p->normal[0] * emins[0] + p->normal[1] * emaxs[1] + p->normal[2] * emins[2];
        break;
    case 3:
        dist1 = p->normal[0] * emins[0] + p->normal[1] * emins[1] + p->normal[2] * emaxs[2];
        dist2 = p->normal[0] * emaxs[0] + p->normal[1] * emaxs[1] + p->normal[2] * emins[2];
        break;
    case 4:
        dist1 = p->normal[0] * emaxs[0] + p->normal[1] * emaxs[1] + p->normal[2] * emins[2];
        dist2 = p->normal[0] * emins[0] + p->normal[1] * emins[1] + p->normal[2] * emaxs[2];
        break;
    case 5:
        dist1 = p->normal[0] * emins[0] + p->normal[1] * emaxs[1] + p->normal[2] * emins[2];
        dist2 = p->normal[0] * emaxs[0] + p->normal[1] * emins[1] + p->normal[2] * emaxs[2];
        break;
    case 6:
        dist1 = p->normal[0] * emaxs[0] + p->normal[1] * emins[1] + p->normal[2] * emins[2];
        dist2 = p->normal[0] * emins[0] + p->normal[1] * emaxs[1] + p->normal[2] * emaxs[2];
        break;
    case 7:
        dist1 = p->normal[0] * emins[0] + p->normal[1] * emins[1] + p->normal[2] * emins[2];
        dist2 = p->normal[0] * emaxs[0] + p->normal[1] * emaxs[1] + p->normal[2] * emaxs[2];
        break;
    default:
        dist1 = 0.0f;
        dist2 = 0.0f;
        break;
    }

    sides = 0;
    if (dist1 >= p->dist)
        sides = 1;
    if (dist2 < p->dist)
        sides |= 2;
    return sides;
}

void Rand_Init(int seed)
{
    holdrand = seed;
}

float flrand(float min, float max)
{
    holdrand = holdrand * 214013 + 2531011;
    return min + ((float)(holdrand >> 17) * (max - min)) * 3.0517578125e-05f;
}

int irand(int min, int max)
{
    holdrand = holdrand * 214013 + 2531011;
    return min + ((int)((holdrand >> 17) * (unsigned int)(max - min)) >> 15);
}

void MatrixTransformVectorQuatTrans(const vec_t *in, const DObjAnimMat *mat, vec_t *out)
{
    float scale = mat->transWeight;
    float qx = mat->quat[0] * scale;
    float qy = mat->quat[1] * scale;
    float qz = mat->quat[2] * scale;

    float xx = qx * mat->quat[0];
    float xy = qx * mat->quat[1];
    float xz = qx * mat->quat[2];
    float xw = qx * mat->quat[3];
    float yy = qy * mat->quat[1];
    float yz = qy * mat->quat[2];
    float yw = qy * mat->quat[3];
    float zz = qz * mat->quat[2];
    float zw = qz * mat->quat[3];

    out[0] = (1.0f - (yy + zz)) * in[0] + (xy - zw) * in[1] + (yw + xz) * in[2] + mat->trans[0];
    out[1] = (xy + zw) * in[0] + (1.0f - (xx + zz)) * in[1] + (yz - xw) * in[2] + mat->trans[1];
    out[2] = (xz - yw) * in[0] + (xw + yz) * in[1] + (1.0f - (xx + yy)) * in[2] + mat->trans[2];
}

float DiffTrack(float tgt, float cur, float rate, float deltaTime)
{
    float diff = tgt - cur;
    float step = diff * rate * deltaTime;
    if (__builtin_fabsf(diff) <= 0.001f)
        return tgt;
    if (__builtin_fabsf(step) > __builtin_fabsf(diff))
        return tgt;
    return cur + step;
}

const vec_t RadiusFromBounds2D(const vec_t *mins, const vec_t *maxs)
{
    int i;
    float corner[2];
    for (i = 0; i < 2; i++) {
        float a = __builtin_fabsf(mins[i]);
        float b = __builtin_fabsf(maxs[i]);
        corner[i] = (a > b) ? a : b;
    }
    return __builtin_sqrtf(corner[0] * corner[0] + corner[1] * corner[1]);
}

void ByteToDir(const int b, vec_t *dir)
{
    if (b < 0 || b > 161) {
        dir[0] = 0.0f;
        dir[1] = 0.0f;
        dir[2] = 0.0f;
        return;
    }
    dir[0] = bytedirs[b][0];
    dir[1] = bytedirs[b][1];
    dir[2] = bytedirs[b][2];
}

void AxisCopy(vec3_t *in, vec3_t *out)
{
    out[0][0] = in[0][0];
    out[0][1] = in[0][1];
    out[0][2] = in[0][2];
    out[1][0] = in[1][0];
    out[1][1] = in[1][1];
    out[1][2] = in[1][2];
    out[2][0] = in[2][0];
    out[2][1] = in[2][1];
    out[2][2] = in[2][2];
}

const vec_t Vec3DistanceSq(const vec_t *p1, const vec_t *p2)
{
    float dx = p2[0] - p1[0];
    float dy = p2[1] - p1[1];
    float dz = p2[2] - p1[2];
    return dx * dx + dy * dy + dz * dz;
}

void MatrixTransposeTransformVector43(const float *in1, const float (*in2)[3], float *out)
{
    float d0 = in1[0] - in2[3][0];
    float d1 = in1[1] - in2[3][1];
    float d2 = in1[2] - in2[3][2];
    out[0] = d0 * in2[0][0] + d1 * in2[0][1] + d2 * in2[0][2];
    out[1] = d0 * in2[1][0] + d1 * in2[1][1] + d2 * in2[1][2];
    out[2] = d0 * in2[2][0] + d1 * in2[2][1] + d2 * in2[2][2];
}

void ExpandBoundsToWidth(vec_t *mins, vec_t *maxs)
{
    float dx = maxs[0] - mins[0];
    float dy = maxs[1] - mins[1];
    float dz = maxs[2] - mins[2];
    float width = (dx >= dy) ? dx : dy;
    if (width > dz) {
        float half = (width - dz) * 0.5f;
        mins[2] -= half;
        maxs[2] += half;
    }
}

void ShrinkBoundsToHeight(vec_t *mins, vec_t *maxs)
{
    float dx = maxs[0] - mins[0];
    float dy = maxs[1] - mins[1];
    float dz = maxs[2] - mins[2];
    if (dx > dz) {
        float half = (dx - dz) * 0.5f;
        mins[0] += half;
        maxs[0] -= half;
    }
    if (dy > dz) {
        float half = (dy - dz) * 0.5f;
        mins[1] += half;
        maxs[1] -= half;
    }
}

qboolean BoxDistSqrdExceeds(const vec_t *absmin, const vec_t *absmax, const vec_t *org, const float fogOpaqueDistSqrd)
{
    int i;
    float distSq = 0.0f;
    for (i = 0; i < 3; i++) {
        float lo = absmin[i] - org[i];
        float hi = absmax[i] - org[i];
        if (lo * hi > 0.0f) {
            float a = lo * lo;
            float b = hi * hi;
            distSq += (a < b) ? a : b;
        }
    }
    return distSq > fogOpaqueDistSqrd;
}

const byte DirToByte(const vec_t *dir)
{
    int i, best;
    float d, bestd;

    if (!dir)
        return 0;

    best = 0;
    bestd = 0.0f;
    for (i = 0; i < 162; i++) {
        d = dir[0] * bytedirs[i][0] + dir[1] * bytedirs[i][1] + dir[2] * bytedirs[i][2];
        if (d > bestd) {
            bestd = d;
            best = i;
        }
    }
    return (byte)best;
}

void GetRotatedBounds(vec3_t *baseBounds, const vec_t *origin, vec3_t *axis, vec3_t *rotatedBounds)
{
    int i, j;
    for (i = 0; i < 3; i++) {
        rotatedBounds[0][i] = origin[i];
        rotatedBounds[1][i] = origin[i];
        for (j = 0; j < 3; j++) {
            float e = axis[j][i];
            if (e >= 0.0f) {
                rotatedBounds[0][i] += baseBounds[0][j] * e;
                rotatedBounds[1][i] += baseBounds[1][j] * e;
            } else {
                rotatedBounds[0][i] += baseBounds[1][j] * e;
                rotatedBounds[1][i] += baseBounds[0][j] * e;
            }
        }
    }
}

const vec_t ColorNormalize(const vec_t *in, vec_t *out)
{
    float max = in[0];
    if (in[1] > max)
        max = in[1];
    if (in[2] > max)
        max = in[2];
    if (max == 0.0f) {
        out[0] = out[1] = out[2] = 1.0f;
        return 0.0f;
    }
    float inv = 1.0f / max;
    out[0] = in[0] * inv;
    out[1] = in[1] * inv;
    out[2] = in[2] * inv;
    return max;
}

void ClearBounds(vec_t *mins, vec_t *maxs)
{

    mins[0] = 131072.0f;
    mins[1] = 131072.0f;
    mins[2] = 131072.0f;
    maxs[0] = -131072.0f;
    maxs[1] = -131072.0f;
    maxs[2] = -131072.0f;
}

static inline __attribute__((always_inline)) vec_t Vec3Normalize_core(vec_t *v)
{
    float len = __builtin_sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    if (len != 0.0f) {
        float ilength = 1.0f / len;
        v[0] *= ilength;
        v[1] *= ilength;
        v[2] *= ilength;
    }
    return len;
}
const vec_t Vec3Normalize(vec_t *v)
{
    return Vec3Normalize_core(v);
}

const vec_t Vec2Normalize(vec_t *v)
{
    float len = __builtin_sqrtf(v[0] * v[0] + v[1] * v[1]);
    if (len != 0.0f) {
        float ilength = 1.0f / len;
        v[0] *= ilength;
        v[1] *= ilength;
    }
    return len;
}

const float vectopitch(const vec_t *vec)
{
    if (vec[1] == 0.0f && vec[0] == 0.0f) {
        return (vec[2] > 0.0f) ? 270.0f : 90.0f;
    }
    float forward = __builtin_sqrtf(vec[0] * vec[0] + vec[1] * vec[1]);
    float pitch = (float)(atan2((double)vec[2], (double)forward) * -180.0 / 3.141592653589793);
    if (pitch < 0.0f)
        pitch += 360.0f;
    return pitch;
}

void vectoangles(const vec_t *vec, vec_t *angles)
{
    float yaw, pitch;

    if (vec[1] == 0.0f && vec[0] == 0.0f) {
        pitch = (vec[2] > 0.0f) ? 270.0f : 90.0f;
        angles[0] = pitch;
        angles[1] = 0.0f;
        angles[2] = 0.0f;
        return;
    }

    yaw = (float)(atan2((double)vec[1], (double)vec[0]) * 180.0 / 3.141592653589793);
    if (yaw < 0.0f)
        yaw += 360.0f;

    {
        float forward = __builtin_sqrtf(vec[0] * vec[0] + vec[1] * vec[1]);
        pitch = (float)(atan2((double)vec[2], (double)forward) * -180.0 / 3.141592653589793);
        if (pitch < 0.0f)
            pitch += 360.0f;
    }

    angles[0] = pitch;
    angles[1] = yaw;
    angles[2] = 0.0f;
}

const vec_t RadiusFromBounds(const vec_t *mins, const vec_t *maxs)
{
    int i;
    vec3_t corner;
    for (i = 0; i < 3; i++) {
        float a = __builtin_fabsf(mins[i]);
        float b = __builtin_fabsf(maxs[i]);
        corner[i] = (a > b) ? a : b;
    }
    return __builtin_sqrtf(corner[0] * corner[0] + corner[1] * corner[1] + corner[2] * corner[2]);
}

void AnglesSubtract(const vec_t *v1, const vec_t *v2, vec_t *v3)
{
    v3[0] = AngleSubtract_core(v1[0], v2[0]);
    v3[1] = AngleSubtract_core(v1[1], v2[1]);
    v3[2] = AngleSubtract_core(v1[2], v2[2]);
}

const vec_t Vec3NormalizeTo(const vec_t *v, vec_t *out)
{
    float len = __builtin_sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    if (len != 0.0f) {
        float ilength = 1.0f / len;
        out[0] = v[0] * ilength;
        out[1] = v[1] * ilength;
        out[2] = v[2] * ilength;
    } else {

        out[0] = 0.0f;
        out[1] = 0.0f;
        out[2] = 0.0f;
    }
    return len;
}

int Vec3MajorAxis(const vec_t *dir)
{
    float dirSq[3];
    int best;
    dirSq[0] = dir[0] * dir[0];
    dirSq[1] = dir[1] * dir[1];
    dirSq[2] = dir[2] * dir[2];
    best = (dirSq[1] > dirSq[0]) ? 1 : 0;
    if (dirSq[2] > dirSq[best])
        best = 2;
    return best;
}

const float AngleNormalize180(const float angle)
{
    float a = ANGLE_NORMALIZE_360(angle);
    if (a > 180.0f)
        a -= 360.0f;
    return a;
}

const float AngleDelta(const float angle1, const float angle2)
{
    float a = ANGLE_NORMALIZE_360(angle1 - angle2);
    if (a > 180.0f)
        a -= 360.0f;
    return a;
}

const vec_t Vec3Distance(const vec_t *v1, const vec_t *v2)
{
    float dx = v2[0] - v1[0];
    float dy = v2[1] - v1[1];
    float dz = v2[2] - v1[2];
    return __builtin_sqrtf(dx * dx + dy * dy + dz * dz);
}

float DiffTrackAngle(float tgt, float cur, float rate, float deltaTime, float f, float granularity, float epsilon)
{
    float diff, step, result;

    while (tgt - cur > 180.0f)
        tgt -= 360.0f;
    while (tgt - cur < -180.0f)
        tgt += 360.0f;

    diff = tgt - cur;
    step = diff * rate * deltaTime;
    if (__builtin_fabsf(diff) > 0.001f && !(__builtin_fabsf(diff) < __builtin_fabsf(step)))
        result = cur + step;
    else
        result = tgt;

    result = ANGLE_NORMALIZE_360(result);
    if (result > 180.0f)
        result -= 360.0f;
    return result;
}

static inline __attribute__((always_inline)) float crandom_core(void)
{
    return ((float)rand() * 4.656612873077393e-10f) * 2.0f - 1.0f;
}

float crandom(void)
{
    return crandom_core();
}

void GaussianRandom(float *f0, float *f1)
{
    float s1, s2, w, mul;
    do {
        s1 = crandom_core();
        s2 = crandom_core();
        w = s1 * s1 + s2 * s2;
    } while (w > 1.0f);
    mul = __builtin_sqrtf((float)(-2.0 * log((double)w) / (double)w));
    *f0 = s1 * mul;
    *f1 = s2 * mul;
}

void MatrixInverseOrthogonal43(const float (*in)[3], float (*out)[3])
{
    float negOrigin[3];
    MatrixTranspose_core(in, out);
    negOrigin[0] = 0.0f - in[3][0];
    negOrigin[1] = 0.0f - in[3][1];
    negOrigin[2] = 0.0f - in[3][2];
    MatrixTransformVector_core(negOrigin, out, out[3]);
}

qboolean WindingContainsCoplanarPoint(vec3_t *verts, int vertCount, const vec_t *normal, const vec_t *point)
{
    int ax, ay;
    int i, prev;
    float nx2, ny2, nz2;

    nx2 = normal[0] * normal[0];
    ny2 = normal[1] * normal[1];
    nz2 = normal[2] * normal[2];

    if (nz2 >= nx2 && nz2 >= ny2) {
        if (normal[2] > 0.0f) {
            ax = 0;
            ay = 1;
        } else {
            ax = 1;
            ay = 0;
        }
    } else if (ny2 >= nx2) {
        if (normal[1] > 0.0f) {
            ax = 2;
            ay = 0;
        } else {
            ax = 0;
            ay = 2;
        }
    } else {
        if (normal[0] > 0.0f) {
            ax = 1;
            ay = 2;
        } else {
            ax = 2;
            ay = 1;
        }
    }

    prev = vertCount - 1;
    for (i = 0; i < vertCount; i++) {
        float cross;
        cross = (verts[i][ay] - verts[prev][ay]) * (point[ax] - verts[prev][ax]) + (verts[prev][ax] - verts[i][ax]) * (point[ay] - verts[prev][ay]);
        if (cross < 0.0f)
            return qfalse;
        prev = i;
    }
    return qtrue;
}

void MakeNormalVectors(const vec_t *forward, vec_t *right, vec_t *up)
{
    float d;
    right[1] = -forward[0];
    right[2] = forward[1];
    right[0] = forward[2];
    d = -(forward[0] * right[0] + right[1] * forward[1] + right[2] * forward[2]);
    right[0] = d * forward[0] + right[0];
    right[1] = d * forward[1] + right[1];
    right[2] = d * forward[2] + right[2];
    Vec3Normalize_core(right);
    Vec3Cross_core(right, forward, up);
}

void PerpendicularVector(const vec_t *src, vec_t *dst)
{
    float srcSq[3];
    float a, b, c, d;
    int minAxis;

    a = src[0] * src[0];
    srcSq[0] = a;
    b = src[1] * src[1];
    srcSq[1] = b;
    c = src[2] * src[2];
    srcSq[2] = c;

    minAxis = (a > b) ? 1 : 0;
    if (c < srcSq[minAxis])
        minAxis = 2;

    d = -src[minAxis];
    dst[0] = src[0] * d;
    dst[1] = src[1] * d;
    dst[2] = src[2] * d;
    dst[minAxis] += 1.0f;
    Vec3Normalize_core(dst);
}

float PitchForYawOnNormal(const float fYaw, const vec_t *normal)
{
    float radians = (float)((double)fYaw * 0.017453292519943295);
    float sy = sinf(radians);
    float cy = cosf(radians);
    float sz = 0.0f;

    float dot = -(cy * normal[0] + sy * normal[1] + sz * normal[2]);

    float px = normal[0] * dot + cy;
    float py = normal[1] * dot + sy;
    float pz = normal[2] * dot + sz;

    if (py == 0.0f && px == 0.0f)
        return (pz > 0.0f) ? 270.0f : 90.0f;
    {
        float forward = __builtin_sqrtf(px * px + py * py);
        float pitch = (float)(atan2((double)pz, (double)forward) * -180.0 / 3.141592653589793);
        if (pitch < 0.0f)
            pitch += 360.0f;
        return pitch;
    }
}

void YawToAxis(float yaw, vec3_t *axis)
{
    float angle = (float)((double)yaw * 0.017453292519943295);
    float sy = sinf(angle);
    float cy = cosf(angle);
    axis[0][0] = cy;
    axis[0][1] = sy;
    axis[0][2] = 0.0f;
    axis[1][0] = -sy;
    axis[1][1] = cy;
    axis[1][2] = 0.0f;
    axis[2][0] = 0.0f;
    axis[2][1] = 0.0f;
    axis[2][2] = 1.0f;
}

void RotatePointAroundVector(vec_t *dst, const vec_t *dir, const vec_t *point, const float degrees)
{
    float vr[3], vup[3];
    float m[3][3], im[3][3], zrot[3][3], tmpmat[3][3], rot[3][3];
    float srcSq[3];
    float d, rad, s, c;
    int minAxis, i;

    srcSq[0] = dir[0] * dir[0];
    srcSq[1] = dir[1] * dir[1];
    srcSq[2] = dir[2] * dir[2];
    minAxis = (srcSq[0] > srcSq[1]) ? 1 : 0;
    if (srcSq[2] < srcSq[minAxis])
        minAxis = 2;
    d = -dir[minAxis];
    vr[0] = dir[0] * d;
    vr[1] = dir[1] * d;
    vr[2] = dir[2] * d;
    vr[minAxis] += 1.0f;
    Vec3Normalize_core(vr);

    vup[0] = dir[1] * vr[2] - dir[2] * vr[1];
    vup[1] = dir[2] * vr[0] - dir[0] * vr[2];
    vup[2] = dir[0] * vr[1] - dir[1] * vr[0];

    m[0][0] = vr[0];
    m[0][1] = vup[0];
    m[0][2] = dir[0];
    m[1][0] = vr[1];
    m[1][1] = vup[1];
    m[1][2] = dir[1];
    m[2][0] = vr[2];
    m[2][1] = vup[2];
    m[2][2] = dir[2];

    im[0][0] = m[0][0];
    im[0][1] = m[1][0];
    im[0][2] = m[2][0];
    im[1][0] = m[0][1];
    im[1][1] = m[1][1];
    im[1][2] = m[2][1];
    im[2][0] = m[0][2];
    im[2][1] = m[1][2];
    im[2][2] = m[2][2];

    __builtin_memset(zrot, 0, sizeof(zrot));
    zrot[2][2] = 1.0f;

    rad = (float)((double)degrees * 0.017453292519943295);
    s = sinf(rad);
    c = cosf(rad);
    zrot[0][0] = c;
    zrot[0][1] = s;
    zrot[1][0] = -s;
    zrot[1][1] = c;

    MatrixMultiply(m, zrot, tmpmat);
    MatrixMultiply(tmpmat, im, rot);

    for (i = 0; i < 3; i++)
        dst[i] = rot[i][0] * point[0] + rot[i][1] * point[1] + rot[i][2] * point[2];
}

void AngleVectors(const vec_t *angles, vec_t *forward, vec_t *right, vec_t *up)
{
    float angle;
    float sr, sp, sy, cr, cp, cy;

    angle = (float)((double)angles[1] * 0.017453292519943295);
    sy = sinf(angle);
    cy = cosf(angle);
    angle = (float)((double)angles[0] * 0.017453292519943295);
    sp = sinf(angle);
    cp = cosf(angle);

    if (forward) {
        forward[0] = cp * cy;
        forward[1] = cp * sy;
        forward[2] = -sp;
    }

    if (right || up) {
        angle = (float)((double)angles[2] * 0.017453292519943295);
        sr = sinf(angle);
        cr = cosf(angle);

        if (right) {
            right[0] = -sr * sp * cy + cr * sy;
            right[1] = -sr * sp * sy - cr * cy;
            right[2] = -sr * cp;
        }

        if (up) {
            up[0] = cr * sp * cy + sr * sy;
            up[1] = cr * sp * sy - sr * cy;
            up[2] = cp * cr;
        }
    }
}

void AnglesToAxis(const vec_t *angles, vec3_t *axis)
{
    vec3_t right;
    AngleVectors(angles, axis[0], right, axis[2]);

    axis[1][0] = 0.0f - right[0];
    axis[1][1] = 0.0f - right[1];
    axis[1][2] = 0.0f - right[2];
}

void YawVectors(const vec_t yaw, vec_t *forward, vec_t *right)
{
    float angle = (float)((double)yaw * 0.017453292519943295);
    float sy = sinf(angle);
    float cy = cosf(angle);
    if (forward) {
        forward[0] = cy;
        forward[1] = sy;
        forward[2] = 0.0f;
    }
    if (right) {
        right[0] = sy;
        right[1] = -cy;
        right[2] = 0.0f;
    }
}

void VectorAngleMultiply(vec_t *vec, float angle)
{
    float radians = (float)((double)angle * 0.017453292519943295);
    float s = sinf(radians);
    float c = cosf(radians);
    float x = vec[0];
    float y = vec[1];
    vec[1] = c * y + s * x;
    vec[0] = c * x - s * y;
}

void AngleAxisToQuat(float angle, const vec_t *axis, vec_t *quat)
{
    float halfRad = (float)((double)angle * 0.008726646259971648);
    float s = sinf(halfRad);
    quat[3] = cosf(halfRad);
    quat[0] = axis[0] * s;
    quat[1] = axis[1] * s;
    quat[2] = axis[2] * s;
}

void AxisToAngles(vec3_t *axis, vec_t *angles)
{
    float sy, cy, sp, cp;
    float temp, temp2;
    float forward_comp, up_comp;
    float roll;

    vectoangles(axis[0], angles);

    {
        float yawRad = (float)(-((double)angles[1]) * 0.017453292519943295);
        sy = sinf(yawRad);
        cy = cosf(yawRad);
    }
    temp = axis[1][0] * cy - axis[1][1] * sy;
    temp2 = axis[1][0] * sy + axis[1][1] * cy;

    {
        float pitchRad = (float)(-((double)angles[0]) * 0.017453292519943295);
        sp = sinf(pitchRad);
        cp = cosf(pitchRad);
    }
    forward_comp = axis[1][2] * sp + temp * cp;
    up_comp = axis[1][2] * cp - temp * sp;

    if (temp2 == 0.0f && forward_comp == 0.0f) {
        if (up_comp > 0.0f)
            roll = -90.0f;
        else
            roll = 90.0f;
    } else {
        float fwd = __builtin_sqrtf(forward_comp * forward_comp + temp2 * temp2);
        roll = (float)(atan2((double)up_comp, (double)fwd) * -180.0 / 3.141592653589793);
    }

    if (temp2 >= 0.0f) {
        angles[2] = -roll;
    } else {
        if (roll < 0.0f)
            angles[2] = roll + 180.0f;
        else
            angles[2] = roll - 180.0f;
    }
}

const float identityMatrix44[4][4] = { { 1.0f, 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 1.0f } }; /* truncated 20->16 (blob over-capture) */

#if !COD2_PLUS_SERVER  /* defined by src/universal/com_math.cpp */
vec3_t vec3_origin = { 0.0f, 0.0f, 0.0f };
#endif

#if !COD2_PLUS_SERVER  /* defined by src/universal/com_math.cpp */
vec2_t vec2_origin = { 0.0f, 0.0f }; /* truncated 161->2 (blob over-capture) */
#endif

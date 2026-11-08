#include "common_types.h"
#include "imports.h"

bool COpenGLMatrix_Inverse(const COpenGLMatrix *_this, COpenGLMatrix *mResult);

static const float *D3DXMatrix_ConstData(const D3DXMATRIX *matrix)
{
    return (const float *)matrix;
}

static float *D3DXMatrix_Data(D3DXMATRIX *matrix)
{
    return (float *)matrix;
}

static void D3DXMatrix_Copy(float *dst, const float *src)
{
    int i;

    for (i = 0; i < 16; ++i) {
        dst[i] = src[i];
    }
}

static float D3DXMatrix_Abs(float value)
{
    return value < 0.0f ? -value : value;
}

static float D3DXMatrix_Determinant(const float *matrix)
{
    float temp[4][4];
    float determinant;
    int sign;
    int column;
    int row;

    for (row = 0; row < 4; ++row) {
        for (column = 0; column < 4; ++column) {
            temp[row][column] = matrix[row * 4 + column];
        }
    }

    determinant = 1.0f;
    sign = 1;

    for (column = 0; column < 4; ++column) {
        int pivotRow;
        float pivotAbs;
        float pivotValue;

        pivotRow = column;
        pivotAbs = D3DXMatrix_Abs(temp[column][column]);

        for (row = column + 1; row < 4; ++row) {
            float candidateAbs;

            candidateAbs = D3DXMatrix_Abs(temp[row][column]);
            if (candidateAbs > pivotAbs) {
                pivotAbs = candidateAbs;
                pivotRow = row;
            }
        }

        if (pivotAbs < 0.0005f) {
            return 0.0f;
        }

        if (pivotRow != column) {
            int swapColumn;

            sign = -sign;
            for (swapColumn = 0; swapColumn < 4; ++swapColumn) {
                float swapValue;

                swapValue = temp[column][swapColumn];
                temp[column][swapColumn] = temp[pivotRow][swapColumn];
                temp[pivotRow][swapColumn] = swapValue;
            }
        }

        pivotValue = temp[column][column];
        determinant *= pivotValue;

        for (row = column + 1; row < 4; ++row) {
            float factor;
            int reduceColumn;

            factor = temp[row][column] / pivotValue;
            for (reduceColumn = column + 1; reduceColumn < 4; ++reduceColumn) {
                temp[row][reduceColumn] -= factor * temp[column][reduceColumn];
            }
        }
    }

    return sign < 0 ? -determinant : determinant;
}

D3DXMATRIX *D3DXMatrixMultiply(D3DXMATRIX *pOut, const D3DXMATRIX *pM1, const D3DXMATRIX *pM2)
{
    float temp[16];
    int row;

    for (row = 0; row < 4; ++row) {
        int column;

        for (column = 0; column < 4; ++column) {
            temp[row * 4 + column] =
                D3DXMatrix_ConstData(pM1)[row * 4 + 0] * D3DXMatrix_ConstData(pM2)[0 * 4 + column] +
                D3DXMatrix_ConstData(pM1)[row * 4 + 1] * D3DXMatrix_ConstData(pM2)[1 * 4 + column] +
                D3DXMatrix_ConstData(pM1)[row * 4 + 2] * D3DXMatrix_ConstData(pM2)[2 * 4 + column] +
                D3DXMatrix_ConstData(pM1)[row * 4 + 3] * D3DXMatrix_ConstData(pM2)[3 * 4 + column];
        }
    }

    D3DXMatrix_Copy(D3DXMatrix_Data(pOut), temp);
    return pOut;
}

intptr_t D3DXVec4Transform(float (*pOut)[10][16], const D3DXVECTOR4 *pV, const D3DXMATRIX *pM)
{
    float *out;
    const float *matrix;

    out = (float *)pOut;
    matrix = D3DXMatrix_ConstData(pM);

    out[0] = pV->x * matrix[0] + pV->y * matrix[4] + pV->z * matrix[8] + pV->w * matrix[12];
    out[1] = pV->x * matrix[1] + pV->y * matrix[5] + pV->z * matrix[9] + pV->w * matrix[13];
    out[2] = pV->x * matrix[2] + pV->y * matrix[6] + pV->z * matrix[10] + pV->w * matrix[14];
    out[3] = pV->x * matrix[3] + pV->y * matrix[7] + pV->z * matrix[11] + pV->w * matrix[15];

    return (intptr_t)pOut;
}

const char *DXGetErrorDescription9A(HRESULT hr)
{
    (void)hr;
    return NULL;
}

D3DXMATRIX *D3DXMatrixInverse(D3DXMATRIX *pOut, FLOAT *pDeterminant, const D3DXMATRIX *pM)
{
    float inverseStorage[16];
    float determinant;

    determinant = D3DXMatrix_Determinant(D3DXMatrix_ConstData(pM));

    if (pDeterminant) {
        *pDeterminant = determinant;
    }

    if (!COpenGLMatrix_Inverse((const COpenGLMatrix *)pM, (COpenGLMatrix *)inverseStorage)) {
        if (pDeterminant) {
            *pDeterminant = 0.0f;
        }

        D3DXMatrix_Copy(D3DXMatrix_Data(pOut), D3DXMatrix_ConstData(pM));
        return pOut;
    }

    D3DXMatrix_Copy(D3DXMatrix_Data(pOut), inverseStorage);
    return pOut;
}

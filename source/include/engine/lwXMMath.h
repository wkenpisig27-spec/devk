#pragma once

// Public XM* names for D3DX-layout math already implemented in lwD3DXCompat.h
// (or d3dx9.h on a dual-build).
//
// These are storage types with D3DX operators — not DirectX::XMVECTOR / XMMATRIX.
// Do not `using namespace DirectX` in the same TU as these global names.
// Asset I/O stays 12-byte vec3 / 64-byte matrix.

#include "MindPowerRenderConfig.h"

#ifndef XM_PI
#define XM_PI D3DX_PI
#endif
#ifndef XM_1DIVPI
#define XM_1DIVPI D3DX_1BYPI
#endif

#ifdef __cplusplus

#if MINDPOWER_USE_D3D9_DEVICE
typedef D3DXVECTOR2 XMVECTOR2;
typedef D3DXVECTOR3 XMVECTOR3;
typedef D3DXVECTOR4 XMVECTOR4;
typedef D3DXMATRIX XMMATRIX;
typedef D3DXMATRIX XMMATRIXA16;
typedef D3DXQUATERNION XMQUATERNION;
typedef D3DXPLANE XMPLANE;
typedef D3DXCOLOR XMCOLORF;
typedef XMVECTOR2* LPXMVECTOR2;
typedef XMVECTOR3* LPXMVECTOR3;
typedef XMVECTOR4* LPXMVECTOR4;
typedef XMMATRIX* LPXMMATRIX;
typedef XMMATRIXA16* LPXMMATRIXA16;
typedef XMQUATERNION* LPXMQUATERNION;
typedef XMPLANE* LPXMPLANE;
typedef XMCOLORF* LPXMCOLORF;
#endif

#if !MINDPOWER_USE_D3D9_DEVICE
using DirectX::XMConvertToRadians;
using DirectX::XMConvertToDegrees;
#else
#ifndef XMConvertToRadians
#define XMConvertToRadians D3DXToRadian
#define XMConvertToDegrees D3DXToDegree
#endif
#endif

D3DXINLINE XMMATRIX* XMMatrixIdentity(XMMATRIX* pOut) { return D3DXMatrixIdentity(pOut); }
D3DXINLINE BOOL XMMatrixIsIdentity(CONST XMMATRIX* pM) { return D3DXMatrixIsIdentity(pM); }
D3DXINLINE XMMATRIX* XMMatrixMultiply(XMMATRIX* pOut, CONST XMMATRIX* pM1, CONST XMMATRIX* pM2) { return D3DXMatrixMultiply(pOut, pM1, pM2); }
D3DXINLINE XMMATRIX* XMMatrixTranspose(XMMATRIX* pOut, CONST XMMATRIX* pM) { return D3DXMatrixTranspose(pOut, pM); }
D3DXINLINE XMMATRIX* XMMatrixInverse(XMMATRIX* pOut, FLOAT* pDeterminant, CONST XMMATRIX* pM) { return D3DXMatrixInverse(pOut, pDeterminant, pM); }
D3DXINLINE FLOAT XMMatrixDeterminant(CONST XMMATRIX* pM) { return D3DXMatrixDeterminant(pM); }
D3DXINLINE XMMATRIX* XMMatrixScaling(XMMATRIX* pOut, FLOAT sx, FLOAT sy, FLOAT sz) { return D3DXMatrixScaling(pOut, sx, sy, sz); }
D3DXINLINE XMMATRIX* XMMatrixTranslation(XMMATRIX* pOut, FLOAT x, FLOAT y, FLOAT z) { return D3DXMatrixTranslation(pOut, x, y, z); }
D3DXINLINE XMMATRIX* XMMatrixRotationX(XMMATRIX* pOut, FLOAT a) { return D3DXMatrixRotationX(pOut, a); }
D3DXINLINE XMMATRIX* XMMatrixRotationY(XMMATRIX* pOut, FLOAT a) { return D3DXMatrixRotationY(pOut, a); }
D3DXINLINE XMMATRIX* XMMatrixRotationZ(XMMATRIX* pOut, FLOAT a) { return D3DXMatrixRotationZ(pOut, a); }
D3DXINLINE XMMATRIX* XMMatrixRotationAxis(XMMATRIX* pOut, CONST XMVECTOR3* pV, FLOAT a) { return D3DXMatrixRotationAxis(pOut, pV, a); }
D3DXINLINE XMMATRIX* XMMatrixRotationQuaternion(XMMATRIX* pOut, CONST XMQUATERNION* pQ) { return D3DXMatrixRotationQuaternion(pOut, pQ); }
D3DXINLINE XMMATRIX* XMMatrixRotationYawPitchRoll(XMMATRIX* pOut, FLOAT yaw, FLOAT pitch, FLOAT roll) { return D3DXMatrixRotationYawPitchRoll(pOut, yaw, pitch, roll); }
D3DXINLINE XMMATRIX* XMMatrixLookAtLH(XMMATRIX* pOut, CONST XMVECTOR3* pEye, CONST XMVECTOR3* pAt, CONST XMVECTOR3* pUp) { return D3DXMatrixLookAtLH(pOut, pEye, pAt, pUp); }
D3DXINLINE XMMATRIX* XMMatrixLookAtRH(XMMATRIX* pOut, CONST XMVECTOR3* pEye, CONST XMVECTOR3* pAt, CONST XMVECTOR3* pUp) { return D3DXMatrixLookAtRH(pOut, pEye, pAt, pUp); }
D3DXINLINE XMMATRIX* XMMatrixPerspectiveFovLH(XMMATRIX* pOut, FLOAT fov, FLOAT aspect, FLOAT zn, FLOAT zf) { return D3DXMatrixPerspectiveFovLH(pOut, fov, aspect, zn, zf); }
D3DXINLINE XMMATRIX* XMMatrixPerspectiveFovRH(XMMATRIX* pOut, FLOAT fov, FLOAT aspect, FLOAT zn, FLOAT zf) { return D3DXMatrixPerspectiveFovRH(pOut, fov, aspect, zn, zf); }
D3DXINLINE XMMATRIX* XMMatrixOrthoLH(XMMATRIX* pOut, FLOAT w, FLOAT h, FLOAT zn, FLOAT zf) { return D3DXMatrixOrthoLH(pOut, w, h, zn, zf); }
D3DXINLINE XMMATRIX* XMMatrixOrthoRH(XMMATRIX* pOut, FLOAT w, FLOAT h, FLOAT zn, FLOAT zf) { return D3DXMatrixOrthoRH(pOut, w, h, zn, zf); }
D3DXINLINE XMMATRIX* XMMatrixTransformation2D(
    XMMATRIX* pOut,
    CONST XMVECTOR2* pScalingCenter, FLOAT scalingRotation, CONST XMVECTOR2* pScaling,
    CONST XMVECTOR2* pRotationCenter, FLOAT rotation, CONST XMVECTOR2* pTranslation)
{
    return D3DXMatrixTransformation2D(pOut, pScalingCenter, scalingRotation, pScaling, pRotationCenter, rotation, pTranslation);
}

D3DXINLINE FLOAT XMVector2Length(CONST XMVECTOR2* pV) { return D3DXVec2Length(pV); }
D3DXINLINE FLOAT XMVector2LengthSq(CONST XMVECTOR2* pV) { return D3DXVec2LengthSq(pV); }
D3DXINLINE FLOAT XMVector2Dot(CONST XMVECTOR2* a, CONST XMVECTOR2* b) { return D3DXVec2Dot(a, b); }
D3DXINLINE XMVECTOR2* XMVector2Add(XMVECTOR2* o, CONST XMVECTOR2* a, CONST XMVECTOR2* b) { return D3DXVec2Add(o, a, b); }
D3DXINLINE XMVECTOR2* XMVector2Subtract(XMVECTOR2* o, CONST XMVECTOR2* a, CONST XMVECTOR2* b) { return D3DXVec2Subtract(o, a, b); }
D3DXINLINE XMVECTOR2* XMVector2Scale(XMVECTOR2* o, CONST XMVECTOR2* v, FLOAT s) { return D3DXVec2Scale(o, v, s); }
D3DXINLINE XMVECTOR2* XMVector2Lerp(XMVECTOR2* o, CONST XMVECTOR2* a, CONST XMVECTOR2* b, FLOAT t) { return D3DXVec2Lerp(o, a, b, t); }
D3DXINLINE XMVECTOR2* XMVector2Normalize(XMVECTOR2* o, CONST XMVECTOR2* v) { return D3DXVec2Normalize(o, v); }
D3DXINLINE XMVECTOR2* XMVector2TransformCoord(XMVECTOR2* o, CONST XMVECTOR2* v, CONST XMMATRIX* m) { return D3DXVec2TransformCoord(o, v, m); }
D3DXINLINE XMVECTOR4* XMVector2Transform(XMVECTOR4* o, CONST XMVECTOR2* v, CONST XMMATRIX* m) { return D3DXVec2Transform(o, v, m); }

D3DXINLINE FLOAT XMVector3Length(CONST XMVECTOR3* pV) { return D3DXVec3Length(pV); }
D3DXINLINE FLOAT XMVector3LengthSq(CONST XMVECTOR3* pV) { return D3DXVec3LengthSq(pV); }
D3DXINLINE FLOAT XMVector3Dot(CONST XMVECTOR3* a, CONST XMVECTOR3* b) { return D3DXVec3Dot(a, b); }
D3DXINLINE XMVECTOR3* XMVector3Cross(XMVECTOR3* o, CONST XMVECTOR3* a, CONST XMVECTOR3* b) { return D3DXVec3Cross(o, a, b); }
D3DXINLINE XMVECTOR3* XMVector3Add(XMVECTOR3* o, CONST XMVECTOR3* a, CONST XMVECTOR3* b) { return D3DXVec3Add(o, a, b); }
D3DXINLINE XMVECTOR3* XMVector3Subtract(XMVECTOR3* o, CONST XMVECTOR3* a, CONST XMVECTOR3* b) { return D3DXVec3Subtract(o, a, b); }
D3DXINLINE XMVECTOR3* XMVector3Scale(XMVECTOR3* o, CONST XMVECTOR3* v, FLOAT s) { return D3DXVec3Scale(o, v, s); }
D3DXINLINE XMVECTOR3* XMVector3Lerp(XMVECTOR3* o, CONST XMVECTOR3* a, CONST XMVECTOR3* b, FLOAT t) { return D3DXVec3Lerp(o, a, b, t); }
D3DXINLINE XMVECTOR3* XMVector3Normalize(XMVECTOR3* o, CONST XMVECTOR3* v) { return D3DXVec3Normalize(o, v); }
D3DXINLINE XMVECTOR4* XMVector3Transform(XMVECTOR4* o, CONST XMVECTOR3* v, CONST XMMATRIX* m) { return D3DXVec3Transform(o, v, m); }
D3DXINLINE XMVECTOR3* XMVector3TransformCoord(XMVECTOR3* o, CONST XMVECTOR3* v, CONST XMMATRIX* m) { return D3DXVec3TransformCoord(o, v, m); }
D3DXINLINE XMVECTOR3* XMVector3TransformNormal(XMVECTOR3* o, CONST XMVECTOR3* v, CONST XMMATRIX* m) { return D3DXVec3TransformNormal(o, v, m); }

D3DXINLINE FLOAT XMVector4Length(CONST XMVECTOR4* pV) { return D3DXVec4Length(pV); }
D3DXINLINE FLOAT XMVector4Dot(CONST XMVECTOR4* a, CONST XMVECTOR4* b) { return D3DXVec4Dot(a, b); }
D3DXINLINE XMVECTOR4* XMVector4Transform(XMVECTOR4* o, CONST XMVECTOR4* v, CONST XMMATRIX* m) { return D3DXVec4Transform(o, v, m); }

D3DXINLINE XMQUATERNION* XMQuaternionIdentity(XMQUATERNION* o) { return D3DXQuaternionIdentity(o); }
D3DXINLINE XMQUATERNION* XMQuaternionConjugate(XMQUATERNION* o, CONST XMQUATERNION* q) { return D3DXQuaternionConjugate(o, q); }
D3DXINLINE XMQUATERNION* XMQuaternionMultiply(XMQUATERNION* o, CONST XMQUATERNION* a, CONST XMQUATERNION* b) { return D3DXQuaternionMultiply(o, a, b); }
D3DXINLINE XMQUATERNION* XMQuaternionRotationAxis(XMQUATERNION* o, CONST XMVECTOR3* axis, FLOAT a) { return D3DXQuaternionRotationAxis(o, axis, a); }
D3DXINLINE XMQUATERNION* XMQuaternionNormalize(XMQUATERNION* o, CONST XMQUATERNION* q) { return D3DXQuaternionNormalize(o, q); }
D3DXINLINE XMQUATERNION* XMQuaternionInverse(XMQUATERNION* o, CONST XMQUATERNION* q) { return D3DXQuaternionInverse(o, q); }

D3DXINLINE XMVECTOR3* XMPlaneIntersectLine(XMVECTOR3* o, CONST XMPLANE* p, CONST XMVECTOR3* v1, CONST XMVECTOR3* v2) { return D3DXPlaneIntersectLine(o, p, v1, v2); }
D3DXINLINE XMPLANE* XMPlaneNormalize(XMPLANE* o, CONST XMPLANE* p) { return D3DXPlaneNormalize(o, p); }
D3DXINLINE XMPLANE* XMPlaneFromPointNormal(XMPLANE* o, CONST XMVECTOR3* v, CONST XMVECTOR3* n) { return D3DXPlaneFromPointNormal(o, v, n); }
D3DXINLINE XMPLANE* XMPlaneFromPoints(XMPLANE* o, CONST XMVECTOR3* p1, CONST XMVECTOR3* p2, CONST XMVECTOR3* p3) { return D3DXPlaneFromPoints(o, p1, p2, p3); }

D3DXINLINE BOOL XMIntersectTri(
    CONST XMVECTOR3* p0, CONST XMVECTOR3* p1, CONST XMVECTOR3* p2,
    CONST XMVECTOR3* pRayPos, CONST XMVECTOR3* pRayDir,
    FLOAT* pU, FLOAT* pV, FLOAT* pDist)
{
    return D3DXIntersectTri(p0, p1, p2, pRayPos, pRayDir, pU, pV, pDist);
}
D3DXINLINE BOOL XMBoxBoundProbe(
    CONST XMVECTOR3* pMin, CONST XMVECTOR3* pMax,
    CONST XMVECTOR3* pRayPosition, CONST XMVECTOR3* pRayDirection)
{
    return D3DXBoxBoundProbe(pMin, pMax, pRayPosition, pRayDirection);
}
D3DXINLINE XMCOLORF* XMColorLerp(XMCOLORF* o, CONST XMCOLORF* a, CONST XMCOLORF* b, FLOAT t) { return D3DXColorLerp(o, a, b, t); }

#endif

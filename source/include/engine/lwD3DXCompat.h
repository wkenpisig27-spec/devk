#pragma once

// DirectXMath-backed math for the DX11-only build.
// XMVECTOR / XMMATRIX / XMCOLORF storage matches d3dx9 (asset I/O + operators).
// Not DirectX::XMVECTOR / DirectX::XMMATRIX. D3DX* names remain as typedef aliases.

#include <math.h>
#include <float.h>
#include <string.h>
#include <DirectXMath.h>

#ifndef D3DXINLINE
#ifdef _MSC_VER
#define D3DXINLINE __forceinline
#else
#define D3DXINLINE inline
#endif
#endif

#ifndef D3DX_DEFAULT
#define D3DX_DEFAULT         ((UINT)-1)
#define D3DX_DEFAULT_NONPOW2 ((UINT)-2)
#define D3DX_DEFAULT_FLOAT   FLT_MAX
#define D3DX_FROM_FILE       ((UINT)-3)
#endif

#define D3DX_PI    ((FLOAT)3.141592654f)
#define D3DX_1BYPI ((FLOAT)0.318309886f)
#define D3DXToRadian(degree) ((degree) * (D3DX_PI / 180.0f))
#define D3DXToDegree(radian) ((radian) * (180.0f / D3DX_PI))

#define D3DX_FILTER_NONE     (1 << 0)
#define D3DX_FILTER_POINT    (2 << 0)
#define D3DX_FILTER_LINEAR   (3 << 0)
#define D3DX_FILTER_TRIANGLE (4 << 0)
#define D3DX_FILTER_BOX      (5 << 0)
#define D3DX_FILTER_MIRROR_U (1 << 16)
#define D3DX_FILTER_MIRROR_V (2 << 16)
#define D3DX_FILTER_MIRROR_W (4 << 16)
#define D3DX_FILTER_MIRROR   (7 << 16)

#define D3DXFX_DONOTSAVESTATE       (1 << 0)
#define D3DXFX_DONOTSAVESHADERSTATE (1 << 1)
#define D3DXSPRITE_ALPHABLEND       (1 << 4)

#define D3DXERR_INVALIDDATA MAKE_HRESULT(1, 0x876, 2905)

typedef enum _D3DXIMAGE_FILEFORMAT
{
    D3DXIFF_BMP = 0,
    D3DXIFF_JPG = 1,
    D3DXIFF_TGA = 2,
    D3DXIFF_PNG = 3,
    D3DXIFF_DDS = 4,
    D3DXIFF_PPM = 5,
    D3DXIFF_DIB = 6,
    D3DXIFF_HDR = 7,
    D3DXIFF_PFM = 8,
    D3DXIFF_FORCE_DWORD = 0x7fffffff
} D3DXIMAGE_FILEFORMAT;

typedef struct _D3DXIMAGE_INFO
{
    UINT Width;
    UINT Height;
    UINT Depth;
    UINT MipLevels;
    D3DFORMAT Format;
    D3DRESOURCETYPE ResourceType;
    D3DXIMAGE_FILEFORMAT ImageFileFormat;
} D3DXIMAGE_INFO;

typedef struct _D3DXMACRO
{
    LPCSTR Name;
    LPCSTR Definition;
} D3DXMACRO, *LPD3DXMACRO;

typedef LPCSTR D3DXHANDLE;

struct ID3DXBuffer : public IUnknown {};
struct ID3DXSprite : public IUnknown {};
struct ID3DXEffect : public IUnknown {};
struct ID3DXConstantTable : public IUnknown {};
struct ID3DXRenderToSurface : public IUnknown {};
struct ID3DXInclude;
typedef ID3DXBuffer* LPD3DXBUFFER;
typedef ID3DXSprite* LPD3DXSPRITE;
typedef ID3DXEffect* LPD3DXEFFECT;
typedef ID3DXConstantTable* LPD3DXCONSTANTTABLE;
typedef ID3DXRenderToSurface* LPD3DXRENDERTOSURFACE;

#ifdef __cplusplus

typedef struct XMVECTOR2
{
    FLOAT x, y;
    XMVECTOR2() {}
    XMVECTOR2(CONST FLOAT* p) { x = p[0]; y = p[1]; }
    XMVECTOR2(FLOAT x, FLOAT y) : x(x), y(y) {}
    operator FLOAT*() { return &x; }
    operator CONST FLOAT*() const { return &x; }
    XMVECTOR2& operator+=(CONST XMVECTOR2& v) { x += v.x; y += v.y; return *this; }
    XMVECTOR2& operator-=(CONST XMVECTOR2& v) { x -= v.x; y -= v.y; return *this; }
    XMVECTOR2& operator*=(FLOAT s) { x *= s; y *= s; return *this; }
    XMVECTOR2& operator/=(FLOAT s) { x /= s; y /= s; return *this; }
    XMVECTOR2 operator+() const { return *this; }
    XMVECTOR2 operator-() const { return XMVECTOR2(-x, -y); }
    XMVECTOR2 operator+(CONST XMVECTOR2& v) const { return XMVECTOR2(x + v.x, y + v.y); }
    XMVECTOR2 operator-(CONST XMVECTOR2& v) const { return XMVECTOR2(x - v.x, y - v.y); }
    XMVECTOR2 operator*(FLOAT s) const { return XMVECTOR2(x * s, y * s); }
    XMVECTOR2 operator/(FLOAT s) const { return XMVECTOR2(x / s, y / s); }
    friend XMVECTOR2 operator*(FLOAT s, CONST XMVECTOR2& v) { return XMVECTOR2(v.x * s, v.y * s); }
    BOOL operator==(CONST XMVECTOR2& v) const { return x == v.x && y == v.y; }
    BOOL operator!=(CONST XMVECTOR2& v) const { return !(*this == v); }
} XMVECTOR2, *LPXMVECTOR2;

typedef struct XMVECTOR3 : public D3DVECTOR
{
    XMVECTOR3() {}
    XMVECTOR3(CONST FLOAT* p) { x = p[0]; y = p[1]; z = p[2]; }
    XMVECTOR3(CONST D3DVECTOR& v) { x = v.x; y = v.y; z = v.z; }
    XMVECTOR3(FLOAT x, FLOAT y, FLOAT z) { this->x = x; this->y = y; this->z = z; }
    operator FLOAT*() { return &x; }
    operator CONST FLOAT*() const { return &x; }
    XMVECTOR3& operator+=(CONST XMVECTOR3& v) { x += v.x; y += v.y; z += v.z; return *this; }
    XMVECTOR3& operator-=(CONST XMVECTOR3& v) { x -= v.x; y -= v.y; z -= v.z; return *this; }
    XMVECTOR3& operator*=(FLOAT s) { x *= s; y *= s; z *= s; return *this; }
    XMVECTOR3& operator/=(FLOAT s) { x /= s; y /= s; z /= s; return *this; }
    XMVECTOR3 operator+() const { return *this; }
    XMVECTOR3 operator-() const { return XMVECTOR3(-x, -y, -z); }
    XMVECTOR3 operator+(CONST XMVECTOR3& v) const { return XMVECTOR3(x + v.x, y + v.y, z + v.z); }
    XMVECTOR3 operator-(CONST XMVECTOR3& v) const { return XMVECTOR3(x - v.x, y - v.y, z - v.z); }
    XMVECTOR3 operator*(FLOAT s) const { return XMVECTOR3(x * s, y * s, z * s); }
    XMVECTOR3 operator/(FLOAT s) const { return XMVECTOR3(x / s, y / s, z / s); }
    friend XMVECTOR3 operator*(FLOAT s, CONST XMVECTOR3& v) { return XMVECTOR3(v.x * s, v.y * s, v.z * s); }
    BOOL operator==(CONST XMVECTOR3& v) const { return x == v.x && y == v.y && z == v.z; }
    BOOL operator!=(CONST XMVECTOR3& v) const { return !(*this == v); }
} XMVECTOR3, *LPXMVECTOR3;

typedef struct XMVECTOR4
{
    FLOAT x, y, z, w;
    XMVECTOR4() {}
    XMVECTOR4(CONST FLOAT* p) { x = p[0]; y = p[1]; z = p[2]; w = p[3]; }
    XMVECTOR4(CONST D3DVECTOR& v, FLOAT w) : x(v.x), y(v.y), z(v.z), w(w) {}
    XMVECTOR4(FLOAT x, FLOAT y, FLOAT z, FLOAT w) : x(x), y(y), z(z), w(w) {}
    operator FLOAT*() { return &x; }
    operator CONST FLOAT*() const { return &x; }
    XMVECTOR4& operator+=(CONST XMVECTOR4& v) { x += v.x; y += v.y; z += v.z; w += v.w; return *this; }
    XMVECTOR4& operator-=(CONST XMVECTOR4& v) { x -= v.x; y -= v.y; z -= v.z; w -= v.w; return *this; }
    XMVECTOR4& operator*=(FLOAT s) { x *= s; y *= s; z *= s; w *= s; return *this; }
    XMVECTOR4& operator/=(FLOAT s) { x /= s; y /= s; z /= s; w /= s; return *this; }
    XMVECTOR4 operator+() const { return *this; }
    XMVECTOR4 operator-() const { return XMVECTOR4(-x, -y, -z, -w); }
    XMVECTOR4 operator+(CONST XMVECTOR4& v) const { return XMVECTOR4(x + v.x, y + v.y, z + v.z, w + v.w); }
    XMVECTOR4 operator-(CONST XMVECTOR4& v) const { return XMVECTOR4(x - v.x, y - v.y, z - v.z, w - v.w); }
    XMVECTOR4 operator*(FLOAT s) const { return XMVECTOR4(x * s, y * s, z * s, w * s); }
    XMVECTOR4 operator/(FLOAT s) const { return XMVECTOR4(x / s, y / s, z / s, w / s); }
    friend XMVECTOR4 operator*(FLOAT s, CONST XMVECTOR4& v) { return XMVECTOR4(v.x * s, v.y * s, v.z * s, v.w * s); }
    BOOL operator==(CONST XMVECTOR4& v) const { return x == v.x && y == v.y && z == v.z && w == v.w; }
    BOOL operator!=(CONST XMVECTOR4& v) const { return !(*this == v); }
} XMVECTOR4, *LPXMVECTOR4;

typedef struct XMMATRIX : public D3DMATRIX
{
    XMMATRIX() {}
    XMMATRIX(CONST FLOAT* p) { memcpy(m, p, sizeof(D3DMATRIX)); }
    XMMATRIX(CONST D3DMATRIX& n) { memcpy(m, n.m, sizeof(D3DMATRIX)); }
    XMMATRIX(FLOAT f11, FLOAT f12, FLOAT f13, FLOAT f14,
               FLOAT f21, FLOAT f22, FLOAT f23, FLOAT f24,
               FLOAT f31, FLOAT f32, FLOAT f33, FLOAT f34,
               FLOAT f41, FLOAT f42, FLOAT f43, FLOAT f44)
    {
        _11 = f11; _12 = f12; _13 = f13; _14 = f14;
        _21 = f21; _22 = f22; _23 = f23; _24 = f24;
        _31 = f31; _32 = f32; _33 = f33; _34 = f34;
        _41 = f41; _42 = f42; _43 = f43; _44 = f44;
    }
    FLOAT& operator()(UINT r, UINT c) { return m[r][c]; }
    FLOAT operator()(UINT r, UINT c) const { return m[r][c]; }
    operator FLOAT*() { return &_11; }
    operator CONST FLOAT*() const { return &_11; }
    XMMATRIX& operator+=(CONST XMMATRIX& n);
    XMMATRIX& operator-=(CONST XMMATRIX& n);
    XMMATRIX& operator*=(CONST XMMATRIX& n);
    XMMATRIX& operator*=(FLOAT s);
    XMMATRIX& operator/=(FLOAT s);
    XMMATRIX operator+() const { return *this; }
    XMMATRIX operator-() const;
    XMMATRIX operator*(CONST XMMATRIX& n) const;
    XMMATRIX operator+(CONST XMMATRIX& n) const;
    XMMATRIX operator-(CONST XMMATRIX& n) const;
    XMMATRIX operator*(FLOAT s) const;
    XMMATRIX operator/(FLOAT s) const;
    friend XMMATRIX operator*(FLOAT s, CONST XMMATRIX& n);
    BOOL operator==(CONST XMMATRIX& n) const { return memcmp(m, n.m, sizeof(D3DMATRIX)) == 0; }
    BOOL operator!=(CONST XMMATRIX& n) const { return !(*this == n); }
} XMMATRIX, *LPXMMATRIX;

typedef XMMATRIX XMMATRIXA16;
typedef XMMATRIXA16* LPXMMATRIXA16;

typedef struct XMQUATERNION
{
    FLOAT x, y, z, w;
    XMQUATERNION() {}
    XMQUATERNION(CONST FLOAT* p) { x = p[0]; y = p[1]; z = p[2]; w = p[3]; }
    XMQUATERNION(FLOAT x, FLOAT y, FLOAT z, FLOAT w) : x(x), y(y), z(z), w(w) {}
    operator FLOAT*() { return &x; }
    operator CONST FLOAT*() const { return &x; }
    XMQUATERNION& operator+=(CONST XMQUATERNION& q) { x += q.x; y += q.y; z += q.z; w += q.w; return *this; }
    XMQUATERNION& operator-=(CONST XMQUATERNION& q) { x -= q.x; y -= q.y; z -= q.z; w -= q.w; return *this; }
    XMQUATERNION& operator*=(CONST XMQUATERNION& q);
    XMQUATERNION& operator*=(FLOAT s) { x *= s; y *= s; z *= s; w *= s; return *this; }
    XMQUATERNION& operator/=(FLOAT s) { x /= s; y /= s; z /= s; w /= s; return *this; }
    XMQUATERNION operator+() const { return *this; }
    XMQUATERNION operator-() const { return XMQUATERNION(-x, -y, -z, -w); }
    XMQUATERNION operator+(CONST XMQUATERNION& q) const { return XMQUATERNION(x + q.x, y + q.y, z + q.z, w + q.w); }
    XMQUATERNION operator-(CONST XMQUATERNION& q) const { return XMQUATERNION(x - q.x, y - q.y, z - q.z, w - q.w); }
    XMQUATERNION operator*(CONST XMQUATERNION& q) const;
    XMQUATERNION operator*(FLOAT s) const { return XMQUATERNION(x * s, y * s, z * s, w * s); }
    XMQUATERNION operator/(FLOAT s) const { return XMQUATERNION(x / s, y / s, z / s, w / s); }
    friend XMQUATERNION operator*(FLOAT s, CONST XMQUATERNION& q) { return XMQUATERNION(q.x * s, q.y * s, q.z * s, q.w * s); }
    BOOL operator==(CONST XMQUATERNION& q) const { return x == q.x && y == q.y && z == q.z && w == q.w; }
    BOOL operator!=(CONST XMQUATERNION& q) const { return !(*this == q); }
} XMQUATERNION, *LPXMQUATERNION;

typedef struct XMPLANE
{
    FLOAT a, b, c, d;
    XMPLANE() {}
    XMPLANE(CONST FLOAT* p) { a = p[0]; b = p[1]; c = p[2]; d = p[3]; }
    XMPLANE(FLOAT a, FLOAT b, FLOAT c, FLOAT d) : a(a), b(b), c(c), d(d) {}
    operator FLOAT*() { return &a; }
    operator CONST FLOAT*() const { return &a; }
    XMPLANE operator+() const { return *this; }
    XMPLANE operator-() const { return XMPLANE(-a, -b, -c, -d); }
    BOOL operator==(CONST XMPLANE& p) const { return a == p.a && b == p.b && c == p.c && d == p.d; }
    BOOL operator!=(CONST XMPLANE& p) const { return !(*this == p); }
} XMPLANE, *LPXMPLANE;

typedef struct XMCOLORF
{
    FLOAT r, g, b, a;
    XMCOLORF() {}
    XMCOLORF(DWORD argb)
    {
        const FLOAT f = 1.0f / 255.0f;
        r = f * (FLOAT)(unsigned char)(argb >> 16);
        g = f * (FLOAT)(unsigned char)(argb >> 8);
        b = f * (FLOAT)(unsigned char)(argb >> 0);
        a = f * (FLOAT)(unsigned char)(argb >> 24);
    }
    XMCOLORF(CONST FLOAT* p) { r = p[0]; g = p[1]; b = p[2]; a = p[3]; }
    XMCOLORF(CONST D3DCOLORVALUE& c) { r = c.r; g = c.g; b = c.b; a = c.a; }
    XMCOLORF(FLOAT r, FLOAT g, FLOAT b, FLOAT a) : r(r), g(g), b(b), a(a) {}
    operator DWORD() const
    {
        DWORD R = r >= 1.0f ? 0xff : r <= 0.0f ? 0u : (DWORD)(r * 255.0f + 0.5f);
        DWORD G = g >= 1.0f ? 0xff : g <= 0.0f ? 0u : (DWORD)(g * 255.0f + 0.5f);
        DWORD B = b >= 1.0f ? 0xff : b <= 0.0f ? 0u : (DWORD)(b * 255.0f + 0.5f);
        DWORD A = a >= 1.0f ? 0xff : a <= 0.0f ? 0u : (DWORD)(a * 255.0f + 0.5f);
        return (A << 24) | (R << 16) | (G << 8) | B;
    }
    operator FLOAT*() { return &r; }
    operator CONST FLOAT*() const { return &r; }
    operator D3DCOLORVALUE*() { return (D3DCOLORVALUE*)&r; }
    operator CONST D3DCOLORVALUE*() const { return (CONST D3DCOLORVALUE*)&r; }
    operator D3DCOLORVALUE&() { return *(D3DCOLORVALUE*)&r; }
    operator CONST D3DCOLORVALUE&() const { return *(CONST D3DCOLORVALUE*)&r; }
    XMCOLORF& operator+=(CONST XMCOLORF& c) { r += c.r; g += c.g; b += c.b; a += c.a; return *this; }
    XMCOLORF& operator-=(CONST XMCOLORF& c) { r -= c.r; g -= c.g; b -= c.b; a -= c.a; return *this; }
    XMCOLORF& operator*=(FLOAT s) { r *= s; g *= s; b *= s; a *= s; return *this; }
    XMCOLORF& operator/=(FLOAT s) { r /= s; g /= s; b /= s; a /= s; return *this; }
    XMCOLORF operator+() const { return *this; }
    XMCOLORF operator-() const { return XMCOLORF(-r, -g, -b, -a); }
    XMCOLORF operator+(CONST XMCOLORF& c) const { return XMCOLORF(r + c.r, g + c.g, b + c.b, a + c.a); }
    XMCOLORF operator-(CONST XMCOLORF& c) const { return XMCOLORF(r - c.r, g - c.g, b - c.b, a - c.a); }
    XMCOLORF operator*(FLOAT s) const { return XMCOLORF(r * s, g * s, b * s, a * s); }
    XMCOLORF operator/(FLOAT s) const { return XMCOLORF(r / s, g / s, b / s, a / s); }
    friend XMCOLORF operator*(FLOAT s, CONST XMCOLORF& c) { return XMCOLORF(c.r * s, c.g * s, c.b * s, c.a * s); }
    BOOL operator==(CONST XMCOLORF& c) const { return r == c.r && g == c.g && b == c.b && a == c.a; }
    BOOL operator!=(CONST XMCOLORF& c) const { return !(*this == c); }
} XMCOLORF, *LPXMCOLORF;

D3DXINLINE DirectX::XMMATRIX lwXMLoadM(CONST XMMATRIX* m)
{
    return DirectX::XMLoadFloat4x4(reinterpret_cast<const DirectX::XMFLOAT4X4*>(m));
}
D3DXINLINE XMMATRIX* lwXMStoreM(XMMATRIX* o, DirectX::FXMMATRIX m)
{
    DirectX::XMStoreFloat4x4(reinterpret_cast<DirectX::XMFLOAT4X4*>(o), m);
    return o;
}

D3DXINLINE XMMATRIX* D3DXMatrixIdentity(XMMATRIX* pOut)
{
    return lwXMStoreM(pOut, DirectX::XMMatrixIdentity());
}
D3DXINLINE BOOL D3DXMatrixIsIdentity(CONST XMMATRIX* pM)
{
    return DirectX::XMMatrixIsIdentity(lwXMLoadM(pM));
}
D3DXINLINE XMMATRIX* D3DXMatrixMultiply(XMMATRIX* pOut, CONST XMMATRIX* pM1, CONST XMMATRIX* pM2)
{
    return lwXMStoreM(pOut, DirectX::XMMatrixMultiply(lwXMLoadM(pM1), lwXMLoadM(pM2)));
}
D3DXINLINE XMMATRIX* D3DXMatrixTranspose(XMMATRIX* pOut, CONST XMMATRIX* pM)
{
    return lwXMStoreM(pOut, DirectX::XMMatrixTranspose(lwXMLoadM(pM)));
}
D3DXINLINE XMMATRIX* D3DXMatrixInverse(XMMATRIX* pOut, FLOAT* pDeterminant, CONST XMMATRIX* pM)
{
    DirectX::XMVECTOR det;
    DirectX::XMMATRIX r = DirectX::XMMatrixInverse(&det, lwXMLoadM(pM));
    if (pDeterminant)
        *pDeterminant = DirectX::XMVectorGetX(det);
    return lwXMStoreM(pOut, r);
}
D3DXINLINE FLOAT D3DXMatrixDeterminant(CONST XMMATRIX* pM)
{
    return DirectX::XMVectorGetX(DirectX::XMMatrixDeterminant(lwXMLoadM(pM)));
}
D3DXINLINE XMMATRIX* D3DXMatrixScaling(XMMATRIX* pOut, FLOAT sx, FLOAT sy, FLOAT sz)
{
    return lwXMStoreM(pOut, DirectX::XMMatrixScaling(sx, sy, sz));
}
D3DXINLINE XMMATRIX* D3DXMatrixTranslation(XMMATRIX* pOut, FLOAT x, FLOAT y, FLOAT z)
{
    return lwXMStoreM(pOut, DirectX::XMMatrixTranslation(x, y, z));
}
D3DXINLINE XMMATRIX* D3DXMatrixRotationX(XMMATRIX* pOut, FLOAT a) { return lwXMStoreM(pOut, DirectX::XMMatrixRotationX(a)); }
D3DXINLINE XMMATRIX* D3DXMatrixRotationY(XMMATRIX* pOut, FLOAT a) { return lwXMStoreM(pOut, DirectX::XMMatrixRotationY(a)); }
D3DXINLINE XMMATRIX* D3DXMatrixRotationZ(XMMATRIX* pOut, FLOAT a) { return lwXMStoreM(pOut, DirectX::XMMatrixRotationZ(a)); }
D3DXINLINE XMMATRIX* D3DXMatrixRotationAxis(XMMATRIX* pOut, CONST XMVECTOR3* pV, FLOAT a)
{
    return lwXMStoreM(pOut, DirectX::XMMatrixRotationAxis(DirectX::XMLoadFloat3(reinterpret_cast<const DirectX::XMFLOAT3*>(pV)), a));
}
D3DXINLINE XMMATRIX* D3DXMatrixRotationQuaternion(XMMATRIX* pOut, CONST XMQUATERNION* pQ)
{
    return lwXMStoreM(pOut, DirectX::XMMatrixRotationQuaternion(DirectX::XMLoadFloat4(reinterpret_cast<const DirectX::XMFLOAT4*>(pQ))));
}
D3DXINLINE XMMATRIX* D3DXMatrixRotationYawPitchRoll(XMMATRIX* pOut, FLOAT yaw, FLOAT pitch, FLOAT roll)
{
    return lwXMStoreM(pOut, DirectX::XMMatrixRotationRollPitchYaw(pitch, yaw, roll));
}
D3DXINLINE XMMATRIX* D3DXMatrixLookAtLH(XMMATRIX* pOut, CONST XMVECTOR3* pEye, CONST XMVECTOR3* pAt, CONST XMVECTOR3* pUp)
{
    return lwXMStoreM(pOut, DirectX::XMMatrixLookAtLH(
        DirectX::XMLoadFloat3(reinterpret_cast<const DirectX::XMFLOAT3*>(pEye)),
        DirectX::XMLoadFloat3(reinterpret_cast<const DirectX::XMFLOAT3*>(pAt)),
        DirectX::XMLoadFloat3(reinterpret_cast<const DirectX::XMFLOAT3*>(pUp))));
}
D3DXINLINE XMMATRIX* D3DXMatrixLookAtRH(XMMATRIX* pOut, CONST XMVECTOR3* pEye, CONST XMVECTOR3* pAt, CONST XMVECTOR3* pUp)
{
    return lwXMStoreM(pOut, DirectX::XMMatrixLookAtRH(
        DirectX::XMLoadFloat3(reinterpret_cast<const DirectX::XMFLOAT3*>(pEye)),
        DirectX::XMLoadFloat3(reinterpret_cast<const DirectX::XMFLOAT3*>(pAt)),
        DirectX::XMLoadFloat3(reinterpret_cast<const DirectX::XMFLOAT3*>(pUp))));
}
D3DXINLINE XMMATRIX* D3DXMatrixPerspectiveFovLH(XMMATRIX* pOut, FLOAT fov, FLOAT aspect, FLOAT zn, FLOAT zf)
{
    return lwXMStoreM(pOut, DirectX::XMMatrixPerspectiveFovLH(fov, aspect, zn, zf));
}
D3DXINLINE XMMATRIX* D3DXMatrixPerspectiveFovRH(XMMATRIX* pOut, FLOAT fov, FLOAT aspect, FLOAT zn, FLOAT zf)
{
    return lwXMStoreM(pOut, DirectX::XMMatrixPerspectiveFovRH(fov, aspect, zn, zf));
}
D3DXINLINE XMMATRIX* D3DXMatrixOrthoLH(XMMATRIX* pOut, FLOAT w, FLOAT h, FLOAT zn, FLOAT zf)
{
    return lwXMStoreM(pOut, DirectX::XMMatrixOrthographicLH(w, h, zn, zf));
}
D3DXINLINE XMMATRIX* D3DXMatrixOrthoRH(XMMATRIX* pOut, FLOAT w, FLOAT h, FLOAT zn, FLOAT zf)
{
    return lwXMStoreM(pOut, DirectX::XMMatrixOrthographicRH(w, h, zn, zf));
}
D3DXINLINE XMMATRIX* D3DXMatrixTransformation2D(
    XMMATRIX* pOut,
    CONST XMVECTOR2* pScalingCenter, FLOAT /*scalingRotation*/, CONST XMVECTOR2* pScaling,
    CONST XMVECTOR2* pRotationCenter, FLOAT rotation, CONST XMVECTOR2* pTranslation)
{
    DirectX::XMVECTOR sc = pScalingCenter ? DirectX::XMLoadFloat2(reinterpret_cast<const DirectX::XMFLOAT2*>(pScalingCenter)) : DirectX::XMVectorZero();
    DirectX::XMVECTOR s = pScaling ? DirectX::XMLoadFloat2(reinterpret_cast<const DirectX::XMFLOAT2*>(pScaling)) : DirectX::XMVectorSplatOne();
    DirectX::XMVECTOR rc = pRotationCenter ? DirectX::XMLoadFloat2(reinterpret_cast<const DirectX::XMFLOAT2*>(pRotationCenter)) : DirectX::XMVectorZero();
    DirectX::XMVECTOR t = pTranslation ? DirectX::XMLoadFloat2(reinterpret_cast<const DirectX::XMFLOAT2*>(pTranslation)) : DirectX::XMVectorZero();
    (void)sc;
    (void)rc;
    return lwXMStoreM(pOut, DirectX::XMMatrixAffineTransformation2D(s, rc, rotation, t));
}

D3DXINLINE FLOAT D3DXVec2Length(CONST XMVECTOR2* pV) { return sqrtf(pV->x * pV->x + pV->y * pV->y); }
D3DXINLINE FLOAT D3DXVec2LengthSq(CONST XMVECTOR2* pV) { return pV->x * pV->x + pV->y * pV->y; }
D3DXINLINE FLOAT D3DXVec2Dot(CONST XMVECTOR2* a, CONST XMVECTOR2* b) { return a->x * b->x + a->y * b->y; }
D3DXINLINE XMVECTOR2* D3DXVec2Add(XMVECTOR2* o, CONST XMVECTOR2* a, CONST XMVECTOR2* b) { o->x = a->x + b->x; o->y = a->y + b->y; return o; }
D3DXINLINE XMVECTOR2* D3DXVec2Subtract(XMVECTOR2* o, CONST XMVECTOR2* a, CONST XMVECTOR2* b) { o->x = a->x - b->x; o->y = a->y - b->y; return o; }
D3DXINLINE XMVECTOR2* D3DXVec2Scale(XMVECTOR2* o, CONST XMVECTOR2* v, FLOAT s) { o->x = v->x * s; o->y = v->y * s; return o; }
D3DXINLINE XMVECTOR2* D3DXVec2Lerp(XMVECTOR2* o, CONST XMVECTOR2* a, CONST XMVECTOR2* b, FLOAT t)
{
    o->x = a->x + t * (b->x - a->x); o->y = a->y + t * (b->y - a->y); return o;
}
D3DXINLINE XMVECTOR2* D3DXVec2Normalize(XMVECTOR2* o, CONST XMVECTOR2* v)
{
    FLOAT l = D3DXVec2Length(v);
    if (l > 0.f) { o->x = v->x / l; o->y = v->y / l; } else { o->x = o->y = 0; }
    return o;
}
D3DXINLINE XMVECTOR2* D3DXVec2TransformCoord(XMVECTOR2* o, CONST XMVECTOR2* v, CONST XMMATRIX* m)
{
    DirectX::XMVECTOR r = DirectX::XMVector2TransformCoord(
        DirectX::XMLoadFloat2(reinterpret_cast<const DirectX::XMFLOAT2*>(v)),
        lwXMLoadM(m));
    DirectX::XMStoreFloat2(reinterpret_cast<DirectX::XMFLOAT2*>(o), r);
    return o;
}
D3DXINLINE XMVECTOR4* D3DXVec2Transform(XMVECTOR4* o, CONST XMVECTOR2* v, CONST XMMATRIX* m)
{
    DirectX::XMVECTOR r = DirectX::XMVector2Transform(
        DirectX::XMLoadFloat2(reinterpret_cast<const DirectX::XMFLOAT2*>(v)),
        lwXMLoadM(m));
    DirectX::XMStoreFloat4(reinterpret_cast<DirectX::XMFLOAT4*>(o), r);
    return o;
}

D3DXINLINE FLOAT D3DXVec3Length(CONST XMVECTOR3* pV) { return sqrtf(pV->x * pV->x + pV->y * pV->y + pV->z * pV->z); }
D3DXINLINE FLOAT D3DXVec3LengthSq(CONST XMVECTOR3* pV) { return pV->x * pV->x + pV->y * pV->y + pV->z * pV->z; }
D3DXINLINE FLOAT D3DXVec3Dot(CONST XMVECTOR3* a, CONST XMVECTOR3* b) { return a->x * b->x + a->y * b->y + a->z * b->z; }
D3DXINLINE XMVECTOR3* D3DXVec3Cross(XMVECTOR3* o, CONST XMVECTOR3* a, CONST XMVECTOR3* b)
{
    XMVECTOR3 r(a->y * b->z - a->z * b->y, a->z * b->x - a->x * b->z, a->x * b->y - a->y * b->x);
    *o = r; return o;
}
D3DXINLINE XMVECTOR3* D3DXVec3Add(XMVECTOR3* o, CONST XMVECTOR3* a, CONST XMVECTOR3* b) { o->x = a->x + b->x; o->y = a->y + b->y; o->z = a->z + b->z; return o; }
D3DXINLINE XMVECTOR3* D3DXVec3Subtract(XMVECTOR3* o, CONST XMVECTOR3* a, CONST XMVECTOR3* b) { o->x = a->x - b->x; o->y = a->y - b->y; o->z = a->z - b->z; return o; }
D3DXINLINE XMVECTOR3* D3DXVec3Scale(XMVECTOR3* o, CONST XMVECTOR3* v, FLOAT s) { o->x = v->x * s; o->y = v->y * s; o->z = v->z * s; return o; }
D3DXINLINE XMVECTOR3* D3DXVec3Lerp(XMVECTOR3* o, CONST XMVECTOR3* a, CONST XMVECTOR3* b, FLOAT t)
{
    o->x = a->x + t * (b->x - a->x); o->y = a->y + t * (b->y - a->y); o->z = a->z + t * (b->z - a->z); return o;
}
D3DXINLINE XMVECTOR3* D3DXVec3Normalize(XMVECTOR3* o, CONST XMVECTOR3* v)
{
    FLOAT l = D3DXVec3Length(v);
    if (l > 0.0f) { o->x = v->x / l; o->y = v->y / l; o->z = v->z / l; } else { o->x = o->y = o->z = 0; }
    return o;
}
D3DXINLINE XMVECTOR4* D3DXVec3Transform(XMVECTOR4* o, CONST XMVECTOR3* v, CONST XMMATRIX* m)
{
    DirectX::XMVECTOR r = DirectX::XMVector3Transform(DirectX::XMLoadFloat3(reinterpret_cast<const DirectX::XMFLOAT3*>(v)), lwXMLoadM(m));
    DirectX::XMStoreFloat4(reinterpret_cast<DirectX::XMFLOAT4*>(o), r);
    return o;
}
D3DXINLINE XMVECTOR3* D3DXVec3TransformCoord(XMVECTOR3* o, CONST XMVECTOR3* v, CONST XMMATRIX* m)
{
    DirectX::XMVECTOR r = DirectX::XMVector3TransformCoord(DirectX::XMLoadFloat3(reinterpret_cast<const DirectX::XMFLOAT3*>(v)), lwXMLoadM(m));
    DirectX::XMStoreFloat3(reinterpret_cast<DirectX::XMFLOAT3*>(o), r);
    return o;
}
D3DXINLINE XMVECTOR3* D3DXVec3TransformNormal(XMVECTOR3* o, CONST XMVECTOR3* v, CONST XMMATRIX* m)
{
    DirectX::XMVECTOR r = DirectX::XMVector3TransformNormal(DirectX::XMLoadFloat3(reinterpret_cast<const DirectX::XMFLOAT3*>(v)), lwXMLoadM(m));
    DirectX::XMStoreFloat3(reinterpret_cast<DirectX::XMFLOAT3*>(o), r);
    return o;
}

D3DXINLINE FLOAT D3DXVec4Length(CONST XMVECTOR4* pV) { return sqrtf(pV->x * pV->x + pV->y * pV->y + pV->z * pV->z + pV->w * pV->w); }
D3DXINLINE FLOAT D3DXVec4Dot(CONST XMVECTOR4* a, CONST XMVECTOR4* b) { return a->x * b->x + a->y * b->y + a->z * b->z + a->w * b->w; }
D3DXINLINE XMVECTOR4* D3DXVec4Transform(XMVECTOR4* o, CONST XMVECTOR4* v, CONST XMMATRIX* m)
{
    DirectX::XMVECTOR r = DirectX::XMVector4Transform(DirectX::XMLoadFloat4(reinterpret_cast<const DirectX::XMFLOAT4*>(v)), lwXMLoadM(m));
    DirectX::XMStoreFloat4(reinterpret_cast<DirectX::XMFLOAT4*>(o), r);
    return o;
}

D3DXINLINE XMQUATERNION* D3DXQuaternionIdentity(XMQUATERNION* o) { o->x = o->y = o->z = 0; o->w = 1; return o; }
D3DXINLINE XMQUATERNION* D3DXQuaternionConjugate(XMQUATERNION* o, CONST XMQUATERNION* q) { o->x = -q->x; o->y = -q->y; o->z = -q->z; o->w = q->w; return o; }
D3DXINLINE XMQUATERNION* D3DXQuaternionMultiply(XMQUATERNION* o, CONST XMQUATERNION* a, CONST XMQUATERNION* b)
{
    DirectX::XMVECTOR r = DirectX::XMQuaternionMultiply(
        DirectX::XMLoadFloat4(reinterpret_cast<const DirectX::XMFLOAT4*>(a)),
        DirectX::XMLoadFloat4(reinterpret_cast<const DirectX::XMFLOAT4*>(b)));
    DirectX::XMStoreFloat4(reinterpret_cast<DirectX::XMFLOAT4*>(o), r);
    return o;
}
D3DXINLINE XMQUATERNION* D3DXQuaternionRotationAxis(XMQUATERNION* o, CONST XMVECTOR3* axis, FLOAT a)
{
    DirectX::XMVECTOR r = DirectX::XMQuaternionRotationAxis(DirectX::XMLoadFloat3(reinterpret_cast<const DirectX::XMFLOAT3*>(axis)), a);
    DirectX::XMStoreFloat4(reinterpret_cast<DirectX::XMFLOAT4*>(o), r);
    return o;
}
D3DXINLINE XMQUATERNION* D3DXQuaternionNormalize(XMQUATERNION* o, CONST XMQUATERNION* q)
{
    DirectX::XMVECTOR r = DirectX::XMQuaternionNormalize(DirectX::XMLoadFloat4(reinterpret_cast<const DirectX::XMFLOAT4*>(q)));
    DirectX::XMStoreFloat4(reinterpret_cast<DirectX::XMFLOAT4*>(o), r);
    return o;
}
D3DXINLINE XMQUATERNION* D3DXQuaternionInverse(XMQUATERNION* o, CONST XMQUATERNION* q)
{
    DirectX::XMVECTOR r = DirectX::XMQuaternionInverse(DirectX::XMLoadFloat4(reinterpret_cast<const DirectX::XMFLOAT4*>(q)));
    DirectX::XMStoreFloat4(reinterpret_cast<DirectX::XMFLOAT4*>(o), r);
    return o;
}

D3DXINLINE XMVECTOR3* D3DXPlaneIntersectLine(XMVECTOR3* o, CONST XMPLANE* p, CONST XMVECTOR3* v1, CONST XMVECTOR3* v2)
{
    DirectX::XMVECTOR r = DirectX::XMPlaneIntersectLine(
        DirectX::XMLoadFloat4(reinterpret_cast<const DirectX::XMFLOAT4*>(p)),
        DirectX::XMLoadFloat3(reinterpret_cast<const DirectX::XMFLOAT3*>(v1)),
        DirectX::XMLoadFloat3(reinterpret_cast<const DirectX::XMFLOAT3*>(v2)));
    if (DirectX::XMVectorGetX(DirectX::XMVectorIsNaN(r)))
        return NULL;
    DirectX::XMStoreFloat3(reinterpret_cast<DirectX::XMFLOAT3*>(o), r);
    return o;
}
D3DXINLINE XMPLANE* D3DXPlaneNormalize(XMPLANE* o, CONST XMPLANE* p)
{
    DirectX::XMVECTOR r = DirectX::XMPlaneNormalize(DirectX::XMLoadFloat4(reinterpret_cast<const DirectX::XMFLOAT4*>(p)));
    DirectX::XMStoreFloat4(reinterpret_cast<DirectX::XMFLOAT4*>(o), r);
    return o;
}
D3DXINLINE XMPLANE* D3DXPlaneFromPointNormal(XMPLANE* o, CONST XMVECTOR3* v, CONST XMVECTOR3* n)
{
    DirectX::XMVECTOR r = DirectX::XMPlaneFromPointNormal(
        DirectX::XMLoadFloat3(reinterpret_cast<const DirectX::XMFLOAT3*>(v)),
        DirectX::XMLoadFloat3(reinterpret_cast<const DirectX::XMFLOAT3*>(n)));
    DirectX::XMStoreFloat4(reinterpret_cast<DirectX::XMFLOAT4*>(o), r);
    return o;
}
D3DXINLINE XMPLANE* D3DXPlaneFromPoints(XMPLANE* o, CONST XMVECTOR3* p1, CONST XMVECTOR3* p2, CONST XMVECTOR3* p3)
{
    DirectX::XMVECTOR r = DirectX::XMPlaneFromPoints(
        DirectX::XMLoadFloat3(reinterpret_cast<const DirectX::XMFLOAT3*>(p1)),
        DirectX::XMLoadFloat3(reinterpret_cast<const DirectX::XMFLOAT3*>(p2)),
        DirectX::XMLoadFloat3(reinterpret_cast<const DirectX::XMFLOAT3*>(p3)));
    DirectX::XMStoreFloat4(reinterpret_cast<DirectX::XMFLOAT4*>(o), r);
    return o;
}

D3DXINLINE BOOL D3DXIntersectTri(
    CONST XMVECTOR3* p0, CONST XMVECTOR3* p1, CONST XMVECTOR3* p2,
    CONST XMVECTOR3* pRayPos, CONST XMVECTOR3* pRayDir,
    FLOAT* pU, FLOAT* pV, FLOAT* pDist)
{
    XMVECTOR3 e1, e2, pvec, tvec, qvec;
    D3DXVec3Subtract(&e1, p1, p0);
    D3DXVec3Subtract(&e2, p2, p0);
    D3DXVec3Cross(&pvec, pRayDir, &e2);
    FLOAT det = D3DXVec3Dot(&e1, &pvec);
    if (det > -1e-6f && det < 1e-6f)
        return FALSE;
    FLOAT inv = 1.0f / det;
    D3DXVec3Subtract(&tvec, pRayPos, p0);
    FLOAT u = D3DXVec3Dot(&tvec, &pvec) * inv;
    if (u < 0.f || u > 1.f)
        return FALSE;
    D3DXVec3Cross(&qvec, &tvec, &e1);
    FLOAT v = D3DXVec3Dot(pRayDir, &qvec) * inv;
    if (v < 0.f || u + v > 1.f)
        return FALSE;
    FLOAT t = D3DXVec3Dot(&e2, &qvec) * inv;
    if (t < 0.f)
        return FALSE;
    if (pU) *pU = u;
    if (pV) *pV = v;
    if (pDist) *pDist = t;
    return TRUE;
}

D3DXINLINE BOOL D3DXBoxBoundProbe(
    CONST XMVECTOR3* pMin, CONST XMVECTOR3* pMax,
    CONST XMVECTOR3* pRayPosition, CONST XMVECTOR3* pRayDirection)
{
    FLOAT tmin = 0.f;
    FLOAT tmax = FLT_MAX;
    const FLOAT* ro = &pRayPosition->x;
    const FLOAT* rd = &pRayDirection->x;
    const FLOAT* bmin = &pMin->x;
    const FLOAT* bmax = &pMax->x;
    for (int i = 0; i < 3; ++i)
    {
        if (fabsf(rd[i]) < 1e-8f)
        {
            if (ro[i] < bmin[i] || ro[i] > bmax[i])
                return FALSE;
        }
        else
        {
            FLOAT t1 = (bmin[i] - ro[i]) / rd[i];
            FLOAT t2 = (bmax[i] - ro[i]) / rd[i];
            if (t1 > t2) { FLOAT tmp = t1; t1 = t2; t2 = tmp; }
            if (t1 > tmin) tmin = t1;
            if (t2 < tmax) tmax = t2;
            if (tmin > tmax)
                return FALSE;
        }
    }
    return tmax >= 0.f;
}

D3DXINLINE XMCOLORF* D3DXColorLerp(XMCOLORF* o, CONST XMCOLORF* a, CONST XMCOLORF* b, FLOAT t)
{
    o->r = a->r + t * (b->r - a->r);
    o->g = a->g + t * (b->g - a->g);
    o->b = a->b + t * (b->b - a->b);
    o->a = a->a + t * (b->a - a->a);
    return o;
}

D3DXINLINE XMMATRIX& XMMATRIX::operator+=(CONST XMMATRIX& n)
{
    for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) m[i][j] += n.m[i][j];
    return *this;
}
D3DXINLINE XMMATRIX& XMMATRIX::operator-=(CONST XMMATRIX& n)
{
    for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) m[i][j] -= n.m[i][j];
    return *this;
}
D3DXINLINE XMMATRIX& XMMATRIX::operator*=(CONST XMMATRIX& n)
{
    XMMATRIX t; D3DXMatrixMultiply(&t, this, &n); *this = t; return *this;
}
D3DXINLINE XMMATRIX& XMMATRIX::operator*=(FLOAT s)
{
    for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) m[i][j] *= s;
    return *this;
}
D3DXINLINE XMMATRIX& XMMATRIX::operator/=(FLOAT s)
{
    for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) m[i][j] /= s;
    return *this;
}
D3DXINLINE XMMATRIX XMMATRIX::operator-() const
{
    XMMATRIX r;
    for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) r.m[i][j] = -m[i][j];
    return r;
}
D3DXINLINE XMMATRIX XMMATRIX::operator*(CONST XMMATRIX& n) const
{
    XMMATRIX r; D3DXMatrixMultiply(&r, this, &n); return r;
}
D3DXINLINE XMMATRIX XMMATRIX::operator+(CONST XMMATRIX& n) const
{
    XMMATRIX r = *this; r += n; return r;
}
D3DXINLINE XMMATRIX XMMATRIX::operator-(CONST XMMATRIX& n) const
{
    XMMATRIX r = *this; r -= n; return r;
}
D3DXINLINE XMMATRIX XMMATRIX::operator*(FLOAT s) const
{
    XMMATRIX r = *this; r *= s; return r;
}
D3DXINLINE XMMATRIX XMMATRIX::operator/(FLOAT s) const
{
    XMMATRIX r = *this; r /= s; return r;
}
D3DXINLINE XMMATRIX operator*(FLOAT s, CONST XMMATRIX& n) { return n * s; }

D3DXINLINE XMQUATERNION& XMQUATERNION::operator*=(CONST XMQUATERNION& q)
{
    XMQUATERNION t; D3DXQuaternionMultiply(&t, this, &q); *this = t; return *this;
}
D3DXINLINE XMQUATERNION XMQUATERNION::operator*(CONST XMQUATERNION& q) const
{
    XMQUATERNION t; D3DXQuaternionMultiply(&t, this, &q); return t;
}

typedef XMVECTOR2 D3DXVECTOR2;
typedef XMVECTOR3 D3DXVECTOR3;
typedef XMVECTOR4 D3DXVECTOR4;
typedef XMMATRIX D3DXMATRIX;
typedef XMMATRIXA16 D3DXMATRIXA16;
typedef XMQUATERNION D3DXQUATERNION;
typedef XMPLANE D3DXPLANE;
typedef XMCOLORF D3DXCOLOR;
typedef LPXMVECTOR2 LPD3DXVECTOR2;
typedef LPXMVECTOR3 LPD3DXVECTOR3;
typedef LPXMVECTOR4 LPD3DXVECTOR4;
typedef LPXMMATRIX LPD3DXMATRIX;
typedef LPXMMATRIXA16 LPD3DXMATRIXA16;
typedef LPXMQUATERNION LPD3DXQUATERNION;
typedef LPXMPLANE LPD3DXPLANE;
typedef LPXMCOLORF LPD3DXCOLOR;

#include "lwXMMath.h"

#endif

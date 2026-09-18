#pragma once

// DirectXMath-backed D3DX9 math / helper stand-ins for the DX11-only build.
// Layout of D3DXVECTOR / D3DXMATRIX / D3DXCOLOR matches d3dx9 (asset I/O).

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

typedef struct D3DXVECTOR2
{
    FLOAT x, y;
    D3DXVECTOR2() {}
    D3DXVECTOR2(CONST FLOAT* p) { x = p[0]; y = p[1]; }
    D3DXVECTOR2(FLOAT x, FLOAT y) : x(x), y(y) {}
    operator FLOAT*() { return &x; }
    operator CONST FLOAT*() const { return &x; }
    D3DXVECTOR2& operator+=(CONST D3DXVECTOR2& v) { x += v.x; y += v.y; return *this; }
    D3DXVECTOR2& operator-=(CONST D3DXVECTOR2& v) { x -= v.x; y -= v.y; return *this; }
    D3DXVECTOR2& operator*=(FLOAT s) { x *= s; y *= s; return *this; }
    D3DXVECTOR2& operator/=(FLOAT s) { x /= s; y /= s; return *this; }
    D3DXVECTOR2 operator+() const { return *this; }
    D3DXVECTOR2 operator-() const { return D3DXVECTOR2(-x, -y); }
    D3DXVECTOR2 operator+(CONST D3DXVECTOR2& v) const { return D3DXVECTOR2(x + v.x, y + v.y); }
    D3DXVECTOR2 operator-(CONST D3DXVECTOR2& v) const { return D3DXVECTOR2(x - v.x, y - v.y); }
    D3DXVECTOR2 operator*(FLOAT s) const { return D3DXVECTOR2(x * s, y * s); }
    D3DXVECTOR2 operator/(FLOAT s) const { return D3DXVECTOR2(x / s, y / s); }
    friend D3DXVECTOR2 operator*(FLOAT s, CONST D3DXVECTOR2& v) { return D3DXVECTOR2(v.x * s, v.y * s); }
    BOOL operator==(CONST D3DXVECTOR2& v) const { return x == v.x && y == v.y; }
    BOOL operator!=(CONST D3DXVECTOR2& v) const { return !(*this == v); }
} D3DXVECTOR2, *LPD3DXVECTOR2;

typedef struct D3DXVECTOR3 : public D3DVECTOR
{
    D3DXVECTOR3() {}
    D3DXVECTOR3(CONST FLOAT* p) { x = p[0]; y = p[1]; z = p[2]; }
    D3DXVECTOR3(CONST D3DVECTOR& v) { x = v.x; y = v.y; z = v.z; }
    D3DXVECTOR3(FLOAT x, FLOAT y, FLOAT z) { this->x = x; this->y = y; this->z = z; }
    operator FLOAT*() { return &x; }
    operator CONST FLOAT*() const { return &x; }
    D3DXVECTOR3& operator+=(CONST D3DXVECTOR3& v) { x += v.x; y += v.y; z += v.z; return *this; }
    D3DXVECTOR3& operator-=(CONST D3DXVECTOR3& v) { x -= v.x; y -= v.y; z -= v.z; return *this; }
    D3DXVECTOR3& operator*=(FLOAT s) { x *= s; y *= s; z *= s; return *this; }
    D3DXVECTOR3& operator/=(FLOAT s) { x /= s; y /= s; z /= s; return *this; }
    D3DXVECTOR3 operator+() const { return *this; }
    D3DXVECTOR3 operator-() const { return D3DXVECTOR3(-x, -y, -z); }
    D3DXVECTOR3 operator+(CONST D3DXVECTOR3& v) const { return D3DXVECTOR3(x + v.x, y + v.y, z + v.z); }
    D3DXVECTOR3 operator-(CONST D3DXVECTOR3& v) const { return D3DXVECTOR3(x - v.x, y - v.y, z - v.z); }
    D3DXVECTOR3 operator*(FLOAT s) const { return D3DXVECTOR3(x * s, y * s, z * s); }
    D3DXVECTOR3 operator/(FLOAT s) const { return D3DXVECTOR3(x / s, y / s, z / s); }
    friend D3DXVECTOR3 operator*(FLOAT s, CONST D3DXVECTOR3& v) { return D3DXVECTOR3(v.x * s, v.y * s, v.z * s); }
    BOOL operator==(CONST D3DXVECTOR3& v) const { return x == v.x && y == v.y && z == v.z; }
    BOOL operator!=(CONST D3DXVECTOR3& v) const { return !(*this == v); }
} D3DXVECTOR3, *LPD3DXVECTOR3;

typedef struct D3DXVECTOR4
{
    FLOAT x, y, z, w;
    D3DXVECTOR4() {}
    D3DXVECTOR4(CONST FLOAT* p) { x = p[0]; y = p[1]; z = p[2]; w = p[3]; }
    D3DXVECTOR4(CONST D3DVECTOR& v, FLOAT w) : x(v.x), y(v.y), z(v.z), w(w) {}
    D3DXVECTOR4(FLOAT x, FLOAT y, FLOAT z, FLOAT w) : x(x), y(y), z(z), w(w) {}
    operator FLOAT*() { return &x; }
    operator CONST FLOAT*() const { return &x; }
    D3DXVECTOR4& operator+=(CONST D3DXVECTOR4& v) { x += v.x; y += v.y; z += v.z; w += v.w; return *this; }
    D3DXVECTOR4& operator-=(CONST D3DXVECTOR4& v) { x -= v.x; y -= v.y; z -= v.z; w -= v.w; return *this; }
    D3DXVECTOR4& operator*=(FLOAT s) { x *= s; y *= s; z *= s; w *= s; return *this; }
    D3DXVECTOR4& operator/=(FLOAT s) { x /= s; y /= s; z /= s; w /= s; return *this; }
    D3DXVECTOR4 operator+() const { return *this; }
    D3DXVECTOR4 operator-() const { return D3DXVECTOR4(-x, -y, -z, -w); }
    D3DXVECTOR4 operator+(CONST D3DXVECTOR4& v) const { return D3DXVECTOR4(x + v.x, y + v.y, z + v.z, w + v.w); }
    D3DXVECTOR4 operator-(CONST D3DXVECTOR4& v) const { return D3DXVECTOR4(x - v.x, y - v.y, z - v.z, w - v.w); }
    D3DXVECTOR4 operator*(FLOAT s) const { return D3DXVECTOR4(x * s, y * s, z * s, w * s); }
    D3DXVECTOR4 operator/(FLOAT s) const { return D3DXVECTOR4(x / s, y / s, z / s, w / s); }
    friend D3DXVECTOR4 operator*(FLOAT s, CONST D3DXVECTOR4& v) { return D3DXVECTOR4(v.x * s, v.y * s, v.z * s, v.w * s); }
    BOOL operator==(CONST D3DXVECTOR4& v) const { return x == v.x && y == v.y && z == v.z && w == v.w; }
    BOOL operator!=(CONST D3DXVECTOR4& v) const { return !(*this == v); }
} D3DXVECTOR4, *LPD3DXVECTOR4;

typedef struct D3DXMATRIX : public D3DMATRIX
{
    D3DXMATRIX() {}
    D3DXMATRIX(CONST FLOAT* p) { memcpy(m, p, sizeof(D3DMATRIX)); }
    D3DXMATRIX(CONST D3DMATRIX& n) { memcpy(m, n.m, sizeof(D3DMATRIX)); }
    D3DXMATRIX(FLOAT f11, FLOAT f12, FLOAT f13, FLOAT f14,
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
    D3DXMATRIX& operator+=(CONST D3DXMATRIX& n);
    D3DXMATRIX& operator-=(CONST D3DXMATRIX& n);
    D3DXMATRIX& operator*=(CONST D3DXMATRIX& n);
    D3DXMATRIX& operator*=(FLOAT s);
    D3DXMATRIX& operator/=(FLOAT s);
    D3DXMATRIX operator+() const { return *this; }
    D3DXMATRIX operator-() const;
    D3DXMATRIX operator*(CONST D3DXMATRIX& n) const;
    D3DXMATRIX operator+(CONST D3DXMATRIX& n) const;
    D3DXMATRIX operator-(CONST D3DXMATRIX& n) const;
    D3DXMATRIX operator*(FLOAT s) const;
    D3DXMATRIX operator/(FLOAT s) const;
    friend D3DXMATRIX operator*(FLOAT s, CONST D3DXMATRIX& n);
    BOOL operator==(CONST D3DXMATRIX& n) const { return memcmp(m, n.m, sizeof(D3DMATRIX)) == 0; }
    BOOL operator!=(CONST D3DXMATRIX& n) const { return !(*this == n); }
} D3DXMATRIX, *LPD3DXMATRIX;

typedef D3DXMATRIX D3DXMATRIXA16;
typedef D3DXMATRIXA16* LPD3DXMATRIXA16;

typedef struct D3DXQUATERNION
{
    FLOAT x, y, z, w;
    D3DXQUATERNION() {}
    D3DXQUATERNION(CONST FLOAT* p) { x = p[0]; y = p[1]; z = p[2]; w = p[3]; }
    D3DXQUATERNION(FLOAT x, FLOAT y, FLOAT z, FLOAT w) : x(x), y(y), z(z), w(w) {}
    operator FLOAT*() { return &x; }
    operator CONST FLOAT*() const { return &x; }
    D3DXQUATERNION& operator+=(CONST D3DXQUATERNION& q) { x += q.x; y += q.y; z += q.z; w += q.w; return *this; }
    D3DXQUATERNION& operator-=(CONST D3DXQUATERNION& q) { x -= q.x; y -= q.y; z -= q.z; w -= q.w; return *this; }
    D3DXQUATERNION& operator*=(CONST D3DXQUATERNION& q);
    D3DXQUATERNION& operator*=(FLOAT s) { x *= s; y *= s; z *= s; w *= s; return *this; }
    D3DXQUATERNION& operator/=(FLOAT s) { x /= s; y /= s; z /= s; w /= s; return *this; }
    D3DXQUATERNION operator+() const { return *this; }
    D3DXQUATERNION operator-() const { return D3DXQUATERNION(-x, -y, -z, -w); }
    D3DXQUATERNION operator+(CONST D3DXQUATERNION& q) const { return D3DXQUATERNION(x + q.x, y + q.y, z + q.z, w + q.w); }
    D3DXQUATERNION operator-(CONST D3DXQUATERNION& q) const { return D3DXQUATERNION(x - q.x, y - q.y, z - q.z, w - q.w); }
    D3DXQUATERNION operator*(CONST D3DXQUATERNION& q) const;
    D3DXQUATERNION operator*(FLOAT s) const { return D3DXQUATERNION(x * s, y * s, z * s, w * s); }
    D3DXQUATERNION operator/(FLOAT s) const { return D3DXQUATERNION(x / s, y / s, z / s, w / s); }
    friend D3DXQUATERNION operator*(FLOAT s, CONST D3DXQUATERNION& q) { return D3DXQUATERNION(q.x * s, q.y * s, q.z * s, q.w * s); }
    BOOL operator==(CONST D3DXQUATERNION& q) const { return x == q.x && y == q.y && z == q.z && w == q.w; }
    BOOL operator!=(CONST D3DXQUATERNION& q) const { return !(*this == q); }
} D3DXQUATERNION, *LPD3DXQUATERNION;

typedef struct D3DXPLANE
{
    FLOAT a, b, c, d;
    D3DXPLANE() {}
    D3DXPLANE(CONST FLOAT* p) { a = p[0]; b = p[1]; c = p[2]; d = p[3]; }
    D3DXPLANE(FLOAT a, FLOAT b, FLOAT c, FLOAT d) : a(a), b(b), c(c), d(d) {}
    operator FLOAT*() { return &a; }
    operator CONST FLOAT*() const { return &a; }
    D3DXPLANE operator+() const { return *this; }
    D3DXPLANE operator-() const { return D3DXPLANE(-a, -b, -c, -d); }
    BOOL operator==(CONST D3DXPLANE& p) const { return a == p.a && b == p.b && c == p.c && d == p.d; }
    BOOL operator!=(CONST D3DXPLANE& p) const { return !(*this == p); }
} D3DXPLANE, *LPD3DXPLANE;

typedef struct D3DXCOLOR
{
    FLOAT r, g, b, a;
    D3DXCOLOR() {}
    D3DXCOLOR(DWORD argb)
    {
        const FLOAT f = 1.0f / 255.0f;
        r = f * (FLOAT)(unsigned char)(argb >> 16);
        g = f * (FLOAT)(unsigned char)(argb >> 8);
        b = f * (FLOAT)(unsigned char)(argb >> 0);
        a = f * (FLOAT)(unsigned char)(argb >> 24);
    }
    D3DXCOLOR(CONST FLOAT* p) { r = p[0]; g = p[1]; b = p[2]; a = p[3]; }
    D3DXCOLOR(CONST D3DCOLORVALUE& c) { r = c.r; g = c.g; b = c.b; a = c.a; }
    D3DXCOLOR(FLOAT r, FLOAT g, FLOAT b, FLOAT a) : r(r), g(g), b(b), a(a) {}
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
    D3DXCOLOR& operator+=(CONST D3DXCOLOR& c) { r += c.r; g += c.g; b += c.b; a += c.a; return *this; }
    D3DXCOLOR& operator-=(CONST D3DXCOLOR& c) { r -= c.r; g -= c.g; b -= c.b; a -= c.a; return *this; }
    D3DXCOLOR& operator*=(FLOAT s) { r *= s; g *= s; b *= s; a *= s; return *this; }
    D3DXCOLOR& operator/=(FLOAT s) { r /= s; g /= s; b /= s; a /= s; return *this; }
    D3DXCOLOR operator+() const { return *this; }
    D3DXCOLOR operator-() const { return D3DXCOLOR(-r, -g, -b, -a); }
    D3DXCOLOR operator+(CONST D3DXCOLOR& c) const { return D3DXCOLOR(r + c.r, g + c.g, b + c.b, a + c.a); }
    D3DXCOLOR operator-(CONST D3DXCOLOR& c) const { return D3DXCOLOR(r - c.r, g - c.g, b - c.b, a - c.a); }
    D3DXCOLOR operator*(FLOAT s) const { return D3DXCOLOR(r * s, g * s, b * s, a * s); }
    D3DXCOLOR operator/(FLOAT s) const { return D3DXCOLOR(r / s, g / s, b / s, a / s); }
    friend D3DXCOLOR operator*(FLOAT s, CONST D3DXCOLOR& c) { return D3DXCOLOR(c.r * s, c.g * s, c.b * s, c.a * s); }
    BOOL operator==(CONST D3DXCOLOR& c) const { return r == c.r && g == c.g && b == c.b && a == c.a; }
    BOOL operator!=(CONST D3DXCOLOR& c) const { return !(*this == c); }
} D3DXCOLOR, *LPD3DXCOLOR;

D3DXINLINE DirectX::XMMATRIX lwXMLoadM(CONST D3DXMATRIX* m)
{
    return DirectX::XMLoadFloat4x4(reinterpret_cast<const DirectX::XMFLOAT4X4*>(m));
}
D3DXINLINE D3DXMATRIX* lwXMStoreM(D3DXMATRIX* o, DirectX::FXMMATRIX m)
{
    DirectX::XMStoreFloat4x4(reinterpret_cast<DirectX::XMFLOAT4X4*>(o), m);
    return o;
}

D3DXINLINE D3DXMATRIX* D3DXMatrixIdentity(D3DXMATRIX* pOut)
{
    return lwXMStoreM(pOut, DirectX::XMMatrixIdentity());
}
D3DXINLINE BOOL D3DXMatrixIsIdentity(CONST D3DXMATRIX* pM)
{
    return DirectX::XMMatrixIsIdentity(lwXMLoadM(pM));
}
D3DXINLINE D3DXMATRIX* D3DXMatrixMultiply(D3DXMATRIX* pOut, CONST D3DXMATRIX* pM1, CONST D3DXMATRIX* pM2)
{
    return lwXMStoreM(pOut, DirectX::XMMatrixMultiply(lwXMLoadM(pM1), lwXMLoadM(pM2)));
}
D3DXINLINE D3DXMATRIX* D3DXMatrixTranspose(D3DXMATRIX* pOut, CONST D3DXMATRIX* pM)
{
    return lwXMStoreM(pOut, DirectX::XMMatrixTranspose(lwXMLoadM(pM)));
}
D3DXINLINE D3DXMATRIX* D3DXMatrixInverse(D3DXMATRIX* pOut, FLOAT* pDeterminant, CONST D3DXMATRIX* pM)
{
    DirectX::XMVECTOR det;
    DirectX::XMMATRIX r = DirectX::XMMatrixInverse(&det, lwXMLoadM(pM));
    if (pDeterminant)
        *pDeterminant = DirectX::XMVectorGetX(det);
    return lwXMStoreM(pOut, r);
}
D3DXINLINE FLOAT D3DXMatrixDeterminant(CONST D3DXMATRIX* pM)
{
    return DirectX::XMVectorGetX(DirectX::XMMatrixDeterminant(lwXMLoadM(pM)));
}
D3DXINLINE D3DXMATRIX* D3DXMatrixScaling(D3DXMATRIX* pOut, FLOAT sx, FLOAT sy, FLOAT sz)
{
    return lwXMStoreM(pOut, DirectX::XMMatrixScaling(sx, sy, sz));
}
D3DXINLINE D3DXMATRIX* D3DXMatrixTranslation(D3DXMATRIX* pOut, FLOAT x, FLOAT y, FLOAT z)
{
    return lwXMStoreM(pOut, DirectX::XMMatrixTranslation(x, y, z));
}
D3DXINLINE D3DXMATRIX* D3DXMatrixRotationX(D3DXMATRIX* pOut, FLOAT a) { return lwXMStoreM(pOut, DirectX::XMMatrixRotationX(a)); }
D3DXINLINE D3DXMATRIX* D3DXMatrixRotationY(D3DXMATRIX* pOut, FLOAT a) { return lwXMStoreM(pOut, DirectX::XMMatrixRotationY(a)); }
D3DXINLINE D3DXMATRIX* D3DXMatrixRotationZ(D3DXMATRIX* pOut, FLOAT a) { return lwXMStoreM(pOut, DirectX::XMMatrixRotationZ(a)); }
D3DXINLINE D3DXMATRIX* D3DXMatrixRotationAxis(D3DXMATRIX* pOut, CONST D3DXVECTOR3* pV, FLOAT a)
{
    return lwXMStoreM(pOut, DirectX::XMMatrixRotationAxis(DirectX::XMLoadFloat3(reinterpret_cast<const DirectX::XMFLOAT3*>(pV)), a));
}
D3DXINLINE D3DXMATRIX* D3DXMatrixRotationQuaternion(D3DXMATRIX* pOut, CONST D3DXQUATERNION* pQ)
{
    return lwXMStoreM(pOut, DirectX::XMMatrixRotationQuaternion(DirectX::XMLoadFloat4(reinterpret_cast<const DirectX::XMFLOAT4*>(pQ))));
}
D3DXINLINE D3DXMATRIX* D3DXMatrixRotationYawPitchRoll(D3DXMATRIX* pOut, FLOAT yaw, FLOAT pitch, FLOAT roll)
{
    return lwXMStoreM(pOut, DirectX::XMMatrixRotationRollPitchYaw(pitch, yaw, roll));
}
D3DXINLINE D3DXMATRIX* D3DXMatrixLookAtLH(D3DXMATRIX* pOut, CONST D3DXVECTOR3* pEye, CONST D3DXVECTOR3* pAt, CONST D3DXVECTOR3* pUp)
{
    return lwXMStoreM(pOut, DirectX::XMMatrixLookAtLH(
        DirectX::XMLoadFloat3(reinterpret_cast<const DirectX::XMFLOAT3*>(pEye)),
        DirectX::XMLoadFloat3(reinterpret_cast<const DirectX::XMFLOAT3*>(pAt)),
        DirectX::XMLoadFloat3(reinterpret_cast<const DirectX::XMFLOAT3*>(pUp))));
}
D3DXINLINE D3DXMATRIX* D3DXMatrixLookAtRH(D3DXMATRIX* pOut, CONST D3DXVECTOR3* pEye, CONST D3DXVECTOR3* pAt, CONST D3DXVECTOR3* pUp)
{
    return lwXMStoreM(pOut, DirectX::XMMatrixLookAtRH(
        DirectX::XMLoadFloat3(reinterpret_cast<const DirectX::XMFLOAT3*>(pEye)),
        DirectX::XMLoadFloat3(reinterpret_cast<const DirectX::XMFLOAT3*>(pAt)),
        DirectX::XMLoadFloat3(reinterpret_cast<const DirectX::XMFLOAT3*>(pUp))));
}
D3DXINLINE D3DXMATRIX* D3DXMatrixPerspectiveFovLH(D3DXMATRIX* pOut, FLOAT fov, FLOAT aspect, FLOAT zn, FLOAT zf)
{
    return lwXMStoreM(pOut, DirectX::XMMatrixPerspectiveFovLH(fov, aspect, zn, zf));
}
D3DXINLINE D3DXMATRIX* D3DXMatrixPerspectiveFovRH(D3DXMATRIX* pOut, FLOAT fov, FLOAT aspect, FLOAT zn, FLOAT zf)
{
    return lwXMStoreM(pOut, DirectX::XMMatrixPerspectiveFovRH(fov, aspect, zn, zf));
}
D3DXINLINE D3DXMATRIX* D3DXMatrixOrthoLH(D3DXMATRIX* pOut, FLOAT w, FLOAT h, FLOAT zn, FLOAT zf)
{
    return lwXMStoreM(pOut, DirectX::XMMatrixOrthographicLH(w, h, zn, zf));
}
D3DXINLINE D3DXMATRIX* D3DXMatrixOrthoRH(D3DXMATRIX* pOut, FLOAT w, FLOAT h, FLOAT zn, FLOAT zf)
{
    return lwXMStoreM(pOut, DirectX::XMMatrixOrthographicRH(w, h, zn, zf));
}
D3DXINLINE D3DXMATRIX* D3DXMatrixTransformation2D(
    D3DXMATRIX* pOut,
    CONST D3DXVECTOR2* pScalingCenter, FLOAT /*scalingRotation*/, CONST D3DXVECTOR2* pScaling,
    CONST D3DXVECTOR2* pRotationCenter, FLOAT rotation, CONST D3DXVECTOR2* pTranslation)
{
    DirectX::XMVECTOR sc = pScalingCenter ? DirectX::XMLoadFloat2(reinterpret_cast<const DirectX::XMFLOAT2*>(pScalingCenter)) : DirectX::XMVectorZero();
    DirectX::XMVECTOR s = pScaling ? DirectX::XMLoadFloat2(reinterpret_cast<const DirectX::XMFLOAT2*>(pScaling)) : DirectX::XMVectorSplatOne();
    DirectX::XMVECTOR rc = pRotationCenter ? DirectX::XMLoadFloat2(reinterpret_cast<const DirectX::XMFLOAT2*>(pRotationCenter)) : DirectX::XMVectorZero();
    DirectX::XMVECTOR t = pTranslation ? DirectX::XMLoadFloat2(reinterpret_cast<const DirectX::XMFLOAT2*>(pTranslation)) : DirectX::XMVectorZero();
    (void)sc;
    (void)rc;
    return lwXMStoreM(pOut, DirectX::XMMatrixAffineTransformation2D(s, rc, rotation, t));
}

D3DXINLINE FLOAT D3DXVec2Length(CONST D3DXVECTOR2* pV) { return sqrtf(pV->x * pV->x + pV->y * pV->y); }
D3DXINLINE FLOAT D3DXVec2LengthSq(CONST D3DXVECTOR2* pV) { return pV->x * pV->x + pV->y * pV->y; }
D3DXINLINE FLOAT D3DXVec2Dot(CONST D3DXVECTOR2* a, CONST D3DXVECTOR2* b) { return a->x * b->x + a->y * b->y; }
D3DXINLINE D3DXVECTOR2* D3DXVec2Add(D3DXVECTOR2* o, CONST D3DXVECTOR2* a, CONST D3DXVECTOR2* b) { o->x = a->x + b->x; o->y = a->y + b->y; return o; }
D3DXINLINE D3DXVECTOR2* D3DXVec2Subtract(D3DXVECTOR2* o, CONST D3DXVECTOR2* a, CONST D3DXVECTOR2* b) { o->x = a->x - b->x; o->y = a->y - b->y; return o; }
D3DXINLINE D3DXVECTOR2* D3DXVec2Scale(D3DXVECTOR2* o, CONST D3DXVECTOR2* v, FLOAT s) { o->x = v->x * s; o->y = v->y * s; return o; }
D3DXINLINE D3DXVECTOR2* D3DXVec2Lerp(D3DXVECTOR2* o, CONST D3DXVECTOR2* a, CONST D3DXVECTOR2* b, FLOAT t)
{
    o->x = a->x + t * (b->x - a->x); o->y = a->y + t * (b->y - a->y); return o;
}
D3DXINLINE D3DXVECTOR2* D3DXVec2Normalize(D3DXVECTOR2* o, CONST D3DXVECTOR2* v)
{
    FLOAT l = D3DXVec2Length(v);
    if (l > 0.f) { o->x = v->x / l; o->y = v->y / l; } else { o->x = o->y = 0; }
    return o;
}
D3DXINLINE D3DXVECTOR2* D3DXVec2TransformCoord(D3DXVECTOR2* o, CONST D3DXVECTOR2* v, CONST D3DXMATRIX* m)
{
    DirectX::XMVECTOR r = DirectX::XMVector2TransformCoord(
        DirectX::XMLoadFloat2(reinterpret_cast<const DirectX::XMFLOAT2*>(v)),
        lwXMLoadM(m));
    DirectX::XMStoreFloat2(reinterpret_cast<DirectX::XMFLOAT2*>(o), r);
    return o;
}
D3DXINLINE D3DXVECTOR4* D3DXVec2Transform(D3DXVECTOR4* o, CONST D3DXVECTOR2* v, CONST D3DXMATRIX* m)
{
    DirectX::XMVECTOR r = DirectX::XMVector2Transform(
        DirectX::XMLoadFloat2(reinterpret_cast<const DirectX::XMFLOAT2*>(v)),
        lwXMLoadM(m));
    DirectX::XMStoreFloat4(reinterpret_cast<DirectX::XMFLOAT4*>(o), r);
    return o;
}

D3DXINLINE FLOAT D3DXVec3Length(CONST D3DXVECTOR3* pV) { return sqrtf(pV->x * pV->x + pV->y * pV->y + pV->z * pV->z); }
D3DXINLINE FLOAT D3DXVec3LengthSq(CONST D3DXVECTOR3* pV) { return pV->x * pV->x + pV->y * pV->y + pV->z * pV->z; }
D3DXINLINE FLOAT D3DXVec3Dot(CONST D3DXVECTOR3* a, CONST D3DXVECTOR3* b) { return a->x * b->x + a->y * b->y + a->z * b->z; }
D3DXINLINE D3DXVECTOR3* D3DXVec3Cross(D3DXVECTOR3* o, CONST D3DXVECTOR3* a, CONST D3DXVECTOR3* b)
{
    D3DXVECTOR3 r(a->y * b->z - a->z * b->y, a->z * b->x - a->x * b->z, a->x * b->y - a->y * b->x);
    *o = r; return o;
}
D3DXINLINE D3DXVECTOR3* D3DXVec3Add(D3DXVECTOR3* o, CONST D3DXVECTOR3* a, CONST D3DXVECTOR3* b) { o->x = a->x + b->x; o->y = a->y + b->y; o->z = a->z + b->z; return o; }
D3DXINLINE D3DXVECTOR3* D3DXVec3Subtract(D3DXVECTOR3* o, CONST D3DXVECTOR3* a, CONST D3DXVECTOR3* b) { o->x = a->x - b->x; o->y = a->y - b->y; o->z = a->z - b->z; return o; }
D3DXINLINE D3DXVECTOR3* D3DXVec3Scale(D3DXVECTOR3* o, CONST D3DXVECTOR3* v, FLOAT s) { o->x = v->x * s; o->y = v->y * s; o->z = v->z * s; return o; }
D3DXINLINE D3DXVECTOR3* D3DXVec3Lerp(D3DXVECTOR3* o, CONST D3DXVECTOR3* a, CONST D3DXVECTOR3* b, FLOAT t)
{
    o->x = a->x + t * (b->x - a->x); o->y = a->y + t * (b->y - a->y); o->z = a->z + t * (b->z - a->z); return o;
}
D3DXINLINE D3DXVECTOR3* D3DXVec3Normalize(D3DXVECTOR3* o, CONST D3DXVECTOR3* v)
{
    FLOAT l = D3DXVec3Length(v);
    if (l > 0.0f) { o->x = v->x / l; o->y = v->y / l; o->z = v->z / l; } else { o->x = o->y = o->z = 0; }
    return o;
}
D3DXINLINE D3DXVECTOR4* D3DXVec3Transform(D3DXVECTOR4* o, CONST D3DXVECTOR3* v, CONST D3DXMATRIX* m)
{
    DirectX::XMVECTOR r = DirectX::XMVector3Transform(DirectX::XMLoadFloat3(reinterpret_cast<const DirectX::XMFLOAT3*>(v)), lwXMLoadM(m));
    DirectX::XMStoreFloat4(reinterpret_cast<DirectX::XMFLOAT4*>(o), r);
    return o;
}
D3DXINLINE D3DXVECTOR3* D3DXVec3TransformCoord(D3DXVECTOR3* o, CONST D3DXVECTOR3* v, CONST D3DXMATRIX* m)
{
    DirectX::XMVECTOR r = DirectX::XMVector3TransformCoord(DirectX::XMLoadFloat3(reinterpret_cast<const DirectX::XMFLOAT3*>(v)), lwXMLoadM(m));
    DirectX::XMStoreFloat3(reinterpret_cast<DirectX::XMFLOAT3*>(o), r);
    return o;
}
D3DXINLINE D3DXVECTOR3* D3DXVec3TransformNormal(D3DXVECTOR3* o, CONST D3DXVECTOR3* v, CONST D3DXMATRIX* m)
{
    DirectX::XMVECTOR r = DirectX::XMVector3TransformNormal(DirectX::XMLoadFloat3(reinterpret_cast<const DirectX::XMFLOAT3*>(v)), lwXMLoadM(m));
    DirectX::XMStoreFloat3(reinterpret_cast<DirectX::XMFLOAT3*>(o), r);
    return o;
}

D3DXINLINE FLOAT D3DXVec4Length(CONST D3DXVECTOR4* pV) { return sqrtf(pV->x * pV->x + pV->y * pV->y + pV->z * pV->z + pV->w * pV->w); }
D3DXINLINE FLOAT D3DXVec4Dot(CONST D3DXVECTOR4* a, CONST D3DXVECTOR4* b) { return a->x * b->x + a->y * b->y + a->z * b->z + a->w * b->w; }
D3DXINLINE D3DXVECTOR4* D3DXVec4Transform(D3DXVECTOR4* o, CONST D3DXVECTOR4* v, CONST D3DXMATRIX* m)
{
    DirectX::XMVECTOR r = DirectX::XMVector4Transform(DirectX::XMLoadFloat4(reinterpret_cast<const DirectX::XMFLOAT4*>(v)), lwXMLoadM(m));
    DirectX::XMStoreFloat4(reinterpret_cast<DirectX::XMFLOAT4*>(o), r);
    return o;
}

D3DXINLINE D3DXQUATERNION* D3DXQuaternionIdentity(D3DXQUATERNION* o) { o->x = o->y = o->z = 0; o->w = 1; return o; }
D3DXINLINE D3DXQUATERNION* D3DXQuaternionConjugate(D3DXQUATERNION* o, CONST D3DXQUATERNION* q) { o->x = -q->x; o->y = -q->y; o->z = -q->z; o->w = q->w; return o; }
D3DXINLINE D3DXQUATERNION* D3DXQuaternionMultiply(D3DXQUATERNION* o, CONST D3DXQUATERNION* a, CONST D3DXQUATERNION* b)
{
    DirectX::XMVECTOR r = DirectX::XMQuaternionMultiply(
        DirectX::XMLoadFloat4(reinterpret_cast<const DirectX::XMFLOAT4*>(a)),
        DirectX::XMLoadFloat4(reinterpret_cast<const DirectX::XMFLOAT4*>(b)));
    DirectX::XMStoreFloat4(reinterpret_cast<DirectX::XMFLOAT4*>(o), r);
    return o;
}
D3DXINLINE D3DXQUATERNION* D3DXQuaternionRotationAxis(D3DXQUATERNION* o, CONST D3DXVECTOR3* axis, FLOAT a)
{
    DirectX::XMVECTOR r = DirectX::XMQuaternionRotationAxis(DirectX::XMLoadFloat3(reinterpret_cast<const DirectX::XMFLOAT3*>(axis)), a);
    DirectX::XMStoreFloat4(reinterpret_cast<DirectX::XMFLOAT4*>(o), r);
    return o;
}
D3DXINLINE D3DXQUATERNION* D3DXQuaternionNormalize(D3DXQUATERNION* o, CONST D3DXQUATERNION* q)
{
    DirectX::XMVECTOR r = DirectX::XMQuaternionNormalize(DirectX::XMLoadFloat4(reinterpret_cast<const DirectX::XMFLOAT4*>(q)));
    DirectX::XMStoreFloat4(reinterpret_cast<DirectX::XMFLOAT4*>(o), r);
    return o;
}
D3DXINLINE D3DXQUATERNION* D3DXQuaternionInverse(D3DXQUATERNION* o, CONST D3DXQUATERNION* q)
{
    DirectX::XMVECTOR r = DirectX::XMQuaternionInverse(DirectX::XMLoadFloat4(reinterpret_cast<const DirectX::XMFLOAT4*>(q)));
    DirectX::XMStoreFloat4(reinterpret_cast<DirectX::XMFLOAT4*>(o), r);
    return o;
}

D3DXINLINE D3DXVECTOR3* D3DXPlaneIntersectLine(D3DXVECTOR3* o, CONST D3DXPLANE* p, CONST D3DXVECTOR3* v1, CONST D3DXVECTOR3* v2)
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
D3DXINLINE D3DXPLANE* D3DXPlaneNormalize(D3DXPLANE* o, CONST D3DXPLANE* p)
{
    DirectX::XMVECTOR r = DirectX::XMPlaneNormalize(DirectX::XMLoadFloat4(reinterpret_cast<const DirectX::XMFLOAT4*>(p)));
    DirectX::XMStoreFloat4(reinterpret_cast<DirectX::XMFLOAT4*>(o), r);
    return o;
}
D3DXINLINE D3DXPLANE* D3DXPlaneFromPointNormal(D3DXPLANE* o, CONST D3DXVECTOR3* v, CONST D3DXVECTOR3* n)
{
    DirectX::XMVECTOR r = DirectX::XMPlaneFromPointNormal(
        DirectX::XMLoadFloat3(reinterpret_cast<const DirectX::XMFLOAT3*>(v)),
        DirectX::XMLoadFloat3(reinterpret_cast<const DirectX::XMFLOAT3*>(n)));
    DirectX::XMStoreFloat4(reinterpret_cast<DirectX::XMFLOAT4*>(o), r);
    return o;
}
D3DXINLINE D3DXPLANE* D3DXPlaneFromPoints(D3DXPLANE* o, CONST D3DXVECTOR3* p1, CONST D3DXVECTOR3* p2, CONST D3DXVECTOR3* p3)
{
    DirectX::XMVECTOR r = DirectX::XMPlaneFromPoints(
        DirectX::XMLoadFloat3(reinterpret_cast<const DirectX::XMFLOAT3*>(p1)),
        DirectX::XMLoadFloat3(reinterpret_cast<const DirectX::XMFLOAT3*>(p2)),
        DirectX::XMLoadFloat3(reinterpret_cast<const DirectX::XMFLOAT3*>(p3)));
    DirectX::XMStoreFloat4(reinterpret_cast<DirectX::XMFLOAT4*>(o), r);
    return o;
}

D3DXINLINE BOOL D3DXIntersectTri(
    CONST D3DXVECTOR3* p0, CONST D3DXVECTOR3* p1, CONST D3DXVECTOR3* p2,
    CONST D3DXVECTOR3* pRayPos, CONST D3DXVECTOR3* pRayDir,
    FLOAT* pU, FLOAT* pV, FLOAT* pDist)
{
    D3DXVECTOR3 e1, e2, pvec, tvec, qvec;
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
    CONST D3DXVECTOR3* pMin, CONST D3DXVECTOR3* pMax,
    CONST D3DXVECTOR3* pRayPosition, CONST D3DXVECTOR3* pRayDirection)
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

D3DXINLINE D3DXCOLOR* D3DXColorLerp(D3DXCOLOR* o, CONST D3DXCOLOR* a, CONST D3DXCOLOR* b, FLOAT t)
{
    o->r = a->r + t * (b->r - a->r);
    o->g = a->g + t * (b->g - a->g);
    o->b = a->b + t * (b->b - a->b);
    o->a = a->a + t * (b->a - a->a);
    return o;
}

D3DXINLINE D3DXMATRIX& D3DXMATRIX::operator+=(CONST D3DXMATRIX& n)
{
    for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) m[i][j] += n.m[i][j];
    return *this;
}
D3DXINLINE D3DXMATRIX& D3DXMATRIX::operator-=(CONST D3DXMATRIX& n)
{
    for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) m[i][j] -= n.m[i][j];
    return *this;
}
D3DXINLINE D3DXMATRIX& D3DXMATRIX::operator*=(CONST D3DXMATRIX& n)
{
    D3DXMATRIX t; D3DXMatrixMultiply(&t, this, &n); *this = t; return *this;
}
D3DXINLINE D3DXMATRIX& D3DXMATRIX::operator*=(FLOAT s)
{
    for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) m[i][j] *= s;
    return *this;
}
D3DXINLINE D3DXMATRIX& D3DXMATRIX::operator/=(FLOAT s)
{
    for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) m[i][j] /= s;
    return *this;
}
D3DXINLINE D3DXMATRIX D3DXMATRIX::operator-() const
{
    D3DXMATRIX r;
    for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) r.m[i][j] = -m[i][j];
    return r;
}
D3DXINLINE D3DXMATRIX D3DXMATRIX::operator*(CONST D3DXMATRIX& n) const
{
    D3DXMATRIX r; D3DXMatrixMultiply(&r, this, &n); return r;
}
D3DXINLINE D3DXMATRIX D3DXMATRIX::operator+(CONST D3DXMATRIX& n) const
{
    D3DXMATRIX r = *this; r += n; return r;
}
D3DXINLINE D3DXMATRIX D3DXMATRIX::operator-(CONST D3DXMATRIX& n) const
{
    D3DXMATRIX r = *this; r -= n; return r;
}
D3DXINLINE D3DXMATRIX D3DXMATRIX::operator*(FLOAT s) const
{
    D3DXMATRIX r = *this; r *= s; return r;
}
D3DXINLINE D3DXMATRIX D3DXMATRIX::operator/(FLOAT s) const
{
    D3DXMATRIX r = *this; r /= s; return r;
}
D3DXINLINE D3DXMATRIX operator*(FLOAT s, CONST D3DXMATRIX& n) { return n * s; }

D3DXINLINE D3DXQUATERNION& D3DXQUATERNION::operator*=(CONST D3DXQUATERNION& q)
{
    D3DXQUATERNION t; D3DXQuaternionMultiply(&t, this, &q); *this = t; return *this;
}
D3DXINLINE D3DXQUATERNION D3DXQUATERNION::operator*(CONST D3DXQUATERNION& q) const
{
    D3DXQUATERNION t; D3DXQuaternionMultiply(&t, this, &q); return t;
}

#endif

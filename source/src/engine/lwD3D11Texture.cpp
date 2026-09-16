#include "stdafx.h"
#include "lwD3D11Texture.h"
#include "lwD3D11Gaps.h"
#include "lwDDS.h"

#include <d3d11.h>
#include <ole2.h>
#include <gdiplus.h>

#include <vector>

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "ole32.lib")

LW_BEGIN

static const GUID s_iid_d11tex =
    { 0xa3e7c101, 0x7d11, 0x4b11, { 0x9c, 0x11, 0xd3, 0xd1, 0x11, 0xc0, 0x11, 0x01 } };

const GUID& lwD3D11TextureGuid()
{
    return s_iid_d11tex;
}

lwD3D11Texture* lwAsD3D11Texture(IDirect3DBaseTextureX* tex)
{
    if (!tex)
        return 0;
    void* p = 0;
    if (FAILED(tex->QueryInterface(s_iid_d11tex, &p)) || !p)
        return 0;
    ((IUnknown*)p)->Release();
    return (lwD3D11Texture*)p;
}

lwD3D11Texture::lwD3D11Texture(ID3D11Texture2D* tex, ID3D11ShaderResourceView* srv, UINT w, UINT h, D3DFORMAT fmt, ID3D11RenderTargetView* rtv)
    : _ref(1)
    , _tex(tex)
    , _srv(srv)
    , _rtv(rtv)
    , _w(w)
    , _h(h)
    , _fmt(fmt)
    , _device(0)
    , _cpu(0)
    , _cpu_bytes(0)
    , _pitch(0)
    , _locked(0)
{
}

lwD3D11Texture::~lwD3D11Texture()
{
    delete[] _cpu;
    if (_rtv)
        _rtv->Release();
    if (_srv)
        _srv->Release();
    if (_tex)
        _tex->Release();
}

HRESULT lwD3D11Texture::QueryInterface(REFIID riid, void** ppvObj)
{
    if (!ppvObj)
        return E_POINTER;
    if (riid == IID_IUnknown || riid == IID_IDirect3DResource9 ||
        riid == IID_IDirect3DBaseTexture9 || riid == IID_IDirect3DTexture9 ||
        riid == s_iid_d11tex)
    {
        *ppvObj = this;
        AddRef();
        return S_OK;
    }
    *ppvObj = 0;
    return E_NOINTERFACE;
}

ULONG lwD3D11Texture::AddRef()
{
    return ++_ref;
}

ULONG lwD3D11Texture::Release()
{
    ULONG n = --_ref;
    if (n == 0)
        delete this;
    return n;
}

HRESULT lwD3D11Texture::GetDevice(IDirect3DDevice9** ppDevice)
{
    if (ppDevice)
        *ppDevice = 0;
    return E_NOTIMPL;
}

HRESULT lwD3D11Texture::SetPrivateData(REFGUID, const void*, DWORD, DWORD) { return E_NOTIMPL; }
HRESULT lwD3D11Texture::GetPrivateData(REFGUID, void*, DWORD*) { return E_NOTIMPL; }
HRESULT lwD3D11Texture::FreePrivateData(REFGUID) { return E_NOTIMPL; }
DWORD lwD3D11Texture::SetPriority(DWORD) { return 0; }
DWORD lwD3D11Texture::GetPriority() { return 0; }
void lwD3D11Texture::PreLoad() {}
D3DRESOURCETYPE lwD3D11Texture::GetType() { return D3DRTYPE_TEXTURE; }
DWORD lwD3D11Texture::SetLOD(DWORD) { return 0; }
DWORD lwD3D11Texture::GetLOD() { return 0; }
DWORD lwD3D11Texture::GetLevelCount() { return 1; }
HRESULT lwD3D11Texture::SetAutoGenFilterType(D3DTEXTUREFILTERTYPE) { return S_OK; }
D3DTEXTUREFILTERTYPE lwD3D11Texture::GetAutoGenFilterType() { return D3DTEXF_LINEAR; }
void lwD3D11Texture::GenerateMipSubLevels() {}

HRESULT lwD3D11Texture::GetLevelDesc(UINT Level, D3DSURFACE_DESC* pDesc)
{
    if (!pDesc || Level != 0)
        return D3DERR_INVALIDCALL;
    memset(pDesc, 0, sizeof(*pDesc));
    pDesc->Width = _w;
    pDesc->Height = _h;
    pDesc->Format = _fmt;
    pDesc->Type = D3DRTYPE_SURFACE;
    pDesc->Pool = D3DPOOL_MANAGED;
    pDesc->Usage = 0;
    pDesc->MultiSampleType = D3DMULTISAMPLE_NONE;
    return S_OK;
}

HRESULT lwD3D11Texture::GetSurfaceLevel(UINT, IDirect3DSurface9** pp)
{
    if (pp)
        *pp = 0;
    return E_NOTIMPL;
}

static UINT LockBpp(D3DFORMAT fmt)
{
    switch (fmt)
    {
    case D3DFMT_R5G6B5:
    case D3DFMT_A1R5G5B5:
    case D3DFMT_X1R5G5B5:
    case D3DFMT_A4R4G4B4:
        return 2;
    default:
        return 4;
    }
}

void lwD3D11Texture::InitCpuLock(ID3D11Device* device, D3DFORMAT lock_fmt)
{
    _device = device;
    if (lock_fmt != D3DFMT_UNKNOWN && lock_fmt != (D3DFORMAT)0)
        _fmt = lock_fmt;
    const UINT bpp = LockBpp(_fmt);
    _pitch = _w * bpp;
    _cpu_bytes = _pitch * _h;
    delete[] _cpu;
    _cpu = new BYTE[_cpu_bytes];
    memset(_cpu, 0, _cpu_bytes);
    _locked = 0;
}

static void ConvertLockToBGRA(const BYTE* src, UINT w, UINT h, UINT src_pitch, D3DFORMAT fmt, BYTE* dst, UINT dst_pitch)
{
    for (UINT y = 0; y < h; ++y)
    {
        const WORD* s16 = (const WORD*)(src + y * src_pitch);
        DWORD* d32 = (DWORD*)(dst + y * dst_pitch);
        if (fmt == D3DFMT_A4R4G4B4)
        {
            for (UINT x = 0; x < w; ++x)
            {
                const WORD p = s16[x];
                const DWORD a = ((p >> 12) & 0xF) * 17u;
                const DWORD r = ((p >> 8) & 0xF) * 17u;
                const DWORD g = ((p >> 4) & 0xF) * 17u;
                const DWORD b = (p & 0xF) * 17u;
                d32[x] = (a << 24) | (r << 16) | (g << 8) | b;
            }
        }
        else if (fmt == D3DFMT_R5G6B5)
        {
            for (UINT x = 0; x < w; ++x)
            {
                const WORD p = s16[x];
                const DWORD r = ((p >> 11) & 0x1F) * 255u / 31u;
                const DWORD g = ((p >> 5) & 0x3F) * 255u / 63u;
                const DWORD b = (p & 0x1F) * 255u / 31u;
                d32[x] = 0xFF000000u | (r << 16) | (g << 8) | b;
            }
        }
        else
        {
            for (UINT x = 0; x < w; ++x)
            {
                const WORD p = s16[x];
                const DWORD a = (fmt == D3DFMT_A1R5G5B5) ? (((p >> 15) & 1) ? 255u : 0u) : 255u;
                const DWORD r = ((p >> 10) & 0x1F) * 255u / 31u;
                const DWORD g = ((p >> 5) & 0x1F) * 255u / 31u;
                const DWORD b = (p & 0x1F) * 255u / 31u;
                d32[x] = (a << 24) | (r << 16) | (g << 8) | b;
            }
        }
    }
}

HRESULT lwD3D11Texture::LockRect(UINT Level, D3DLOCKED_RECT* pLockedRect, const RECT* pRect, DWORD)
{
    if (Level != 0 || !pLockedRect || !_cpu)
        return D3DERR_INVALIDCALL;
    if (_locked || pRect)
        return D3DERR_INVALIDCALL;
    _locked = 1;
    pLockedRect->pBits = _cpu;
    pLockedRect->Pitch = (INT)_pitch;
    return S_OK;
}

HRESULT lwD3D11Texture::UnlockRect(UINT Level)
{
    if (Level != 0 || !_locked || !_cpu || !_tex || !_device)
        return D3DERR_INVALIDCALL;
    _locked = 0;

    ID3D11DeviceContext* ctx = 0;
    _device->GetImmediateContext(&ctx);
    if (!ctx)
        return D3DERR_INVALIDCALL;

    const BYTE* src = _cpu;
    UINT src_pitch = _pitch;
    std::vector<BYTE> bgra;
    if (LockBpp(_fmt) == 2)
    {
        bgra.resize((size_t)_w * _h * 4);
        ConvertLockToBGRA(_cpu, _w, _h, _pitch, _fmt, bgra.data(), _w * 4);
        src = bgra.data();
        src_pitch = _w * 4;
    }

    ID3D11ShaderResourceView* none = 0;
    ctx->PSSetShaderResources(0, 1, &none);
    ctx->UpdateSubresource(_tex, 0, 0, src, src_pitch, 0);
    ctx->Release();
    return S_OK;
}

HRESULT lwD3D11Texture::AddDirtyRect(const RECT*) { return S_OK; }

static LW_RESULT CreateFromPixels(
    ID3D11Device* device,
    const void* pixels,
    UINT w,
    UINT h,
    UINT pitch,
    DXGI_FORMAT dxgi,
    D3DFORMAT d3d9,
    IDirect3DTextureX** out_tex)
{
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = w;
    desc.Height = h;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = dxgi;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA init = {};
    init.pSysMem = pixels;
    init.SysMemPitch = pitch;

    ID3D11Texture2D* tex = 0;
    HRESULT hr = device->CreateTexture2D(&desc, &init, &tex);
    if (FAILED(hr) || !tex)
        return LW_RET_FAILED;

    ID3D11ShaderResourceView* srv = 0;
    hr = device->CreateShaderResourceView(tex, 0, &srv);
    if (FAILED(hr) || !srv)
    {
        tex->Release();
        return LW_RET_FAILED;
    }

    *out_tex = new lwD3D11Texture(tex, srv, w, h, d3d9);
    return LW_RET_OK;
}

static void ApplyColorKey(BYTE* bgra, UINT w, UINT h, D3DCOLOR colorkey)
{
    if (!colorkey)
        return;
    const DWORD rgb = colorkey & 0x00FFFFFFu;
    DWORD* p = (DWORD*)bgra;
    const UINT n = w * h;
    for (UINT i = 0; i < n; ++i)
    {
        if ((p[i] & 0x00FFFFFFu) == rgb)
            p[i] &= 0x00FFFFFFu;
    }
}

static void ForceOpaque(BYTE* bgra, UINT w, UINT h)
{
    DWORD* p = (DWORD*)bgra;
    const UINT n = w * h;
    for (UINT i = 0; i < n; ++i)
        p[i] |= 0xFF000000u;
}

// True-alpha sources (TGA/32-bit) sometimes decode with A=0 for every texel.
// Only fill alpha when the whole image is empty; mixed alpha is real.
static void ForceOpaqueIfNoAlpha(BYTE* bgra, UINT w, UINT h)
{
    DWORD* p = (DWORD*)bgra;
    const UINT n = w * h;
    for (UINT i = 0; i < n; ++i)
    {
        if (p[i] & 0xFF000000u)
            return;
    }
    ForceOpaque(bgra, w, h);
}

static UINT ReadU16(const BYTE* p)
{
    return (UINT)p[0] | ((UINT)p[1] << 8);
}

static UINT ReadU32(const BYTE* p)
{
    return (UINT)p[0] | ((UINT)p[1] << 8) | ((UINT)p[2] << 16) | ((UINT)p[3] << 24);
}

static INT ReadI32(const BYTE* p)
{
    return (INT)ReadU32(p);
}

// Paletted Ami faces (8-bit BMP, biClrUsed=255) do not round-trip through
// GDI+ the same way D3DX did: palette peFlags is treated as alpha, so
// alphatest punches out eyes/mouth and leaves a peach blob. Decode BMPs
// ourselves and force opaque before colorkey.
static LW_RESULT LoadBmp(
    ID3D11Device* device,
    const BYTE* data,
    UINT data_size,
    D3DCOLOR colorkey,
    D3DXIMAGE_INFO* src_info,
    IDirect3DTextureX** out_tex)
{
    if (data_size < 54 || data[0] != 'B' || data[1] != 'M')
        return LW_RET_FAILED;

    const UINT off_bits = ReadU32(data + 10);
    const UINT hdr_size = ReadU32(data + 14);
    if (hdr_size < 12 || off_bits >= data_size || 14 + hdr_size > data_size)
        return LW_RET_FAILED;

    INT width = 0;
    INT height_signed = 0;
    UINT bpp = 0;
    UINT compression = 0;
    UINT clr_used = 0;
    UINT pal_entry = 4;
    if (hdr_size == 12)
    {
        width = (INT)ReadU16(data + 18);
        height_signed = (INT)ReadU16(data + 20);
        bpp = ReadU16(data + 24);
        pal_entry = 3;
    }
    else
    {
        width = ReadI32(data + 18);
        height_signed = ReadI32(data + 22);
        bpp = ReadU16(data + 28);
        compression = ReadU32(data + 30);
        clr_used = ReadU32(data + 46);
    }
    if (width <= 0 || height_signed == 0)
        return LW_RET_FAILED;
    if (compression != 0)
        return LW_RET_FAILED;
    if (bpp != 8 && bpp != 24 && bpp != 32)
        return LW_RET_FAILED;

    const int top_down = height_signed < 0;
    const UINT height = (UINT)(top_down ? -height_signed : height_signed);
    const UINT w = (UINT)width;
    if (w > 8192 || height > 8192)
        return LW_RET_FAILED;

    std::vector<BYTE> pixels((size_t)w * height * 4);
    const BYTE* bits = data + off_bits;

    if (bpp == 8)
    {
        UINT ncolors = clr_used ? clr_used : 256;
        if (ncolors > 256)
            ncolors = 256;
        const BYTE* pal = data + 14 + hdr_size;
        if (pal + ncolors * pal_entry > data + data_size)
            return LW_RET_FAILED;
        const UINT src_stride = (w + 3u) & ~3u;
        if (off_bits + src_stride * height > data_size)
            return LW_RET_FAILED;
        for (UINT y = 0; y < height; ++y)
        {
            const UINT src_y = top_down ? y : (height - 1 - y);
            const BYTE* row = bits + src_y * src_stride;
            BYTE* dst = pixels.data() + y * w * 4;
            for (UINT x = 0; x < w; ++x)
            {
                UINT idx = row[x];
                if (idx >= ncolors)
                    idx = 0;
                const BYTE* c = pal + idx * pal_entry;
                dst[0] = c[0];
                dst[1] = pal_entry == 3 ? c[1] : c[1];
                dst[2] = pal_entry == 3 ? c[2] : c[2];
                dst[3] = 255;
                dst += 4;
            }
        }
    }
    else
    {
        const UINT src_bpp = bpp / 8;
        const UINT src_stride = (w * src_bpp + 3u) & ~3u;
        if (off_bits + src_stride * height > data_size)
            return LW_RET_FAILED;
        for (UINT y = 0; y < height; ++y)
        {
            const UINT src_y = top_down ? y : (height - 1 - y);
            const BYTE* row = bits + src_y * src_stride;
            BYTE* dst = pixels.data() + y * w * 4;
            for (UINT x = 0; x < w; ++x)
            {
                dst[0] = row[0];
                dst[1] = row[1];
                dst[2] = row[2];
                dst[3] = (src_bpp == 4) ? row[3] : 255;
                row += src_bpp;
                dst += 4;
            }
        }
        if (src_bpp == 4)
            ForceOpaqueIfNoAlpha(pixels.data(), w, height);
        else
            ForceOpaque(pixels.data(), w, height);
    }

    ApplyColorKey(pixels.data(), w, height, colorkey);

    if (src_info)
    {
        memset(src_info, 0, sizeof(*src_info));
        src_info->Width = w;
        src_info->Height = height;
        src_info->Depth = 1;
        src_info->MipLevels = 1;
        src_info->Format = D3DFMT_A8R8G8B8;
        src_info->ResourceType = D3DRTYPE_TEXTURE;
        src_info->ImageFileFormat = D3DXIFF_BMP;
    }

    return CreateFromPixels(device, pixels.data(), w, height, w * 4, DXGI_FORMAT_B8G8R8A8_UNORM, D3DFMT_A8R8G8B8, out_tex);
}

static int EnsureGdiplus()
{
    static int s_ok = 0;
    static ULONG_PTR s_token = 0;
    if (s_ok)
        return 1;
    Gdiplus::GdiplusStartupInput input;
    if (Gdiplus::GdiplusStartup(&s_token, &input, 0) != Gdiplus::Ok)
        return 0;
    s_ok = 1;
    return 1;
}

static LW_RESULT LoadGdiplus(
    ID3D11Device* device,
    const void* data,
    UINT data_size,
    D3DCOLOR colorkey,
    D3DXIMAGE_INFO* src_info,
    IDirect3DTextureX** out_tex)
{
    if (!EnsureGdiplus())
        return LW_RET_FAILED;

    HGLOBAL mem = GlobalAlloc(GMEM_MOVEABLE, data_size);
    if (!mem)
        return LW_RET_FAILED;
    void* locked = GlobalLock(mem);
    if (!locked)
    {
        GlobalFree(mem);
        return LW_RET_FAILED;
    }
    memcpy(locked, data, data_size);
    GlobalUnlock(mem);
    IStream* stream = 0;
    if (FAILED(CreateStreamOnHGlobal(mem, TRUE, &stream)) || !stream)
    {
        GlobalFree(mem);
        return LW_RET_FAILED;
    }

    Gdiplus::Bitmap bitmap(stream);
    if (bitmap.GetLastStatus() != Gdiplus::Ok)
    {
        stream->Release();
        return LW_RET_FAILED;
    }

    const UINT w = bitmap.GetWidth();
    const UINT h = bitmap.GetHeight();
    if (w == 0 || h == 0)
    {
        stream->Release();
        return LW_RET_FAILED;
    }

    Gdiplus::BitmapData bd;
    memset(&bd, 0, sizeof(bd));
    Gdiplus::Rect rc(0, 0, (INT)w, (INT)h);
    if (bitmap.LockBits(&rc, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &bd) != Gdiplus::Ok)
    {
        stream->Release();
        return LW_RET_FAILED;
    }

    std::vector<BYTE> pixels((size_t)w * h * 4);
    const BYTE* src = (const BYTE*)bd.Scan0;
    BYTE* dst = pixels.data();
    for (UINT y = 0; y < h; ++y)
    {
        memcpy(dst + y * w * 4, src + y * bd.Stride, (size_t)w * 4);
    }
    bitmap.UnlockBits(&bd);
    stream->Release();

    const BYTE* raw = (const BYTE*)data;
    const int bmp_or_jpeg = (data_size >= 2 && ((raw[0] == 'B' && raw[1] == 'M') || (raw[0] == 0xFF && raw[1] == 0xD8)));
    if (bmp_or_jpeg)
        ForceOpaque(pixels.data(), w, h);
    else
        ForceOpaqueIfNoAlpha(pixels.data(), w, h);
    ApplyColorKey(pixels.data(), w, h, colorkey);

    if (src_info)
    {
        memset(src_info, 0, sizeof(*src_info));
        src_info->Width = w;
        src_info->Height = h;
        src_info->Depth = 1;
        src_info->MipLevels = 1;
        src_info->Format = D3DFMT_A8R8G8B8;
        src_info->ResourceType = D3DRTYPE_TEXTURE;
        src_info->ImageFileFormat = D3DXIFF_PNG;
    }

    return CreateFromPixels(device, pixels.data(), w, h, w * 4, DXGI_FORMAT_B8G8R8A8_UNORM, D3DFMT_A8R8G8B8, out_tex);
}

static LW_RESULT LoadDds(
    ID3D11Device* device,
    const BYTE* data,
    UINT data_size,
    D3DXIMAGE_INFO* src_info,
    IDirect3DTextureX** out_tex)
{
    if (data_size < 4 + sizeof(lwDDSHeader))
        return LW_RET_FAILED;
    if (memcmp(data, "DDS ", 4) != 0)
        return LW_RET_FAILED;

    const lwDDSHeader* hdr = (const lwDDSHeader*)(data + 4);
    if (hdr->size != 124 || hdr->width == 0 || hdr->height == 0)
        return LW_RET_FAILED;

    DXGI_FORMAT dxgi = DXGI_FORMAT_UNKNOWN;
    D3DFORMAT d3d9 = D3DFMT_UNKNOWN;
    UINT block = 0;
    UINT bpp = 0;

    if (hdr->ddspf.flag & DDS_FOURCC)
    {
        switch (hdr->ddspf.four_cc)
        {
        case MAKEFOURCC('D', 'X', 'T', '1'):
            dxgi = DXGI_FORMAT_BC1_UNORM;
            d3d9 = D3DFMT_DXT1;
            block = 8;
            break;
        case MAKEFOURCC('D', 'X', 'T', '2'):
        case MAKEFOURCC('D', 'X', 'T', '3'):
            dxgi = DXGI_FORMAT_BC2_UNORM;
            d3d9 = D3DFMT_DXT3;
            block = 16;
            break;
        case MAKEFOURCC('D', 'X', 'T', '4'):
        case MAKEFOURCC('D', 'X', 'T', '5'):
            dxgi = DXGI_FORMAT_BC3_UNORM;
            d3d9 = D3DFMT_DXT5;
            block = 16;
            break;
        default:
            return LW_RET_FAILED;
        }
    }
    else if (hdr->ddspf.bit_count == 32)
    {
        dxgi = DXGI_FORMAT_B8G8R8A8_UNORM;
        d3d9 = D3DFMT_A8R8G8B8;
        bpp = 4;
    }
    else if (hdr->ddspf.bit_count == 24)
    {
        d3d9 = D3DFMT_R8G8B8;
        bpp = 3;
    }
    else if (hdr->ddspf.bit_count == 16)
    {
        d3d9 = D3DFMT_A1R5G5B5;
        bpp = 2;
    }

    const BYTE* bits = data + 4 + sizeof(lwDDSHeader);
    const UINT remain = data_size - (4 + sizeof(lwDDSHeader));
    UINT pitch = 0;
    UINT bytes = 0;
    if (block)
    {
        pitch = ((hdr->width + 3) / 4) * block;
        bytes = pitch * ((hdr->height + 3) / 4);
    }
    else
    {
        pitch = hdr->width * bpp;
        bytes = pitch * hdr->height;
    }
    if (bytes == 0 || bytes > remain)
        return LW_RET_FAILED;

    if (src_info)
    {
        memset(src_info, 0, sizeof(*src_info));
        src_info->Width = hdr->width;
        src_info->Height = hdr->height;
        src_info->Depth = 1;
        src_info->MipLevels = 1;
        src_info->Format = d3d9;
        src_info->ResourceType = D3DRTYPE_TEXTURE;
        src_info->ImageFileFormat = D3DXIFF_DDS;
    }

    if (dxgi != DXGI_FORMAT_UNKNOWN)
        return CreateFromPixels(device, bits, hdr->width, hdr->height, pitch, dxgi, d3d9, out_tex);

    if (bpp != 2 && bpp != 3)
        return LW_RET_FAILED;

    std::vector<BYTE> rgba((size_t)hdr->width * hdr->height * 4);
    const UINT rm = hdr->ddspf.bitmask_r;
    const UINT gm = hdr->ddspf.bitmask_g;
    const UINT bm = hdr->ddspf.bitmask_b;
    const UINT am = hdr->ddspf.bitmask_a;
    for (UINT y = 0; y < hdr->height; ++y)
    {
        const BYTE* row = bits + y * pitch;
        BYTE* out = rgba.data() + (size_t)y * hdr->width * 4;
        for (UINT x = 0; x < hdr->width; ++x)
        {
            BYTE b = 0, g = 0, r = 0, a = 255;
            if (bpp == 3)
            {
                b = row[0];
                g = row[1];
                r = row[2];
                row += 3;
            }
            else
            {
                const USHORT p = (USHORT)(row[0] | (row[1] << 8));
                row += 2;
                if (am == 0x8000 || (rm == 0x7C00 && gm == 0x03E0))
                {
                    r = (BYTE)(((p >> 10) & 31) * 255 / 31);
                    g = (BYTE)(((p >> 5) & 31) * 255 / 31);
                    b = (BYTE)((p & 31) * 255 / 31);
                    a = (p & 0x8000) ? 255 : 0;
                }
                else if (rm == 0xF800 || gm == 0x07E0)
                {
                    r = (BYTE)(((p >> 11) & 31) * 255 / 31);
                    g = (BYTE)(((p >> 5) & 63) * 255 / 63);
                    b = (BYTE)((p & 31) * 255 / 31);
                }
                else
                {
                    r = (BYTE)(((p >> 8) & 0xF) * 17);
                    g = (BYTE)(((p >> 4) & 0xF) * 17);
                    b = (BYTE)((p & 0xF) * 17);
                    a = (BYTE)(((p >> 12) & 0xF) * 17);
                }
            }
            out[0] = b;
            out[1] = g;
            out[2] = r;
            out[3] = a;
            out += 4;
        }
    }
    ForceOpaqueIfNoAlpha(rgba.data(), hdr->width, hdr->height);
    return CreateFromPixels(device, rgba.data(), hdr->width, hdr->height, hdr->width * 4,
        DXGI_FORMAT_B8G8R8A8_UNORM, D3DFMT_A8R8G8B8, out_tex);
}

#pragma pack(push, 1)
struct TgaHdr
{
    BYTE id_len;
    BYTE cm_type;
    BYTE type;
    USHORT cm_start;
    USHORT cm_len;
    BYTE cm_bits;
    USHORT xorg;
    USHORT yorg;
    USHORT width;
    USHORT height;
    BYTE bpp;
    BYTE flags;
};
#pragma pack(pop)

static LW_RESULT LoadTga(
    ID3D11Device* device,
    const BYTE* data,
    UINT data_size,
    D3DCOLOR colorkey,
    D3DXIMAGE_INFO* src_info,
    IDirect3DTextureX** out_tex)
{
    if (data_size < sizeof(TgaHdr))
        return LW_RET_FAILED;
    TgaHdr hdr;
    memcpy(&hdr, data, sizeof(hdr));
    if ((hdr.type != 2 && hdr.type != 10) || (hdr.bpp != 24 && hdr.bpp != 32) || hdr.width == 0 || hdr.height == 0)
        return LW_RET_FAILED;

    const BYTE* src = data + sizeof(TgaHdr) + hdr.id_len;
    if (src > data + data_size)
        return LW_RET_FAILED;

    const UINT w = hdr.width;
    const UINT h = hdr.height;
    const UINT src_bpp = hdr.bpp / 8;
    const int origin_top = (hdr.flags & 0x20) != 0;

    std::vector<BYTE> pixels((size_t)w * h * 4);
    BYTE* dst = pixels.data();

    if (hdr.type == 2)
    {
        const UINT row = w * src_bpp;
        if (src + row * h > data + data_size)
            return LW_RET_FAILED;
        for (UINT y = 0; y < h; ++y)
        {
            const BYTE* rowp = src + (origin_top ? y : (h - 1 - y)) * row;
            BYTE* out = dst + y * w * 4;
            for (UINT x = 0; x < w; ++x)
            {
                out[0] = rowp[0];
                out[1] = rowp[1];
                out[2] = rowp[2];
                out[3] = (src_bpp == 4) ? rowp[3] : 255;
                rowp += src_bpp;
                out += 4;
            }
        }
    }
    else
    {
        UINT px = 0;
        const UINT total = w * h;
        const BYTE* end = data + data_size;
        std::vector<BYTE> raw((size_t)total * src_bpp);
        BYTE* rp = raw.data();
        while (px < total)
        {
            if (src >= end)
                return LW_RET_FAILED;
            BYTE packet = *src++;
            UINT count = (packet & 0x7F) + 1;
            if (px + count > total)
                return LW_RET_FAILED;
            if (packet & 0x80)
            {
                if (src + src_bpp > end)
                    return LW_RET_FAILED;
                for (UINT i = 0; i < count; ++i)
                {
                    memcpy(rp, src, src_bpp);
                    rp += src_bpp;
                }
                src += src_bpp;
            }
            else
            {
                if (src + count * src_bpp > end)
                    return LW_RET_FAILED;
                memcpy(rp, src, count * src_bpp);
                rp += count * src_bpp;
                src += count * src_bpp;
            }
            px += count;
        }
        for (UINT y = 0; y < h; ++y)
        {
            const BYTE* rowp = raw.data() + (origin_top ? y : (h - 1 - y)) * w * src_bpp;
            BYTE* out = dst + y * w * 4;
            for (UINT x = 0; x < w; ++x)
            {
                out[0] = rowp[0];
                out[1] = rowp[1];
                out[2] = rowp[2];
                out[3] = (src_bpp == 4) ? rowp[3] : 255;
                rowp += src_bpp;
                out += 4;
            }
        }
    }

    ForceOpaqueIfNoAlpha(pixels.data(), w, h);
    ApplyColorKey(pixels.data(), w, h, colorkey);

    if (src_info)
    {
        memset(src_info, 0, sizeof(*src_info));
        src_info->Width = w;
        src_info->Height = h;
        src_info->Depth = 1;
        src_info->MipLevels = 1;
        src_info->Format = D3DFMT_A8R8G8B8;
        src_info->ResourceType = D3DRTYPE_TEXTURE;
        src_info->ImageFileFormat = D3DXIFF_TGA;
    }

    return CreateFromPixels(device, pixels.data(), w, h, w * 4, DXGI_FORMAT_B8G8R8A8_UNORM, D3DFMT_A8R8G8B8, out_tex);
}

LW_RESULT lwD3D11CreateTextureFromMemory(
    ID3D11Device* device,
    const void* data,
    UINT data_size,
    D3DCOLOR colorkey,
    D3DXIMAGE_INFO* src_info,
    IDirect3DTextureX** out_tex)
{
    if (!device || !data || !out_tex || data_size < 4)
        return LW_RET_FAILED;
    *out_tex = 0;

    const BYTE* b = (const BYTE*)data;
    if (data_size >= 4 && memcmp(b, "DDS ", 4) == 0)
    {
        if (LoadDds(device, b, data_size, src_info, out_tex) == LW_RET_OK)
            return LW_RET_OK;
    }
    if (LoadBmp(device, b, data_size, colorkey, src_info, out_tex) == LW_RET_OK)
        return LW_RET_OK;
    if (LoadGdiplus(device, data, data_size, colorkey, src_info, out_tex) == LW_RET_OK)
        return LW_RET_OK;
    if (LoadTga(device, b, data_size, colorkey, src_info, out_tex) == LW_RET_OK)
        return LW_RET_OK;

    lwD3D11Gap(LW_D3D11_GAP, "tex-decode", "CreateTextureFromMemory failed (%u bytes)", (unsigned)data_size);
    return LW_RET_FAILED;
}

LW_RESULT lwD3D11CreateTextureFromFile(
    ID3D11Device* device,
    const char* path,
    D3DCOLOR colorkey,
    IDirect3DTextureX** out_tex)
{
    if (!device || !path || !out_tex)
        return LW_RET_FAILED;
    *out_tex = 0;

    FILE* fp = 0;
    if (fopen_s(&fp, path, "rb") != 0 || !fp)
        return LW_RET_FAILED;
    fseek(fp, 0, SEEK_END);
    long sz = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    if (sz <= 0)
    {
        fclose(fp);
        return LW_RET_FAILED;
    }
    std::vector<BYTE> buf((size_t)sz);
    size_t rd = fread(buf.data(), 1, (size_t)sz, fp);
    fclose(fp);
    if (rd != (size_t)sz)
        return LW_RET_FAILED;
    return lwD3D11CreateTextureFromMemory(device, buf.data(), (UINT)sz, colorkey, 0, out_tex);
}

LW_RESULT lwD3D11CreateEmptyTexture(
    ID3D11Device* device,
    UINT width,
    UINT height,
    D3DFORMAT format,
    IDirect3DTextureX** out_tex)
{
    if (!device || !out_tex || width == 0 || height == 0)
        return LW_RET_FAILED;
    if (format == 0 || format == D3DFMT_UNKNOWN)
        format = D3DFMT_A8R8G8B8;
    std::vector<BYTE> zeros((size_t)width * height * 4, 0);
    if (CreateFromPixels(device, zeros.data(), width, height, width * 4,
        DXGI_FORMAT_B8G8R8A8_UNORM, format, out_tex) != LW_RET_OK || !*out_tex)
        return LW_RET_FAILED;
    if (lwD3D11Texture* t = lwAsD3D11Texture(*out_tex))
        t->InitCpuLock(device, format);
    return LW_RET_OK;
}

LW_RESULT lwD3D11CreateRenderTargetTexture(
    ID3D11Device* device,
    UINT width,
    UINT height,
    IDirect3DTextureX** out_tex)
{
    if (!device || !out_tex || width == 0 || height == 0)
        return LW_RET_FAILED;
    *out_tex = 0;

    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

    ID3D11Texture2D* tex = 0;
    if (FAILED(device->CreateTexture2D(&desc, 0, &tex)) || !tex)
        return LW_RET_FAILED;

    ID3D11RenderTargetView* rtv = 0;
    ID3D11ShaderResourceView* srv = 0;
    if (FAILED(device->CreateRenderTargetView(tex, 0, &rtv)) || !rtv)
    {
        tex->Release();
        return LW_RET_FAILED;
    }
    if (FAILED(device->CreateShaderResourceView(tex, 0, &srv)) || !srv)
    {
        rtv->Release();
        tex->Release();
        return LW_RET_FAILED;
    }

    *out_tex = new lwD3D11Texture(tex, srv, width, height, D3DFMT_A8R8G8B8, rtv);
    return LW_RET_OK;
}

LW_RESULT lwD3D11CreateDepthStencil(
    ID3D11Device* device,
    UINT width,
    UINT height,
    ID3D11Texture2D** out_tex,
    ID3D11DepthStencilView** out_dsv)
{
    if (!device || !out_tex || !out_dsv || width == 0 || height == 0)
        return LW_RET_FAILED;
    *out_tex = 0;
    *out_dsv = 0;

    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

    ID3D11Texture2D* tex = 0;
    if (FAILED(device->CreateTexture2D(&desc, 0, &tex)) || !tex)
        return LW_RET_FAILED;
    ID3D11DepthStencilView* dsv = 0;
    if (FAILED(device->CreateDepthStencilView(tex, 0, &dsv)) || !dsv)
    {
        tex->Release();
        return LW_RET_FAILED;
    }
    *out_tex = tex;
    *out_dsv = dsv;
    return LW_RET_OK;
}

LW_END

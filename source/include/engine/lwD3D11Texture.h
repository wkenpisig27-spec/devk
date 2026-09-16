#pragma once

#include "MindPowerAPI.h"
#include "lwHeader.h"
#include "lwDirectX.h"

struct ID3D11Device;
struct ID3D11Texture2D;
struct ID3D11ShaderResourceView;
struct ID3D11RenderTargetView;
struct ID3D11DepthStencilView;

LW_BEGIN

// COM wrapper so existing IDirect3DTexture9* / lwITex paths can hold a D3D11 SRV.
class lwD3D11Texture : public IDirect3DTexture9
{
public:
    lwD3D11Texture(ID3D11Texture2D* tex, ID3D11ShaderResourceView* srv, UINT w, UINT h, D3DFORMAT fmt, ID3D11RenderTargetView* rtv = 0);
    ~lwD3D11Texture();

    ID3D11ShaderResourceView* GetSRV() const { return _srv; }
    ID3D11RenderTargetView* GetRTV() const { return _rtv; }
    UINT GetWidth() const { return _w; }
    UINT GetHeight() const { return _h; }

    STDMETHOD(QueryInterface)(REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    STDMETHOD(GetDevice)(IDirect3DDevice9** ppDevice);
    STDMETHOD(SetPrivateData)(REFGUID, const void*, DWORD, DWORD);
    STDMETHOD(GetPrivateData)(REFGUID, void*, DWORD*);
    STDMETHOD(FreePrivateData)(REFGUID);
    STDMETHOD_(DWORD, SetPriority)(DWORD);
    STDMETHOD_(DWORD, GetPriority)();
    STDMETHOD_(void, PreLoad)();
    STDMETHOD_(D3DRESOURCETYPE, GetType)();
    STDMETHOD_(DWORD, SetLOD)(DWORD);
    STDMETHOD_(DWORD, GetLOD)();
    STDMETHOD_(DWORD, GetLevelCount)();
    STDMETHOD(SetAutoGenFilterType)(D3DTEXTUREFILTERTYPE);
    STDMETHOD_(D3DTEXTUREFILTERTYPE, GetAutoGenFilterType)();
    STDMETHOD_(void, GenerateMipSubLevels)();
    STDMETHOD(GetLevelDesc)(UINT Level, D3DSURFACE_DESC* pDesc);
    STDMETHOD(GetSurfaceLevel)(UINT, IDirect3DSurface9**);
    STDMETHOD(LockRect)(UINT, D3DLOCKED_RECT*, const RECT*, DWORD);
    STDMETHOD(UnlockRect)(UINT);
    STDMETHOD(AddDirtyRect)(const RECT*);

private:
    ULONG _ref;
    ID3D11Texture2D* _tex;
    ID3D11ShaderResourceView* _srv;
    ID3D11RenderTargetView* _rtv;
    UINT _w;
    UINT _h;
    D3DFORMAT _fmt;
};

MINDPOWER_API const GUID& lwD3D11TextureGuid();
MINDPOWER_API lwD3D11Texture* lwAsD3D11Texture(IDirect3DBaseTextureX* tex);

MINDPOWER_API LW_RESULT lwD3D11CreateTextureFromMemory(
    ID3D11Device* device,
    const void* data,
    UINT data_size,
    D3DCOLOR colorkey,
    D3DXIMAGE_INFO* src_info,
    IDirect3DTextureX** out_tex);

MINDPOWER_API LW_RESULT lwD3D11CreateTextureFromFile(
    ID3D11Device* device,
    const char* path,
    D3DCOLOR colorkey,
    IDirect3DTextureX** out_tex);

MINDPOWER_API LW_RESULT lwD3D11CreateEmptyTexture(
    ID3D11Device* device,
    UINT width,
    UINT height,
    IDirect3DTextureX** out_tex);

MINDPOWER_API LW_RESULT lwD3D11CreateRenderTargetTexture(
    ID3D11Device* device,
    UINT width,
    UINT height,
    IDirect3DTextureX** out_tex);

MINDPOWER_API LW_RESULT lwD3D11CreateDepthStencil(
    ID3D11Device* device,
    UINT width,
    UINT height,
    ID3D11Texture2D** out_tex,
    ID3D11DepthStencilView** out_dsv);

LW_END

#pragma once

#include "MindPowerAPI.h"
#include "lwHeader.h"
#include "lwDirectX.h"

LW_BEGIN

#if MINDPOWER_USE_D3D9_DEVICE
class lwD3D11VertexBuffer : public IDirect3DVertexBuffer9
#else
class lwD3D11VertexBuffer : public lwDx11IVertexBuffer
#endif
{
public:
    lwD3D11VertexBuffer(ID3D11Device* device, ID3D11DeviceContext* context, ID3D11Buffer* buf, UINT bytes, DWORD fvf);
    ~lwD3D11VertexBuffer();

    ID3D11Buffer* GetBuffer() const { return _buf; }
    UINT GetBytes() const { return _bytes; }
    DWORD GetFVF() const { return _fvf; }

    STDMETHOD(QueryInterface)(REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();
    STDMETHOD(GetDevice)(IDirect3DDevice9** ppDevice);
#if MINDPOWER_USE_D3D9_DEVICE
    STDMETHOD(SetPrivateData)(REFGUID, const void*, DWORD, DWORD);
    STDMETHOD(GetPrivateData)(REFGUID, void*, DWORD*);
    STDMETHOD(FreePrivateData)(REFGUID);
    STDMETHOD_(DWORD, SetPriority)(DWORD);
    STDMETHOD_(DWORD, GetPriority)();
    STDMETHOD_(void, PreLoad)();
#endif
    STDMETHOD_(D3DRESOURCETYPE, GetType)();
    STDMETHOD(Lock)(UINT OffsetToLock, UINT SizeToLock, void** ppbData, DWORD Flags);
    STDMETHOD(Unlock)();
    STDMETHOD(GetDesc)(D3DVERTEXBUFFER_DESC* pDesc);

private:
    ULONG _ref;
    ID3D11Device* _device;
    ID3D11DeviceContext* _context;
    ID3D11Buffer* _buf;
    UINT _bytes;
    DWORD _fvf;
};

#if MINDPOWER_USE_D3D9_DEVICE
class lwD3D11IndexBuffer : public IDirect3DIndexBuffer9
#else
class lwD3D11IndexBuffer : public lwDx11IIndexBuffer
#endif
{
public:
    lwD3D11IndexBuffer(ID3D11Device* device, ID3D11DeviceContext* context, ID3D11Buffer* buf, UINT bytes, D3DFORMAT fmt);
    ~lwD3D11IndexBuffer();

    ID3D11Buffer* GetBuffer() const { return _buf; }
    UINT GetBytes() const { return _bytes; }
    D3DFORMAT GetFormat() const { return _fmt; }

    STDMETHOD(QueryInterface)(REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();
    STDMETHOD(GetDevice)(IDirect3DDevice9** ppDevice);
#if MINDPOWER_USE_D3D9_DEVICE
    STDMETHOD(SetPrivateData)(REFGUID, const void*, DWORD, DWORD);
    STDMETHOD(GetPrivateData)(REFGUID, void*, DWORD*);
    STDMETHOD(FreePrivateData)(REFGUID);
    STDMETHOD_(DWORD, SetPriority)(DWORD);
    STDMETHOD_(DWORD, GetPriority)();
    STDMETHOD_(void, PreLoad)();
#endif
    STDMETHOD_(D3DRESOURCETYPE, GetType)();
    STDMETHOD(Lock)(UINT OffsetToLock, UINT SizeToLock, void** ppbData, DWORD Flags);
    STDMETHOD(Unlock)();
    STDMETHOD(GetDesc)(D3DINDEXBUFFER_DESC* pDesc);

private:
    ULONG _ref;
    ID3D11Device* _device;
    ID3D11DeviceContext* _context;
    ID3D11Buffer* _buf;
    UINT _bytes;
    D3DFORMAT _fmt;
};

MINDPOWER_API const GUID& lwD3D11VertexBufferGuid();
MINDPOWER_API const GUID& lwD3D11IndexBufferGuid();
MINDPOWER_API lwD3D11VertexBuffer* lwAsD3D11VertexBuffer(IDirect3DVertexBufferX* vb);
MINDPOWER_API lwD3D11IndexBuffer* lwAsD3D11IndexBuffer(IDirect3DIndexBufferX* ib);

MINDPOWER_API LW_RESULT lwD3D11CreateVertexBuffer(
    ID3D11Device* device,
    ID3D11DeviceContext* context,
    UINT length,
    DWORD fvf,
    IDirect3DVertexBufferX** out_vb);

MINDPOWER_API LW_RESULT lwD3D11CreateIndexBuffer(
    ID3D11Device* device,
    ID3D11DeviceContext* context,
    UINT length,
    D3DFORMAT fmt,
    IDirect3DIndexBufferX** out_ib);

LW_END

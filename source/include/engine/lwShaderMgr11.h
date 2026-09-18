#pragma once

#include "MindPowerAPI.h"
#include "lwHeader.h"
#include "lwDirectX.h"

class lwDeviceObject11;

LW_BEGIN

#if MINDPOWER_USE_D3D9_DEVICE
class lwD3D11VertexShader : public IDirect3DVertexShader9
#else
class lwD3D11VertexShader : public lwDx11IVertexShader
#endif
{
public:
    lwD3D11VertexShader(ID3D11VertexShader* vs, void* blob);
    ~lwD3D11VertexShader();

    ID3D11VertexShader* GetVS() const { return _vs; }
    void* GetBlob() const { return _blob; }
    SIZE_T GetBlobSize() const;

    STDMETHOD(QueryInterface)(REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();
    STDMETHOD(GetDevice)(IDirect3DDevice9** ppDevice);
    STDMETHOD(GetFunction)(void* pData, UINT* pSizeOfData);

private:
    ULONG _ref;
    ID3D11VertexShader* _vs;
    void* _blob;
};

#if MINDPOWER_USE_D3D9_DEVICE
class lwD3D11VertexDecl : public IDirect3DVertexDeclaration9
#else
class lwD3D11VertexDecl : public lwDx11IVertexDecl
#endif
{
public:
    lwD3D11VertexDecl(const D3DVERTEXELEMENT9* elems, UINT count);
    ~lwD3D11VertexDecl();

    const D3DVERTEXELEMENT9* GetElements() const { return _elems; }
    UINT GetElementCount() const { return _count; }

    STDMETHOD(QueryInterface)(REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();
    STDMETHOD(GetDevice)(IDirect3DDevice9** ppDevice);
    STDMETHOD(GetDeclaration)(D3DVERTEXELEMENT9* pData, UINT* pNumElements);

private:
    ULONG _ref;
    D3DVERTEXELEMENT9* _elems;
    UINT _count;
};

MINDPOWER_API const GUID& lwD3D11VertexShaderGuid();
MINDPOWER_API const GUID& lwD3D11VertexDeclGuid();
MINDPOWER_API lwD3D11VertexShader* lwAsD3D11VertexShader(IDirect3DVertexShaderX* vs);
MINDPOWER_API lwD3D11VertexDecl* lwAsD3D11VertexDecl(IDirect3DVertexDeclarationX* decl);

MINDPOWER_API void lwD3D11ShaderMgrInit(ID3D11Device* device, ID3D11DeviceContext* context);
MINDPOWER_API void lwD3D11ShaderMgrShutdown();

MINDPOWER_API LW_RESULT lwD3D11CompileVertexShader(
    const char* file,
    const D3DXMACRO* defines,
    IDirect3DVertexShaderX** out_vs);

MINDPOWER_API LW_RESULT lwD3D11CreateVertexDecl(
    const D3DVERTEXELEMENT9* data,
    IDirect3DVertexDeclarationX** out_decl);

// Bind ShaderMgr11 VS/PS/cbuffers and return an input layout. 0 = use mesh fallback.
MINDPOWER_API int lwD3D11ShaderMgrPrepareDraw(lwDeviceObject11* dev, ID3D11InputLayout** out_layout);

LW_END

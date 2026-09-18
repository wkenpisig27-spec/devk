#include "stdafx.h"
#include "lwD3D11Buffer.h"
#include "lwD3D11Gaps.h"

#include <d3d11.h>

LW_BEGIN

static const GUID s_iid_d11vb =
    { 0xb3e7c201, 0x7d11, 0x4b11, { 0x9c, 0x11, 0xd3, 0xd1, 0x11, 0xc0, 0x11, 0x02 } };
static const GUID s_iid_d11ib =
    { 0xb3e7c202, 0x7d11, 0x4b11, { 0x9c, 0x11, 0xd3, 0xd1, 0x11, 0xc0, 0x11, 0x02 } };

const GUID& lwD3D11VertexBufferGuid() { return s_iid_d11vb; }
const GUID& lwD3D11IndexBufferGuid() { return s_iid_d11ib; }

template <typename T>
static T* AsWrap(IUnknown* obj, const GUID& guid)
{
    if (!obj)
        return 0;
    void* p = 0;
    if (FAILED(obj->QueryInterface(guid, &p)) || !p)
        return 0;
    ((IUnknown*)p)->Release();
    return (T*)p;
}

lwD3D11VertexBuffer* lwAsD3D11VertexBuffer(IDirect3DVertexBufferX* vb)
{
    return AsWrap<lwD3D11VertexBuffer>(vb, s_iid_d11vb);
}

lwD3D11IndexBuffer* lwAsD3D11IndexBuffer(IDirect3DIndexBufferX* ib)
{
    return AsWrap<lwD3D11IndexBuffer>(ib, s_iid_d11ib);
}

static D3D11_MAP MapFlags(DWORD flags)
{
    if (flags & D3DLOCK_NOOVERWRITE)
        return D3D11_MAP_WRITE_NO_OVERWRITE;
    return D3D11_MAP_WRITE_DISCARD;
}

lwD3D11VertexBuffer::lwD3D11VertexBuffer(ID3D11Device* device, ID3D11DeviceContext* context, ID3D11Buffer* buf, UINT bytes, DWORD fvf)
    : _ref(1), _device(device), _context(context), _buf(buf), _bytes(bytes), _fvf(fvf)
{
}

lwD3D11VertexBuffer::~lwD3D11VertexBuffer()
{
    if (_buf)
        _buf->Release();
}

HRESULT lwD3D11VertexBuffer::QueryInterface(REFIID riid, void** ppvObj)
{
    if (!ppvObj)
        return E_POINTER;
    if (riid == IID_IUnknown || riid == s_iid_d11vb
#if MINDPOWER_USE_D3D9_DEVICE
        || riid == IID_IDirect3DResource9 || riid == IID_IDirect3DVertexBuffer9
#endif
        )
    {
        *ppvObj = this;
        AddRef();
        return S_OK;
    }
    *ppvObj = 0;
    return E_NOINTERFACE;
}

ULONG lwD3D11VertexBuffer::AddRef() { return ++_ref; }
ULONG lwD3D11VertexBuffer::Release()
{
    ULONG n = --_ref;
    if (n == 0)
        delete this;
    return n;
}

HRESULT lwD3D11VertexBuffer::GetDevice(IDirect3DDevice9** ppDevice)
{
    if (ppDevice)
        *ppDevice = 0;
    return E_NOTIMPL;
}

#if MINDPOWER_USE_D3D9_DEVICE
HRESULT lwD3D11VertexBuffer::SetPrivateData(REFGUID, const void*, DWORD, DWORD) { return E_NOTIMPL; }
HRESULT lwD3D11VertexBuffer::GetPrivateData(REFGUID, void*, DWORD*) { return E_NOTIMPL; }
HRESULT lwD3D11VertexBuffer::FreePrivateData(REFGUID) { return E_NOTIMPL; }
DWORD lwD3D11VertexBuffer::SetPriority(DWORD) { return 0; }
DWORD lwD3D11VertexBuffer::GetPriority() { return 0; }
void lwD3D11VertexBuffer::PreLoad() {}
#endif
D3DRESOURCETYPE lwD3D11VertexBuffer::GetType() { return D3DRTYPE_VERTEXBUFFER; }

HRESULT lwD3D11VertexBuffer::Lock(UINT OffsetToLock, UINT SizeToLock, void** ppbData, DWORD Flags)
{
    if (!ppbData || !_context || !_buf)
        return D3DERR_INVALIDCALL;
    if (OffsetToLock >= _bytes)
        return D3DERR_INVALIDCALL;
    D3D11_MAPPED_SUBRESOURCE mapped = {};
    if (FAILED(_context->Map(_buf, 0, MapFlags(Flags), 0, &mapped)))
        return D3DERR_INVALIDCALL;
    *ppbData = (BYTE*)mapped.pData + OffsetToLock;
    (void)SizeToLock;
    return S_OK;
}

HRESULT lwD3D11VertexBuffer::Unlock()
{
    if (_context && _buf)
        _context->Unmap(_buf, 0);
    return S_OK;
}

HRESULT lwD3D11VertexBuffer::GetDesc(D3DVERTEXBUFFER_DESC* pDesc)
{
    if (!pDesc)
        return D3DERR_INVALIDCALL;
    memset(pDesc, 0, sizeof(*pDesc));
    pDesc->Format = D3DFMT_VERTEXDATA;
    pDesc->Type = D3DRTYPE_VERTEXBUFFER;
    pDesc->Usage = D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY;
    pDesc->Pool = D3DPOOL_DEFAULT;
    pDesc->Size = _bytes;
    pDesc->FVF = _fvf;
    return S_OK;
}

lwD3D11IndexBuffer::lwD3D11IndexBuffer(ID3D11Device* device, ID3D11DeviceContext* context, ID3D11Buffer* buf, UINT bytes, D3DFORMAT fmt)
    : _ref(1), _device(device), _context(context), _buf(buf), _bytes(bytes), _fmt(fmt)
{
}

lwD3D11IndexBuffer::~lwD3D11IndexBuffer()
{
    if (_buf)
        _buf->Release();
}

HRESULT lwD3D11IndexBuffer::QueryInterface(REFIID riid, void** ppvObj)
{
    if (!ppvObj)
        return E_POINTER;
    if (riid == IID_IUnknown || riid == s_iid_d11ib
#if MINDPOWER_USE_D3D9_DEVICE
        || riid == IID_IDirect3DResource9 || riid == IID_IDirect3DIndexBuffer9
#endif
        )
    {
        *ppvObj = this;
        AddRef();
        return S_OK;
    }
    *ppvObj = 0;
    return E_NOINTERFACE;
}

ULONG lwD3D11IndexBuffer::AddRef() { return ++_ref; }
ULONG lwD3D11IndexBuffer::Release()
{
    ULONG n = --_ref;
    if (n == 0)
        delete this;
    return n;
}

HRESULT lwD3D11IndexBuffer::GetDevice(IDirect3DDevice9** ppDevice)
{
    if (ppDevice)
        *ppDevice = 0;
    return E_NOTIMPL;
}

#if MINDPOWER_USE_D3D9_DEVICE
HRESULT lwD3D11IndexBuffer::SetPrivateData(REFGUID, const void*, DWORD, DWORD) { return E_NOTIMPL; }
HRESULT lwD3D11IndexBuffer::GetPrivateData(REFGUID, void*, DWORD*) { return E_NOTIMPL; }
HRESULT lwD3D11IndexBuffer::FreePrivateData(REFGUID) { return E_NOTIMPL; }
DWORD lwD3D11IndexBuffer::SetPriority(DWORD) { return 0; }
DWORD lwD3D11IndexBuffer::GetPriority() { return 0; }
void lwD3D11IndexBuffer::PreLoad() {}
#endif
D3DRESOURCETYPE lwD3D11IndexBuffer::GetType() { return D3DRTYPE_INDEXBUFFER; }

HRESULT lwD3D11IndexBuffer::Lock(UINT OffsetToLock, UINT SizeToLock, void** ppbData, DWORD Flags)
{
    if (!ppbData || !_context || !_buf)
        return D3DERR_INVALIDCALL;
    if (OffsetToLock >= _bytes)
        return D3DERR_INVALIDCALL;
    D3D11_MAPPED_SUBRESOURCE mapped = {};
    if (FAILED(_context->Map(_buf, 0, MapFlags(Flags), 0, &mapped)))
        return D3DERR_INVALIDCALL;
    *ppbData = (BYTE*)mapped.pData + OffsetToLock;
    (void)SizeToLock;
    return S_OK;
}

HRESULT lwD3D11IndexBuffer::Unlock()
{
    if (_context && _buf)
        _context->Unmap(_buf, 0);
    return S_OK;
}

HRESULT lwD3D11IndexBuffer::GetDesc(D3DINDEXBUFFER_DESC* pDesc)
{
    if (!pDesc)
        return D3DERR_INVALIDCALL;
    memset(pDesc, 0, sizeof(*pDesc));
    pDesc->Format = _fmt;
    pDesc->Type = D3DRTYPE_INDEXBUFFER;
    pDesc->Usage = D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY;
    pDesc->Pool = D3DPOOL_DEFAULT;
    pDesc->Size = _bytes;
    return S_OK;
}

static LW_RESULT CreateDynBuffer(ID3D11Device* device, UINT length, UINT bind, ID3D11Buffer** out)
{
    if (!device || !out || length == 0)
        return LW_RET_FAILED;
    D3D11_BUFFER_DESC bd = {};
    bd.ByteWidth = (length + 15u) & ~15u;
    if (bd.ByteWidth < 16)
        bd.ByteWidth = 16;
    bd.Usage = D3D11_USAGE_DYNAMIC;
    bd.BindFlags = bind;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    if (FAILED(device->CreateBuffer(&bd, 0, out)))
        return LW_RET_FAILED;
    return LW_RET_OK;
}

LW_RESULT lwD3D11CreateVertexBuffer(ID3D11Device* device, ID3D11DeviceContext* context, UINT length, DWORD fvf, IDirect3DVertexBufferX** out_vb)
{
    if (out_vb)
        *out_vb = 0;
    ID3D11Buffer* buf = 0;
    if (CreateDynBuffer(device, length, D3D11_BIND_VERTEX_BUFFER, &buf) != LW_RET_OK)
    {
        lwD3D11Gap(LW_D3D11_GAP, "create-vb", "ID3D11Buffer VB failed (%u bytes)", (unsigned)length);
        return LW_RET_FAILED;
    }
    *out_vb = new lwD3D11VertexBuffer(device, context, buf, length, fvf);
    return LW_RET_OK;
}

LW_RESULT lwD3D11CreateIndexBuffer(ID3D11Device* device, ID3D11DeviceContext* context, UINT length, D3DFORMAT fmt, IDirect3DIndexBufferX** out_ib)
{
    if (out_ib)
        *out_ib = 0;
    ID3D11Buffer* buf = 0;
    if (CreateDynBuffer(device, length, D3D11_BIND_INDEX_BUFFER, &buf) != LW_RET_OK)
    {
        lwD3D11Gap(LW_D3D11_GAP, "create-ib", "ID3D11Buffer IB failed (%u bytes)", (unsigned)length);
        return LW_RET_FAILED;
    }
    *out_ib = new lwD3D11IndexBuffer(device, context, buf, length, fmt);
    return LW_RET_OK;
}

LW_END

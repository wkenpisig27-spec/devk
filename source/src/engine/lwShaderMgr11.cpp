#include "stdafx.h"
#include "lwShaderMgr11.h"
#include "lwDeviceObject11.h"
#include "lwD3D11Gaps.h"
#include "lwRenderBackend.h"
#include "lwD3D11Mesh.h"

#include <d3d11.h>
#include <d3dcompiler.h>
#include <map>
#include <stdio.h>
#include <string.h>

#pragma comment(lib, "d3dcompiler.lib")

LW_BEGIN

static const GUID s_iid_d11vs =
    { 0xb3e7c211, 0x7d11, 0x4b11, { 0x9c, 0x11, 0xd3, 0xd1, 0x11, 0xc0, 0x11, 0x02 } };
static const GUID s_iid_d11decl =
    { 0xb3e7c212, 0x7d11, 0x4b11, { 0x9c, 0x11, 0xd3, 0xd1, 0x11, 0xc0, 0x11, 0x02 } };

const GUID& lwD3D11VertexShaderGuid() { return s_iid_d11vs; }
const GUID& lwD3D11VertexDeclGuid() { return s_iid_d11decl; }

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

lwD3D11VertexShader* lwAsD3D11VertexShader(IDirect3DVertexShaderX* vs)
{
    return AsWrap<lwD3D11VertexShader>(vs, s_iid_d11vs);
}

lwD3D11VertexDecl* lwAsD3D11VertexDecl(IDirect3DVertexDeclarationX* decl)
{
    return AsWrap<lwD3D11VertexDecl>(decl, s_iid_d11decl);
}

lwD3D11VertexShader::lwD3D11VertexShader(ID3D11VertexShader* vs, void* blob)
    : _ref(1), _vs(vs), _blob(blob)
{
}

lwD3D11VertexShader::~lwD3D11VertexShader()
{
    if (_vs)
        _vs->Release();
    if (_blob)
        ((ID3DBlob*)_blob)->Release();
}

HRESULT lwD3D11VertexShader::QueryInterface(REFIID riid, void** ppvObj)
{
    if (!ppvObj)
        return E_POINTER;
    if (riid == IID_IUnknown || riid == IID_IDirect3DVertexShader9 || riid == s_iid_d11vs)
    {
        *ppvObj = this;
        AddRef();
        return S_OK;
    }
    *ppvObj = 0;
    return E_NOINTERFACE;
}

ULONG lwD3D11VertexShader::AddRef() { return ++_ref; }
ULONG lwD3D11VertexShader::Release()
{
    ULONG n = --_ref;
    if (n == 0)
        delete this;
    return n;
}

HRESULT lwD3D11VertexShader::GetDevice(IDirect3DDevice9** ppDevice)
{
    if (ppDevice)
        *ppDevice = 0;
    return E_NOTIMPL;
}

HRESULT lwD3D11VertexShader::GetFunction(void* pData, UINT* pSizeOfData)
{
    ID3DBlob* blob = (ID3DBlob*)_blob;
    if (!blob || !pSizeOfData)
        return D3DERR_INVALIDCALL;
    UINT sz = (UINT)blob->GetBufferSize();
    if (!pData)
    {
        *pSizeOfData = sz;
        return S_OK;
    }
    if (*pSizeOfData < sz)
        return D3DERR_MOREDATA;
    memcpy(pData, blob->GetBufferPointer(), sz);
    *pSizeOfData = sz;
    return S_OK;
}

SIZE_T lwD3D11VertexShader::GetBlobSize() const
{
    ID3DBlob* blob = (ID3DBlob*)_blob;
    return blob ? blob->GetBufferSize() : 0;
}

lwD3D11VertexDecl::lwD3D11VertexDecl(const D3DVERTEXELEMENT9* elems, UINT count)
    : _ref(1), _elems(0), _count(count)
{
    if (count)
    {
        _elems = new D3DVERTEXELEMENT9[count];
        memcpy(_elems, elems, sizeof(D3DVERTEXELEMENT9) * count);
    }
}

lwD3D11VertexDecl::~lwD3D11VertexDecl()
{
    delete[] _elems;
}

HRESULT lwD3D11VertexDecl::QueryInterface(REFIID riid, void** ppvObj)
{
    if (!ppvObj)
        return E_POINTER;
    if (riid == IID_IUnknown || riid == IID_IDirect3DVertexDeclaration9 || riid == s_iid_d11decl)
    {
        *ppvObj = this;
        AddRef();
        return S_OK;
    }
    *ppvObj = 0;
    return E_NOINTERFACE;
}

ULONG lwD3D11VertexDecl::AddRef() { return ++_ref; }
ULONG lwD3D11VertexDecl::Release()
{
    ULONG n = --_ref;
    if (n == 0)
        delete this;
    return n;
}

HRESULT lwD3D11VertexDecl::GetDevice(IDirect3DDevice9** ppDevice)
{
    if (ppDevice)
        *ppDevice = 0;
    return E_NOTIMPL;
}

HRESULT lwD3D11VertexDecl::GetDeclaration(D3DVERTEXELEMENT9* pData, UINT* pNumElements)
{
    if (!pNumElements)
        return D3DERR_INVALIDCALL;
    if (!pData)
    {
        *pNumElements = _count;
        return S_OK;
    }
    if (*pNumElements < _count)
        return D3DERR_MOREDATA;
    if (_elems && _count)
        memcpy(pData, _elems, sizeof(D3DVERTEXELEMENT9) * _count);
    *pNumElements = _count;
    return S_OK;
}

//------------------------------------------------------------------------------

enum { VS_CONST_FLOAT4 = 256 };

static const char* kModulatePS =
    "cbuffer PSExtra : register(b1) {\n"
    "  float4 extra;\n"
    "};\n"
    "Texture2D tex0 : register(t0);\n"
    "SamplerState samp0 : register(s0);\n"
    "float4 main(float4 pos : SV_POSITION, float4 col : COLOR0, float2 uv : TEXCOORD0) : SV_TARGET {\n"
    "  float4 tex = tex0.Sample(samp0, uv);\n"
    "  float4 c = (extra.x > 0.5) ? float4(col.rgb, tex.a) : tex * col;\n"
    "  if (extra.w >= 0 && c.a < extra.w) discard;\n"
    "  return c;\n"
    "}\n";

struct ShaderMgr11State
{
    ID3D11Device* device;
    ID3D11DeviceContext* context;
    ID3D11PixelShader* ps;
    ID3D11Buffer* vs_cb;
    ID3D11Buffer* ps_cb;
    int ready;
};

static ShaderMgr11State s_sm = {};
static std::map<unsigned long long, ID3D11InputLayout*> s_layouts;

struct VshMap
{
    const char* vsh;
    const char* hlsl;
    const char* macro;
};

static const VshMap kVshMap[] = {
    { "skinmesh8_1.vsh", "pu4nt0_ld.hlsl", 0 },
    { "skinmesh8_2.vsh", "pb1u4nt0_ld.hlsl", 0 },
    { "skinmesh8_3.vsh", "pb2u4nt0_ld.hlsl", 0 },
    { "skinmesh8_4.vsh", "pb3u4nt0_ld.hlsl", 0 },
    { "vs_pnt0_ld_t0uvmat.vsh", "vs_pnt0_ld_t0uvmat.hlsl", 0 },
    { "vs_pnt0_t0uvmat.vsh", "vs_pnt0_t0uvmat.hlsl", 0 },
    { "vs_pndt0_ld_t0uvmat.vsh", "vs_pndt0_ld_t0uvmat.hlsl", 0 },
    { "vs_pnt0_ld.vsh", "vs_pnt0_ld.hlsl", 0 },
    { "vs_pndt0.vsh", "vs_pndt0.hlsl", 0 },
    { "vs_pndt0_ld.vsh", "vs_pndt0_ld.hlsl", 0 },
    { "vs_pndt0_t0uvmat.vsh", "vs_pndt0_t0uvmat.hlsl", 0 },
    { "skinmesh8_1_tt1.vsh", "pu4nt0_ld.hlsl", "USE_UVMAT0" },
    { "skinmesh8_2_tt1.vsh", "pb1u4nt0_ld.hlsl", "USE_UVMAT0" },
    { "skinmesh8_3_tt1.vsh", "pb2u4nt0_ld.hlsl", "USE_UVMAT0" },
    { "skinmesh8_4_tt1.vsh", "pb3u4nt0_ld.hlsl", "USE_UVMAT0" },
    { "skinmesh8_1_tt2.vsh", "pu4nt0_ld.hlsl", "USE_UVMAT1" },
    { "skinmesh8_2_tt2.vsh", "pb1u4nt0_ld.hlsl", "USE_UVMAT1" },
    { "skinmesh8_3_tt2.vsh", "pb2u4nt0_ld.hlsl", "USE_UVMAT1" },
    { "skinmesh8_4_tt2.vsh", "pb3u4nt0_ld.hlsl", "USE_UVMAT1" },
    { "skinmesh8_1_tt3.vsh", "pu4nt0_ld.hlsl", "USE_UVMAT2" },
    { "skinmesh8_2_tt3.vsh", "pb1u4nt0_ld.hlsl", "USE_UVMAT2" },
    { "skinmesh8_3_tt3.vsh", "pb2u4nt0_ld.hlsl", "USE_UVMAT2" },
    { "skinmesh8_4_tt3.vsh", "pb3u4nt0_ld.hlsl", "USE_UVMAT2" },
    { "skinmesh8_1_outline.vsh", "pu4nt0_ld_outline.hlsl", 0 },
    { "skinmesh8_2_outline.vsh", "pb1u4nt0_ld_outline.hlsl", 0 },
    { "skinmesh8_3_outline.vsh", "pb2u4nt0_ld_outline.hlsl", 0 },
    { "skinmesh8_4_outline.vsh", "pb3u4nt0_ld_outline.hlsl", 0 },
};

static const char* Basename(const char* path)
{
    const char* s = strrchr(path, '\\');
    const char* f = strrchr(path, '/');
    if (f > s)
        s = f;
    return s ? s + 1 : path;
}

static int FileExists(const char* path)
{
    FILE* fp = fopen(path, "rb");
    if (!fp)
        return 0;
    fclose(fp);
    return 1;
}

static const VshMap* FindMap(const char* base)
{
    for (size_t i = 0; i < sizeof(kVshMap) / sizeof(kVshMap[0]); ++i)
    {
        if (_stricmp(base, kVshMap[i].vsh) == 0)
            return &kVshMap[i];
    }
    return 0;
}

static int ResolveHlsl(const char* file, char* out, size_t outsz, const char** extra_macro)
{
    if (!file || !out || outsz < 8)
        return 0;
    *extra_macro = 0;

    size_t n = strlen(file);
    if (n > 5 && _stricmp(file + n - 5, ".hlsl") == 0 && FileExists(file))
    {
        strncpy(out, file, outsz - 1);
        out[outsz - 1] = 0;
        return 1;
    }

    const char* base = Basename(file);
    const VshMap* m = FindMap(base);
    const char* hlsl = m ? m->hlsl : base;
    if (m)
        *extra_macro = m->macro;

    char dir[MAX_PATH];
    strncpy(dir, file, MAX_PATH - 1);
    dir[MAX_PATH - 1] = 0;
    char* slash = strrchr(dir, '\\');
    if (!slash)
        slash = strrchr(dir, '/');
    if (slash)
        slash[1] = 0;
    else
        dir[0] = 0;

    const char* tries[8];
    char a[MAX_PATH], b[MAX_PATH], c[MAX_PATH], d[MAX_PATH], e[MAX_PATH], f[MAX_PATH];
    _snprintf(a, MAX_PATH, "%shlsl\\%s", dir, hlsl);
    _snprintf(b, MAX_PATH, "shader\\hlsl\\%s", hlsl);
    _snprintf(c, MAX_PATH, "shader\\dx9\\hlsl\\%s", hlsl);
    _snprintf(d, MAX_PATH, "..\\helper\\shaders\\hlsl\\%s", hlsl);
    _snprintf(e, MAX_PATH, "..\\..\\helper\\shaders\\hlsl\\%s", hlsl);
    _snprintf(f, MAX_PATH, "helper\\shaders\\hlsl\\%s", hlsl);
    tries[0] = a;
    tries[1] = b;
    tries[2] = c;
    tries[3] = d;
    tries[4] = e;
    tries[5] = f;
    tries[6] = 0;

    for (int i = 0; tries[i]; ++i)
    {
        if (FileExists(tries[i]))
        {
            strncpy(out, tries[i], outsz - 1);
            out[outsz - 1] = 0;
            return 1;
        }
    }
    return 0;
}

class HlslInclude : public ID3DInclude
{
public:
    explicit HlslInclude(const char* shaderFile)
    {
        strncpy(_base, shaderFile, MAX_PATH - 1);
        _base[MAX_PATH - 1] = 0;
        char* last = strrchr(_base, '\\');
        if (!last)
            last = strrchr(_base, '/');
        if (last)
            last[1] = 0;
        else
            _base[0] = 0;
    }

    STDMETHOD(Open)(D3D_INCLUDE_TYPE, LPCSTR pFileName, LPCVOID, LPCVOID* ppData, UINT* pBytes)
    {
        char full[MAX_PATH];
        _snprintf(full, MAX_PATH, "%s%s", _base, pFileName);
        FILE* fp = fopen(full, "rb");
        if (!fp)
            return E_FAIL;
        fseek(fp, 0, SEEK_END);
        long size = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        char* data = new char[size + 1];
        fread(data, 1, size, fp);
        data[size] = 0;
        fclose(fp);
        *ppData = data;
        *pBytes = (UINT)size;
        return S_OK;
    }

    STDMETHOD(Close)(LPCVOID pData)
    {
        delete[] (char*)pData;
        return S_OK;
    }

private:
    char _base[MAX_PATH];
};

static DXGI_FORMAT DeclTypeToDxgi(BYTE type)
{
    switch (type)
    {
    case D3DDECLTYPE_FLOAT1: return DXGI_FORMAT_R32_FLOAT;
    case D3DDECLTYPE_FLOAT2: return DXGI_FORMAT_R32G32_FLOAT;
    case D3DDECLTYPE_FLOAT3: return DXGI_FORMAT_R32G32B32_FLOAT;
    case D3DDECLTYPE_FLOAT4: return DXGI_FORMAT_R32G32B32A32_FLOAT;
    case D3DDECLTYPE_D3DCOLOR: return DXGI_FORMAT_B8G8R8A8_UNORM;
    case D3DDECLTYPE_UBYTE4: return DXGI_FORMAT_R8G8B8A8_UINT;
    case D3DDECLTYPE_SHORT2: return DXGI_FORMAT_R16G16_SINT;
    case D3DDECLTYPE_SHORT4: return DXGI_FORMAT_R16G16B16A16_SINT;
    case D3DDECLTYPE_UBYTE4N: return DXGI_FORMAT_R8G8B8A8_UNORM;
    case D3DDECLTYPE_SHORT2N: return DXGI_FORMAT_R16G16_SNORM;
    case D3DDECLTYPE_SHORT4N: return DXGI_FORMAT_R16G16B16A16_SNORM;
    case D3DDECLTYPE_USHORT2N: return DXGI_FORMAT_R16G16_UNORM;
    case D3DDECLTYPE_USHORT4N: return DXGI_FORMAT_R16G16B16A16_UNORM;
    case D3DDECLTYPE_UDEC3: return DXGI_FORMAT_R10G10B10A2_UINT;
    case D3DDECLTYPE_DEC3N: return DXGI_FORMAT_R10G10B10A2_UNORM;
    case D3DDECLTYPE_FLOAT16_2: return DXGI_FORMAT_R16G16_FLOAT;
    case D3DDECLTYPE_FLOAT16_4: return DXGI_FORMAT_R16G16B16A16_FLOAT;
    default: return DXGI_FORMAT_R32G32B32_FLOAT;
    }
}

static const char* DeclUsageName(BYTE usage)
{
    switch (usage)
    {
    case D3DDECLUSAGE_POSITION: return "POSITION";
    case D3DDECLUSAGE_BLENDWEIGHT: return "BLENDWEIGHT";
    case D3DDECLUSAGE_BLENDINDICES: return "BLENDINDICES";
    case D3DDECLUSAGE_NORMAL: return "NORMAL";
    case D3DDECLUSAGE_PSIZE: return "PSIZE";
    case D3DDECLUSAGE_TEXCOORD: return "TEXCOORD";
    case D3DDECLUSAGE_TANGENT: return "TANGENT";
    case D3DDECLUSAGE_BINORMAL: return "BINORMAL";
    case D3DDECLUSAGE_TESSFACTOR: return "TESSFACTOR";
    case D3DDECLUSAGE_POSITIONT: return "POSITIONT";
    case D3DDECLUSAGE_COLOR: return "COLOR";
    case D3DDECLUSAGE_FOG: return "FOG";
    case D3DDECLUSAGE_DEPTH: return "DEPTH";
    case D3DDECLUSAGE_SAMPLE: return "SAMPLE";
    default: return "POSITION";
    }
}

static ID3D11InputLayout* LayoutFor(lwD3D11VertexDecl* decl, lwD3D11VertexShader* vs)
{
    if (!decl || !vs || !s_sm.device)
        return 0;
    ID3DBlob* blob = (ID3DBlob*)vs->GetBlob();
    if (!blob)
        return 0;

    unsigned long long key = ((unsigned long long)(ULONG_PTR)decl << 1) ^ (unsigned long long)(ULONG_PTR)vs;
    std::map<unsigned long long, ID3D11InputLayout*>::iterator it = s_layouts.find(key);
    if (it != s_layouts.end())
        return it->second;

    const D3DVERTEXELEMENT9* src = decl->GetElements();
    UINT nsrc = decl->GetElementCount();
    D3D11_INPUT_ELEMENT_DESC elems[32] = {};
    UINT n = 0;
    for (UINT i = 0; i < nsrc && n < 32; ++i)
    {
        if (src[i].Stream == 0xff || src[i].Type == D3DDECLTYPE_UNUSED)
            break;
        elems[n].SemanticName = DeclUsageName(src[i].Usage);
        elems[n].SemanticIndex = src[i].UsageIndex;
        elems[n].Format = DeclTypeToDxgi(src[i].Type);
        elems[n].InputSlot = src[i].Stream;
        elems[n].AlignedByteOffset = src[i].Offset;
        elems[n].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
        ++n;
    }
    if (n == 0)
        return 0;

    ID3D11InputLayout* layout = 0;
    HRESULT hr = s_sm.device->CreateInputLayout(
        elems, n, blob->GetBufferPointer(), blob->GetBufferSize(), &layout);
    if (FAILED(hr) || !layout)
    {
        lwD3D11Gap(LW_D3D11_GAP, "sm11-layout", "CreateInputLayout failed hr=0x%08X elems=%u", (unsigned)hr, n);
        return 0;
    }
    s_layouts[key] = layout;
    return layout;
}

void lwD3D11ShaderMgrInit(ID3D11Device* device, ID3D11DeviceContext* context)
{
    lwD3D11ShaderMgrShutdown();
    if (!device || !context)
        return;

    s_sm.device = device;
    s_sm.context = context;

    ID3DBlob* psb = 0;
    ID3DBlob* err = 0;
    HRESULT hr = D3DCompile(kModulatePS, strlen(kModulatePS), "lwShaderMgr11PS", 0, 0, "main", "ps_4_0",
        D3DCOMPILE_OPTIMIZATION_LEVEL1, 0, &psb, &err);
    if (FAILED(hr) || !psb)
    {
        if (err)
        {
            lwD3D11Gap(LW_D3D11_GAP, "sm11-ps", "%s", (const char*)err->GetBufferPointer());
            err->Release();
        }
        return;
    }
    if (err)
        err->Release();
    device->CreatePixelShader(psb->GetBufferPointer(), psb->GetBufferSize(), 0, &s_sm.ps);
    psb->Release();

    D3D11_BUFFER_DESC bd = {};
    bd.ByteWidth = VS_CONST_FLOAT4 * 16;
    bd.Usage = D3D11_USAGE_DYNAMIC;
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    device->CreateBuffer(&bd, 0, &s_sm.vs_cb);

    bd.ByteWidth = 16;
    device->CreateBuffer(&bd, 0, &s_sm.ps_cb);

    s_sm.ready = (s_sm.ps && s_sm.vs_cb && s_sm.ps_cb) ? 1 : 0;
    if (s_sm.ready)
        lwD3D11Gap(LW_D3D11_INVENTORY, "shadermgr11-init", "SM4 modulate PS + %u-reg VS cbuffer", VS_CONST_FLOAT4);
}

void lwD3D11ShaderMgrShutdown()
{
    for (std::map<unsigned long long, ID3D11InputLayout*>::iterator it = s_layouts.begin(); it != s_layouts.end(); ++it)
    {
        if (it->second)
            it->second->Release();
    }
    s_layouts.clear();
    if (s_sm.ps_cb)
        s_sm.ps_cb->Release();
    if (s_sm.vs_cb)
        s_sm.vs_cb->Release();
    if (s_sm.ps)
        s_sm.ps->Release();
    memset(&s_sm, 0, sizeof(s_sm));
}

LW_RESULT lwD3D11CompileVertexShader(const char* file, const D3DXMACRO* defines, IDirect3DVertexShaderX** out_vs)
{
    if (!out_vs)
        return LW_RET_FAILED;
    *out_vs = 0;
    if (!file)
        return LW_RET_FAILED;

    if (!s_sm.ready)
    {
        lwDeviceObject11* d11 = lwGetActiveDeviceObject11();
        if (d11)
            lwD3D11ShaderMgrInit(d11->GetD3D11Device(), d11->GetD3D11Context());
    }
    if (!s_sm.ready || !s_sm.device)
        return LW_RET_FAILED;

    const char* extra_macro = 0;
    char hlsl_path[MAX_PATH];
    if (!ResolveHlsl(file, hlsl_path, MAX_PATH, &extra_macro))
    {
        lwD3D11Gap(LW_D3D11_GAP, "sm11-missing-hlsl", "no SM4 hlsl for %s", file);
        return LW_RET_FAILED;
    }

    D3D_SHADER_MACRO macros[8] = {};
    int mi = 0;
    macros[mi].Name = "SHADER_MODEL_4";
    macros[mi].Definition = "1";
    ++mi;
    if (extra_macro)
    {
        macros[mi].Name = extra_macro;
        macros[mi].Definition = "1";
        ++mi;
    }
    if (defines)
    {
        for (const D3DXMACRO* d = defines; d->Name && mi < 6; ++d, ++mi)
        {
            macros[mi].Name = d->Name;
            macros[mi].Definition = d->Definition ? d->Definition : "1";
        }
    }

    FILE* fp = fopen(hlsl_path, "rb");
    if (!fp)
    {
        lwD3D11Gap(LW_D3D11_GAP, "sm11-open", "cannot read %s", hlsl_path);
        return LW_RET_FAILED;
    }
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char* src = new char[size + 1];
    fread(src, 1, size, fp);
    src[size] = 0;
    fclose(fp);

    HlslInclude include(hlsl_path);
    ID3DBlob* blob = 0;
    ID3DBlob* err = 0;
    HRESULT hr = D3DCompile(src, (SIZE_T)size, hlsl_path, macros, &include, "main", "vs_4_0",
        D3DCOMPILE_OPTIMIZATION_LEVEL1, 0, &blob, &err);
    delete[] src;
    if (FAILED(hr) || !blob)
    {
        if (err)
        {
            lwD3D11Gap(LW_D3D11_GAP, "sm11-compile", "%s — %s", hlsl_path,
                (const char*)err->GetBufferPointer());
            err->Release();
        }
        return LW_RET_FAILED;
    }
    if (err)
        err->Release();

    ID3D11VertexShader* vs = 0;
    hr = s_sm.device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), 0, &vs);
    if (FAILED(hr) || !vs)
    {
        blob->Release();
        lwD3D11Gap(LW_D3D11_GAP, "sm11-create-vs", "CreateVertexShader failed hr=0x%08X %s", (unsigned)hr, hlsl_path);
        return LW_RET_FAILED;
    }

    lwD3D11VertexShader* wrap = new lwD3D11VertexShader(vs, blob);
    *out_vs = wrap;
    lwD3D11Gap(LW_D3D11_INVENTORY, "sm11-vs", "compiled %s", hlsl_path);
    return LW_RET_OK;
}

LW_RESULT lwD3D11CreateVertexDecl(const D3DVERTEXELEMENT9* data, IDirect3DVertexDeclarationX** out_decl)
{
    if (!out_decl || !data)
        return LW_RET_FAILED;
    UINT n = 0;
    const D3DVERTEXELEMENT9* p = data;
    while (p->Stream != 0xff && n < 32)
    {
        ++n;
        ++p;
    }
    ++n; // include the 0xFF terminator
    *out_decl = new lwD3D11VertexDecl(data, n);
    return LW_RET_OK;
}

int lwD3D11ShaderMgrPrepareDraw(lwDeviceObject11* dev, ID3D11InputLayout** out_layout)
{
    if (out_layout)
        *out_layout = 0;
    if (!s_sm.ready || !dev || !s_sm.context)
        return 0;

    lwD3D11VertexShader* vs = lwAsD3D11VertexShader(dev->GetBoundVS());
    if (!vs || !vs->GetVS())
        return 0;

    lwD3D11VertexDecl* decl = lwAsD3D11VertexDecl(dev->GetBoundDecl());
    ID3D11InputLayout* layout = LayoutFor(decl, vs);
    if (!layout)
        return 0;

    D3D11_MAPPED_SUBRESOURCE mapped = {};
    if (SUCCEEDED(s_sm.context->Map(s_sm.vs_cb, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
    {
        memcpy(mapped.pData, dev->GetVSConstants(), VS_CONST_FLOAT4 * 16);
        s_sm.context->Unmap(s_sm.vs_cb, 0);
    }

    float ps_extra[4] = { 0, 0, 0, -1.0f };
    if (lwD3D11MeshIsOutline())
        ps_extra[0] = 1.0f;
    DWORD atest = dev->GetCachedRS(D3DRS_ALPHATESTENABLE);
    if (atest && atest != 0xffffffff)
    {
        DWORD aref = dev->GetCachedRS(D3DRS_ALPHAREF);
        if (aref == 0xffffffff)
            aref = 0;
        DWORD afunc = dev->GetCachedRS(D3DRS_ALPHAFUNC);
        if (afunc == D3DCMP_GREATER || afunc == D3DCMP_GREATEREQUAL || afunc == 0xffffffff)
            ps_extra[3] = (float)(aref & 0xff) / 255.0f;
    }
    if (SUCCEEDED(s_sm.context->Map(s_sm.ps_cb, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
    {
        memcpy(mapped.pData, ps_extra, 16);
        s_sm.context->Unmap(s_sm.ps_cb, 0);
    }

    s_sm.context->VSSetShader(vs->GetVS(), 0, 0);
    s_sm.context->PSSetShader(s_sm.ps, 0, 0);
    s_sm.context->VSSetConstantBuffers(0, 1, &s_sm.vs_cb);
    s_sm.context->PSSetConstantBuffers(1, 1, &s_sm.ps_cb);

    if (out_layout)
        *out_layout = layout;
    static int logged = 0;
    if (!logged)
    {
        logged = 1;
        lwD3D11Gap(LW_D3D11_INVENTORY, "sm11-preparedraw", "bound ShaderMgr11 VS + decl layout");
    }
    return 1;
}

LW_END

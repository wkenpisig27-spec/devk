#include "stdafx.h"
#include "lwShaderMgr.h"
#include "lwInterface.h"
#include "lwShaderDeclMgr.h"
#include "lwShaderMgr11.h"
#include "lwD3D11Gaps.h"
#include "MindPowerRenderConfig.h"

LW_BEGIN

#if (defined LW_USE_DX9)

LW_STD_IMPLEMENTATION(lwShaderMgr9)

lwShaderMgr9::lwShaderMgr9(lwIDeviceObject* dev_obj)
: _dev_obj(dev_obj), _vs_seq(0), _vs_size(0), _vs_num(0),
  _decl_seq(0), _decl_size(0), _decl_num(0), _decl_mgr(0)
{
}

lwShaderMgr9::~lwShaderMgr9()
{
    for (DWORD i = 0; _vs_num > 0; i++)
    {
        if (_vs_seq[i].handle)
        {
            LW_DELETE_A(_vs_seq[i].data);
            LW_RELEASE(_vs_seq[i].handle);
            _vs_num -= 1;
        }
    }

    for (DWORD i = 0; _decl_num > 0; i++)
    {
        if (_decl_seq[i].handle)
        {
            LW_DELETE_A(_decl_seq[i].data);
            LW_RELEASE(_decl_seq[i].handle);
            _decl_num -= 1;
        }
    }

    LW_IF_RELEASE(_decl_mgr);
}

LW_RESULT lwShaderMgr9::Init(DWORD vs_buf_size, DWORD decl_buf_size, DWORD ps_buf_size)
{
    (void)ps_buf_size;
    _vs_num = 0;
    _vs_size = vs_buf_size;
    _vs_seq = LW_NEW(lwVertexShaderInfo[_vs_size]);
    memset(_vs_seq, 0, sizeof(lwVertexShaderInfo) * _vs_size);

    _decl_num = 0;
    _decl_size = decl_buf_size;
    _decl_seq = LW_NEW(lwVertDeclInfo9[_decl_size]);
    memset(_decl_seq, 0, sizeof(lwVertDeclInfo9) * _decl_size);

    _decl_mgr = LW_NEW(lwShaderDeclMgr(this));
    return LW_RET_OK;
}

LW_RESULT lwShaderMgr9::RegisterVertexShader(DWORD type, BYTE* data, DWORD size)
{
    (void)type;
    (void)data;
    (void)size;
    lwD3D11Gap(LW_D3D11_SKIP, "shadermgr-create-vs-bytecode",
        "DX11 RegisterVertexShader(bytecode) is unused; LoadShader registers .hlsl files");
    return LW_RET_FAILED;
}

LW_RESULT lwShaderMgr9::RegisterVertexShader(DWORD type, const char* file, DWORD file_flag, const D3DXMACRO* defines)
{
    (void)file_flag;
    if (type >= _vs_size || !file)
        return LW_RET_FAILED;
    if (_vs_seq[type].handle)
        return LW_RET_FAILED;

    IDirect3DVertexShaderX* handle = 0;
    if (LW_FAILED(lwD3D11CompileVertexShader(file, defines, &handle)) || !handle)
        return LW_RET_FAILED;

    _vs_seq[type].handle = handle;
    _vs_seq[type].data = 0;
    _vs_seq[type].size = 0;
    _vs_num += 1;
    return LW_RET_OK;
}

LW_RESULT lwShaderMgr9::RegisterVertexDeclaration(DWORD type, D3DVERTEXELEMENT9* data)
{
    if (type >= _decl_size || !data)
        return LW_RET_FAILED;
    if (_decl_seq[type].handle)
        return LW_RET_FAILED;

    IDirect3DVertexDeclarationX* handle = 0;
    if (LW_FAILED(lwD3D11CreateVertexDecl(data, &handle)) || !handle)
        return LW_RET_FAILED;

    int n = 0;
    D3DVERTEXELEMENT9* p = data;
    while (p->Stream != 0xFF)
    {
        ++n;
        ++p;
    }
    ++n;

    _decl_seq[type].handle = handle;
    _decl_seq[type].data = LW_NEW(D3DVERTEXELEMENT9[n]);
    memcpy(_decl_seq[type].data, data, sizeof(D3DVERTEXELEMENT9) * n);
    _decl_num += 1;
    return LW_RET_OK;
}

LW_RESULT lwShaderMgr9::LoseDevice()
{
    return LW_RET_OK;
}

LW_RESULT lwShaderMgr9::ResetDevice()
{
    return LW_RET_OK;
}

LW_RESULT lwShaderMgr9::QueryVertexShader(IDirect3DVertexShaderX** ret_obj, DWORD type)
{
    if (!ret_obj || type >= _vs_size || !_vs_seq[type].handle)
        return LW_RET_FAILED;
    *ret_obj = _vs_seq[type].handle;
    return LW_RET_OK;
}

LW_RESULT lwShaderMgr9::QueryVertexDeclaration(IDirect3DVertexDeclarationX** ret_obj, DWORD type)
{
    if (!ret_obj || type >= _decl_size || !_decl_seq[type].handle)
        return LW_RET_FAILED;
    *ret_obj = _decl_seq[type].handle;
    return LW_RET_OK;
}

#endif

LW_END

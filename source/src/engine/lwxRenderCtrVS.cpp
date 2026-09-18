//

#include "stdafx.h"
#include "lwxRenderCtrlVS.h"
#include "MindPowerRenderConfig.h"
#if MINDPOWER_USE_D3D9_DEVICE
#include "lwDeviceObject.h"
#endif
#include "lwRenderBackend.h"
#include "lwD3D11Mesh.h"
#include "lwD3D11Gaps.h"
#include "ShaderLoad.h"

LW_BEGIN

static void Dx11ResetTexTransform(lwIDeviceObject* dev_obj)
{
    if (!dev_obj)
        return;
    lwMatrix44 tex_id;
    lwMatrix44Identity(&tex_id);
    for (DWORD s = 0; s < 2; ++s)
    {
        dev_obj->SetTransform((D3DTRANSFORMSTATETYPE)(D3DTS_TEXTURE0 + s), &tex_id);
        dev_obj->SetTextureStageState(s, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
    }
}

static int Dx11BindShaderMgrVS(lwIDeviceObject* dev_obj, lwIResourceMgr* res_mgr, lwIRenderCtrlAgent* agent);

static void Dx11ResetInheritedTexState(lwIDeviceObject* dev_obj)
{
    Dx11ResetTexTransform(dev_obj);
    if (!dev_obj)
        return;
    dev_obj->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
    dev_obj->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
}

static void Dx11ApplyVertexBlend(lwIDeviceObject* dev_obj, lwIRenderCtrlAgent* agent)
{
    lwMatrix44* mat_global = agent ? agent->GetGlobalMatrix() : 0;
    if (mat_global)
        dev_obj->SetTransformWorld(mat_global);

    Dx11ResetInheritedTexState(dev_obj);

    DWORD bone_num = 0;
    const lwMatrix44* rtmat = 0;
    lwIAnimCtrlAgent* bones_agent = agent ? agent->GetAnimCtrlAgent() : 0;
    if (bones_agent)
    {
        DWORD n = bones_agent->GetAnimCtrlObjNum();
        for (DWORD i = 0; i < n; ++i)
        {
            lwIAnimCtrlObj* obj = bones_agent->GetAnimCtrlObj(i);
            if (!obj)
                continue;
            lwAnimCtrlObjTypeInfo info;
            obj->GetTypeInfo(&info);
            if (info.type != ANIM_CTRL_TYPE_BONE)
                continue;
            lwIAnimCtrlObjBone* bone_ctrl = (lwIAnimCtrlObjBone*)obj;
            bone_num = bone_ctrl->GetBoneRTTMNum();
            rtmat = (const lwMatrix44*)bone_ctrl->GetBoneRTMSeq();
            break;
        }
    }
    if (bone_num && rtmat)
        lwD3D11MeshSetBonePalette(rtmat, bone_num);
    else
        lwD3D11MeshSetBonePalette(0, 0);

    lwIResourceMgr* res_mgr = agent ? agent->GetResourceMgr() : 0;
    Dx11BindShaderMgrVS(dev_obj, res_mgr, agent);

    static int logged = 0;
    if (!logged)
    {
        logged = 1;
        lwD3D11Gap(LW_D3D11_INVENTORY, "char-vertexblend",
            "world=(%.2f,%.2f,%.2f) bones=%u",
            mat_global ? mat_global->_41 : 0.f,
            mat_global ? mat_global->_42 : 0.f,
            mat_global ? mat_global->_43 : 0.f,
            bone_num);
    }
}

static int Dx11QueryVS(lwIResourceMgr* res_mgr, lwIRenderCtrlAgent* agent,
    IDirect3DVertexShaderX** vs, IDirect3DVertexDeclarationX** decl)
{
    if (vs)
        *vs = 0;
    if (decl)
        *decl = 0;
    if (!res_mgr || !agent)
        return 0;
    lwIShaderMgr* sm = res_mgr->GetShaderMgr();
    if (!sm)
        return 0;
    IDirect3DVertexShaderX* v = 0;
    if (LW_FAILED(sm->QueryVertexShader(&v, agent->GetVertexShader())) || !v)
        return 0;
    if (vs)
        *vs = v;
    if (decl)
        sm->QueryVertexDeclaration(decl, agent->GetVertexDeclaration());
    return 1;
}

static void Dx11UploadVsBlendConstants(lwIDeviceObject* dev_obj, lwIRenderCtrlAgent* agent)
{
    if (!dev_obj || !agent)
        return;

    lwMatrix44* mat_global = agent->GetGlobalMatrix();
    if (!mat_global)
        return;

    lwVector4 const_base(1.0f, 0.0f, 0.0f, 765.01f);
    lwVector4 light_dir(0.0f, 0.0f, 0.0f, 0.0f);

    const lwMatrix44* pViewProj = dev_obj->GetMatViewProj();
    if (pViewProj)
    {
        lwMatrix44 mat = *pViewProj;
        lwMatrix44Multiply(&mat, mat_global, &mat);
        lwMatrix44Transpose(&mat, &mat);
        dev_obj->SetVertexShaderConstantF(VS_CONST_REG_VIEWPROJ, (float*)&mat, 4);
    }

    {
        lwMatrix44 uv_id;
        lwMatrix44Identity(&uv_id);
        lwMatrix44Transpose(&uv_id, &uv_id);
        dev_obj->SetVertexShaderConstantF(VS_CONST_REG_TS0_UVMAT, (float*)&uv_id, 4);
        dev_obj->SetVertexShaderConstantF(VS_CONST_REG_TS1_UVMAT, (float*)&uv_id, 4);
        dev_obj->SetVertexShaderConstantF(VS_CONST_REG_TS2_UVMAT, (float*)&uv_id, 4);
    }

    DWORD rs_amb = 0;
    DWORD rs_lgt = 0;
    BOOL lgt_enable = FALSE;
    D3DLIGHTX lgt;
    memset(&lgt, 0, sizeof(lgt));
    dev_obj->GetRenderState(D3DRS_AMBIENT, &rs_amb);
    dev_obj->GetLight(0, &lgt);
    dev_obj->GetRenderState(D3DRS_LIGHTING, &rs_lgt);
    dev_obj->GetLightEnable(0, &lgt_enable);

    if (rs_lgt && lgt_enable && lgt.Type == D3DLIGHT_DIRECTIONAL)
    {
        *(lwVector3*)&light_dir = *(lwVector3*)&lgt.Direction;
        light_dir.x = -light_dir.x;
        light_dir.y = -light_dir.y;
        light_dir.z = -light_dir.z;
        lwMatrix44 mat_light;
        lwMatrix44InverseNoScaleFactor(&mat_light, mat_global);
        lwVec3Mat44MulNormal((lwVector3*)&light_dir, &mat_light);
    }

    dev_obj->SetVertexShaderConstantF(VS_CONST_REG_BASE, (float*)&const_base, 1);
    dev_obj->SetVertexShaderConstantF(VS_CONST_REG_LIGHT_DIR, (float*)&light_dir, 1);

    const lwMatrix44* mat_view = dev_obj->GetMatView();
    if (mat_view)
    {
        lwMatrix44 inv_view;
        lwMatrix44InverseNoScaleFactor(&inv_view, mat_view);
        lwVector3 eye_os(inv_view._41, inv_view._42, inv_view._43);
        lwMatrix44 inv_world;
        lwMatrix44InverseNoScaleFactor(&inv_world, mat_global);
        lwVec3Mat44Mul(&eye_os, &inv_world);
        lwVector4 eye4(eye_os.x, eye_os.y, eye_os.z, 1.0f);
        dev_obj->SetVertexShaderConstantF(VS_CONST_REG_EYE_POS, (float*)&eye4, 1);
    }

    lwIAnimCtrlAgent* anim_agent = agent->GetAnimCtrlAgent();
    static float pal[50 * 12];
    for (DWORD bi = 0; bi < 50; ++bi)
    {
        float* r = &pal[bi * 12];
        r[0] = 1; r[1] = 0; r[2] = 0; r[3] = 0;
        r[4] = 0; r[5] = 1; r[6] = 0; r[7] = 0;
        r[8] = 0; r[9] = 0; r[10] = 1; r[11] = 0;
    }
    if (anim_agent)
    {
        DWORD animobj_num = anim_agent->GetAnimCtrlObjNum();
        for (DWORD idx = 0; idx < animobj_num; ++idx)
        {
            lwIAnimCtrlObj* animctrl_obj = anim_agent->GetAnimCtrlObj(idx);
            if (!animctrl_obj)
                continue;
            lwAnimCtrlObjTypeInfo type_info;
            animctrl_obj->GetTypeInfo(&type_info);
            if (type_info.type != ANIM_CTRL_TYPE_BONE)
                continue;
            lwIAnimCtrlObjBone* bone_ctrl = (lwIAnimCtrlObjBone*)animctrl_obj;
            DWORD bone_num = bone_ctrl->GetBoneRTTMNum();
            const lwMatrix44* rtmat = (const lwMatrix44*)bone_ctrl->GetBoneRTMSeq();
            if (bone_num && rtmat)
            {
                if (bone_num > 50)
                    bone_num = 50;
                for (DWORD bi = 0; bi < bone_num; ++bi)
                    lwMatrix44Transpose((lwMatrix44*)&pal[bi * 12], &rtmat[bi]);
            }
            break;
        }
    }
    dev_obj->SetVertexShaderConstantF(VS_CONST_REG_MAT_PALETTE, pal, 50 * 3);
}

static void Dx11UploadVsSubsetConstants(lwIDeviceObject* dev_obj, lwIRenderCtrlAgent* agent, DWORD subset,
    const lwMaterial* mtl)
{
    if (!dev_obj || !mtl)
        return;

    DWORD rs_amb_d = 0;
    DWORD rs_lgt = 0;
    BOOL lgt_enable = FALSE;
    D3DLIGHTX lgt;
    memset(&lgt, 0, sizeof(lgt));
    dev_obj->GetRenderState(D3DRS_AMBIENT, &rs_amb_d);
    dev_obj->GetLight(0, &lgt);
    dev_obj->GetRenderState(D3DRS_LIGHTING, &rs_lgt);
    dev_obj->GetLightEnable(0, &lgt_enable);

    lwColorValue4f rs_amb;
    rs_amb.a = LW_ARGB_A(rs_amb_d);
    rs_amb.r = LW_ARGB_R(rs_amb_d);
    rs_amb.g = LW_ARGB_G(rs_amb_d);
    rs_amb.b = LW_ARGB_B(rs_amb_d);

    lwColorValue4f amb_dif[2];
    lwColorValue4f* c;
    if (rs_lgt && lgt_enable && lgt.Type == D3DLIGHT_DIRECTIONAL)
    {
        c = &amb_dif[0];
        c->r = (lgt.Ambient.r + rs_amb.r) * mtl->amb.r;
        c->g = (lgt.Ambient.g + rs_amb.g) * mtl->amb.g;
        c->b = (lgt.Ambient.b + rs_amb.b) * mtl->amb.b;
        c->a = (lgt.Ambient.a + rs_amb.a) * mtl->amb.a;
        c = &amb_dif[1];
        c->r = lgt.Diffuse.r * mtl->dif.r;
        c->g = lgt.Diffuse.g * mtl->dif.g;
        c->b = lgt.Diffuse.b * mtl->dif.b;
        c->a = lgt.Diffuse.a * mtl->dif.a;
    }
    else
    {
        c = &amb_dif[0];
        c->r = rs_amb.r * mtl->amb.r;
        c->g = rs_amb.g * mtl->amb.g;
        c->b = rs_amb.b * mtl->amb.b;
        c->a = rs_amb.a * mtl->amb.a;
        c = &amb_dif[1];
        c->r = c->g = c->b = c->a = 0.0f;
    }
    dev_obj->SetVertexShaderConstantF(VS_CONST_REG_LIGHT_AMB, (float*)&amb_dif, 2);

    if (lwD3D11MeshIsOutline())
        lwApplyOutlineVSConstants(dev_obj);

    if (!agent)
        return;
    lwIAnimCtrlAgent* anim_agent = agent->GetAnimCtrlAgent();
    if (!anim_agent)
        return;

    DWORD stage_tab[3] = {
        VS_CONST_REG_TS0_UVMAT,
        VS_CONST_REG_TS1_UVMAT,
        VS_CONST_REG_TS2_UVMAT,
    };
    DWORD animobj_num = anim_agent->GetAnimCtrlObjNum();
    for (DWORD i = 0; i < animobj_num; ++i)
    {
        lwIAnimCtrlObj* animctrl_obj = anim_agent->GetAnimCtrlObj(i);
        if (!animctrl_obj)
            continue;
        lwAnimCtrlObjTypeInfo type_info;
        animctrl_obj->GetTypeInfo(&type_info);
        if (type_info.data[1] >= 3)
            continue;
        if ((type_info.data[0] == subset) && (type_info.type == ANIM_CTRL_TYPE_TEXUV))
        {
            lwIAnimCtrlObjTexUV* texuv_ctrl = (lwIAnimCtrlObjTexUV*)animctrl_obj;
            DWORD stage_id = stage_tab[type_info.data[1]];
            lwMatrix44 mat, mat_src;
            texuv_ctrl->GetRTM(&mat_src);
            lwMatrix44Transpose(&mat, &mat_src);
            dev_obj->SetVertexShaderConstantF(stage_id, (float*)&mat, 4);
        }
    }
}

static int Dx11BindShaderMgrVS(lwIDeviceObject* dev_obj, lwIResourceMgr* res_mgr, lwIRenderCtrlAgent* agent)
{
    IDirect3DVertexShaderX* vs = 0;
    IDirect3DVertexDeclarationX* decl = 0;
    if (!Dx11QueryVS(res_mgr, agent, &vs, &decl) || !vs)
        return 0;
    if (decl)
        dev_obj->SetVertexDeclarationForced(decl);
    dev_obj->SetVertexShader(vs);
    Dx11UploadVsBlendConstants(dev_obj, agent);
    return 1;
}

lwIRenderCtrlVS* __RenderCtrlVSProcVSVertexBlend_dx8()
{
    return LW_NEW(lwxRenderCtrlVSVertexBlend_dx8);
}
#if(defined LW_USE_DX9)
lwIRenderCtrlVS* __RenderCtrlVSProcVSVertexBlend_dx9()
{
    return LW_NEW(lwxRenderCtrlVSVertexBlend);
}
#endif

LW_RESULT lwInitUserRenderCtrlVSProc(lwIResourceMgr* mgr)
{
    mgr->RegisterRenderCtrlProc(RENDERCTRL_VS_VERTEXBLEND, __RenderCtrlVSProcVSVertexBlend_dx8);
#if(defined LW_USE_DX9)
    mgr->RegisterRenderCtrlProc(RENDERCTRL_VS_VERTEXBLEND_DX9, __RenderCtrlVSProcVSVertexBlend_dx9);
#endif
    return LW_RET_OK;
}

// lwxRenderCtrlVSVertexBlend_dx8
LW_STD_IMPLEMENTATION(lwxRenderCtrlVSVertexBlend_dx8);

lwxRenderCtrlVSVertexBlend_dx8::lwxRenderCtrlVSVertexBlend_dx8()
: mPixelShaderReload( false ),
  mPixelShader( -1 )
{
}


LW_RESULT lwxRenderCtrlVSVertexBlend_dx8::Clone(lwIRenderCtrlVS** obj)
{
    this_type* o = LW_NEW(this_type);
    *o = *this;

    *obj = o;

    return LW_RET_OK;
}

LW_RESULT lwxRenderCtrlVSVertexBlend_dx8::Initialize(lwIRenderCtrlAgent* agent)
{
    return LW_RET_OK;
}

LW_RESULT lwxRenderCtrlVSVertexBlend_dx8::BeginSet(lwIRenderCtrlAgent* agent)
{
    LW_RESULT ret = LW_RET_FAILED;

    // ===== declara��es antecipadas (evita warnings com goto) =====
    lwIResourceMgr*              res_mgr   = 0;
    lwIDeviceObject*             dev_obj   = 0;
    lwMatrix44*                  mat_global= 0;
    const lwMatrix44*            pViewProj = 0;
    lwMatrix44                   mat;
    lwVector4                    const_base(1.0f, 0.0f, 0.0f, 765.01f);
    lwVector4                    light_dir(0.0f, 0.0f, 0.0f, 0.0f);
    DWORD                        rs_amb = 0;

    lwIAnimCtrlAgent*            anim_agent = 0;
    DWORD                        animobj_num = 0;
    lwIAnimCtrlObj*              animctrl_obj = 0;
    lwAnimCtrlObjTypeInfo        type_info;

    // DX9: deixar declaradas antes dos gotos
#if defined(LW_USE_DX9)
    IDirect3DVertexShaderX*      vs   = 0;
    IDirect3DVertexDeclarationX* decl = 0;
#endif

    // DX8: id do VS
#if defined(LW_USE_DX8)
    DWORD                        vs_id = 0;
#endif

    // ===== valida��es b�sicas =====
    if (!agent)
        goto __ret;

    res_mgr = agent->GetResourceMgr();
    if (!res_mgr)
        goto __ret;

    dev_obj = res_mgr->GetDeviceObject();
    if (!dev_obj)
        goto __ret;

    // ===== FOG: salvar e desabilitar =====
    dev_obj->GetRenderState(D3DRS_FOGENABLE, &_rs_fog);
    if (_rs_fog)
        dev_obj->SetRenderState(D3DRS_FOGENABLE, FALSE);

    // ===== matrizes =====
    mat_global = agent->GetGlobalMatrix();
    if (!mat_global)
        goto __ret;

    if (lwIsDx11Active())
    {
        Dx11ApplyVertexBlend(dev_obj, agent);
        ret = LW_RET_OK;
        goto __ret;
    }

    pViewProj = dev_obj->GetMatViewProj();
    if (!pViewProj)
        goto __ret;

    mat = *pViewProj;
    lwMatrix44Multiply(&mat, mat_global, &mat);
    lwMatrix44Transpose(&mat, &mat);

    // ===== ambiente / luz =====
    dev_obj->GetRenderState(D3DRS_AMBIENT, &rs_amb);
    _rs_amb.a = LW_ARGB_A(rs_amb);
    _rs_amb.r = LW_ARGB_R(rs_amb);
    _rs_amb.g = LW_ARGB_G(rs_amb);
    _rs_amb.b = LW_ARGB_B(rs_amb);

    dev_obj->GetLight(0, &_lgt);
    dev_obj->GetRenderState(D3DRS_LIGHTING, &_rs_lgt);
    dev_obj->GetLightEnable(0, &_lgt_enable);

    if (_rs_lgt && _lgt_enable && _lgt.Type == D3DLIGHT_DIRECTIONAL)
    {
        *(lwVector3*)&light_dir = *(lwVector3*)&_lgt.Direction;
        light_dir.x = -light_dir.x;
        light_dir.y = -light_dir.y;
        light_dir.z = -light_dir.z;

        lwMatrix44 mat_light;
        lwMatrix44InverseNoScaleFactor(&mat_light, mat_global);
        lwVec3Mat44MulNormal((lwVector3*)&light_dir, &mat_light);
    }

    // ===== constantes de VS =====
    dev_obj->SetVertexShaderConstantF(VS_CONST_REG_BASE,      (float*)&const_base, 1);
    dev_obj->SetVertexShaderConstantF(VS_CONST_REG_VIEWPROJ,  (float*)&mat,        4);
    dev_obj->SetVertexShaderConstantF(VS_CONST_REG_LIGHT_DIR, (float*)&light_dir,  1);

    // ===== camera position in object space (rim / toon-spec) =====
    // World-space eye = inverse(view).translation. Then transform by inverse(world)
    // to bring it into the same space as input.Position / skinnedPos in the VS.
    {
        const lwMatrix44* mat_view = dev_obj->GetMatView();
        if (mat_view)
        {
            lwMatrix44 inv_view;
            lwMatrix44InverseNoScaleFactor(&inv_view, mat_view);
            lwVector3 eye_os(inv_view._41, inv_view._42, inv_view._43);

            lwMatrix44 inv_world;
            lwMatrix44InverseNoScaleFactor(&inv_world, mat_global);
            lwVec3Mat44Mul(&eye_os, &inv_world);

            lwVector4 eye4(eye_os.x, eye_os.y, eye_os.z, 1.0f);
            dev_obj->SetVertexShaderConstantF(VS_CONST_REG_EYE_POS, (float*)&eye4, 1);
        }
    }

    // ===== paleta de ossos =====
    anim_agent = agent->GetAnimCtrlAgent();
    if (!anim_agent)
        goto __ret;

    animobj_num = anim_agent->GetAnimCtrlObjNum();

    {
        DWORD idx;
        for (idx = 0; idx < animobj_num; ++idx)
        {
            animctrl_obj = anim_agent->GetAnimCtrlObj(idx);
            if (!animctrl_obj)
                continue;

            animctrl_obj->GetTypeInfo(&type_info);
            if (type_info.type == ANIM_CTRL_TYPE_BONE)
            {
                lwIAnimCtrlObjBone* bone_ctrl = (lwIAnimCtrlObjBone*)animctrl_obj;
                DWORD bone_num = bone_ctrl->GetBoneRTTMNum();
                const lwMatrix44* rtmat = (const lwMatrix44*)bone_ctrl->GetBoneRTMSeq();

                if (lwIsDx11Active())
                {
                    if (bone_num && rtmat)
                        lwD3D11MeshSetBonePalette(rtmat, bone_num);
                    break;
                }

                if (bone_num == 0 || !rtmat)
                    goto __ret;

                // buffer est�tico: 50 ossos * (3 registros * 4 floats) = 600 floats
                static float __this_buf[50 * 12];
                if (bone_num > 50)
                    bone_num = 50; // evita overflow, mantido simples

                DWORD bi;
                for (bi = 0; bi < bone_num; ++bi)
                {
                    lwMatrix44Transpose((lwMatrix44*)&__this_buf[bi * 12], &rtmat[bi]);
                }

                dev_obj->SetVertexShaderConstantF(VS_CONST_REG_MAT_PALETTE, __this_buf, bone_num * 3);
                break;
            }
        }
    }

    // ===== pixel shader (bloco adicionado, mantido pr�ximo ao original) =====
#if MINDPOWER_USE_D3D9_DEVICE
    if (!mPixelShaderName.empty())
    {
        IDirect3DDeviceX* device = dev_obj->GetDevice();
        if (device)
        {
            if (mPixelShaderReload)
            {
                // opcional: liberar anterior para evitar leak ao recarregar
                if (mPixelShader)
                {
                    device->SetPixelShader(0);
                    ((IDirect3DPixelShaderX*)mPixelShader)->Release();
                    mPixelShader = 0;
                }

                LPD3DXBUFFER codeBuffer  = 0;
                LPD3DXBUFFER errorBuffer = 0;
                HRESULT hr = D3DXAssembleShaderFromFile(
                    mPixelShaderName.c_str(), NULL, NULL, 0, &codeBuffer, &errorBuffer);

                if (SUCCEEDED(hr) && codeBuffer)
                {
                    hr = device->CreatePixelShader(
                        (DWORD*)codeBuffer->GetBufferPointer(),
                        (IDirect3DPixelShaderX**)&mPixelShader);
                }
                else
                {
                    mPixelShader = 0;
                    if (errorBuffer)
                    {
                        const char* str = (const char*)errorBuffer->GetBufferPointer();
                        MessageBox(0, str, "D3DXAssembleShaderFromFile Failed", 0);
                    }
                }

                if (errorBuffer) { errorBuffer->Release(); errorBuffer = 0; }
                if (codeBuffer)  { codeBuffer->Release();  codeBuffer  = 0; }
                mPixelShaderReload = false;
            }

            // Don't activate pixel shader during shadow pass - we need the
            // fixed-function TFACTOR pipeline to force black output
            lwDeviceObject* pDevObjShadow = static_cast<lwDeviceObject*>(dev_obj);
            if (!pDevObjShadow->IsShadowPassMode())
                device->SetPixelShader((IDirect3DPixelShaderX*)mPixelShader);
        }
    }
#endif

    // ===== vertex shader =====
    {
        lwIShaderMgr* shader_mgr = res_mgr->GetShaderMgr();
        if (!shader_mgr)
            goto __ret;

    #if defined(LW_USE_DX8)
        if (LW_FAILED(shader_mgr->QueryVertexShader(&vs_id, agent->GetVertexShader())))
            goto __ret;

        dev_obj->SetVertexShader(vs_id);
    #endif

    #if defined(LW_USE_DX9)
        if (!lwIsDx11Active())
        {
            if (LW_FAILED(shader_mgr->QueryVertexShader(&vs, agent->GetVertexShader())))
                goto __ret;

            if (LW_FAILED(shader_mgr->QueryVertexDeclaration(&decl, agent->GetVertexDeclaration())))
                goto __ret;

            dev_obj->SetVertexDeclarationForced(decl);
            dev_obj->SetVertexShader(vs);
        }
    #endif
    }

    ret = LW_RET_OK;

__ret:
    // restaura fog se t�nhamos desligado e falhou
    if (LW_FAILED(ret) && _rs_fog)
        dev_obj->SetRenderState(D3DRS_FOGENABLE, TRUE);

    return ret;
}
LW_RESULT lwxRenderCtrlVSVertexBlend_dx8::EndSet(lwIRenderCtrlAgent* agent)
{
    lwIResourceMgr* res_mgr = agent->GetResourceMgr();
    lwIDeviceObject* dev_obj = res_mgr->GetDeviceObject();

    if(_rs_fog)
    {
        dev_obj->SetRenderState(D3DRS_FOGENABLE, TRUE);
    }

#if MINDPOWER_USE_D3D9_DEVICE
	if (IDirect3DDeviceX* end_dev = dev_obj->GetDevice())
		end_dev->SetPixelShader(0);
#endif

#if(defined LW_USE_DX9)
    dev_obj->SetVertexShader(NULL);
    dev_obj->SetVertexDeclaration(NULL);
#endif
    return LW_RET_OK;
}
LW_RESULT lwxRenderCtrlVSVertexBlend_dx8::BeginSetSubset(DWORD subset, lwIRenderCtrlAgent* agent)
{
    lwIResourceMgr* res_mgr = agent->GetResourceMgr();
    lwIDeviceObject* dev_obj = res_mgr->GetDeviceObject();
    lwIMtlTexAgent* mtltex_agent = agent->GetMtlTexAgent();
    if (!mtltex_agent)
        return LW_RET_FAILED;

    lwMaterial* mtl = mtltex_agent->GetMaterial();

    if (lwIsDx11Active())
    {
        if (mtl)
            dev_obj->SetMaterial(mtl);
        Dx11ResetTexTransform(dev_obj);
        lwIAnimCtrlAgent* anim_agent = agent->GetAnimCtrlAgent();
        if (anim_agent)
        {
            DWORD animobj_num = anim_agent->GetAnimCtrlObjNum();
            for (DWORD i = 0; i < animobj_num; ++i)
            {
                lwIAnimCtrlObj* animctrl_obj = anim_agent->GetAnimCtrlObj(i);
                if (!animctrl_obj)
                    continue;
                lwAnimCtrlObjTypeInfo type_info;
                animctrl_obj->GetTypeInfo(&type_info);
                if (type_info.data[0] != subset || type_info.type != ANIM_CTRL_TYPE_TEXUV)
                    continue;
                if (type_info.data[1] >= 3)
                    continue;
                lwIAnimCtrlObjTexUV* texuv_ctrl = (lwIAnimCtrlObjTexUV*)animctrl_obj;
                lwMatrix44 mat;
                texuv_ctrl->GetRTM(&mat);
                DWORD stage = type_info.data[1];
                dev_obj->SetTransform((D3DTRANSFORMSTATETYPE)(D3DTS_TEXTURE0 + stage), &mat);
                dev_obj->SetTextureStageState(stage, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2);
            }
        }
        if (mtl)
            Dx11UploadVsSubsetConstants(dev_obj, agent, subset, mtl);
        return LW_RET_OK;
    }

    lwColorValue4f amb_dif[2];
    lwColorValue4f* c;
    amb_dif[0];

    if(_rs_lgt && _lgt_enable && _lgt.Type == D3DLIGHT_DIRECTIONAL)
    {
        c = &amb_dif[0];
        c->r = (_lgt.Ambient.r + _rs_amb.r) * mtl->amb.r;
        c->g = (_lgt.Ambient.g + _rs_amb.g) * mtl->amb.g;
        c->b = (_lgt.Ambient.b + _rs_amb.b) * mtl->amb.b;
        c->a = (_lgt.Ambient.a + _rs_amb.a) * mtl->amb.a;
            
        c = &amb_dif[1];
        c->r = _lgt.Diffuse.r * mtl->dif.r;
        c->g = _lgt.Diffuse.g * mtl->dif.g;
        c->b = _lgt.Diffuse.b * mtl->dif.b;
        c->a = _lgt.Diffuse.a * mtl->dif.a;
    }
    else
    {
        c = &amb_dif[0];
        c->r = _rs_amb.r * mtl->amb.r;
        c->g = _rs_amb.g * mtl->amb.g;
        c->b = _rs_amb.b * mtl->amb.b;
        c->a = _rs_amb.a * mtl->amb.a;

        c = &amb_dif[1];
        c->r = c->g = c->b = c->a = 0.0f;
    }
    
    //c = &amb_dif[0];
    //if(c->r > 1.0f)
    //    c->r = 1.0f;
    //if(c->g > 1.0f)
    //    c->g = 1.0f;
    //if(c->b > 1.0f)
    //    c->b = 1.0f;
    //if(c->a > 1.0f)
    //    c->a = 1.0f;

    dev_obj->SetVertexShaderConstantF(VS_CONST_REG_LIGHT_AMB, (float*)&amb_dif, 2);

    // set texture uv data
    DWORD stage_tab[3] =
    {
        VS_CONST_REG_TS0_UVMAT,
        VS_CONST_REG_TS1_UVMAT,
        VS_CONST_REG_TS2_UVMAT,
    };

    lwIAnimCtrlAgent* anim_agent = agent->GetAnimCtrlAgent();
    DWORD animobj_num = anim_agent->GetAnimCtrlObjNum();
    lwIAnimCtrlObj* animctrl_obj;
    lwAnimCtrlObjTypeInfo type_info;

    for(DWORD i = 0; i < animobj_num; i++)
    {
        animctrl_obj = anim_agent->GetAnimCtrlObj(i);
        animctrl_obj->GetTypeInfo(&type_info);

        if((type_info.data[0] == subset) && (type_info.type == ANIM_CTRL_TYPE_TEXUV))
        {
            lwIAnimCtrlObjTexUV* texuv_ctrl = (lwIAnimCtrlObjTexUV*)animctrl_obj;
            DWORD stage_id = stage_tab[type_info.data[1]];
            lwMatrix44 mat, mat_src;
            texuv_ctrl->GetRTM(&mat_src);
            lwMatrix44Transpose(&mat, &mat_src);

            dev_obj->SetVertexShaderConstantF(stage_id, (float*)&mat, 4);
        }
    }

    return LW_RET_OK;
}
LW_RESULT lwxRenderCtrlVSVertexBlend_dx8::EndSetSubset(DWORD subset, lwIRenderCtrlAgent* agent)
{
    return LW_RET_OK;
}


#if(defined LW_USE_DX9)

// lwxRenderCtrlVSVertexBlend
LW_STD_IMPLEMENTATION(lwxRenderCtrlVSVertexBlend);


LW_RESULT lwxRenderCtrlVSVertexBlend::Clone(lwIRenderCtrlVS** obj)
{
    this_type* o = LW_NEW(this_type);
    *o = *this;

    *obj = o;

    return LW_RET_OK;
}
LW_RESULT lwxRenderCtrlVSVertexBlend::Initialize(lwIRenderCtrlAgent* agent)
{
    _const_tab = 0;
    if (lwIsDx11Active())
        return LW_RET_OK;

    LW_RESULT ret = LW_RET_FAILED;

    lwIResourceMgr* res_mgr = agent->GetResourceMgr();
    lwIShaderMgr* shader_mgr = res_mgr->GetShaderMgr();

    lwVertexShaderInfo* vs_info = shader_mgr->GetVertexShaderInfo(agent->GetVertexShader());
    if(!vs_info || !vs_info->data)
        goto __ret;
    if(FAILED(D3DXGetShaderConstantTable((DWORD*)vs_info->data, &_const_tab)))
        goto __ret;

    ret = LW_RET_OK;
__ret:
    return ret;
}
LW_RESULT lwxRenderCtrlVSVertexBlend::BeginSet(lwIRenderCtrlAgent* agent)
{
    LW_RESULT ret = LW_RET_FAILED;

    // --- declara��es antecipadas (evita warnings com goto) ---
    lwIResourceMgr*             res_mgr      = 0;
    lwIDeviceObject*            dev_obj      = 0;
    IDirect3DDeviceX*           dev          = 0;
    lwIMeshAgent*               mesh_agent   = 0;
    lwIMesh*                    mesh         = 0;
    DWORD                       blend_factor = 0;

    lwMatrix44*                 mat_global   = 0;
    lwMatrix44                  mat; // viewProj * global (sem transpose, como no seu c�digo)
    lwVector3                   light_dir(0.0f, 0.0f, 0.0f);

    DWORD                       rs_amb       = 0;

    lwIAnimCtrlAgent*           anim_agent   = 0;
    DWORD                       animobj_num  = 0;
    lwIAnimCtrlObj*             animctrl_obj = 0;

    // --- valida��es m�nimas preservando o estilo original ---
    if (!agent)
        goto __ret;

    res_mgr = agent->GetResourceMgr();
    if (!res_mgr)
        goto __ret;

    dev_obj = res_mgr->GetDeviceObject();
    if (!dev_obj)
        goto __ret;

    if (lwIsDx11Active())
    {
        Dx11ApplyVertexBlend(dev_obj, agent);
        ret = LW_RET_OK;
        goto __ret;
    }

#if MINDPOWER_USE_D3D9_DEVICE
    dev = dev_obj->GetDevice();
    if (!dev)
        goto __ret;

    mesh_agent = agent->GetMeshAgent();
    if (!mesh_agent)
        goto __ret;

    mesh = mesh_agent->GetMesh();
    if (!mesh || !mesh->GetMeshInfo())
        goto __ret;

    blend_factor = mesh->GetMeshInfo()->bone_infl_factor;

    // --- FOG: salvar e desabilitar (restauro no __ret em caso de falha) ---
    dev_obj->GetRenderState(D3DRS_FOGENABLE, &_rs_fog);
    if (_rs_fog)
        dev_obj->SetRenderState(D3DRS_FOGENABLE, FALSE);

    // --- matrizes (mantido sem transpose, como no seu c�digo) ---
    mat_global = agent->GetGlobalMatrix();
    if (!mat_global)
        goto __ret;

    {
        const lwMatrix44* viewProj = dev_obj->GetMatViewProj();
        if (!viewProj)
            goto __ret;

        mat = *viewProj;
        lwMatrix44Multiply(&mat, mat_global, &mat);
        // lwMatrix44Transpose(&mat, &mat); // (mantido comentado como estava)
    }

    // --- ambiente e luz (mantido) ---
    dev_obj->GetRenderState(D3DRS_AMBIENT, &rs_amb);
    _rs_amb.a = LW_ARGB_A(rs_amb);
    _rs_amb.r = LW_ARGB_R(rs_amb);
    _rs_amb.g = LW_ARGB_G(rs_amb);
    _rs_amb.b = LW_ARGB_B(rs_amb);

    dev_obj->GetLight(0, &_lgt);
    dev_obj->GetRenderState(D3DRS_LIGHTING, &_rs_lgt);
    dev_obj->GetLightEnable(0, &_lgt_enable);

    if (_rs_lgt && _lgt_enable && _lgt.Type == D3DLIGHT_DIRECTIONAL)
    {
        light_dir = *(lwVector3*)&_lgt.Direction;
        light_dir.x = -light_dir.x;
        light_dir.y = -light_dir.y;
        light_dir.z = -light_dir.z;

        lwMatrix44 mat_light;
        lwMatrix44InverseNoScaleFactor(&mat_light, mat_global);
        lwVec3Mat44MulNormal(&light_dir, &mat_light);
    }

    // --- constantes de shader (mantido seu _const_tab) ---
    if (FAILED(_const_tab->SetInt(dev, "blend_num", blend_factor)))
        goto __ret;

    if (FAILED(_const_tab->SetMatrix(dev, "mat_viewproj", &mat)))
        goto __ret;

    if (FAILED(_const_tab->SetValue(dev, "light_dir", &light_dir, sizeof(light_dir))))
        goto __ret;

    // --- paleta de ossos (mantido) ---
    anim_agent = agent->GetAnimCtrlAgent();
    if (!anim_agent)
        goto __ret;

    animobj_num = anim_agent->GetAnimCtrlObjNum();

    {
        DWORD i;
        for (i = 0; i < animobj_num; ++i)
        {
            animctrl_obj = anim_agent->GetAnimCtrlObj(i);
            if (!animctrl_obj)
                continue;

            lwAnimCtrlTypeInfo info;
            animctrl_obj->GetTypeInfo(&info);
            if (info.type == ANIM_CTRL_TYPE_BONE)
            {
                lwIAnimCtrlObjBone* bone_ctrl = (lwIAnimCtrlObjBone*)animctrl_obj;
                DWORD reg_num = bone_ctrl->GetBoneRTTMNum();
                if (reg_num == 0)
                    goto __ret;

                // Se GetConstantByName falhar, retorna NULL e SetMatrixArray falha:
                if (FAILED(_const_tab->SetMatrixArray(
                        dev,
                        _const_tab->GetConstantByName(NULL, "mat_bonepallette"),
                        (D3DXMATRIX*)bone_ctrl->GetBoneRTMSeq(),
                        reg_num)))
                {
                    goto __ret;
                }

                break; // usa o primeiro bone controller encontrado
            }
        }
    }

    // --- vertex shader + decl (mantido) ---
    {
        lwIShaderMgr* shader_mgr = res_mgr->GetShaderMgr();
        if (!shader_mgr)
            goto __ret;

        IDirect3DVertexShaderX*      shader = 0;
        IDirect3DVertexDeclarationX* decl   = 0;

        if (LW_FAILED(shader_mgr->QueryVertexShader(&shader, agent->GetVertexShader())))
            goto __ret;

        if (LW_FAILED(shader_mgr->QueryVertexDeclaration(&decl, agent->GetVertexDeclaration())))
            goto __ret;

        dev_obj->SetVertexDeclarationForced(decl);
        dev_obj->SetVertexShader(shader);
    }

    ret = LW_RET_OK;
#endif

__ret:
    // restaura FOG se falhou e t�nhamos desligado
    if (LW_FAILED(ret) && _rs_fog)
        dev_obj->SetRenderState(D3DRS_FOGENABLE, TRUE);

    return ret;
}
LW_RESULT lwxRenderCtrlVSVertexBlend::EndSet(lwIRenderCtrlAgent* agent)
{
    lwIResourceMgr* res_mgr = agent->GetResourceMgr();
    lwIDeviceObject* dev_obj = res_mgr->GetDeviceObject();

    if(_rs_fog)
    {
        dev_obj->SetRenderState(D3DRS_FOGENABLE, TRUE);
    }

    dev_obj->SetVertexShader(NULL);
    dev_obj->SetVertexDeclaration(NULL);

    return LW_RET_OK;

}
LW_RESULT lwxRenderCtrlVSVertexBlend::BeginSetSubset(DWORD subset, lwIRenderCtrlAgent* agent)
{
    LW_RESULT ret = LW_RET_FAILED;

    // --- declara��es antecipadas (evita warnings com goto) ---
    lwIResourceMgr*     res_mgr       = 0;
    lwIDeviceObject*    dev_obj       = 0;
    lwIMtlTexAgent*     mtltex_agent  = 0;
    IDirect3DDeviceX*   dev           = 0;
    lwMaterial*         mtl           = 0;

    lwColorValue4f      amb_dif[2];
    lwColorValue4f*     c             = 0;
    DWORD               rs_amb        = 0;

    lwIAnimCtrlAgent*   anim_agent    = 0;
    DWORD               animobj_num   = 0;
    lwIAnimCtrlObj*     animctrl_obj  = 0;

    // tabela de est�gios para UV (mantida como no original)
    DWORD stage_tab[3] =
    {
        VS_CONST_REG_TS0_UVMAT,
        VS_CONST_REG_TS1_UVMAT,
        VS_CONST_REG_TS2_UVMAT,
    };

    // --- valida��es m�nimas ---
    if (!agent)
        goto __ret;

    res_mgr = agent->GetResourceMgr();
    if (!res_mgr)
        goto __ret;

    dev_obj = res_mgr->GetDeviceObject();
    if (!dev_obj)
        goto __ret;

    mtltex_agent = agent->GetMtlTexAgent();
    if (!mtltex_agent)
        goto __ret;

    if (lwIsDx11Active())
    {
        mtl = mtltex_agent->GetMaterial();
        if (mtl)
            dev_obj->SetMaterial(mtl);
        Dx11ResetTexTransform(dev_obj);

        anim_agent = agent->GetAnimCtrlAgent();
        if (anim_agent)
        {
            animobj_num = anim_agent->GetAnimCtrlObjNum();
            for (DWORD i = 0; i < animobj_num; ++i)
            {
                animctrl_obj = anim_agent->GetAnimCtrlObj(i);
                if (!animctrl_obj)
                    continue;
                lwAnimCtrlTypeInfo uvinfo;
                animctrl_obj->GetTypeInfo(&uvinfo);
                if (uvinfo.data[0] != subset || uvinfo.type != ANIM_CTRL_TYPE_TEXUV)
                    continue;
                if (uvinfo.data[1] >= 3)
                    continue;
                lwIAnimCtrlObjTexUV* texuv_ctrl = (lwIAnimCtrlObjTexUV*)animctrl_obj;
                lwMatrix44 mat;
                texuv_ctrl->GetRTM(&mat);
                DWORD stage = uvinfo.data[1];
                dev_obj->SetTransform((D3DTRANSFORMSTATETYPE)(D3DTS_TEXTURE0 + stage), &mat);
                dev_obj->SetTextureStageState(stage, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2);
            }
        }
        if (mtl)
            Dx11UploadVsSubsetConstants(dev_obj, agent, subset, mtl);
        ret = LW_RET_OK;
        goto __ret;
    }

#if MINDPOWER_USE_D3D9_DEVICE
    dev = dev_obj->GetDevice();
    if (!dev)
        goto __ret;

    mtl = mtltex_agent->GetMaterial();
    if (!mtl)
        goto __ret;

    // --- ambient state (mantido) ---
    dev_obj->GetRenderState(D3DRS_AMBIENT, &rs_amb);
    _rs_amb.a = LW_ARGB_A(rs_amb);
    _rs_amb.r = LW_ARGB_R(rs_amb);
    _rs_amb.g = LW_ARGB_G(rs_amb);
    _rs_amb.b = LW_ARGB_B(rs_amb);

    // --- combina��es amb/dif conforme luz direcional (mantido) ---
    if (_rs_lgt && _lgt_enable && _lgt.Type == D3DLIGHT_DIRECTIONAL)
    {
        c = &amb_dif[0];
        c->r = (_lgt.Ambient.r + _rs_amb.r) * mtl->amb.r;
        c->g = (_lgt.Ambient.g + _rs_amb.g) * mtl->amb.g;
        c->b = (_lgt.Ambient.b + _rs_amb.b) * mtl->amb.b;
        c->a = (_lgt.Ambient.a + _rs_amb.a) * mtl->amb.a;

        c = &amb_dif[1];
        c->r = _lgt.Diffuse.r * mtl->dif.r;
        c->g = _lgt.Diffuse.g * mtl->dif.g;
        c->b = _lgt.Diffuse.b * mtl->dif.b;
        c->a = _lgt.Diffuse.a * mtl->dif.a;
    }
    else
    {
        c = &amb_dif[0];
        c->r = _rs_amb.r * mtl->amb.r;
        c->g = _rs_amb.g * mtl->amb.g;
        c->b = _rs_amb.b * mtl->amb.b;
        c->a = _rs_amb.a * mtl->amb.a;

        c = &amb_dif[1];
        c->r = c->g = c->b = c->a = 0.0f;
    }

    // --- envia constantes para o VS via constant table (mantido) ---
    if (!_const_tab)
        goto __ret;

    if (FAILED(_const_tab->SetVector(dev, "mtl_amb", (D3DXVECTOR4*)&amb_dif[0])))
        goto __ret;

    if (FAILED(_const_tab->SetVector(dev, "mtl_dif", (D3DXVECTOR4*)&amb_dif[1])))
        goto __ret;

    // --- UV anim (mantido; s� adicionei bounds-check) ---
    anim_agent = agent->GetAnimCtrlAgent();
    if (!anim_agent)
        goto __ret;

    animobj_num = anim_agent->GetAnimCtrlObjNum();

    {
        DWORD i;
        for (i = 0; i < animobj_num; ++i)
        {
            animctrl_obj = anim_agent->GetAnimCtrlObj(i);
            if (!animctrl_obj)
                continue;

            lwAnimCtrlTypeInfo info;
            animctrl_obj->GetTypeInfo(&info);

            // checa subset e tipo
            if (info.data[0] == subset && info.type == ANIM_CTRL_TYPE_TEXUV)
            {
                // garante �ndice v�lido em stage_tab
                if (info.data[1] >= 0 && info.data[1] < 3)
                {
                    DWORD stage_id = stage_tab[info.data[1]];
                    lwIAnimCtrlObjTexUV* texuv_ctrl = (lwIAnimCtrlObjTexUV*)animctrl_obj;

                    lwMatrix44 rtm, mat;
                    texuv_ctrl->GetRTM(&rtm);
                    lwMatrix44Transpose(&mat, &rtm);

                    // Se quiser reativar o envio para VS, descomente:
                    // dev_obj->SetVertexShaderConstantF(stage_id, (float*)&mat, 4);

                    // OU, se existir constante nomeada no shader (ex.: "ts0_uvmat" / "ts1_uvmat"...):
                    // if (FAILED(_const_tab->SetMatrix(dev, "ts0_uvmat", &mat)))
                    //     goto __ret;
                }
                // se n�o for [0..2], ignora silenciosamente
            }
        }
    }

    ret = LW_RET_OK;
#endif

__ret:
    return ret;
}
LW_RESULT lwxRenderCtrlVSVertexBlend::EndSetSubset(DWORD subset, lwIRenderCtrlAgent* agent)
{
    return LW_RET_OK;
}


#if 0
// lwxRenderCtrlVSVertexBlend_fx
LW_STD_IMPLEMENTATION(lwxRenderCtrlVSVertexBlend_fx);


LW_RESULT lwxRenderCtrlVSVertexBlend_fx::Clone(lwIRenderCtrlVS** obj)
{
    this_type* o = LW_NEW(this_type);
    *o = *this;

    *obj = o;

    return LW_RET_OK;
}
LW_RESULT lwxRenderCtrlVSVertexBlend_fx::Initialize(lwIRenderCtrlAgent* agent)
{
    _const_tab = 0;
    if (lwIsDx11Active())
        return LW_RET_OK;

    LW_RESULT ret = LW_RET_FAILED;

    lwIResourceMgr* res_mgr = agent->GetResourceMgr();
    lwIShaderMgr* shader_mgr = res_mgr->GetShaderMgr();

    lwVertexShaderInfo* vs_info = shader_mgr->GetVertexShaderInfo(agent->GetVertexShader());
    if(!vs_info || !vs_info->data)
        goto __ret;
    if(FAILED(D3DXGetShaderConstantTable((DWORD*)vs_info->data, &_const_tab)))
        goto __ret;

    ret = LW_RET_OK;
__ret:
    return ret;
}
LW_RESULT lwxRenderCtrlVSVertexBlend_fx::BeginSet(lwIRenderCtrlAgent* agent)
{
    LW_RESULT ret = LW_RET_FAILED;

    lwIResourceMgr* res_mgr = agent->GetResourceMgr();
    lwIDeviceObject* dev_obj = res_mgr->GetDeviceObject();
    if (lwIsDx11Active())
    {
        Dx11ApplyVertexBlend(dev_obj, agent);
        return LW_RET_OK;
    }
#if MINDPOWER_USE_D3D9_DEVICE
    IDirect3DDeviceX* dev = dev_obj->GetDevice();
    if (!dev)
        return LW_RET_FAILED;
    lwIMeshAgent* mesh_agent = agent->GetMeshAgent();
    lwIMesh* mesh = mesh_agent->GetMesh();
    DWORD blend_factor = mesh->GetMeshInfo()->bone_infl_factor;

    dev_obj->GetRenderState(D3DRS_FOGENABLE, &_rs_fog);
    if(_rs_fog)
    {
        dev_obj->SetRenderState(D3DRS_FOGENABLE, FALSE);
    }

    lwMatrix44* mat_global = agent->GetGlobalMatrix();
    lwMatrix44 mat(*dev_obj->GetMatViewProj());

    lwMatrix44Multiply(&mat, mat_global, &mat);
    //lwMatrix44Transpose(&mat, &mat);

    lwVector3 light_dir(0.0f, 0.0f, 0.0f);
    
    DWORD rs_amb;
    dev_obj->GetRenderState(D3DRS_AMBIENT, &rs_amb);
    _rs_amb.a = LW_ARGB_A(rs_amb);
    _rs_amb.r = LW_ARGB_R(rs_amb);
    _rs_amb.g = LW_ARGB_G(rs_amb);
    _rs_amb.b = LW_ARGB_B(rs_amb);

    dev_obj->GetLight(0, &_lgt);
    dev_obj->GetRenderState(D3DRS_LIGHTING, &_rs_lgt);
    dev_obj->GetLightEnable(0, &_lgt_enable);

    if(_rs_lgt && _lgt_enable && _lgt.Type == D3DLIGHT_DIRECTIONAL)
    {
        light_dir = *(lwVector3*)&_lgt.Direction;
        light_dir.x = -light_dir.x;
        light_dir.y = -light_dir.y;
        light_dir.z = -light_dir.z;

        lwMatrix44 mat_light;
        lwMatrix44InverseNoScaleFactor(&mat_light, mat_global);
        lwVec3Mat44MulNormal(&light_dir, &mat_light);
    }

    if(FAILED(_const_tab->SetInt(dev, "blend_num", blend_factor)))
        goto __ret;

    if(FAILED(_const_tab->SetMatrix(dev, "mat_viewproj", &mat)))
        goto __ret;

    if(FAILED(_const_tab->SetValue(dev, "light_dir", &light_dir, sizeof(light_dir))))
        goto __ret;

    //D3DXCONSTANT_DESC desc;
    //UINT dn = 1;
    //_const_tab->GetConstantDesc("light_dir", &desc, &dn);
    //lwVector4 io;
    //dev->GetVertexShaderConstantF(desc.RegisterIndex, &io.x, 1);

    // set bone matrices pallette
    lwIAnimCtrlAgent* anim_agent = agent->GetAnimCtrlAgent();
    DWORD animobj_num = anim_agent->GetAnimCtrlObjNum();
    lwIAnimCtrlObj* animctrl_obj;
    for(DWORD i = 0; i < animobj_num; i++)
    {
        animctrl_obj = anim_agent->GetAnimCtrlObj(i);
        if(animctrl_obj->GetType() == ANIM_CTRL_TYPE_BONE)
        {
            lwIAnimCtrlObjBone* bone_ctrl = (lwIAnimCtrlObjBone*)animctrl_obj;
            DWORD reg_num = bone_ctrl->GetBoneRTTMNum();
            if(reg_num == 0)
                goto __ret;

            if(FAILED(_const_tab->SetMatrixArray(dev, "mat_bonepallette", (D3DXMATRIX*)bone_ctrl->GetBoneRTTMSeq(), reg_num)))
                goto __ret;

            break;
        }
    }

    // set vertex shader
    lwIShaderMgr* shader_mgr = res_mgr->GetShaderMgr();

    IDirect3DVertexShaderX* shader = 0;
    IDirect3DVertexDeclarationX* decl = 0;

    if(LW_FAILED(shader_mgr->QueryVertexShader(&shader, agent->GetVertexShader())))
        goto __ret;

    if(LW_FAILED(shader_mgr->QueryVertexDeclaration(&decl, agent->GetVertexDeclaration())))
        goto __ret;

    dev_obj->SetVertexDeclarationForced(decl);
    dev_obj->SetVertexShader(shader);

    ret = LW_RET_OK;
__ret:
    return ret;
#else
    return LW_RET_FAILED;
#endif

}
LW_RESULT lwxRenderCtrlVSVertexBlend_fx::EndSet(lwIRenderCtrlAgent* agent)
{
    lwIResourceMgr* res_mgr = agent->GetResourceMgr();
    lwIDeviceObject* dev_obj = res_mgr->GetDeviceObject();

    if(_rs_fog)
    {
        dev_obj->SetRenderState(D3DRS_FOGENABLE, TRUE);
    }

    dev_obj->SetVertexShader(NULL);

    return LW_RET_OK;

}
LW_RESULT lwxRenderCtrlVSVertexBlend_fx::BeginSetSubset(DWORD subset, lwIRenderCtrlAgent* agent)
{
    LW_RESULT ret = LW_RET_FAILED;

    lwIResourceMgr* res_mgr = agent->GetResourceMgr();
    lwIDeviceObject* dev_obj = res_mgr->GetDeviceObject();
    lwIMtlTexAgent* mtltex_agent = agent->GetMtlTexAgent();
    if (lwIsDx11Active())
    {
        if (mtltex_agent)
        {
            lwMaterial* mtl = mtltex_agent->GetMaterial();
            if (mtl)
                dev_obj->SetMaterial(mtl);
        }
        Dx11ResetTexTransform(dev_obj);
        return LW_RET_OK;
    }
#if MINDPOWER_USE_D3D9_DEVICE
    IDirect3DDeviceX* dev = dev_obj->GetDevice();
    if (!dev)
        return LW_RET_FAILED;

    lwMaterial* mtl = mtltex_agent->GetMaterial();

    lwColorValue4f amb_dif[2];
    lwColorValue4f* c;
    amb_dif[0];

    if(_rs_lgt && _lgt_enable && _lgt.Type == D3DLIGHT_DIRECTIONAL)
    {
        c = &amb_dif[0];
        c->r = (_lgt.Ambient.r + _rs_amb.r) * mtl->amb.r;
        c->g = (_lgt.Ambient.g + _rs_amb.g) * mtl->amb.g;
        c->b = (_lgt.Ambient.b + _rs_amb.b) * mtl->amb.b;
        c->a = (_lgt.Ambient.a + _rs_amb.a) * mtl->amb.a;
            
        c = &amb_dif[1];
        c->r = _lgt.Diffuse.r * mtl->dif.r;
        c->g = _lgt.Diffuse.g * mtl->dif.g;
        c->b = _lgt.Diffuse.b * mtl->dif.b;
        c->a = _lgt.Diffuse.a * mtl->dif.a;
    }
    else
    {
        c = &amb_dif[0];
        c->r = _rs_amb.r * mtl->amb.r;
        c->g = _rs_amb.g * mtl->amb.g;
        c->b = _rs_amb.b * mtl->amb.b;
        c->a = _rs_amb.a * mtl->amb.a;

        c = &amb_dif[1];
        c->r = c->g = c->b = c->a = 0.0f;
    }
    
    //c = &amb_dif[0];
    //if(c->r > 1.0f)
    //    c->r = 1.0f;
    //if(c->g > 1.0f)
    //    c->g = 1.0f;
    //if(c->b > 1.0f)
    //    c->b = 1.0f;
    //if(c->a > 1.0f)
    //    c->a = 1.0f;

    //dev_obj->SetVertexShaderConstantF(VS_CONST_REG_LIGHT_AMB, (float*)&amb_dif, 2);
    if(FAILED(_const_tab->SetVector(dev, "mtl_amb", (D3DXVECTOR4*)&amb_dif[0])))
        goto __ret;

    if(FAILED(_const_tab->SetVector(dev, "mtl_dif", (D3DXVECTOR4*)&amb_dif[1])))
        goto __ret;

    // set texture uv data
    DWORD stage_tab[3] =
    {
        VS_CONST_REG_TS0_UVMAT,
        VS_CONST_REG_TS1_UVMAT,
        VS_CONST_REG_TS2_UVMAT,
    };

    lwIAnimCtrlAgent* anim_agent = agent->GetAnimCtrlAgent();
    DWORD animobj_num = anim_agent->GetAnimCtrlObjNum();
    lwIAnimCtrlObj* animctrl_obj;
    for(DWORD i = 0; i < animobj_num; i++)
    {
        animctrl_obj = anim_agent->GetAnimCtrlObj(i);
        if((animctrl_obj->GetSubset()) == subset && (animctrl_obj->GetType() == ANIM_CTRL_TYPE_TEXUV))
        {
            lwIAnimCtrlObjTexUV* texuv_ctrl = (lwIAnimCtrlObjTexUV*)animctrl_obj;
            DWORD stage_id = stage_tab[texuv_ctrl->GetStage()];
            lwMatrix44 mat;
            lwMatrix44Transpose(&mat, texuv_ctrl->GetTexRTTM());

            //dev_obj->SetVertexShaderConstantF(stage_id, (float*)&mat, 4);
        }
    }

    ret = LW_RET_OK;
__ret:
    return ret;
#else
    return LW_RET_FAILED;
#endif

}
LW_RESULT lwxRenderCtrlVSVertexBlend_fx::EndSetSubset(DWORD subset, lwIRenderCtrlAgent* agent)
{
    return LW_RET_OK;
}
#endif

#endif


LW_END

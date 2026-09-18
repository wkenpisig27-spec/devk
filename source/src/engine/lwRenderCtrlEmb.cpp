//
#include "stdafx.h"


#include "lwRenderCtrlEmb.h"
#include "lwRenderBackend.h"

LW_BEGIN

static BOOL g_ff_fullbright = FALSE;

void lwEnableFullbrightFixedFunction(BOOL enable)
{
	g_ff_fullbright = enable ? TRUE : FALSE;
}

lwIRenderCtrlVS* __RenderCtrlVSProcFixedFuntion()
{
    return LW_NEW(lwRenderCtrlVSFixedFunction);
}

LW_RESULT lwInitInternalRenderCtrlVSProc(lwIResourceMgr* mgr)
{
    mgr->RegisterRenderCtrlProc(RENDERCTRL_VS_FIXEDFUNCTION, __RenderCtrlVSProcFixedFuntion);

    return LW_RET_OK;
}

// lwRenderCtrlVSFixedFunction
LW_STD_IMPLEMENTATION(lwRenderCtrlVSFixedFunction);

LW_RESULT lwRenderCtrlVSFixedFunction::Clone(lwIRenderCtrlVS** obj)
{
    this_type* o = LW_NEW(this_type);
    *o = *this;

    *obj = o;

    return LW_RET_OK;
}
LW_RESULT lwRenderCtrlVSFixedFunction::Initialize(lwIRenderCtrlAgent* agent)
{
    return LW_RET_OK;
}

LW_RESULT lwRenderCtrlVSFixedFunction::BeginSet(lwIRenderCtrlAgent* agent)
{
    lwIResourceMgr* res_mgr = agent->GetResourceMgr();
    lwIDeviceObject* dev_obj = res_mgr->GetDeviceObject();
    // Items are fixed-function. Characters/scene props leave a VS bound;
    // if we don't clear it, FF items inherit that shader + its lighting
    // constants (including the darkened scene ambient) and wash out
    // UI markers like target.lgo / sighyellow.lgo.
#if defined(LW_USE_DX9)
    dev_obj->SetVertexShader((IDirect3DVertexShaderX*)NULL);
#else
    dev_obj->SetVertexShader((IDirect3DVertexShaderX)NULL);
#endif
    dev_obj->SetTransformWorld(agent->GetGlobalMatrix());

    if (lwIsDx11Active())
    {
        lwMatrix44 tex_id;
        lwMatrix44Identity(&tex_id);
        for (DWORD s = 0; s < 2; ++s)
        {
            dev_obj->SetTransform((D3DTRANSFORMSTATETYPE)(D3DTS_TEXTURE0 + s), &tex_id);
            dev_obj->SetTextureStageState(s, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
        }
        dev_obj->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
        dev_obj->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
    }
    
    return LW_RET_OK;
}
LW_RESULT lwRenderCtrlVSFixedFunction::EndSet(lwIRenderCtrlAgent* agent)
{
    if (lwIsDx11Active() && agent)
    {
        lwIDeviceObject* dev_obj = agent->GetResourceMgr()->GetDeviceObject();
        lwMatrix44 tex_id;
        lwMatrix44Identity(&tex_id);
        for (DWORD s = 0; s < 2; ++s)
        {
            dev_obj->SetTransform((D3DTRANSFORMSTATETYPE)(D3DTS_TEXTURE0 + s), &tex_id);
            dev_obj->SetTextureStageState(s, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
        }
    }
    return LW_RET_OK;
}
void lwApplySubsetTexUV(lwIDeviceObject* dev_obj, DWORD subset, lwIAnimCtrlAgent* anim_agent)
{
    if (!dev_obj || !anim_agent)
        return;

    const DWORD animobj_num = anim_agent->GetAnimCtrlObjNum();
    for (DWORD i = 0; i < animobj_num; i++)
    {
        lwIAnimCtrlObj* animctrl_obj = anim_agent->GetAnimCtrlObj(i);
        lwAnimCtrlObjTypeInfo type_info;
        animctrl_obj->GetTypeInfo(&type_info);

        if (type_info.type != ANIM_CTRL_TYPE_TEXUV)
            continue;
        if (type_info.data[0] != subset)
            continue;

        lwIAnimCtrlObjTexUV* texuv = (lwIAnimCtrlObjTexUV*)animctrl_obj;
        texuv->UpdateObject();

        lwMatrix44 mat;
        if (LW_FAILED(texuv->GetRTM(&mat)))
            continue;

        const DWORD stage_id = type_info.data[1];
        dev_obj->SetTransform((D3DTRANSFORMSTATETYPE)(D3DTS_TEXTURE0 + stage_id), &mat);
        dev_obj->SetTextureStageState(stage_id, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2);
    }
}

LW_RESULT lwRenderCtrlVSFixedFunction::BeginSetSubset(DWORD subset, lwIRenderCtrlAgent* agent)
{
    lwIResourceMgr* res_mgr = agent->GetResourceMgr();
    lwIDeviceObject* dev_obj = res_mgr->GetDeviceObject();

	// Re-assert AFTER mtl/tex RSA so system markers keep true texture color.
	if (g_ff_fullbright)
	{
		dev_obj->SetRenderStateForced(D3DRS_LIGHTING, FALSE);
		dev_obj->SetRenderStateForced(D3DRS_FOGENABLE, FALSE);
		dev_obj->SetRenderStateForced(D3DRS_AMBIENT, 0xffffffff);
		dev_obj->SetRenderStateForced(D3DRS_TEXTUREFACTOR, 0xffffffff);
		dev_obj->SetTextureStageStateForced(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
		dev_obj->SetTextureStageStateForced(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
		dev_obj->SetTextureStageStateForced(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
		dev_obj->SetTextureStageStateForced(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
		dev_obj->SetTextureStageStateForced(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
	}

    lwIAnimCtrlAgent* anim_agent = agent->GetAnimCtrlAgent();

    if (lwIsDx11Active())
    {
        lwMatrix44 tex_id;
        lwMatrix44Identity(&tex_id);
        for (DWORD s = 0; s < 2; ++s)
        {
            dev_obj->SetTransform((D3DTRANSFORMSTATETYPE)(D3DTS_TEXTURE0 + s), &tex_id);
            dev_obj->SetTextureStageState(s, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
        }
    }

    if (anim_agent)
        lwApplySubsetTexUV(dev_obj, subset, anim_agent);
__ret:
    return LW_RET_OK;
}
LW_RESULT lwRenderCtrlVSFixedFunction::EndSetSubset(DWORD subset, lwIRenderCtrlAgent* agent)
{
    lwIResourceMgr* res_mgr = agent->GetResourceMgr();
    lwIDeviceObject* dev_obj = res_mgr->GetDeviceObject();

    lwIAnimCtrlAgent* anim_agent = agent->GetAnimCtrlAgent();

    if (anim_agent == 0)
        goto __ret;
    {
        DWORD animobj_num = anim_agent->GetAnimCtrlObjNum();
        lwIAnimCtrlObj* animctrl_obj;
        lwAnimCtrlObjTypeInfo type_info;

        for (DWORD i = 0; i < animobj_num; i++)
        {
            animctrl_obj = anim_agent->GetAnimCtrlObj(i);
            animctrl_obj->GetTypeInfo(&type_info);

            BOOL play_type = animctrl_obj->IsPlaying();

            if ((type_info.data[0] == subset) && (play_type != PLAY_INVALID))
            {
                DWORD anim_type = type_info.type;
                DWORD stage_id = type_info.data[1];

                if (anim_type == ANIM_CTRL_TYPE_TEXUV)
                {
                    lwMatrix44 tex_id;
                    lwMatrix44Identity(&tex_id);
                    dev_obj->SetTransform((D3DTRANSFORMSTATETYPE)(D3DTS_TEXTURE0 + stage_id), &tex_id);
                    dev_obj->SetTextureStageState(stage_id, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
                }
                //else if(anim_type == ANIM_CTRL_TYPE_TEXIMG)
                //{
                //    lwIAnimCtrlObjTexImg* this_ctrl = (lwIAnimCtrlObjTexImg*)animctrl_obj;
                //    lwITex* this_tex = this_ctrl->GetRunTimeTex();
                //    this_tex->EndSet();
                //}
            }
        }
    }
__ret:
    return LW_RET_OK;
}


LW_END

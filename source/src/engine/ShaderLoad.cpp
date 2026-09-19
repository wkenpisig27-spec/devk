//
#include "stdafx.h"

#include "ShaderLoad.h"
#include "lwgraphicsutil.h"
#include "lwxRenderCtrlVS.h"
#include "lwRenderBackend.h"
#include "lwDeviceObject11.h"


#define USER_SHADER_NUM             8

// --- Character-physique outline globals (lwPhysique only; items never outline)
bool g_lwOutlineEnabled = true;
static float g_lwOutlineWidth = 0.014f; // DX9 world extrusion; DX11 maps <0.5 to pixels
static float g_lwOutlineColorR = 0.33f;
static float g_lwOutlineColorG = 0.25f;
static float g_lwOutlineColorB = 0.20f;
static float g_lwOutlineRefDepth = 50.0f; // reserved (unused; kept for API compat)

static float OutlinePixelWidth()
{
    float px = g_lwOutlineWidth;
    if (px < 0.5f)
        return 1.15f;
    if (px > 6.0f)
        return 6.0f;
    return px;
}

extern "C" MINDPOWER_API void lwSetOutlineEnabled(int enabled)
{
    g_lwOutlineEnabled = (enabled != 0);
}

extern "C" MINDPOWER_API void lwSetOutlineParams(float worldWidth, float r, float g, float b, float refDepth)
{
    if (worldWidth > 0.0f)
        g_lwOutlineWidth = worldWidth;
    g_lwOutlineColorR = r;
    g_lwOutlineColorG = g;
    g_lwOutlineColorB = b;
    if (refDepth > 0.0f)
        g_lwOutlineRefDepth = refDepth;
}

void lwGetOutlineScreenScale(lwIDeviceObject* dev_obj, float* ndcX, float* ndcY)
{
    const float px = OutlinePixelWidth();
    float vw = 1280.0f;
    float vh = 720.0f;
    int got_vp = 0;
    if (lwIsDx11Active())
    {
        lwDeviceObject11* d11 = lwGetActiveDeviceObject11();
        if (d11)
        {
            D3DVIEWPORTX vp = {};
            if (LW_SUCCEEDED(d11->GetViewPort(&vp)) && vp.Width > 1 && vp.Height > 1)
            {
                vw = (float)vp.Width;
                vh = (float)vp.Height;
                got_vp = 1;
            }
        }
    }
    if (!got_vp && dev_obj)
    {
        RECT wnd = {}, client = {};
        if (LW_SUCCEEDED(dev_obj->GetWindowRect(&wnd, &client)))
        {
            const int cw = client.right - client.left;
            const int ch = client.bottom - client.top;
            if (cw > 1)
                vw = (float)cw;
            if (ch > 1)
                vh = (float)ch;
        }
    }
    if (ndcX)
        *ndcX = px * 2.0f / vw;
    if (ndcY)
        *ndcY = px * 2.0f / vh;
}

void lwApplyOutlineVSConstants(lwIDeviceObject* dev_obj)
{
    if (!dev_obj)
        return;
    lwVector4 base(1.0f, g_lwOutlineWidth, g_lwOutlineRefDepth, 765.01f);
    if (lwIsDx11Active())
    {
        float sx = 0.0f, sy = 0.0f;
        lwGetOutlineScreenScale(dev_obj, &sx, &sy);
        base.y = sx;
        base.z = sy;
    }
    lwVector4 outlineColor(g_lwOutlineColorR, g_lwOutlineColorG, g_lwOutlineColorB, 1.0f);
    dev_obj->SetVertexShaderConstantF(VS_CONST_REG_BASE, (float*)&base, 1);
    dev_obj->SetVertexShaderConstantF(VS_CONST_REG_LIGHT_DIF, (float*)&outlineColor, 1);
}

void lwGetOutlineParams(float* worldWidth, float* r, float* g, float* b)
{
    if (worldWidth)
        *worldWidth = g_lwOutlineWidth;
    if (r)
        *r = g_lwOutlineColorR;
    if (g)
        *g = g_lwOutlineColorG;
    if (b)
        *b = g_lwOutlineColorB;
}

// Native DX11: LoadShader0/1 register .hlsl keys + decls. ShaderMgr11 compiles SM4.
LW_RESULT LoadShader0(lwISysGraphics* sys_graphics)
{
    LW_RESULT ret = LW_RET_FAILED;

    // --- declara??es antecipadas (evita warnings com goto) ---
    lwISystem*        sys        = 0;
    lwIPathInfo*      path_info  = 0;
    lwIResourceMgr*   res_mgr    = 0;
    lwIShaderMgr*     shader_mgr = 0;
    lwIShaderDeclMgr* decl_mgr   = 0;

    char path[LW_MAX_PATH];

    if (!sys_graphics)
        goto __ret;

    sys = sys_graphics->GetSystem();
    if (!sys)
        goto __ret;

    if (LW_FAILED(sys->GetInterface((LW_VOID**)&path_info, LW_GUID_PATHINFO)) || !path_info)
        goto __ret;

    if (LW_FAILED(sys_graphics->GetInterface((LW_VOID**)&res_mgr, LW_GUID_RESOURCEMGR)) || !res_mgr)
        goto __ret;

    shader_mgr = res_mgr->GetShaderMgr();
    if (!shader_mgr)
        goto __ret;

#if defined(LW_USE_DX9)

    // ======== DX9: Vertex Decls ========
    // torne os arrays static para evitar o warning com goto
    static D3DVERTEXELEMENT9 ve0[] = {
        {0, 0,  D3DDECLTYPE_FLOAT3,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION,     0},
        {0, 12, D3DDECLTYPE_D3DCOLOR,D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDINDICES, 0},
        {0, 16, D3DDECLTYPE_FLOAT3,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL,       0},
        {0, 28, D3DDECLTYPE_FLOAT2,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,     0},
        {0xFF, 0, D3DDECLTYPE_UNUSED,0, 0, 0},
    };
    static D3DVERTEXELEMENT9 ve1[] = {
        {0, 0,  D3DDECLTYPE_FLOAT3,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION,     0},
        {0, 12, D3DDECLTYPE_FLOAT1,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDWEIGHT,  0},
        {0, 16, D3DDECLTYPE_D3DCOLOR,D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDINDICES, 0},
        {0, 20, D3DDECLTYPE_FLOAT3,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL,       0},
        {0, 32, D3DDECLTYPE_FLOAT2,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,     0},
        {0xFF, 0, D3DDECLTYPE_UNUSED,0, 0, 0},
    };
    static D3DVERTEXELEMENT9 ve2[] = {
        {0, 0,  D3DDECLTYPE_FLOAT3,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION,     0},
        {0, 12, D3DDECLTYPE_FLOAT2,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDWEIGHT,  0},
        {0, 20, D3DDECLTYPE_D3DCOLOR,D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDINDICES, 0},
        {0, 24, D3DDECLTYPE_FLOAT3,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL,       0},
        {0, 36, D3DDECLTYPE_FLOAT2,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,     0},
        {0xFF, 0, D3DDECLTYPE_UNUSED,0, 0, 0},
    };
    static D3DVERTEXELEMENT9 ve3[] = {
        {0, 0,  D3DDECLTYPE_FLOAT3,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION,     0},
        {0, 12, D3DDECLTYPE_FLOAT3,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDWEIGHT,  0},
        {0, 24, D3DDECLTYPE_D3DCOLOR,D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDINDICES, 0},
        {0, 28, D3DDECLTYPE_FLOAT3,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL,       0},
        {0, 40, D3DDECLTYPE_FLOAT2,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,     0},
        {0xFF, 0, D3DDECLTYPE_UNUSED,0, 0, 0},
    };
    static D3DVERTEXELEMENT9 vepnt0[] = {
        {0, 0,  D3DDECLTYPE_FLOAT3,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {0, 12, D3DDECLTYPE_FLOAT3,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL,   0},
        {0, 24, D3DDECLTYPE_FLOAT2,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
        {0xFF, 0, D3DDECLTYPE_UNUSED,0, 0, 0},
    };
    static D3DVERTEXELEMENT9 vepndt0[] = {
        {0, 0,  D3DDECLTYPE_FLOAT3,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {0, 12, D3DDECLTYPE_FLOAT3,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL,   0},
        {0, 24, D3DDECLTYPE_D3DCOLOR,D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,    0},
        {0, 28, D3DDECLTYPE_FLOAT2,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
        {0xFF, 0, D3DDECLTYPE_UNUSED,0, 0, 0},
    };

    static D3DVERTEXELEMENT9* ve_buf[] = {
        ve0, ve1, ve2, ve3,
        vepnt0, vepnt0, vepndt0, vepnt0, vepndt0, vepndt0, vepndt0
    };
    static const DWORD decl_type[] = {
        VDT_PU4NT0, VDT_PB1U4NT0, VDT_PB2U4NT0, VDT_PB3U4NT0,
        VDT_PNT0,   VDT_PNT0,     VDT_PNDT0,    VDT_PNT0,
        VDT_PNDT0,  VDT_PNDT0,    VDT_PNDT0,
    };
    {
        const int decl_num = (int)(sizeof(decl_type) / sizeof(decl_type[0]));
        int i;
        IDirect3DVertexDeclarationX* this_decl = 0;
        for (i = 0; i < decl_num; ++i)
        {
            this_decl = 0;
            if (LW_SUCCEEDED(shader_mgr->QueryVertexDeclaration(&this_decl, decl_type[i])))
                continue;
            if (LW_FAILED(shader_mgr->RegisterVertexDeclaration(decl_type[i], ve_buf[i])))
                goto __ret;
        }
    }

#endif // DX9


#if defined(LW_USE_DX9)

    // ======== DX9 path names: ShaderMgr11 compiles SM4 HLSL ========
    static const DWORD shader_type[] = {
        VST_PU4NT0_LD, VST_PB1U4NT0_LD, VST_PB2U4NT0_LD, VST_PB3U4NT0_LD,
        VST_PNT0_LD_TT0, VST_PNT0_TT0, VST_PNDT0_LD_TT0,
        VST_PNT0_LD, VST_PNDT0, VST_PNDT0_LD, VST_PNDT0_TT0,
    };
    static const char* shader_file[] = {
        "skinmesh8_1.hlsl", "skinmesh8_2.hlsl", "skinmesh8_3.hlsl", "skinmesh8_4.hlsl",
        "vs_pnt0_ld_t0uvmat.hlsl", "vs_pnt0_t0uvmat.hlsl", "vs_pndt0_ld_t0uvmat.hlsl",
        "vs_pnt0_ld.hlsl", "vs_pndt0.hlsl", "vs_pndt0_ld.hlsl", "vs_pndt0_t0uvmat.hlsl",
    };
    static const DWORD file_types[] = {
        VS_FILE_ASM, VS_FILE_ASM, VS_FILE_ASM, VS_FILE_ASM,
        VS_FILE_ASM, VS_FILE_ASM, VS_FILE_ASM,
        VS_FILE_ASM, VS_FILE_ASM, VS_FILE_ASM, VS_FILE_ASM,
    };
    // No defines needed for compiled assembly
    static D3DXMACRO defines[] = {
        {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL},
        {NULL, NULL}, {NULL, NULL}, {NULL, NULL},
        {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL},
    };
    static const int shader_num = (int)(sizeof(shader_type) / sizeof(shader_type[0]));

    {
        int i;
        for (i = 0; i < shader_num; ++i)
        {
            sprintf(path, "%s%s", path_info->GetPath(PATH_TYPE_SHADER), shader_file[i]);
            // Pass defines even though empty, structure expected
            if (LW_FAILED(shader_mgr->RegisterVertexShader(shader_type[i], path, file_types[i], &defines[i])))
                goto __ret;
        }
    }

#elif defined(LW_USE_DX8)

    // ======== DX8: Vertex Shaders ========
    static const DWORD shader_type[] = {
        VST_PU4NT0_LD, VST_PB1U4NT0_LD, VST_PB2U4NT0_LD, VST_PB3U4NT0_LD,
        VST_PNT0_LD_TT0, VST_PNT0_TT0, VST_PNDT0_LD_TT0,
        VST_PNT0_LD, VST_PNDT0, VST_PNDT0_LD, VST_PNDT0_TT0,
    };
    static DWORD dwDecl0[] = {
        D3DVSD_STREAM(0),
        D3DVSD_REG(VSREG_V_POSITION,    D3DVSDT_FLOAT3),
        // D3DVSD_REG(VSREG_V_BLENDWEIGHT,D3DVSDT_D3DCOLOR),
        D3DVSD_REG(VSREG_V_BLENDINDICES,D3DVSDT_D3DCOLOR),
        D3DVSD_REG(VSREG_V_NORMAL,      D3DVSDT_FLOAT3),
        D3DVSD_REG(VSREG_V_TEXCOORD0,   D3DVSDT_FLOAT2),
        D3DVSD_END()
    };
    static DWORD dwDecl1[] = {
        D3DVSD_STREAM(0),
        D3DVSD_REG(VSREG_V_POSITION,    D3DVSDT_FLOAT3),
        D3DVSD_REG(VSREG_V_BLENDWEIGHT, D3DVSDT_FLOAT1),
        D3DVSD_REG(VSREG_V_BLENDINDICES,D3DVSDT_D3DCOLOR),
        D3DVSD_REG(VSREG_V_NORMAL,      D3DVSDT_FLOAT3),
        D3DVSD_REG(VSREG_V_TEXCOORD0,   D3DVSDT_FLOAT2),
        D3DVSD_END()
    };
    static DWORD dwDecl2[] = {
        D3DVSD_STREAM(0),
        D3DVSD_REG(VSREG_V_POSITION,    D3DVSDT_FLOAT3),
        D3DVSD_REG(VSREG_V_BLENDWEIGHT, D3DVSDT_FLOAT2),
        D3DVSD_REG(VSREG_V_BLENDINDICES,D3DVSDT_D3DCOLOR),
        D3DVSD_REG(VSREG_V_NORMAL,      D3DVSDT_FLOAT3),
        D3DVSD_REG(VSREG_V_TEXCOORD0,   D3DVSDT_FLOAT2),
        D3DVSD_END()
    };
    static DWORD dwDecl3[] = {
        D3DVSD_STREAM(0),
        D3DVSD_REG(VSREG_V_POSITION,    D3DVSDT_FLOAT3),
        D3DVSD_REG(VSREG_V_BLENDWEIGHT, D3DVSDT_FLOAT3),
        D3DVSD_REG(VSREG_V_BLENDINDICES,D3DVSDT_D3DCOLOR),
        D3DVSD_REG(VSREG_V_NORMAL,      D3DVSDT_FLOAT3),
        D3DVSD_REG(VSREG_V_TEXCOORD0,   D3DVSDT_FLOAT2),
        D3DVSD_END()
    };
    static DWORD decl_pnt0[] = {
        D3DVSD_STREAM(0),
        D3DVSD_REG(VSREG_V_POSITION,   D3DVSDT_FLOAT3),
        D3DVSD_REG(VSREG_V_NORMAL,     D3DVSDT_FLOAT3),
        D3DVSD_REG(VSREG_V_TEXCOORD0,  D3DVSDT_FLOAT2),
        D3DVSD_END()
    };
    static DWORD decl_pndt0[] = {
        D3DVSD_STREAM(0),
        D3DVSD_REG(VSREG_V_POSITION,   D3DVSDT_FLOAT3),
        D3DVSD_REG(VSREG_V_NORMAL,     D3DVSDT_FLOAT3),
        D3DVSD_REG(VSREG_V_DIFFUSE,    D3DVSDT_D3DCOLOR),
        D3DVSD_REG(VSREG_V_TEXCOORD0,  D3DVSDT_FLOAT2),
        D3DVSD_END()
    };
    static DWORD* decl_tab[] = {
        dwDecl0, dwDecl1, dwDecl2, dwDecl3,
        decl_pnt0, decl_pnt0, decl_pndt0, decl_pnt0,
        decl_pndt0, decl_pndt0, decl_pndt0,
    };
    static DWORD decl_size[] = {
        sizeof(dwDecl0), sizeof(dwDecl1), sizeof(dwDecl2), sizeof(dwDecl3),
        sizeof(decl_pnt0), sizeof(decl_pnt0), sizeof(decl_pndt0), sizeof(decl_pnt0),
        sizeof(decl_pndt0), sizeof(decl_pndt0), sizeof(decl_pndt0),
    };
    static const char* shader_file[] = {
        "pu4nt0_ld.vso", "pb1u4nt0_ld.vso", "pb2u4nt0_ld.vso", "pb3u4nt0_ld.vso",
        "pnt0_ld_t0uvmat.vso", "pnt0_t0uvmat.vso", "pndt0_ld_t0uvmat.vso",
        "pnt0_ld.vso", "pndt0.vso", "pndt0_ld.vso", "pndt0_t0uvmat.vso",
    };
    static const int shader_num = (int)(sizeof(shader_type) / sizeof(shader_type[0]));

    {
        int i;
        for (i = 0; i < shader_num; ++i)
        {
            sprintf(path, "%s%s", path_info->GetPath(PATH_TYPE_SHADER), shader_file[i]);
            if (LW_FAILED(shader_mgr->RegisterVertexShader(shader_type[i], path, 0, decl_tab[i], decl_size[i], 1)))
            {
                LG_MSGBOX("Load Vertex Shader Error");
                return LW_RET_FAILED; // como no original
            }
        }
    }

#endif // DX8 / DX9

    // ======== ShaderDeclMgr ========
    decl_mgr = shader_mgr->GetShaderDeclMgr();  // <- sem inicializador na declara??o
    if (!decl_mgr)
        goto __ret;

    decl_mgr->CreateShaderDeclSet(VDT_PNT0,  8);
    decl_mgr->CreateShaderDeclSet(VDT_PNDT0, 8);

    // tamb��m deixe est��tico o sdci_num e o array sdci
    static const DWORD sdci_num = 4;
    static lwShaderDeclCreateInfo sdci[sdci_num] = {
        SDCI_VALUE(VST_PNT0_LD_TT0,  VDT_PNT0,  VSLT_DIRECTIONAL, VSAT_TEXTURETRANSFORM0, "vs_pnt0_ld_t0uvmat.hlsl"),
        SDCI_VALUE(VST_PNT0_TT0,     VDT_PNT0,  VSLT_INVALID,     VSAT_TEXTURETRANSFORM0, "vs_pnt0_t0uvmat.hlsl"),
        SDCI_VALUE(VST_PNDT0_LD_TT0, VDT_PNDT0, VSLT_DIRECTIONAL, VSAT_TEXTURETRANSFORM0, "vs_pndt0_ld_t0uvmat.hlsl"),
        SDCI_VALUE(VST_PNDT0_TT0,    VDT_PNDT0, VSLT_INVALID,     VSAT_TEXTURETRANSFORM0, "vs_pndt0_t0uvmat.hlsl"),
    };
    {
        DWORD i;
        for (i = 0; i < sdci_num; ++i)
            decl_mgr->SetShaderDeclInfo(&sdci[i]);
    }

    ret = LW_RET_OK;

__ret:
    if (ret != LW_RET_OK)
        LG_MSGBOX("LoadShader0 error");
    return ret;
}


LW_RESULT LoadShader1(lwISysGraphics* sys_graphics)
{
    LW_RESULT ret = LW_RET_FAILED;

    lwISystem* sys = sys_graphics->GetSystem();

    char path[LW_MAX_PATH];
    lwIPathInfo* path_info = 0;
    sys->GetInterface((LW_VOID**)&path_info, LW_GUID_PATHINFO);
    
    lwIResourceMgr* res_mgr;
    lwIShaderMgr* shader_mgr;

    sys_graphics->GetInterface((LW_VOID**)&res_mgr, LW_GUID_RESOURCEMGR);
    shader_mgr = res_mgr->GetShaderMgr();


    DWORD shader_type[] =
    {
        VSTU_SKINMESH0_TT1,
        VSTU_SKINMESH1_TT1,
        VSTU_SKINMESH2_TT1,
        VSTU_SKINMESH3_TT1,

        VSTU_SKINMESH0_TT2,
        VSTU_SKINMESH1_TT2,
        VSTU_SKINMESH2_TT2,
        VSTU_SKINMESH3_TT2,

        VSTU_SKINMESH0_TT3,
        VSTU_SKINMESH1_TT3,
        VSTU_SKINMESH2_TT3,
        VSTU_SKINMESH3_TT3,

    };

    int shader_num = sizeof(shader_type) / sizeof(shader_type[0]);

#if (defined LW_USE_DX9)

    // �й�dx9����û�и���
    //const char* shader_file[] = 
    //{
    //    "skinmesh9_1.vsh",
    //    "skinmesh9_2.vsh",
    //    "skinmesh9_3.vsh",
    //    "skinmesh9_4.vsh",
    //};

	const char* shader_file[] = 
    {
        "skinmesh8_1_tt1.hlsl",
        "skinmesh8_2_tt1.hlsl",
        "skinmesh8_1_tt2.hlsl",
        "skinmesh8_2_tt2.hlsl",
        "skinmesh8_1_tt3.hlsl",
        "skinmesh8_2_tt3.hlsl",
    };

    for(int i = 0; i < 6; i++)
    {
        sprintf(path, "%s%s", path_info->GetPath(PATH_TYPE_SHADER), shader_file[i]);
        if(LW_FAILED(shader_mgr->RegisterVertexShader(shader_type[i], path, VS_FILE_ASM)))
            goto __ret;
    }

    // ---- Character-physique inverted-hull outline shaders (skinned only) ----
    {
        static const DWORD outline_shader_type[] = {
            VSTU_PU4NT0_OUTLINE,
            VSTU_PB1U4NT0_OUTLINE,
            VSTU_PB2U4NT0_OUTLINE,
            VSTU_PB3U4NT0_OUTLINE,
        };
        static const char* outline_shader_file[] = {
            "skinmesh8_1_outline.hlsl",
            "skinmesh8_2_outline.hlsl",
            "skinmesh8_3_outline.hlsl",
            "skinmesh8_4_outline.hlsl",
        };
        for (int i = 0; i < 4; i++)
        {
            sprintf(path, "%s%s", path_info->GetPath(PATH_TYPE_SHADER), outline_shader_file[i]);
            if (LW_FAILED(shader_mgr->RegisterVertexShader(outline_shader_type[i], path, VS_FILE_ASM)))
                goto __ret;
        }
    }

#elif (defined LW_USE_DX8)

    DWORD dwDecl0[] =
    {
        D3DVSD_STREAM(0),
        D3DVSD_REG(VSREG_V_POSITION,  D3DVSDT_FLOAT3),
//        D3DVSD_REG(VSREG_V_BLENDWEIGHT,  D3DVSDT_D3DCOLOR),
        D3DVSD_REG(VSREG_V_BLENDINDICES,  D3DVSDT_D3DCOLOR),        
        D3DVSD_REG(VSREG_V_NORMAL,    D3DVSDT_FLOAT3),
        D3DVSD_REG(VSREG_V_TEXCOORD0, D3DVSDT_FLOAT2),
        D3DVSD_END()
    };

    DWORD dwDecl1[] =
    {
        D3DVSD_STREAM(0),
        D3DVSD_REG(VSREG_V_POSITION,  D3DVSDT_FLOAT3),
        D3DVSD_REG(VSREG_V_BLENDWEIGHT,  D3DVSDT_FLOAT1),
        D3DVSD_REG(VSREG_V_BLENDINDICES,  D3DVSDT_D3DCOLOR),        
        D3DVSD_REG(VSREG_V_NORMAL,    D3DVSDT_FLOAT3),
        D3DVSD_REG(VSREG_V_TEXCOORD0, D3DVSDT_FLOAT2),
        D3DVSD_END()
    };

    DWORD dwDecl2[] =
    {
        D3DVSD_STREAM(0),
        D3DVSD_REG(VSREG_V_POSITION,  D3DVSDT_FLOAT3),
        D3DVSD_REG(VSREG_V_BLENDWEIGHT,  D3DVSDT_FLOAT2),
        D3DVSD_REG(VSREG_V_BLENDINDICES,  D3DVSDT_D3DCOLOR),        
        D3DVSD_REG(VSREG_V_NORMAL,    D3DVSDT_FLOAT3),
        D3DVSD_REG(VSREG_V_TEXCOORD0, D3DVSDT_FLOAT2),
        D3DVSD_END()
    };

    DWORD dwDecl3[] =
    {
        D3DVSD_STREAM(0),
        D3DVSD_REG(VSREG_V_POSITION,  D3DVSDT_FLOAT3),
        D3DVSD_REG(VSREG_V_BLENDWEIGHT,  D3DVSDT_FLOAT3),
        D3DVSD_REG(VSREG_V_BLENDINDICES,  D3DVSDT_D3DCOLOR),        
        D3DVSD_REG(VSREG_V_NORMAL,    D3DVSDT_FLOAT3),
        D3DVSD_REG(VSREG_V_TEXCOORD0, D3DVSDT_FLOAT2),
        D3DVSD_END()
    };

    DWORD decl_pnt0[] =
    {
        D3DVSD_STREAM(0),
        D3DVSD_REG(VSREG_V_POSITION,  D3DVSDT_FLOAT3),
        D3DVSD_REG(VSREG_V_NORMAL,    D3DVSDT_FLOAT3),
        D3DVSD_REG(VSREG_V_TEXCOORD0,  D3DVSDT_FLOAT2),
        D3DVSD_END()
    };
    DWORD decl_pndt0[] =
    {
        D3DVSD_STREAM(0),
        D3DVSD_REG(VSREG_V_POSITION,  D3DVSDT_FLOAT3),
        D3DVSD_REG(VSREG_V_NORMAL,    D3DVSDT_FLOAT3),
        D3DVSD_REG(VSREG_V_DIFFUSE,   D3DVSDT_D3DCOLOR),        
        D3DVSD_REG(VSREG_V_TEXCOORD0,  D3DVSDT_FLOAT2),
        D3DVSD_END()
    };

    DWORD* decl_tab[] =
    {
        dwDecl0,
        dwDecl1,
        dwDecl2,
        dwDecl3,

        dwDecl0,
        dwDecl1,
        dwDecl2,
        dwDecl3,

        dwDecl0,
        dwDecl1,
        dwDecl2,
        dwDecl3,

        decl_pnt0,
        decl_pnt0,
        decl_pndt0,
        decl_pnt0,
        decl_pndt0,
        decl_pndt0,
        decl_pndt0,
    };
    DWORD decl_size[] =
    {
        // tt1
        sizeof(dwDecl0),
        sizeof(dwDecl1),
        sizeof(dwDecl2),
        sizeof(dwDecl3),
        // tt2
        sizeof(dwDecl0),
        sizeof(dwDecl1),
        sizeof(dwDecl2),
        sizeof(dwDecl3),
        // tt3
        sizeof(dwDecl0),
        sizeof(dwDecl1),
        sizeof(dwDecl2),
        sizeof(dwDecl3),

        // unused
        sizeof(decl_pnt0),
        sizeof(decl_pnt0),
        sizeof(decl_pndt0),
        sizeof(decl_pnt0),
        sizeof(decl_pndt0),
        sizeof(decl_pndt0),
        sizeof(decl_pndt0),
    };

	const char* shader_file[] = 
    {
        "pu4nt0_ld_tt1.vso",
        "pb1u4nt0_ld_tt1.vso",
        "pu4nt0_ld_tt2.vso",
        "pb1u4nt0_ld_tt2.vso",
        "pu4nt0_ld_tt3.vso",
        "pb1u4nt0_ld_tt3.vso",
    };


    for(int i = 0; i < 6; i++)
    {
        sprintf(path, "%s%s", path_info->GetPath(PATH_TYPE_SHADER), shader_file[i]);

        if(LW_FAILED(shader_mgr->RegisterVertexShader(shader_type[i], path, 0, decl_tab[i], decl_size[i], 1)))
        {
            LG_MSGBOX("Load Vertex Shader Error\n%s", shader_file[i]);
            goto __ret;
        }
    }

#endif


    ret = LW_RET_OK;
__ret:
    if(ret != LW_RET_OK)
    {
        LG_MSGBOX("LoadShader1 error");
    }

    return ret;
}



// DXEffectFile.cpp: implementation of the CMPEffectFile class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
//#include "../../../proj/EffectEditer.h"
//#include <mindpower.h>
#include "GlobalInc.h"

#include "EffectFile.h"
//#include "DXUtil.h"
#include "MPRender.h"
#include "lwRenderBackend.h"
#include "lwD3D11Gaps.h"
#include "MindPowerRenderConfig.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CMPEffectFile::CMPEffectFile()
{
	m_pEffect = NULL;
	m_pDev = NULL;
	_vecTechniques.clear();
	_iTechNum = 0;
	_iCurTech = 0;
	_dwVShader = 0;
}
#ifdef USE_RENDER
CMPEffectFile::CMPEffectFile(MPRender*	 pDev)
#else
CMPEffectFile::CMPEffectFile(IDirect3DDeviceX* pDev)
#endif
{
    m_pDev = pDev;
    m_pEffect   = NULL;
	_vecTechniques.clear();
	_iTechNum = 0;
	_iCurTech = 0;
	_dwVShader = 0;

}
CMPEffectFile::~CMPEffectFile()
{
	free();
}
#ifdef USE_RENDER
void CMPEffectFile::InitDev(MPRender* pDev)
#else
void CMPEffectFile::InitDev(IDirect3DDeviceX* pDev)
#endif
{
    m_pDev = pDev;
}

BOOL CMPEffectFile::LoadEffectFromFile( LPCSTR pszfile)
{
	HRESULT hr;
	ID3DXBuffer* pErrorBuffer = NULL;
#ifdef USE_RENDER
#if !MINDPOWER_USE_D3D9_DEVICE
	if (!m_pDev) {
		return FALSE;
	}
	free();
	_iTechNum = 7;
	_vecTechniques.resize(7);
	for (int i = 0; i < 7; ++i)
		_vecTechniques[i] = (D3DXHANDLE)(INT_PTR)(i + 1);
	m_passes = 1;
	lwD3D11Gap(LW_D3D11_FALLBACK, "eff-fx-state-table",
		"shader\\eff.fx t0-t6 applied as DeviceObject FF states (no D3DX)");
	return TRUE;
#else
	if (MindPowerDx11OnlyBuild() || lwIsDx11Active() || !m_pDev || !m_pDev->GetDevice()) {
		free();
		_iTechNum = 7;
		_vecTechniques.resize(7);
		for (int i = 0; i < 7; ++i)
			_vecTechniques[i] = (D3DXHANDLE)(INT_PTR)(i + 1);
		m_passes = 1;
		lwD3D11Gap(LW_D3D11_FALLBACK, "eff-fx-state-table",
			"shader\\eff.fx t0-t6 applied as DeviceObject FF states (no D3DX)");
		return TRUE;
	}
	hr = D3DXCreateEffectFromFile(m_pDev->GetDevice(), pszfile, NULL, NULL, 0, NULL, &m_pEffect, &pErrorBuffer);
#endif
#else
	hr = D3DXCreateEffectFromFile(m_pDev, pszfile, &m_pEffect, &pErrorBuffer);
#endif
#if MINDPOWER_USE_D3D9_DEVICE
	if(FAILED(hr))
	{
		char errorMsg[1024];
		if(pErrorBuffer)
		{
			sprintf(errorMsg, "Effect load failed: %s\n\nError: %s", pszfile, (char*)pErrorBuffer->GetBufferPointer());
			pErrorBuffer->Release();
		}
		else
		{
			sprintf(errorMsg, "Effect load failed: %s\n\nHRESULT: 0x%08X", pszfile, hr);
		}
		MessageBox(NULL, errorMsg, "D3DX Effect Error", MB_OK | MB_ICONERROR);
		return FALSE;
	}
	if(pErrorBuffer)
		pErrorBuffer->Release();

	D3DXHANDLE   technique;

	//if (FAILED(m_pEffect->FindNextValidTechnique(NULL, &technique)))
	//{
	//	return FALSE;
	//}
	//_vecTechniques.push_back(technique);
	//_iTechNum++;

	//_DbgOut( " technique.Name", _iTechNum, S_OK,  (TCHAR*)technique.Name );

	char t_psz[4];
	strcpy(t_psz, "t0");
	while(SUCCEEDED(m_pEffect->FindNextValidTechnique(t_psz, &technique)))
	{
		_vecTechniques.push_back(technique);
		_iTechNum++;
		//_DbgOut( " technique.Name", _iTechNum, S_OK,  (TCHAR*)technique.Name );

		sprintf(t_psz,"t%d",_iTechNum);
	}
	return TRUE;
#else
	return TRUE;
#endif
}

//BOOL CMPEffectFile::LoadEffectFromResource(TCHAR*  pszsrc)
//{
//    HRESULT hr;
//    HMODULE hModule = NULL;
//    HRSRC rsrc;
//    HGLOBAL hgData;
//    LPVOID pvData;
//    DWORD cbData;
//	D3DXTECHNIQUE_DESC   technique;
//
//    rsrc = FindResource( hModule, pszsrc, "EFFECT" );
//    if( rsrc != NULL )
//    {
//        cbData = SizeofResource( hModule, rsrc );
//        if( cbData > 0 )
//        {
//            hgData = LoadResource( hModule, rsrc );
//            if( hgData != NULL )
//            {
//                pvData = LockResource( hgData );
//                if( pvData != NULL )
//                {
//                    if( FAILED( hr = D3DXCreateEffect( 
//                        m_pDev, pvData, cbData, 
//						&m_pEffect,NULL) ) )
//                    {
//                        return FALSE;
//                    }
//					else
//						return TRUE;
//                }
//            }
//        }
//    }
//	if (FAILED(m_pEffect->FindNextValidTechnique(NULL, &technique)))
//	{
//		return FALSE;
//	}
//	_vecTechniques.push_back(technique);
//	_iTechNum++;
//
//	while(SUCCEEDED(m_pEffect->FindNextValidTechnique(technique.Name, &technique)))
//	{
//		_vecTechniques.push_back(technique);
//		_iTechNum++;
//	}
//
//	return FALSE;
//}


void CMPEffectFile::free()
{
	SAFE_RELEASE(m_pEffect);
	_vecTechniques.clear();
	_iTechNum = 0;
	_dwVShader = 0;
//	if(_dwVShader)
//	{
//		m_pDev->DeleteVertexShader(_dwVShader);
//		_dwVShader = 0;
//	}
}

BOOL CMPEffectFile::OnLostDevice()
{
#if MINDPOWER_USE_D3D9_DEVICE
    if(m_pEffect)
	{
		if(FAILED(m_pEffect->OnLostDevice()))
			return FALSE;
	}
#endif
	return TRUE;
}

BOOL CMPEffectFile::OnResetDevice()
{
#if MINDPOWER_USE_D3D9_DEVICE
    if(m_pEffect)
	{
		if(FAILED(m_pEffect->OnResetDevice()))
			return FALSE;
	}
#endif
	return TRUE;
}
#ifdef USE_RENDER
MPRender*	CMPEffectFile::GetDev()
#else
IDirect3DDeviceX*	CMPEffectFile::GetDev()
#endif
{
	return m_pDev;
}

void CMPEffectFile::ApplySoftPass()
{
	if (!m_pDev)
		return;

	// Documented FF subset of client/shader/eff.fx (PixelShader=NULL).
	// Index i is technique ti. Src/Dest blend is left alone except t5/t6;
	// model/particle code sets those after Pass().
	const int tech = _iCurTech;
	const int zenable = (tech == 5 || tech == 6) ? FALSE : TRUE;
	const int zwrite = (tech == 1) ? TRUE : FALSE;
	const int alphablend = (tech == 1) ? FALSE : TRUE;
	const int alphatest = (tech == 4) ? TRUE : FALSE;
	const int lighting = FALSE;
	const int specular = (tech == 0 || tech == 1) ? TRUE : FALSE;
	const int cull = (tech == 5 || tech == 6) ? D3DCULL_CCW : D3DCULL_NONE;
	const int clamp_uv = (tech == 2 || tech == 3 || tech == 5) ? 1 : 0;
	const int tfactor_arg = (tech == 3) ? 1 : 0;
	const int point_filter = (tech == 5) ? 1 : 0;
	const int set_filter = (tech == 1 || tech == 3) ? 0 : 1;

	m_pDev->SetRenderState(D3DRS_ZENABLE, zenable);
	m_pDev->SetRenderState(D3DRS_ZWRITEENABLE, zwrite);
	m_pDev->SetRenderState(D3DRS_LIGHTING, lighting);
	m_pDev->SetRenderState(D3DRS_FOGENABLE, FALSE);
	m_pDev->SetRenderState(D3DRS_STENCILENABLE, FALSE);
	m_pDev->SetRenderState(D3DRS_DITHERENABLE, FALSE);
	m_pDev->SetRenderState(D3DRS_SPECULARENABLE, specular);
	m_pDev->SetRenderState(D3DRS_CULLMODE, cull);
	m_pDev->SetRenderState(D3DRS_ALPHATESTENABLE, alphatest);
	if (alphatest)
	{
		m_pDev->SetRenderState(D3DRS_ALPHAREF, 0xff000000);
		m_pDev->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_NOTEQUAL);
	}
	m_pDev->SetRenderState(D3DRS_ALPHABLENDENABLE, alphablend);
	if (tech == 5 || tech == 6)
	{
		m_pDev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
		m_pDev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
		m_pDev->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_FLAT);
	}
	if (tech == 5)
		m_pDev->SetRenderState(D3DRS_CLIPPING, FALSE);
	m_pDev->SetRenderState(D3DRS_VERTEXBLEND, D3DVBF_DISABLE);

	m_pDev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
	m_pDev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	m_pDev->SetTextureStageState(0, D3DTSS_COLORARG2, tfactor_arg ? D3DTA_TFACTOR : D3DTA_DIFFUSE);
	m_pDev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
	m_pDev->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
	m_pDev->SetTextureStageState(0, D3DTSS_ALPHAARG2, tfactor_arg ? D3DTA_TFACTOR : D3DTA_DIFFUSE);
	m_pDev->SetTextureStageStateForced(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
	m_pDev->SetTextureStageStateForced(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
	m_pDev->SetTextureStageStateForced(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
	m_pDev->SetTextureStageStateForced(1, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
	m_pDev->SetTexture(1, NULL);

	m_pDev->SetSamplerState(0, D3DSAMP_ADDRESSU, clamp_uv ? D3DTADDRESS_CLAMP : D3DTADDRESS_WRAP);
	m_pDev->SetSamplerState(0, D3DSAMP_ADDRESSV, clamp_uv ? D3DTADDRESS_CLAMP : D3DTADDRESS_WRAP);
	if (set_filter)
	{
		const DWORD filt = point_filter ? D3DTEXF_POINT : D3DTEXF_LINEAR;
		m_pDev->SetSamplerState(0, D3DSAMP_MINFILTER, filt);
		m_pDev->SetSamplerState(0, D3DSAMP_MAGFILTER, filt);
	}
}


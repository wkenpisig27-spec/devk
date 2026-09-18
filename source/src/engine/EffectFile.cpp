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
#include "lwD3D11Mesh.h"
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
	const char* hlsl = "shader\\eff.hlsl";
	if (!lwD3D11MeshCompileEff(hlsl))
		lwD3D11MeshCompileEff(pszfile);
	lwD3D11Gap(LW_D3D11_INVENTORY, "eff-fx-hlsl",
		"shader\\eff.hlsl compiled for t0-t6 (OM still from Pass)");
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

void CMPEffectFile::ApplySoftEnd()
{
	lwD3D11MeshSetEffTech(-1);
}

void CMPEffectFile::ApplySoftPass()
{
	// Native kEffPass + compiled eff.hlsl. Model/particle code may still set
	// dest-blend / TFACTOR after Pass(); those stay on the device cache.
	lwD3D11MeshSetEffTech(_iCurTech);
}


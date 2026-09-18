#include "stdafx.h"
#include "mygraph.h"
#include "MindPowerRenderConfig.h"
#if !MINDPOWER_USE_D3D9_DEVICE
#include "lwDeviceObject11.h"
#include "lwD3D11Texture.h"
#endif

IDirect3DDeviceX* g_pd3dDevice     = NULL; // Our rendering device


LPTEXTURE LoadTextureFromRawFile(char *strFileName)
{
	FILE *fp = fopen(strFileName , "rb");
	if(fp==NULL)
	{	
		Log("Open File %s Error!(LoadTextureFromRawFile())\n" , strFileName);
		return NULL;
	}
	
	int w , h , d;
  	
	fread(&w , 4 , 1 , fp);
	fread(&h , 4 , 1 , fp);
	fread(&d , 4 , 1 , fp);
    
	Log("load raw file( w = %3d" , w); 
	Log("  h = %3d" , h); 
	Log("  d = %3d )\n" , d);

	int iImageSize = w * h * d;
	LPBYTE pbImageBuf = new BYTE[iImageSize];
	fread(pbImageBuf , iImageSize , 1 , fp);
	fclose(fp);
	
	IDirect3DTextureX* pTexture;
#if MINDPOWER_USE_D3D9_DEVICE
	if(D3DXCreateTexture(g_pd3dDevice , w , h , D3DX_DEFAULT , 0 , D3DFMT_A8R8G8B8 ,  D3DPOOL_MANAGED , &pTexture)!=D3D_OK)                              
#else
	MindPower::lwDeviceObject11* d11 = MindPower::lwGetActiveDeviceObject11();
	if(!d11 || !d11->GetD3D11Device() ||
		MindPower::lwD3D11CreateEmptyTexture(d11->GetD3D11Device(), (UINT)w, (UINT)h, D3DFMT_A8R8G8B8, &pTexture) != LW_RET_OK)
#endif
	{
		Log("ERR : %s\n" , "CreateTexture Failed");
		delete[] pbImageBuf;
		return NULL;
	}
	
	
	int x = 0 , y = 0;
	
	LPBYTE pbData = pbImageBuf + w * d * (h - 1);
	D3DLOCKED_RECT rcLock;
	if((pTexture->LockRect(0 , &rcLock , 0 , 0))==D3D_OK)
	{
		LPDWORD pdwTex = (LPDWORD)rcLock.pBits;
		int iPitch = rcLock.Pitch / 4;
		for(y = 0; y < h ; y++)
		{
			for(x = 0 ; x < w ; x++)
			{
				DWORD dwR = 0 , dwG = 0 , dwB = 0;
				dwR = *(pbData + 0); dwR<<=16;
				dwG = *(pbData + 1); dwG<<=8;
				dwB = *(pbData + 2); 
				*(pdwTex + x) = dwR|dwG|dwB;
				pbData+=d;
			}
			pbData-=(w * d * 2);
			pdwTex+=iPitch;
		}
		pTexture->UnlockRect(0);
	}
	else
	{
		Log("ERR : %s\n" , "Lock Texture Failed!");
		SAFE_RELEASE(pTexture);
	}
	
	delete[] pbImageBuf;
	return pTexture;
}



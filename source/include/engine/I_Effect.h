#pragma once
#define		USE_MGR
#define		USE_RENDER

//#define		USE_GAME


//#define		USE_DDS_FILE USE_DDS_FILE_EFFECT

//#ifdef USE_DDS_FILE
//#undef USE_DDS_FILE 

//#endif

#include <Util.h>
//stl
#include <string>
#include <list>
#include <map>
#include <vector>
#include <algorithm>
#include <cassert>

#include "lwDirectX.h"

class   MPRender;
class   CMPResManger;

//--------------------------------------------------------------------------------------
//���õ�struct or type.
//--------------------------------------------------------------------------------------
#define    s_string								std::string          
#define    LIST_string							std::list<s_string>  
#define    VEC_string							std::vector<s_string>
#define    LIST_int								std::list<int>       
#define    LIST_dword							std::list<DWORD>     
#define    VEC_dword							std::vector<DWORD>   
#define    VEC_word								std::vector<WORD>    
#define    VEC_int								std::vector<int>     
#define    VEC_bool								std::vector<bool>    
#define    VEC_ptr								std::vector<DWORD_PTR>   // x64 fix: pointers need 8 bytes
#define    VEC_BYTE								std::vector<BYTE>    
#define    VEC_float							std::vector<float>   

/////////////////////////////////////////////////////////////////////////
template<class _Ty>	class S_BVECTOR
{
	std::vector<_Ty>	m_VECPath;
	int m_nCount;
	int m_nPos;
public:
	S_BVECTOR()
	{
		m_VECPath.clear();
		m_nCount =0;
		m_nPos = 0;
	}
	void resize(WORD _nSize = 100)
	{
		m_VECPath.clear();
		m_VECPath.resize(_nSize);
		clear();
	}
	void addsize(WORD _nSize = 100)
	{
		WORD size = (WORD)m_VECPath.size();
		m_VECPath.resize(size + _nSize);
	}
	void setsize(WORD _nSize = 1)
	{		
		if( m_VECPath.size() < _nSize )
		{
			m_VECPath.resize( _nSize );
			//setsizeNew( _nSize );
		}
		clear();
		m_nCount = _nSize;
	}
	void setsizeNew(WORD _nSize = 1)
	{
		std::vector<_Ty> temp;
		temp.resize( m_nCount );
		for( WORD i = 0; i < m_nCount; i++ )
		{
			temp[i] = m_VECPath[i];
		}
		
		m_VECPath.resize( _nSize );
		for( WORD i = 0; i < m_nCount; i++ )
		{
			m_VECPath[i] = temp[i];
		}
	}
	void clear()
	{
		m_nCount = 0;
		m_nPos = 0;
	}
	void push_back(_Ty &_Base)
	{
		m_VECPath[m_nCount] = _Base;
		++m_nCount;
	}
	void pop_front()
	{
		if(m_nCount > m_nPos)
		{
			++m_nPos;
			if(m_nPos == m_nCount)
			{
				clear();
			}
		}
	}
	void pop_back()
	{
		if(m_nCount > m_nPos)
		{
			m_nCount--;
			if(m_nPos == m_nCount)
			{
				clear();
			}
		}
	}
	int size() const {
 		return m_nCount - m_nPos;
 	}
	
	bool empty() const {
 		return m_nPos == m_nCount ? true : false;
 	}
	
	_Ty* front()
	{
		if(!empty())
			return &m_VECPath[m_nPos];
		return NULL;
	}
	_Ty* next()
	{
		if(!empty())
		{
			if( (m_nPos + 1) != m_nCount)
				return &m_VECPath[m_nPos + 1];
		}
		return NULL;
	}
	_Ty* end()
	{
		if(!empty())
		{
			return &m_VECPath[m_nCount - 1];
		}
		return NULL;
	}
	_Ty* operator[] ( int i )
	{
		if(!empty() && i >= 0 && i < size())
		{
			return &m_VECPath[m_nPos + i];
		}
		return NULL;
	}
	void remove(int _iIndex)
	{
		if(empty())
			return;
		int ipos = m_nPos + _iIndex;

			for(int n = ipos; n < m_nCount-1; ++n)
			{
				m_VECPath[n] = m_VECPath[n+1];
			}
			m_nCount--;
			if(m_nPos == m_nCount)
			{
				clear();
			}
			return;
	}
};

class CMemoryBuf
{
public:
	CMemoryBuf()
	{
		_pData = NULL;
		_lpos = 0;
		_size = 0;
	}
	~CMemoryBuf()
	{
		SAFE_DELETE_ARRAY(_pData);
	}
	bool	LoadFile(char* pszName)
	{
		FILE* t_pFile;
		t_pFile = fopen(pszName, "rb");
		if(!t_pFile)
			return false;
		fseek(t_pFile,0,SEEK_END);
		_size = ftell(t_pFile);
		SAFE_DELETE_ARRAY(_pData);
		_pData = new BYTE[_size];
		fseek(t_pFile,0,SEEK_SET);
		fread(_pData,sizeof(BYTE),_size,t_pFile);
		fclose(t_pFile);
		_lpos = 0;
		return true;
	}
	void	mseek(long ioffset, int ipos)
	{
		if(ipos == SEEK_CUR)
			_lpos += ioffset;
		if(ipos == SEEK_SET)
			_lpos = ioffset;
		if(ipos == SEEK_END)
			_lpos = _size;
	}
	long	mtell()
	{
		return _lpos;
	}
	void	mread(void* pmem,size_t psize, size_t pcount)
	{
		memcpy(pmem,&_pData[_lpos],pcount * psize);
		_lpos += (long)pcount * (long)psize;
	}

protected:
	long    _size;
	BYTE*	_pData;
	long	_lpos;
};
//#include <mindpower.h>

class   MPRender;
////////////////////////////////////////////////////////////////////
class	CMPResManger;
class	CEffectModel;
class	CTexCoordList;
class	CTexList;
class	I_Effect;
class   CEffPath;
class	EffParameter;

//class lwSceneItem ;
//!����󶥵�������Ϊ40����Ϊ��ʹ��VS���ı䶥��,����ʹ��������95 - 15
//ÿ�μ��������Ӱ�Ӷ����������ܴ������ֵ
#define  MAX_SHADER_VERNUM		300
#define  MAX_SHADER_IDXNUM		MAX_SHADER_VERNUM  * 3 * 3

/************************************************************************/
/* Ч��ģ�� */
/************************************************************************/
struct SEFFECT_VERTEX
{
	D3DXVECTOR3		m_SPos;
	FLOAT			m_fIdx;//ֻ������Ƭ��MESH����ΪUV���Ѱַ������
	DWORD			m_dwDiffuse;
	D3DXVECTOR2		m_SUV;
};
#define		EFFECT_VER_FVF	(D3DFVF_XYZB1 | D3DFVF_DIFFUSE | D3DFVF_TEX1)


struct SEFFECT_SHADE_VERTEX
{
	D3DXVECTOR3		m_SPos;
	//float			m_fIdx;//ֻ������Ӱ��MESH����Ϊ���Ѱַ������
							  //m_fIdx[0]Ϊ����λ�õ�������m_fIdx[1]Ϊuv���������
	DWORD			m_dwDiffuse;
	D3DXVECTOR2		m_SUV;
	D3DXVECTOR2		m_SUV2;
};
#define		EFFECT_SHADE_FVF	(D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX2)


#define		MESH_TRI		"Triangle"
#define		MESH_PLANETRI	"TrianglePlane"
#define		MESH_RECT		"Rect"
#define		MESH_RECTZ		"RectZ"
#define		MESH_PLANERECT	"RectPlane"

#define		MESH_CYLINDER	"Cylinder"
#define		MESH_CONE		"Cone"
#define		MESH_SPHERE		"Sphere"


inline	bool IsTobMesh(const s_string& strName)
{
	return ((strName==MESH_CYLINDER)||(strName==MESH_CONE)||(strName==MESH_SPHERE));
}
inline	bool IsCylinderMesh(const s_string& strName)
{
	return (strName==MESH_CYLINDER);
}
inline	bool IsDefaultMesh(const s_string& strName)
{
	static s_string str[] = 
	{
		MESH_TRI		,
		MESH_PLANETRI	,
		MESH_RECT		,
		MESH_RECTZ		,
		MESH_PLANERECT	,
		MESH_CYLINDER	,
		MESH_CONE		,
		MESH_SPHERE		,
	};
	for (int n = 0; n < 8; ++n)
	{
		if(strName == str[n])
			return true;
	}
	return false;
}

#include "MPSceneItem.h"
#include "MPCharacter.h"

#define _MAX_STRING					256
struct ActionInfo
{
	short		m_sActionNO;	// �������
	short		m_sStartFrame;	// ��ʼ֡
	short		m_sEndFrame;	// ����֡

	short		m_sKeyFrameNum;	// �ؼ�֡����
	short		*m_sKeyFrame;	// �ؼ�֡��
};
struct SChaAction
{
	short		m_iCharacterType;	// ��ɫ����
	short		m_iMaxActionNum;	// ��ɫ���������(���ļ��ж����Ľ�ɫ���������)
	short		m_iActualActionNum;	// ��ɫ����Ч������(���ļ��ж����Ľ�ɫ�Ķ�������)
	ActionInfo*  m_SActionInfo;
};
class  MINDPOWER_API CScriptFile
{
private:
	FILE *m_fp;
	bool m_bRead;


	BYTE* m_pData;

	int   m_iSize;

	int   m_iP;

	bool  m_bReadMem;

	bool  m_bOpen;

public:

	CScriptFile();
	~CScriptFile();

	bool OpenFileRead( const char *filename );

	void CloseFile();

	bool ReadSection(  char *section );

	bool ReadLine( const char *name, char *value, int nSize );
	bool ReadLine( const char *name, int *value );
	bool ReadLine( const char *name, float *value );
	bool ReadLine( const char *name, double *value );


	bool ReadLine( const char *name, char* value );

	//////////////////////////////////////////////////////////////////////////
	bool		InitAction(const char *ptcsFileName);
	void		Free(void);

	void		GetMaxCharType(int *iMaxType) {*iMaxType = m_iMaxCharacterType;};
	bool		GetCharAction(int iCharType, SChaAction *SCharAct);

	static		CScriptFile		m_ctScript;
protected:
	short		m_iMaxCharacterType;		// ����ɫ������(���ļ��ж��������Ľ�ɫ���ͺ�)
	short		m_iActualCharacterType;		// ��Ч��ɫ������(���ļ��ж����Ľ�ɫ����)
	SChaAction	*m_SCharacterAction;

};
long StringGetT(char* out, long out_max, const char* in, long* in_from, const char* end_list, long end_len);
void StringSkipCompartmentT(const char* in, long* in_from, const char* skip_list, long skip_len);

//class  lwSceneItem;
LW_USING



struct ModelParam
{
	int   iSegments;
	float fHei;
	float fTopRadius;
	float fBottomRadius;
	std::vector<D3DXVECTOR3> vecVer;
	void	Create();
};

class  MINDPOWER_API CEffectModel : public MPSceneItem
{
public:
	CEffectModel();

	bool Copy(const CEffectModel& rhs);

#ifdef		USE_RENDER
	CEffectModel(MPRender*		  pDev,lwIResourceMgr*	pRes = NULL);
#else
	CEffectModel(IDirect3DDeviceX*  pDev,lwIResourceMgr*	pRes = NULL);
#endif
	virtual ~CEffectModel();

public:
	void	ReleaseModel();

#ifdef USE_RENDER
	void	InitDevice(MPRender*	  pDev,lwIResourceMgr*	pRes = NULL);
#else
	void	InitDevice(IDirect3DDeviceX*  pDev,lwIResourceMgr*	pRes = NULL);
#endif

	bool	CreateTriangle();
	bool	CreatePlaneTriangle();
	bool	CreateRect();
	bool	CreatePlaneRect();
	bool	CreateRectZ();

	//������Ӱģ��
	bool	CreateShadeModel(WORD wVerNum = 6, WORD wFaceNum = 2,int iGridCrossNum = 1,bool usesoft = false);

	//�����ɱ��ε�ģ��
	bool	CreateTob(const s_string& str, int nSeg,float fHei,float fTopRadius,float fBottomRadius)
	{
		if(str==MESH_CYLINDER)
			return CreateCylinder(nSeg,fHei,fTopRadius,fBottomRadius);
		if(str==MESH_CONE)
			return CreateCone(nSeg,fHei,fBottomRadius);
		return false;
	}
	bool	CreateCylinder(int nSeg,float fHei,float fTopRadius,float fBottomRadius);
	bool	CreateCone(int nSeg,float fHei,float fRadius);
	//bool	CreateSphere(int nSeg,float fHei,float fRadius);


	bool	IsBoard(){return (!IsChangeably() && !IsItem());}
	bool	IsChangeably()			{return m_bChangeably;}

	bool	IsItem()
	{	
#ifdef USE_MGR
		return _lwMesh ? false : true; 
#else
		return _lpVB ? false : true; 
#endif

	}
	bool	LoadModel(const char* pszName);

	void	Begin();

	void	FrameMove(DWORD dwDailTime);
	void	RenderModel();

	void	RenderTob(ModelParam* last, ModelParam* next, float lerp);

	void	End();

	DWORD	GetVerCount()				{ return _dwVerCount;}
	DWORD	GetFaceCount()				{ return _dwFaceCount;}


#ifdef USE_MGR
	lwILockableStreamIB*		GetIndexBuffer()	
	{
		return _lpSIB;
	}
#else
	IDirect3DIndexBufferX*		GetIndexBuffer()	
	{
		return _lpIB;
	}
#endif

#ifdef USE_MGR
	lwILockableStreamVB*		GetVertexBuffer()	
	{
		return _lpSVB;
	}
#else
	IDirect3DVertexBufferX*		GetVertexBuffer()	
	{
		return _lpVB;
	}
#endif

	void						Lock(BYTE** pvEffVer)
	{
#ifdef USE_MGR
		if (_lpSVB == 0)
		{
			*pvEffVer = 0;
			return;
		}

		if(LW_FAILED(_lpSVB->Lock(0, 0, (void**)pvEffVer, 0)))
		{
			MessageBox(NULL, "lock error msglock error", "error", 0);
			*pvEffVer = 0;
			assert(false);
		}
#else
		_lpVB->Lock(0, 0, pvEffVer, 0 );
#endif
	}
	void						Unlock()
	{ 
#ifdef USE_MGR
		_lpSVB->Unlock();
#else
		_lpVB->Unlock();
#endif
	}

	void						LockIB(BYTE** pIdx)
	{
#ifdef USE_MGR
		if(LW_FAILED(_lpSIB->Lock(0, 0, (void**)pIdx, 0)))
		{
			MessageBox(NULL, "lock error msglock error", "error", 0);
			assert(false);
		}
#else
		_lpIB->Lock(0, 0, pIdx, 0 );
#endif
	}
	void						UnlockIB()
	{ 
#ifdef USE_MGR
		_lpSIB->Unlock();
#else
		_lpIB->Unlock();
#endif
	}
	void						SetRenderNum(WORD wVer,WORD wFace);

#ifdef USE_RENDER
	MPRender*	GetDev();
#else
	IDirect3DDeviceX*	GetDev();
#endif
public:
	//!3D�豸
#ifdef USE_RENDER
	MPRender*					m_pDev;
#else
	IDirect3DDeviceX*			m_pDev;
#endif

	lwIResourceMgr*				m_pRes;

	s_string					m_strName;

	bool						m_bChangeably;

	int							m_nSegments;
	float						m_rHeight;
	float						m_rRadius;
	float						m_rBotRadius;

	SEFFECT_VERTEX*				m_vEffVer;

	//MPSceneItem*				m_pModel;
	lwITex*						m_oldtex;
	lwITex*						m_oldtex2;
	bool						m_bItem;

	int							m_iID;

	bool						m_bUsing;

protected:
#ifdef USE_MGR
	lwIMesh*					_lwMesh;
	lwILockableStreamVB*		_lpSVB;
	lwILockableStreamIB*		_lpSIB;

#else
	IDirect3DIndexBufferX*		_lpIB;
	IDirect3DVertexBufferX*		_lpVB;
#endif

	DWORD						_dwVerCount;
	DWORD						_dwFaceCount;

	//CChaModel*					_pChaModel;

public:
	// Getters and Setters
	bool IsUsing() { return m_bUsing; }
	void SetUsing(bool bUsing) { m_bUsing = bUsing; }
};

/************************************************************************/
/* ��������任*/
/************************************************************************/
typedef     std::vector<D3DXVECTOR2>  TEXCOORD;

class CTexCoordList
{
public:
	CTexCoordList();
	~CTexCoordList();

public:
	void		GetCoordFromModel(CEffectModel *pCModel);
	void		CreateTranslateCoord();

	void		GetCurCoord(S_BVECTOR<D3DXVECTOR2>& vecOutCoord, WORD& wCurIndex,float &fCurTime, float fDailTime);

	void		Reset();

	void		Clear();

	void		Copy(CTexCoordList* pList)
	{
		m_wVerCount = pList->m_wVerCount;
		//����������.
		m_wCoordCount = pList->m_wCoordCount;
		//��������任ʱ��
		m_fFrameTime = pList->m_fFrameTime;

		m_vecCoordList.resize(m_wCoordCount);

		//��������任����
		int n;
		for(n = 0; n < m_wCoordCount; ++n)
		{
			m_vecCoordList[n].resize(m_wVerCount);
			m_vecCoordList[n] =  pList->m_vecCoordList[n];
		}
	}
public:
	//!��������
	WORD		m_wVerCount;
	//����������.
	WORD		m_wCoordCount;
	//��������任ʱ��
	float		m_fFrameTime;
	//��������任����
	std::vector<TEXCOORD>	m_vecCoordList;

	////!��ǰ��������
	//WORD					m_wCurIndex;
	////!��ǰʱ��
	//float					m_fCurTime;
	//!��ǰ������
	//TEXCOORD				m_vecCurCoord;
};

/************************************************************************/
/* �����任*/
/************************************************************************/
class CTexList
{
public:
	CTexList();
	~CTexList();

public:
	void					SetTextureName(const s_string&  pszName);

	void					GetTextureFromModel(CEffectModel *pCModel);

	void					CreateSpliteTexture(int iRow, int iColnum);
	
	void					GetCurTexture(S_BVECTOR<D3DXVECTOR2>& coord, WORD&  wCurIndex,float& fCurTime, float fDailTime);
	void					Reset();

	void					Clear();

	void					Remove();

	void					Copy(CTexList* pList)
	{
		m_wTexCount = pList->m_wTexCount;
		//�����任ʱ��
		m_fFrameTime = pList->m_fFrameTime;
		//�����任����
		m_vecTexList.resize(m_wTexCount);
		int n;
		for(n = 0; n < m_wTexCount; ++n)
		{
			m_vecTexList[n].resize(4);
			m_vecTexList[n] =  pList->m_vecTexList[n];
		}

		//!������
		m_vecTexName = pList->m_vecTexName;
		//!��ǰ������ָ��
		m_lpCurTex = NULL;

		m_pTex = NULL;
	}
public:
	//������
	WORD		m_wTexCount;
	//�����任ʱ��
	float		m_fFrameTime;
	//�����任����
	std::vector<TEXCOORD>	m_vecTexList;

	//!������
	s_string	m_vecTexName;
	//!��ǰ������ָ��
	IDirect3DTextureX*		m_lpCurTex;

	lwITex*					m_pTex;

};
/************************************************************************/
/* �����任*/
/************************************************************************/
class CTexFrame
{
public:
	CTexFrame();
	~CTexFrame();

public:
	void					GetCoordFromModel(CEffectModel *pCModel);

	void					AddTexture(const s_string&  pszName);

	lwITex*					GetCurTexture(WORD&  wCurIndex,float& fCurTime, float fDailTime);

	void					Remove();

	void					Copy(CTexFrame* pList)
	{
		m_wTexCount = pList->m_wTexCount;
		m_fFrameTime = pList->m_fFrameTime;
		m_fFrameTime = pList->m_fFrameTime;

		m_vecTexName.resize(m_wTexCount);
		m_vecTexs.resize(m_wTexCount);
		int n;
		for (n = 0; n < m_wTexCount; ++n)
		{
			m_vecTexName[n] = pList->m_vecTexName[n];
		}
		m_vecCoord.resize(pList->m_vecCoord.size());
		m_vecCoord = pList->m_vecCoord;
	}
public:
	//������
	WORD		m_wTexCount;
	//�����任ʱ��
	float		m_fFrameTime;
	//!������
	std::vector<s_string>	m_vecTexName;
	//!��ǰ������ָ��
	lwITex*					m_lpCurTex;

	std::vector<lwITex*>	m_vecTexs;

	TEXCOORD				m_vecCoord;
};

class CEffectFont : public CTexList, CEffectModel
{
public:
	CEffectFont();
	~CEffectFont();

public:
#ifdef USE_RENDER
	bool	CreateEffectFont(MPRender*  pDev,
		CMPResManger	*pCResMagr,int iTexID,D3DXCOLOR dwColor, bool bUseBack = false,bool bmain =false);
#else
	bool	CreateEffectFont(IDirect3DDeviceX*  pDev,
		CMPResManger	*pCResMagr,int iTexID,D3DXCOLOR dwColor, bool bUseBack = false,bool bmain = false);
#endif
	
	void	SetRenderText(char* pszText);

	void	RenderEffectFont(D3DXMATRIX* pmat);
	void	RenderEffectFontBack(D3DXMATRIX* pmat);

protected:
	s_string	_strText;
	int			_iTextNum;
	std::vector<int>	_vecCurText;
	int			_iTextureID;

	//����ͼ
	bool		_bUseBack;
	s_string	_strBackBmp;
#ifdef USE_RENDER
	lwITex*					_lpBackTex;
#else
	IDirect3DTextureX*		_lpBackTex;
#endif

//#ifdef USE_MGR
//	lwIMesh*					_lwBackMesh;
//#else
//	LPDIRECT3DVERTEXBUFFER8	_lpBackVB;
//#endif
	SEFFECT_VERTEX			t_SEffVer[4];


	D3DXCOLOR				_dwColor;

};

enum  EFFECT_TYPE
{
	EFFECT_NONE			= 0,
	EFFECT_FRAMETEX		= 1,
	EFFECT_MODELUV		= 2,
	EFFECT_MODELTEXTURE	= 3,
	EFFECT_MODEL		= 4,
};
#define ENUMTOSTR(s)  #s

/************************************************************************/
/* Effect base class */
/************************************************************************/
class MINDPOWER_API I_Effect
{
public:
	I_Effect(void);
	~I_Effect(void);
public:

	void DestroyTobMesh(CMPResManger* resMgr);

	//!	�ͷ�ȫ����Դ
	virtual void ReleaseAll();
	//!
	virtual void Reset();
	//����ʼ��
#ifdef USE_RENDER
	virtual	void Init(MPRender*	 pDev,EFFECT_TYPE  eType,
		WPARAM wParam, LPARAM lParam);
#else
	virtual	void Init(IDirect3DDeviceX*	 pDev,EFFECT_TYPE  eType,
		WPARAM wParam, LPARAM lParam);
#endif

	void		 SetTexture();			
	//{ 
	//	if(m_pCModel->IsItem())
	//	{
	//		lwITex* tex;
	//		m_pCModel->ResetItemTexture(m_CTextruelist.m_pTex,&tex);
	//	}else
	//		m_pDev->SetTexture(0, m_CTextruelist.m_lpCurTex);
	//}
	void		 SetVertexShader();//		{ m_pDev->SetVertexShader(*_pdwVShader);}
	//!��Ⱦ
	virtual void Begin()
	{
		if (m_pCModel)
			m_pCModel->Begin();
	}
	virtual void Render();				
	//{
	//	m_pDev->SetRenderState( D3DRS_SRCBLEND,_eSrcBlend );
	//	m_pDev->SetRenderState( D3DRS_DESTBLEND,_eDestBlend);
	//	m_pCModel->RenderModel();
	//}
	virtual void End()								
	{
		if (m_pCModel)
			m_pCModel->End();
	}

//!��Ա����
public:
	//! �õ�����������
	WORD			getFrameCount(){ return _wFrameCount;}
	WORD			setFrameCount(WORD  wFrameCount){ return _wFrameCount = wFrameCount;}

	//!�õ�����
	EFFECT_TYPE		getType()						{ return _eEffectType;}
	void			setType(EFFECT_TYPE eType)		{ _eEffectType =  eType;}

	//!�õ�����
	float			getLength()						{ return _fLength;}
	void			setLength(float fLength)		{ _fLength = fLength;}

	//!�õ�֡ʱ��
	float			getFrameTime(WORD wIndex)		{ return _vecFrameTime[wIndex];}
	void			setFrameTime(WORD wIndex,float fTime){ _vecFrameTime[wIndex] = fTime;}

	//!�õ�֡��С
	D3DXVECTOR3		getFrameSize(WORD wIndex)		{ return _vecFrameSize[wIndex];}
	void			setFrameSize(WORD wIndex,D3DXVECTOR3& SVerSize){ _vecFrameSize[wIndex] = SVerSize;}

	//!�õ�֡�Ƕ�
	D3DXVECTOR3&	getFrameAngle(WORD wIndex)		{ return _vecFrameAngle[wIndex];}
	void			setFrameAngle(WORD wIndex,D3DXVECTOR3& SVerAngle){_vecFrameAngle[wIndex]=SVerAngle;}

	//!�õ�֡λ��
	D3DXVECTOR3&	getFramePos(WORD wIndex)		{ return _vecFramePos[wIndex];}
	void			setFramePos(WORD wIndex,D3DXVECTOR3& SVerPos){_vecFramePos[wIndex]=SVerPos;}

	//!�õ�֡��ɫ
	D3DXCOLOR&		getFrameColor(WORD wIndex)		{ return _vecFrameColor[wIndex];}
	void			setFrameColor(WORD wIndex,D3DXCOLOR& SVerColor){_vecFrameColor[wIndex]=SVerColor;}

	//!�õ�֡����ʱ��
	float	getFrameCoordTime()						{ return m_CTexCoordlist.m_fFrameTime; }
	void    setFrameCoordTime(float fTime)			{ m_CTexCoordlist.m_fFrameTime = fTime;}

	int		getFrameCoordCount()					{ return (int)m_CTexCoordlist.m_vecCoordList.size(); }
	void	setFrameCoordCount(int iNum)					
	{ 
		m_CTexCoordlist.m_wCoordCount = iNum;
		m_CTexCoordlist.m_vecCoordList.resize(m_CTexCoordlist.m_wCoordCount);
	}

	//!�õ�֡����
	void	getFrameCoord(TEXCOORD& vecOutCoord, WORD wIndex)
	{
		vecOutCoord.clear();
		vecOutCoord.resize(m_CTexCoordlist.m_wVerCount);
		for(WORD n = 0; n < m_CTexCoordlist.m_wVerCount; ++n)
		{
			vecOutCoord[n] = m_CTexCoordlist.m_vecCoordList[wIndex][n];
		}
	}
	void	setFrameCoord(TEXCOORD& vecInCoord, WORD wIndex)
	{
		//for(WORD n = 0; n < m_CTexCoordlist.m_wVerCount; ++n)
		//{
		//	m_CTexCoordlist.m_vecCoordList[wIndex][n] = vecInCoord[n];
		//}
		m_CTexCoordlist.m_vecCoordList[wIndex] = vecInCoord;
	}

	//!�õ�֡����
	TEXCOORD& getFrameTexture(WORD  wIndex)
	{
		return m_CTextruelist.m_vecTexList[wIndex];
	}
	void	setFrameTexture(WORD  wIndex,TEXCOORD& lptex)
	{
		m_CTextruelist.m_vecTexList[wIndex] = lptex;
	}
	void    SpliteTexture(int iRow, int iCol);
	void	SetTextureTime(float ftime);

	///////////////////////////////////////////////////////////
	bool	 IsModelRect()		{ return m_strModelName == MESH_RECT;}
	bool	 IsModelPlaneRect()	{ return m_strModelName == MESH_PLANERECT;}
	bool	 IsModelTri()		{ return m_strModelName == MESH_TRI;}

	bool	IsItem()
	{
		if(m_pCModel)
			return m_pCModel->IsItem();
		if(strstr( m_strModelName.c_str(),".lgo"))
			return true;
		return false;
	}
		//{ return m_pCModel->IsItem();}

	bool	IsChangeably();

	//////////////////////////////////////////////////////////////////////////
	//!�õ���ֵ��С
	void	GetLerpSize(D3DXVECTOR3 *pSOut, WORD wIdx1, WORD wIdx2, float fLerp)
	{
		if(_wFrameCount == 1 || _bSizeSame)
		{	*pSOut = _vecFrameSize[0];return;}
		D3DXVec3Lerp(pSOut, &_vecFrameSize[wIdx1], &_vecFrameSize[wIdx2], fLerp);
	}
	//!�õ���ֵ�Ƕ�
	void	GetLerpAngle(D3DXVECTOR3 *pSOut, WORD wIdx1, WORD wIdx2, float fLerp)
	{
		if(_wFrameCount == 1 || _bAngleSame)
		{	*pSOut = _vecFrameAngle[0];return;}
		D3DXVec3Lerp(pSOut, &_vecFrameAngle[wIdx1], &_vecFrameAngle[wIdx2], fLerp);
	}
	//!�õ���ֵλ��
	void	GetLerpPos(D3DXVECTOR3 *pSOut, WORD wIdx1, WORD wIdx2, float fLerp)
	{
		if(_wFrameCount == 1 || _bPosSame)
		{	*pSOut =   _vecFramePos[0];return; }
		D3DXVec3Lerp(pSOut, &_vecFramePos[wIdx1], &_vecFramePos[wIdx2], fLerp);
	}
	//!�õ���ֵ��ɫ
	void	GetLerpColor(D3DXCOLOR *pSOut, WORD wIdx1, WORD wIdx2, float fLerp)
	{
		if(_wFrameCount == 1 || _bColorSame)
		{	*pSOut =  _vecFrameColor[0];return; }
		D3DXColorLerp( pSOut, &_vecFrameColor[wIdx1], &_vecFrameColor[wIdx2], fLerp );
	}
	//!�õ���ֵ����
	void	GetLerpCoord(S_BVECTOR<D3DXVECTOR2>& vecOutCoord, WORD& wCurIndex,float &fCurTime, float fDailTime)
	{
		m_CTexCoordlist.GetCurCoord(vecOutCoord,wCurIndex,fCurTime,fDailTime);
	}
	void GetLerpTexture(S_BVECTOR<D3DXVECTOR2>& vecOutCoord, WORD&  wCurIndex,float& fCurTime, float fDailTime)
	{
		m_CTextruelist.GetCurTexture(vecOutCoord, wCurIndex,fCurTime,fDailTime);
	}
	void GetLerpFrame(WORD&  wCurIndex,float& fCurTime, float fDailTime)
	{
		m_CTexFrame.GetCurTexture(wCurIndex,fCurTime,fDailTime);
	}

	void GetLerpVertex(WORD wIdx1, WORD wIdx2, float fLerp)
	{
		m_ilast = wIdx1;
		m_inext = wIdx2;
		m_flerp = fLerp;
	}

	void GetRotaLoopMatrix(D3DXMATRIX* pmat, float& pCurRota, float fTime) {
		pCurRota += _vRotaLoop.w * fTime;
		if (pCurRota >= 6.283185f)
		{
			pCurRota = pCurRota - 6.283185f;
		}
		const auto v = D3DXVECTOR3(_vRotaLoop.x, _vRotaLoop.y, _vRotaLoop.z);
		D3DXMatrixRotationAxis(pmat, &v, pCurRota);
	}

	//////////////////////////////////////////////////////////////////////////
	//!��������Դ
	void		 BindingResInit(CMPResManger	*m_CResMagr);
	//!����0��ʾ����װ�룬1��ʾȱ����ͼ��2��ʾȱ��ģ�͡�3��ʾ��shade
	int 		 BoundingRes(CMPResManger	*m_CResMagr, const char* pszParentName = "temp");

	s_string	 GetTextureName()		{ return m_CTextruelist.m_vecTexName;}

	void		 SetTextureName(const s_string&  pszName)		{ m_CTextruelist.SetTextureName(pszName);}

	void		 SetModel(CEffectModel*	pCModel);
	//���滻����
	void		 ChangeTexture(const s_string&  pszName)
	{
		if(_eEffectType == EFFECT_FRAMETEX)
		{

		}else
		{
			SetTextureName(pszName);	
			m_CTextruelist.GetTextureFromModel(m_pCModel);
		}
	}
	//!�滻ģ��
	void		 ChangeModel(CEffectModel*	pCModel,CMPResManger	*pCResMagr);

	///////////////////////////////////////////////////////////////////////////
	//!�õ�Ч������
	s_string&	 getEffectName()							{ return m_strEffectName;}
	void		 setEffectName(const s_string& strName)		{ m_strEffectName = strName;}

	//!�õ�ģ������
	s_string&	 getEffectModelName();
	void		 setEffectModelName(const s_string& strModelName)	{ m_strModelName = strModelName;}

	//!����BILLBOARD����
	void		 setBillBoardMatrix(D3DXMATRIX* pMatBBoard)	{ _SpmatBBoard = pMatBBoard;}
	D3DXMATRIX*	 getBillBoardMatrix()						{ return _SpmatBBoard;}

	bool		 IsBillBoard()								{ return _bBillBoard;}

	void		 SetLoop(bool	bloop)						{ _bRotaLoop = bloop; }
	bool		 IsRotaLoop()								{ return _bRotaLoop;}
	D3DXVECTOR4* GetRotaLoop()								{ return &_vRotaLoop;}

	bool		 IsAlpah()									{ return _bAlpha;}
	void		 EnableAlpha(bool balpha)					{ _bAlpha = balpha;}


	D3DBLEND	GetAlphaTypeSrc()	{ return _eSrcBlend;}
	D3DBLEND	GetAlphaTypeDest()	{ return _eDestBlend;}

	void		SetAlphaType(D3DBLEND eSrcBlend, D3DBLEND eDestBlend)
									{ _eSrcBlend = eSrcBlend; _eDestBlend = eDestBlend;}

	//!����Ч�����ļ�	
	bool		 SaveToFile(FILE* pFile);
	//!װ��Ч�����ļ�
	bool		 LoadFromFile(FILE* pFile,DWORD dwVersion);

	void		 IsSame();

	void		 RemoveTexBack()
	{
		m_CTextruelist.Remove();
	}
	void		 AddFrameTex(const s_string& str)
	{
		if(_eEffectType == EFFECT_FRAMETEX)
		{
			m_CTexFrame.AddTexture(str);
		}
	}
	VEC_string&		 GetFrameTex()
	{
		//if(_eEffectType == EFFECT_FRAMETEX)
		{
			return m_CTexFrame.m_vecTexName;
		}
	}
	void			RemoveFrameTex()
	{
		m_CTexFrame.Remove();
	}
	
	void	InitTopParam()
	{
		if(_iUseParam > 0)
			return;
		_iUseParam = 1;
	}
	void	ClearTopParam(){ _iUseParam = 0; }
	void	SetTobParam(int nFrame, int	nSegments,float	rHeight,float rRadius,float	rBotRadius)
	{
		_iUseParam = 0;
		_CylinderParam[nFrame].iSegments = nSegments;
		_CylinderParam[nFrame].fTopRadius = rRadius;
		_CylinderParam[nFrame].fBottomRadius = rBotRadius;
		_CylinderParam[nFrame].fHei = rHeight;
		_CylinderParam[nFrame].Create();

		_iUseParam = 1;
	}
	void	GetTobParam(int nFrame, int&	nSegments,float&	rHeight,float& rRadius,float&	rBotRadius)
	{
		 nSegments = _CylinderParam[nFrame].iSegments;
		rRadius = _CylinderParam[nFrame].fTopRadius;
		rBotRadius = _CylinderParam[nFrame].fBottomRadius ;
		rHeight = _CylinderParam[nFrame].fHei;
	}
	int		IsUseParam(){ return _iUseParam;}

	bool	IsRotaBoard(){ return _bRotaBoard;}
	void	SetRoatBoard(bool bRota){ _bRotaBoard = bRota;}


	void	GetRes(CMPResManger	*pCResMagr,std::vector<INT>& vecTex,std::vector<INT>& vecModel);

	void	PlayModel(float velocity = 1.0f)
	{
		if (!m_pCModel) return;
		if(m_pCModel->IsItem())
			m_pCModel->PlayDefaultAnimation(velocity);
	}
	void	ResetModel();
	void	CopyEffect(I_Effect* pEff);

	void	DeleteItem(CMPResManger* pResMgr);

//����Ա����
public:
	//!3D�豸
#ifdef		USE_RENDER
	MPRender*					m_pDev;
#else
	IDirect3DDeviceX*			m_pDev;
#endif
	//!���������б�
	CTexCoordList				m_CTexCoordlist;
	//!�����б�
	CTexList					m_CTextruelist;
	//!����֡
	CTexFrame					m_CTexFrame;

	CEffectModel*				m_pCModel;
	s_string					m_strModelName;

	int							m_nSegments;
	float						m_rHeight;
	float						m_rRadius;
	float						m_rBotRadius;

	s_string					m_strEffectName;

	int							m_ilast,m_inext;
	float						m_flerp;
protected:
	//!	��������
	EFFECT_TYPE			_eEffectType;
	//��������ʱ�䳤��(��)
	float				_fLength; 
	//������������
	WORD				_wFrameCount;
	//! ÿһ���ʱ��
	VEC_float			_vecFrameTime;
	//!	ÿһ��Ĵ�С(���ŵı�����Ĭ��Ϊ1.0f)
	std::vector<D3DXVECTOR3>			_vecFrameSize;
	//!	ÿһ��ĽǶ�
	std::vector<D3DXVECTOR3>			_vecFrameAngle;
	//!	ÿһ���λ��
	std::vector<D3DXVECTOR3>			_vecFramePos;

	//!	ÿһ��Ķ�����ɫ(Ĭ��Ϊ0xffffffff)
	std::vector<D3DXCOLOR>				_vecFrameColor;

	//!	ÿһ��Ķ���任
	INT									_iUseParam;
	std::vector<ModelParam>				_CylinderParam;

	bool				_bBillBoard;
	D3DXMATRIX*			_SpmatBBoard;

	bool				_bRotaLoop;
	D3DXVECTOR4			_vRotaLoop;

	bool				_bRotaBoard;


	bool				_bAlpha;

	int					_iVSIndex;
	

	bool				_bSizeSame;
	bool				_bAngleSame;
	bool				_bPosSame;
	bool				_bColorSame;

	D3DBLEND			_eSrcBlend;
	D3DBLEND			_eDestBlend;
};

//class MINDPOWER_API CEffectBase
//{
//public:
//	CEffectBase(){}
//	~CEffectBase(){}
//public:
//	virtual void FrameMove(DWORD	dwDailTime)	{}
//	//!��Ⱦ
//	virtual void Render()						{}
//
//	virtual void RenderVS()						{}
//
//	virtual void RenderSoft()					{}
//
//
//protected:
//private:
//};

inline void Transpose(D3DMATRIX &result, D3DMATRIX &m)
{

	result.m[0][0] = m.m[0][0];  result.m[0][1] = m.m[1][0];  result.m[0][2] = m.m[2][0]; result.m[0][3] = m.m[3][0];
	result.m[1][0] = m.m[0][1];  result.m[1][1] = m.m[1][1];  result.m[1][2] = m.m[2][1]; result.m[1][3] = m.m[3][1];
	result.m[2][0] = m.m[0][2];  result.m[2][1] = m.m[1][2];  result.m[2][2] = m.m[2][2]; result.m[2][3] = m.m[3][2];
	result.m[3][0] = m.m[0][3];  result.m[3][1] = m.m[1][3];  result.m[3][2] = m.m[2][3]; result.m[3][3] = m.m[3][3];
}
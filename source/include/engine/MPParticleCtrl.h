#pragma once

#include "MPCamera.h"

#include "i_effect.h"

#include ".\mpparticlesys.h"

#define MAX_PART_SYS	20
class   MPRender;

class CMPLink;

class  CMPStrip;

class  CMPModelEff;

///************************************************************************/
///* class CMPParticleCtrl*/ 
///************************************************************************/
//class MINDPOWER_API CMPParticleCtrl: publ ic CEffectBase
//{
//public:
//	CMPParticleCtrl(void);
//	~CMPParticleCtrl(void);
//
//public:
//	virtual void				BindingRes(CMPResManger* pResMagr) {}
//
//	virtual void				FrameMove(DWORD	dwDailTime)		 {}
//
//	virtual void				Render()						 {}
//	virtual void				RenderVS()						 {}
//	virtual void				RenderSoft()					 {}
//
//	virtual void				Emission(WORD wID, XMVECTOR3 vBegin, XMVECTOR3 vEnd) {}
//
//public:
//
//	XMVECTOR3			m_CPath[2];
//
//	WORD				m_wID;
//
//
//	float*				m_pfDailTime;
//	bool				m_bEmiss;
//};
//
///************************************************************************/
///* class CMPParticleTrace*/
///************************************************************************/
//class MINDPOWER_API CMPParticleTrace :public CMPParticleCtrl
//{
//public:
//	CMPParticleTrace(void);
//	~CMPParticleTrace(void);
//
//public:
//	void				BindingRes(CMPResManger* pResMagr);
//
//	void				FrameMove(DWORD	dwDailTime);
//
//	void				Render();
//
//	void				Emission(WORD wID, XMVECTOR3 vBegin, XMVECTOR3 vEnd);
//
//public:
//	bool				IsPlaying()					{ return (m_bEmiss || m_CBlast->IsPlaying());}
//	bool				IsHit()						{ bool bhit = _bHit; _bHit = false; return bhit;}
//	void				SetTarget(XMVECTOR3 vTarget);
//
//	void				Blast();
//
//	void				MoveTo(XMVECTOR3 vPos,MPMap* pMap);
//public:
//	bool				_bHit;
//	CMPModelEff*		_pCModelEff;
//
//	CMPShadeMap*		_pCShade;
//
//	CMPFire*			m_CFire;
//
//	CMPBlast*			m_CBlast;
//
//
//	float				m_fTargDist;
//	XMVECTOR3			m_vTargDir;
//
//
//	XMVECTOR3			m_vPos;
//	float				m_fDist;
//	XMVECTOR3			m_vDir;
//
//	float				m_fVel;
//
//	float				m_fRadius;
//};
//
///************************************************************************/
///* class CMPParticleRipple*/
///************************************************************************/
//class MINDPOWER_API CMPParticleRipple :public CMPParticleCtrl
//{
//public:
//	CMPParticleRipple(void);
//	~CMPParticleRipple(void);
//
//public:
//	void				BindingRes(CMPResManger* pResMagr);
//
//	void				FrameMove(DWORD	dwDailTime);
//
//	void				Render();
//
//	void				Emission(WORD wID, XMVECTOR3 vBegin, XMVECTOR3 vEnd);
//
//public:
//	void				SetTarget(XMVECTOR3 vTarget, XMVECTOR3 vTarget2, float fAngle);
//	void				Walk();
//	void				Stop();
//public:
//	CMPRipple*			m_CRippleStop;
//
//	CMPRipple*			m_CRippleMove;
//
//	XMVECTOR3			m_vPos;
//};
//////////////////////////////////////////////////////////////////////////
#define		LINK_FVF	(D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1)
#define     LINK_FACE	100


class CMPLink
{
public:
	struct LinkVer
	{
		XMVECTOR3		m_SPos;
		DWORD			m_dwDiffuse;
		XMVECTOR2		m_SUV;
	};
	struct MPFrame
	{
		LinkVer	vPos1;
		LinkVer	vPos2;
	};
public:
	CMPLink()
	{
		_pChaMain = NULL;
		_pChaTag = NULL;
		_pTex = NULL;

		_pFrame = NULL;
		_pCEffFile = NULL;
		_fDailTime = NULL;

		_iCurTex = 0;
		_fCurTime = 0;
		//_fRadius = 1.0f;

		//_vStart = XMVECTOR3(0,0,0);
		//_vEnd = XMVECTOR3(0,0,0);
	}
	~CMPLink()
	{
	}

	bool	Create(MPCharacter *pChaMain,int iDummy1,MPCharacter *pChaTag, int iDummy2, \
		char* pszTex, int iTexNum,CMPResManger* pResMgr,XMVECTOR3* pEyePos,MPRender*	pDev);

	void	FrameMove();

	void	Render();
	
	virtual void	GetPhysique();
protected:
	void	ColArc(float fradius);

public:
	MPRender*	m_pDev;

protected:
	CMPEffectFile* _pCEffFile;
	XMVECTOR3*  _pEyePos;
	MPCharacter *_pChaMain;
	MPCharacter *_pChaTag;
	lwITex		**_pTex;

	MPFrame*	_pFrame;


	XMVECTOR3 _vStart;
	XMVECTOR3 _vEnd;

	float		_fRadius;

	int			 _iDummy1;
	int			 _iDummy2;


	float       _fdist;
	XMVECTOR3 _vdir;

	int			_iCurTex;
	float       _fCurTime;
	float*		_fDailTime;
};
//////////////////////////////////////////////////////////////////////////


class MINDPOWER_API CChaModel : public MPCharacter
{
public:
	CChaModel()
	{
		_iID = 0;_fVel = 0.3f; _iPlayType = PLAY_ONCE;_iCurPose = 0;_bPlaying = false;
		_wFrameCount = 0;
		_eSrcBlend		= D3DBLEND_SRCALPHA;
		_eDestBlend		= D3DBLEND_INVSRCALPHA;
		_dwCurColor = 0xffffffff;
		XMMatrixIdentity(&_matBone);
		XMMatrixIdentity(&_matWorld);
	}
	~CChaModel(){}
	void	SetDivece(MPRender*	pDev)	{ m_pDev = pDev; }

	bool	LoadScript(const s_string& strModel);
	void	PlayPose(DWORD id, DWORD type);

	void	SetVel(int iVel)			{ _fVel = (float)iVel / 1000;}
	int		GetVel()					{ return (int)(_fVel * 1000);}

	void	SetPlayType(int iType)		{ _iPlayType = iType;}
	int		GetPlayType()				{ return _iPlayType;}

	void	SetCurPose(int iPose)		{ _iCurPose = iPose;}
	int		GetCurPose()				{ return _iCurPose;}

	void	SetPlaying(bool	bPlay)		{ _bPlaying = bPlay;}
	bool	IsPlaying()					{ return _bPlaying;}

	int		GetID()						{ return _iID;}

	void	Play()						{ _bPlaying = true;PlayPose(_iCurPose,_iPlayType);}

	void	SaveToFile(FILE* file);
	void	LoadFromFile(FILE* file);
	void	LoadFromMemory(CMemoryBuf* pbuf);


	void	Begin();
	void	End();

	void		SetCurColor(DWORD	dwcolor){ _dwCurColor = dwcolor;}
	XMCOLORF	GetCurColor()				{ return _dwCurColor;}

	D3DBLEND	GetSrcBlend(){return _eSrcBlend;}
	void		SetSrcBlend(D3DBLEND srcblend){ _eSrcBlend = srcblend;}
	D3DBLEND	GetDestBlend(){return _eDestBlend;}
	void		SetDestBlend(D3DBLEND destblend){ _eDestBlend = destblend;}

	void	BindingBone(XMMATRIX* pBone)
	{
		if(pBone)
			_matBone = *pBone;
	}
	void	MoveTo(const XMVECTOR3* pPos)
	{
		if(pPos)
			XMMatrixTranslation(&_matWorld,pPos->x,pPos->y,pPos->z);
	}
	void UpdateMatrix()
	{
		_matResult = _matWorld * _matBone;
		SetMatrix((lwMatrix44*)&_matResult);
	}
public:
#ifdef		USE_RENDER
	MPRender*					m_pDev;
#else
	IDirect3DDeviceX*			m_pDev;
#endif

protected:
	bool	LoadChaModel(MPChaLoadInfo& info);

	bool	LoadPose(SChaAction& SCharAct);

	XMMATRIX								_matBone;
	XMMATRIX								_matWorld;
	XMMATRIX								_matResult;

protected:
	WORD									_wFrameCount;
	//S_BVECTOR<float>						_vecFrameSize;
	//S_BVECTOR<XMCOLORF>					_vecFrameColor;

	D3DBLEND								_eSrcBlend;
	D3DBLEND								_eDestBlend;
	XMCOLORF								_dwCurColor;	

private:
	int		_iID;
	float	_fVel;
	int		_iPlayType;
	int		_iCurPose;
	bool	_bPlaying;
};
/************************************************************************/
/* class CMPPartCtrl*/
/************************************************************************/
class MINDPOWER_API CMPPartCtrl//: public CEffectBase
{
public:
	static const int ParVersion = 15;
public:
	CMPPartCtrl(void);
	~CMPPartCtrl(void);

public:
	virtual void				BindingRes(CMPResManger* pResMagr);

	virtual void				FrameMove(DWORD	dwDailTime);

	virtual void				Render();
	virtual void				RenderVS()						 {}
	virtual void				RenderSoft()					 {}

	void	SetStripCharacter(MPCharacter* pCha);
	void	SetStripItem(MPSceneItem* pItem,bool bloop = false);

	void	SetItemDummy(MPSceneItem* pItem, int dummy1, int dummy2);

	void	SetPlayType(int nType);

	void	SetSkillCtrl(SkillCtrl* pCtrl)
	{
		for (int n = 0; n < m_iPartNum; ++n)
		{
			if( m_vecPartSys[n] ) m_vecPartSys[n]->SetSkillCtrl(pCtrl);
		}
	}

	void	SetAlpha(float falpha)
	{
		for (int n = 0; n < m_iPartNum; ++n)
		{
			if( m_vecPartSys[n] ) m_vecPartSys[n]->SetAlpha(falpha);
		}
	}

	void						Reset()
	{
		m_fCurTime = 0;
		const auto v = XMVECTOR3(0, 0, 0);
		MoveTo(&v);
		for (int n = 0; n < m_iPartNum; ++n)
		{
			if( m_vecPartSys[n] )
			{
				m_vecPartSys[n]->Reset(false);
				m_vecPartSys[n]->unFontEffCom();
			}
		}
	}
	void						Play(int iTime);
	void						Stop()
	{
		Reset();
		for (int n = 0; n < m_iPartNum; ++n)
		{
			if( m_vecPartSys[n] ) m_vecPartSys[n]->Stop();
		}
	}
	void						End()
	{
		for (int n = 0; n < m_iPartNum; ++n)
		{
			if( m_vecPartSys[n] ) m_vecPartSys[n]->End();
		}
	}
	bool						IsPlaying();

	void						MoveTo(const XMVECTOR3* vPos,MPMap* pmap = NULL)
	{
		for (auto n = 0; n < m_iPartNum; ++n)
		{
			if (m_vecPartSys[n]) 
			{
				m_vecPartSys[n]->MoveTo(vPos, pmap);
			}
		}
		for (auto n = 0; n <m_iModelNum; n++ )
		{
			if(m_vecModel[n]->IsPlaying())
			{
				m_vecModel[n]->MoveTo(vPos);
			}
		}
	}
	void						BindingBone(XMMATRIX* pMatBone)
	{
		for (auto  n = 0; n < m_iPartNum; ++n)
		{
			if (m_vecPartSys[n]) {
				m_vecPartSys[n]->BindingBone(pMatBone);
			}
		}
		for (auto n = 0; n <m_iModelNum; n++ )
		{
			if(m_vecModel[n]->IsPlaying())
			{
				m_vecModel[n]->BindingBone(pMatBone);
			}
		}
	}

	//void						BindingBoneDir(XMMATRIX* pMatBone)
	//{
	//	for(int i(0); i<m_iPartNum; ++i)  
	//		m_vecPartSys[i]->BindingBoneDir(pMatBone);
	//	for(int i(0); i<m_iModelNum; ++i) 
	//		if(m_vecModel[i]->IsPlaying())
	//			m_vecModel[i]->BindingBone(pMatBone);
	//}

	void setYaw(float fYaw)
	{
		for (int n = 0; n < m_iPartNum; ++n)
		{
			if( m_vecPartSys[n] ) m_vecPartSys[n]->setYaw( fYaw);
		}
	}
	void setPitch(float fPitch)
	{
		for (int n = 0; n < m_iPartNum; ++n)
		{
			if( m_vecPartSys[n] ) m_vecPartSys[n]->setPitch(fPitch);
		}
	}
	void setRoll(float fRoll)
	{
		for (int n = 0; n < m_iPartNum; ++n)
		{
			if( m_vecPartSys[n] ) m_vecPartSys[n]->setRoll(fRoll);
		}
	}
	void setScale(float fx,float fy,float fz)
	{
		for (int n = 0; n < m_iPartNum; ++n)
		{
			if( m_vecPartSys[n] ) m_vecPartSys[n]->setScale(fx,fy,fz);
		}
	}
	void setFontEffect(const char* pszText,CMPFont* pfont)
	{
		for (int n = 0; n < m_iPartNum; ++n)
		{
			if( m_vecPartSys[n] )
			{
				m_vecPartSys[n]->setFontEffect(pfont);
				m_vecPartSys[n]->setFontEffText(pszText);
			}
		}
	}
	void setFontEffectCom(VEC_string& vecText, int num,
		CMPResManger *pCResMagr ,XMVECTOR3* pvDir,int iTexID,XMCOLORF dwColor,bool bUseBack,bool bmain= false)
	{
		for (int n = 0; n < m_iPartNum; ++n)
		{
			if( m_vecPartSys[n] ) m_vecPartSys[n]->setFontEffectCom(vecText, num, pCResMagr,pvDir,iTexID,dwColor,bUseBack,bmain);
		}
	}
	void setRenderIdx(int idx)
	{
		for (int n = 0; n < m_iPartNum; ++n)
		{
			if( m_vecPartSys[n] ) m_vecPartSys[n]->setRenderIdx(idx);
		}
	}
	void setUseZBuff(bool bUseZ)
	{
		for (int n = 0; n < m_iPartNum; ++n)
		{
			if( m_vecPartSys[n] ) m_vecPartSys[n]->setUseZBuff(bUseZ);
		}
	}
	void setDir(XMVECTOR3* pvPos)
	{
		for (int n = 0; n < m_iPartNum; ++n)
		{
			if( m_vecPartSys[n] ) m_vecPartSys[n]->setDir(pvPos->x,pvPos->y,pvPos->z);
		}
	}
	void UpdateStrip();
public:
	void						CopyPartCtrl(CMPPartCtrl* pPart);
	CMPPartSys*					AddPartSys(CMPPartSys* part);
	CMPPartSys*					NewPartSys();
	void						DeletePartSys(int iID);
	void						Clear();
	void						SetLength(float fLength){ m_fLength = fLength;}


	CMPStrip*					NewStrip(const s_string& strTex,CMPResManger* pResMagr);
	CMPStrip*					GetStrip(int iIdx);
	int							GetStripNum(){return m_iStripNum;}

	CChaModel*					NewModel(const s_string& strID,CMPResManger* pResMagr);
	CChaModel*					GetModel(int iIdx);
	int							GetModelNum(){return m_iModelNum;}

	void						GetRes(CMPResManger* pResMagr,std::vector<INT>& vecTex,std::vector<INT>& vecModel,
		std::vector<INT>& vecEff);
	void						GetHitRes(CMPResManger* pResMagr,std::vector<s_string>& vecPar);
	//CMPShadeCtrl*				NewShade(s_string strID,CMPResManger* pResMagr);
	//CMPShadeCtrl*				GetShade(int iIdx);
	//int							GetShadeNum(){return m_iShadeNum;}

	//!�������ӵ��ļ�	
	bool		 SaveToFile(char* pszName);
	//!װ�����Ӵ��ļ�
	bool		 LoadFromFile(char* pszName);

	bool		 LoadFromMemory(CMemoryBuf*	pbuf);

public:
	s_string					m_strName;
	int							m_iPartNum;
	S_BVECTOR<CMPPartSys>		m_vecPartSys;

	float*						m_pfDailTime;
	float						m_fLength;
	float						m_fCurTime;

	//������������������
	int							m_iStripNum;
	CMPStrip*					m_pcStrip;

	//��������
	int							m_iModelNum;
	std::vector<CChaModel*>		m_vecModel;

	////��������
	//int							m_iShadeNum;
	//std::vector<CMPShadeCtrl>	m_vecShade;
};



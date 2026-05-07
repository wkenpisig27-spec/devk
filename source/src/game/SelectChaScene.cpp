#include "stdafx.h"

#include "SelectChaScene.h"

#include "GameApp.h"
#include "Character.h"
#include "SceneObj.h"
#include "UiFormMgr.h"
#include "UiTextButton.h"
#include "CharacterAction.h"
#include "SceneItem.h"
#include "ItemRecord.h"
#include "PacketCmd.h"
#include "GameConfig.h"

#include "Character.h"
#include "caLua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "UIRender.h"
#include "UIEdit.h"
#include "UILabel.h"
#include "uiformmgr.h"
#include "uitextbutton.h"
#include "uilabel.h"
#include "uiprogressbar.h"
#include "uiscroll.h"
#include "uilist.h"
#include "uicombo.h"
#include "uiimage.h"
#include "UICheckBox.h"
#include "uiimeinput.h"
#include "uigrid.h"
#include "uilistview.h"
#include "uipage.h"
#include "uitreeview.h"
#include "uiimage.h"
#include "UILabel.h"
#include "RenderStateMgr.h"
#include "uiDoublePwdForm.h"

#include "UIMemo.h"
#include "caLua.h"
#include "cameractrl.h"

#include "Connection.h"
#include "ServerSet.h"
#include "GameAppMsg.h"

#include "UI3DCompent.h"
#include "UIForm.h"
#include "UITemplete.h"
#include "commfunc.h"
#include "uiboxform.h"
#include "CreateChaScene.h"
#include "loginscene.h"
#include "UIItemCommand.h"
#include "GuildData.h"
#include "UIChat.h"
#include "NetGuild.h"

using namespace std;

// Helper for rendering name text - uses Render() if font has pre-baked outline, else ORender()
inline void RenderNameText(const char* text, int x, int y, DWORD textColor, DWORD outlineColor = COLOR_BLACK) {
	if (CGuiFont::s_NameFontHasPrebakedOutline) {
		CGuiFont::s_NameFont.Render(text, x, y, textColor);
	} else {
		CGuiFont::s_NameFont.ORender(text, x, y, textColor, outlineColor);
	}
}

/* #AUTOLOG
char autoLoginChar[32] ;
bool canAutoLoginChar = false;
*/
//~ Constructors ==============================================================
CSelectChaScene::CSelectChaScene(stSceneInitParam& param)
	: CGameScene(param), m_nCurChaIndex(-1), m_isInit(false), m_isCreateCha(false),
	  frmSelectCha(nullptr), btnDel(nullptr), btnYes(nullptr), btnCreate(nullptr), btnExit(nullptr), btnChangePassConf(nullptr) {
	LG("scene memory", "CSelectChaScene Create\n");

	// distance between each char
	m_XPositions.push_back(2164);
	m_XPositions.push_back(2410);
	m_XPositions.push_back(2614);

	m_YPositions.push_back(1657);
	m_YPositions.push_back(1389);
	m_YPositions.push_back(1579);

	m_ZPositions.push_back(0);
	m_ZPositions.push_back(0);
	m_ZPositions.push_back(0);

	// angle
	m_Yaws.push_back(140);
	m_Yaws.push_back(180);
	m_Yaws.push_back(220);

	m_FreePositions.push_back(0); // 0 ??????,1?????
	m_FreePositions.push_back(0);
	m_FreePositions.push_back(0);

	ChaFont* pkChaFont = new ChaFont();
	m_CharactorPtrs.push_back(pkChaFont);
	pkChaFont = new ChaFont();
	m_CharactorPtrs.push_back(pkChaFont);
	pkChaFont = new ChaFont();
	m_CharactorPtrs.push_back(pkChaFont);
}

//~ Destructors ===============================================================
CSelectChaScene::~CSelectChaScene() {
	LG("scene memory", "CSelectChaScene Destroy\n");
	const ChaFontPtrContainerIter end = m_CharactorPtrs.end();
	ChaFontPtrContainerIter iter = m_CharactorPtrs.begin();
	for (; iter != end; ++iter) {
		delete (*iter);
	}
}

//~ ???????? ===============================================================

//-----------------------------------------------------------------------
bool CSelectChaScene::_Init() {
	if (!CGameScene::_Init()) {
		// ????_Init()??,?????false.
		return false;
	}

	{ // save loading res mt flag, and resume these flags in _Clear() before this scene destoried.
		lwIByteSet* res_bs = g_Render.GetInterfaceMgr()->res_mgr->GetByteSet();
		_loadtex_flag = res_bs->GetValue(OPT_RESMGR_LOADTEXTURE_MT);
		_loadmesh_flag = res_bs->GetValue(OPT_RESMGR_LOADMESH_MT);
		res_bs->SetValue(OPT_RESMGR_LOADTEXTURE_MT, 0);
		res_bs->SetValue(OPT_RESMGR_LOADMESH_MT, 0);
	}

	_bEnableCamDrag = TRUE;
	MPTimer tInit;
	tInit.Begin();

	// Init Once
	if (!m_isInit) {
		m_isInit = true;

		// ??????
		pObj = AddSceneObj(512);

		if (pObj) {
			pObj->SetCullingFlag(0);
			// position of the scene
			pObj->setPos(0, 0);
			pObj->setYaw(180);

			DWORD num = pObj->GetPrimitiveNum();
			for (DWORD i = 0; i < num; i++) {
				pObj->GetPrimitive(i)->SetState(STATE_TRANSPARENT, 0);
				pObj->GetPrimitive(i)->SetState(STATE_UPDATETRANSPSTATE, 0);
			}

			{
				pObj->PlayDefaultAnimation(1.0f / CSteadyFrame::GetAnimMultiplier());

				const DWORD p_id_num = 6;
				DWORD p_id[p_id_num][5] =
					{
						{0, -1, -1, ANIM_CTRL_TYPE_BONE, PLAY_ONCE},
						{1, -1, -1, ANIM_CTRL_TYPE_BONE, PLAY_ONCE},
						{2, -1, -1, ANIM_CTRL_TYPE_BONE, PLAY_ONCE},
						{3, -1, -1, ANIM_CTRL_TYPE_BONE, PLAY_ONCE},
						{4, -1, -1, ANIM_CTRL_TYPE_BONE, PLAY_ONCE},
						{26, 0, 0, ANIM_CTRL_TYPE_TEXIMG, PLAY_LOOP},
					};

				lwIPrimitive* p = 0;

				lwPlayPoseInfo ppi;
				memset(&ppi, 0, sizeof(ppi));
				ppi.bit_mask = PPI_MASK_DEFAULT;
				ppi.pose = 0;
				ppi.frame = 0.0f;
				ppi.type = PLAY_ONCE;
				// FPS-aware: halved at 60 FPS to preserve real-time animation speed.
				ppi.velocity = 1.0f / CSteadyFrame::GetAnimMultiplier();

				lwAnimCtrlObjTypeInfo type_info;

				for (DWORD i = 0; i < num; i++) {
					p = pObj->GetPrimitive(i);

					for (DWORD j = 0; j < p_id_num; j++) {
						if (p->GetID() == p_id[j][0]) {
							lwIAnimCtrlAgent* anim_agent = p->GetAnimAgent();
							if (anim_agent == 0)
								continue;

							type_info.type = p_id[j][3];
							type_info.data[0] = p_id[j][1];
							type_info.data[1] = p_id[j][2];
							lwIAnimCtrlObj* ctrl_obj = anim_agent->GetAnimCtrlObj(&type_info);
							if (ctrl_obj == 0)
								continue;

							ppi.type = p_id[j][4];
							ctrl_obj->PlayPose(&ppi);
						}
					}
				}
			}

			{
				const DWORD id_num = 3;
				const DWORD id_buf[id_num] =
					{46, 47, 48};

				lwIPrimitive* pri;
				DWORD pri_num = pObj->GetPrimitiveNum();

				for (DWORD j = 0; j < pri_num; j++) {
					pri = pObj->GetPrimitive(j);

					for (DWORD i = 0; i < id_num; i++) {
						if (pri->GetID() == id_buf[i]) {
							pAure[i] = pri;
							pAure[i]->SetState(STATE_VISIBLE, 0);
						}
					}
				}
			}
		}
	}

	g_Render.SetClip(g_Config.fnear, g_Config.ffar);

	CCameraCtrl* pCam = g_pGameApp->GetMainCam();
	if (pCam) {
		g_pGameApp->EnableCameraFollow(TRUE);
		pCam->m_EyePos.x = 23.749f;
		pCam->m_EyePos.y = 20.923f;
		pCam->m_EyePos.z = 1.982f;

		pCam->m_RefPos.x = 20.034f;
		pCam->m_RefPos.y = -194.137f;
		pCam->m_RefPos.z = 0.868f;
	}
	g_Render.SetWorldViewFOV(Angle2Radian(70.0f));
	g_Render.SetWorldViewAspect(1.33f);
	g_Render.SetClip(1.0f, 2000.0f);

	g_Render.LookAt(pCam->m_EyePos, pCam->m_RefPos);
	g_Render.SetCurrentView(MPRender::VIEW_WORLD);
	MPIDeviceObject* dev_obj = g_Render.GetInterfaceMgr()->dev_obj;

	// SetupVertexFog(dev_obj, 0, 0, D3DCOLOR_XRGB(28, 221, 246), D3DFOG_EXP2, 1, 0.0006f);

	g_Render.SetRenderStateForced(D3DRS_LIGHTING, 0);
	g_Render.SetRenderState(D3DRS_AMBIENT, 0xffffffff);

	// ???UI
	if (!_InitUI()) {
		return false;
	}

	/* if(canAutoLoginChar){
		CS_BeginPlay( autoLoginChar );
		canAutoLoginChar = false;
	} */

	return true;
}

//-----------------------------------------------------------------------
bool CSelectChaScene::_Clear() {
	if (frmSelectCha)
		frmSelectCha->SetIsShow(false);

	if (!CGameScene::_Clear()) {
		// ??Clear??,????false.
		return false;
	}

	{ // reset loading res mt flag
		if (_loadtex_flag != 9 && _loadmesh_flag != 9) {
			lwIByteSet* res_bs = g_Render.GetInterfaceMgr()->res_mgr->GetByteSet();
			res_bs->SetValue(OPT_RESMGR_LOADTEXTURE_MT, _loadtex_flag);
			res_bs->SetValue(OPT_RESMGR_LOADMESH_MT, _loadmesh_flag);
		}
	}

	g_Render.SetClip(1.0f, 1000.0f);

	return true;
}
//-----------------------------------------------------------------------
void CSelectChaScene::_Render() {
	// CGameScene::_Render();
	/*
	if(pObj == 0)
		return;
*/
	MPTimer mpt;
	mpt.Begin();
	// CGameScene::_Render();
	MPIDeviceObject* dev_obj = g_Render.GetInterfaceMgr()->dev_obj;
	RenderStateMgr* rsm = g_pGameApp->GetRenderStateMgr();
	DWORD dwOldState;

	rsm->BeginScene();

	// ????
	rsm->BeginCharacter();

	D3DLIGHTX env_light;
	D3DLIGHTX env_light_old;
	memset(&env_light, 0, sizeof(env_light));
	env_light.Type = D3DLIGHT_DIRECTIONAL;

	env_light.Direction.x = -1.0f;
	env_light.Direction.y = -1.0f;
	env_light.Direction.z = -0.5f;
	D3DXVec3Normalize((D3DXVECTOR3*)&env_light.Direction, (D3DXVECTOR3*)&env_light.Direction);

	MPDwordByte4 c;
	c.b[3] = 0xff;
	c.b[2] = 185;
	c.b[1] = 36;
	c.b[0] = 54;
	env_light.Diffuse.r = (float)(c.b[2] / 255.0f);
	env_light.Diffuse.g = (float)(c.b[1] / 255.0f);
	env_light.Diffuse.b = (float)(c.b[0] / 255.0f);

	g_Render.GetLight(0, &env_light_old);
	g_Render.SetLight(0, &env_light);

	const ChaFontPtrContainerIter end = m_CharactorPtrs.end();
	const ChaFontPtrContainerIter begin = m_CharactorPtrs.begin();
	ChaFontPtrContainerIter iter = begin;

	dev_obj->GetRenderState(D3DRS_LIGHTING, &dwOldState);
	/*for (int i(0); i<3; i++)
	{
		pAure[i]->SetState(STATE_VISIBLE, 0);
	}*/
	for (; iter != end; ++iter) {
		if ((*iter)->pCha == nullptr)
			continue;

		dev_obj->SetRenderState(D3DRS_LIGHTING, 0);
		if (m_nCurChaIndex == iter - begin) {
			dev_obj->SetRenderState(D3DRS_LIGHTING, 1);
			// pAure[(*iter)->iPos]->SetState(STATE_VISIBLE, 1);
			(*iter)->pCha->setYaw(m_Yaws[m_nCurChaIndex]);
		}

		(*iter)->pCha->Render();
	}
	dev_obj->SetRenderState(D3DRS_LIGHTING, dwOldState);

	g_Render.SetLight(0, &env_light_old);

	rsm->EndCharacter();

	// ????
	rsm->BeginSceneObject();

	if (pObj)
		pObj->FrameMove(0);

	dev_obj->GetRenderState(D3DRS_LIGHTING, &dwOldState);
	dev_obj->SetRenderState(D3DRS_LIGHTING, FALSE);

	// orange background
	// SetupVertexFog(dev_obj, 0, 0, D3DCOLOR_XRGB(255, 104, 13), D3DFOG_EXP2, 1, 0.0025f);

	if (pObj)
		pObj->Render();

	rsm->EndSceneObject();

	rsm->BeginTranspObject();
	lwUpdateSceneTransparentObject();
	rsm->EndTranspObject();

	rsm->EndScene();

	dev_obj->SetRenderState(D3DRS_FOGENABLE, FALSE);
}

void CSelectChaScene::_RenderUI() {
	const ChaFontPtrContainerIter end = m_CharactorPtrs.end();
	for (ChaFontPtrContainerIter iter = m_CharactorPtrs.begin(); iter != end; ++iter) {
		if ((*iter)->pCha == nullptr)
			continue;

		// ????????

		if (((*iter)->iFontX == -1) || ((*iter)->iFontY == -1)) {
			int nScreenX, nScreenY;
			lwMatrix44 mat;

			D3DXVECTOR3 pos = (*iter)->pCha->GetPos();
			(*iter)->pCha->GetRunTimeMatrix(&mat, 1);

			pos.x = mat._41;
			pos.y = mat._42;
			pos.z = mat._43;

			g_Render.WorldToScreen(pos.x, pos.y, pos.z, &nScreenX, &nScreenY);

			// ????????????
			nScreenY -= 80;

			(*iter)->iFontX = nScreenX;
			(*iter)->iFontY = nScreenY;
		}

		char szBuf[8];
		string sFirst = string((*iter)->pCha->getName());
		string sSecond = string("Lv") + string(itoa((*iter)->iLevel, szBuf, 10)) +
						 " " + (*iter)->sProfession;
		
		// Character name - use s_NameFont with outline (same as in-game names)
		int nNameWidth = CGuiFont::s_NameFont.GetWidth(sFirst.c_str());
		int nNameHeight = CGuiFont::s_NameFont.GetHeight(sFirst.c_str());
		int nNameX = (*iter)->iFontX - nNameWidth / 2;
		RenderNameText(sFirst.c_str(), nNameX, (*iter)->iFontY, 0xFFFFFFFF, 0xFF000000);
		
		// Level and profession - use regular font with outline
		int nSecondWidth = CGuiFont::s_NameFont.GetWidth(sSecond.c_str());
		int nSecondX = (*iter)->iFontX - nSecondWidth / 2;
		RenderNameText(sSecond.c_str(), nSecondX, (*iter)->iFontY + nNameHeight + 2, 0xFFFFFFFF, 0xFF000000);
	}
}

//-----------------------------------------------------------------------
void CSelectChaScene::_FrameMove(DWORD dwTimeParam) {
	CGameScene::_FrameMove(dwTimeParam);
}

//-----------------------------------------------------------------------
bool CSelectChaScene::_MouseButtonDown(int nButton) {
	// ???????????,???????  add by Philip.Wu  2006-07-20
	if (g_stUIDoublePwd.GetIsShowCreateForm() || g_stUIDoublePwd.GetIsShowAlterForm() || g_stUIDoublePwd.GetIsShowDoublePwdForm()) {
		return false;
	}

	// ??????????
	CCharacter* pCha = this->HitTestCharacter(g_pGameApp->GetMouseX(),
											  g_pGameApp->GetMouseY());
	if (nullptr == pCha) {
		return false;
	}

	// ?????????
	int nIndex(0);
	const ChaFontPtrContainerIter end = m_CharactorPtrs.end();
	ChaFontPtrContainerIter iter = m_CharactorPtrs.begin();
	for (; iter != end; ++iter, ++nIndex) {
		if ((*iter)->pCha == pCha)
			break;
	}
	if (nIndex > 2) {
		return false;
	}

	if (m_nCurChaIndex != nIndex && m_nCurChaIndex != -1) {
		m_CharactorPtrs[m_nCurChaIndex]->pCha->PlayPose(1, PLAY_LOOP, -1,
														CGameApp::GetFrameFPS(), true);
		SetChaDark(m_CharactorPtrs[m_nCurChaIndex]->pCha);
	}

	m_nCurChaIndex = nIndex;

	m_CharactorPtrs[m_nCurChaIndex]->pCha->PlayPose(2, PLAY_LOOP, -1,
													CGameApp::GetFrameFPS(), true);
	m_CharactorPtrs[m_nCurChaIndex]->pCha->SetColor(m_ChaColors[m_nCurChaIndex][0],
													m_ChaColors[m_nCurChaIndex][1], m_ChaColors[m_nCurChaIndex][2]);

	// ????????????
	UpdateButton();

	return true;
}

//-----------------------------------------------------------------------
bool CSelectChaScene::_MouseButtonDB(int nButton) {
	if (!_MouseButtonDown(nButton))
		return false;

	SendBeginPlayToServer();
	g_pGameApp->Waiting();
	return true;
}

//-----------------------------------------------------------------------
void CSelectChaScene::_KeyDownEvent(int key) {
	if (m_nCurChaIndex != -1) { /*??????????*/
		int iRotate = 0;		// left:-1	right:1
		if (VK_LEFT == key) {
			iRotate = -1;
		} else if (VK_RIGHT == key) {
			iRotate = 1;
		}

		m_Yaws[m_nCurChaIndex] += -(iRotate) * 15;
		m_Yaws[m_nCurChaIndex] = (m_Yaws[m_nCurChaIndex] + 360) % 360;
	}
}

//-----------------------------------------------------------------------
void CSelectChaScene::LoadingCall() // ?????loading??,???
{
	CGameScene::LoadingCall();

	// �????????????????????????????????COOLDOWN???
	CItemCommand::ClearCoolDown();

	// ?????????????????�????????????????????????jh?eL???????
	// CGuildData::SetGuildMasterID(nullptr);
	// CGuildData::SetGuildName("");
	// if(g_stUIChat.GetGuildNode()) g_stUIChat.GetGuildNode()->Clear();

	NetPC_GUILD_START_BEGIN(0, 0, 0);
	NetPC_GUILD_START_END();

	static bool bLoadRes2 = false;
	if (!bLoadRes2) {
		bLoadRes2 = true;
		// g_pGameApp->LoadRes2();
		g_pGameApp->LoadRes3();
		// g_pGameApp->LoadRes4();
	}

	if (!g_Config.m_IsDoublePwd) {
		// ?????????????????
		g_stUIDoublePwd.ShowCreateForm();

		// CBoxMgr::ShowSelectBox(_evtCreateDoublePwdEvent, RES_STRING(CL_LANGUAGE_MATCH_800), true);//"??j?"?d????????????\n\n???????????"
	} else if (GetChaCount() == 0 && frmWelcomeNotice) {
		// ??j????????????????
		frmWelcomeNotice->ShowModal();
	} else if (m_isCreateCha) {
		m_isCreateCha = false;

		if (GetChaCount() == 1 && frmCreateOKNotice) {
			// ???????h?????
			frmCreateOKNotice->ShowModal();
		}
	}

	if (g_dwCurMusicID != 1)
		g_pGameApp->PlayMusic(1);

	/* #AUTOLOG
		if(canAutoLoginChar){
			CS_BeginPlay( autoLoginChar );
			canAutoLoginChar = false;
		}
	*/
}

//-----------------------------------------------------------------------
void CSelectChaScene::SetMainCha(int nChaID) {
	CGameScene::SetMainCha(nChaID);
}

//~ UI???l??? =============================================================

//-----------------------------------------------------------------------
bool CSelectChaScene::_InitUI() {
	// ??????i???
	frmSelectCha = CFormMgr::s_Mgr.Find("frmSelect", GetInitParam()->nUITemplete);
	if (!frmSelectCha)
		return false;

	btnDel = dynamic_cast<CTextButton*>(frmSelectCha->Find("btnDel"));
	if (!btnDel)
		return false;
	btnYes = dynamic_cast<CTextButton*>(frmSelectCha->Find("btnYes"));
	if (!btnYes)
		return false;
	btnCreate = dynamic_cast<CTextButton*>(frmSelectCha->Find("btnCreate"));
	if (!btnCreate)
		return false;
	btnExit = dynamic_cast<CTextButton*>(frmSelectCha->Find("btnNo"));
	if (!btnExit)
		return false;
	btnAlter = dynamic_cast<CTextButton*>(frmSelectCha->Find("btnAlter"));
	if (!btnAlter)
		return false;

	CForm* frmUpdPas = CFormMgr::s_Mgr.Find("frmUpdPas");
	btnChangePassConf = dynamic_cast<CTextButton*>(frmUpdPas->Find("btnChangePassConf"));
	btnChangePassConf->evtMouseClick = _OnClickUpdatePass;
	btnChangePassConf->SetIsEnabled(true);
	CEdit* edtPassword = dynamic_cast<CEdit*>(frmUpdPas->Find("edtPassword"));
	CEdit* edtPassword2 = dynamic_cast<CEdit*>(frmUpdPas->Find("edtPassword2"));
	CEdit* confirmPIN = dynamic_cast<CEdit*>(frmUpdPas->Find("confirmPIN"));

	edtPassword->SetIsPassWord(true);
	edtPassword2->SetIsPassWord(true);
	confirmPIN->SetIsPassWord(true);

	CForm* frmGMLogin = CFormMgr::s_Mgr.Find("frmGMLogin");
	CTextButton* btnGMLogin = dynamic_cast<CTextButton*>(frmGMLogin->Find("btnGMLogin"));
	btnGMLogin->evtMouseClick = _OnClickGMLogin;
	btnGMLogin->SetIsEnabled(true);

	btnCreate->SetFlashCycle();

	frmSelectCha->SetPos(
		(g_pGameApp->GetWindowWidth() - frmSelectCha->GetWidth()) / 2,
		g_pGameApp->GetWindowHeight() - frmSelectCha->GetHeight() - 20);
	frmSelectCha->Refresh();
	frmSelectCha->Show();

	frmSelectCha->evtEntrustMouseEvent = _SelChaFrmMouseEvent;

	// ???????????   ?�???????j?"????????????
	frmWelcomeNotice = CFormMgr::s_Mgr.Find("frmWelcomeNotice");
	if (!frmWelcomeNotice)
		return false;
	frmWelcomeNotice->evtEntrustMouseEvent = _evtWelcomeNoticeEvent;

	// ??????d????????????????   ?�????????"??????h??????J???????????
	frmCreateOKNotice = CFormMgr::s_Mgr.Find("frmCreateOKNotice");
	if (!frmCreateOKNotice)
		return false;
	frmCreateOKNotice->evtEntrustMouseEvent = _evtCreateOKNoticeEvent;

	// ??????t????????
	UpdateButton();

	frmChaNameAlter = CFormMgr::s_Mgr.Find("frmChaNameAlter");
	if (!frmChaNameAlter)
		return false;
	frmChaNameAlter->evtEntrustMouseEvent = _evtChaNameAlterMouseEvent;

	return true;
}

void CSelectChaScene::_OnClickGMLogin(CGuiData* pSender, int x, int y, DWORD key) {
	CForm* frmGMLogin = CFormMgr::s_Mgr.Find("frmGMLogin");
	CEdit* edtCha = dynamic_cast<CEdit*>(frmGMLogin->Find("edtCha"));
	CS_BeginPlay(edtCha->GetCaption());
}

void CSelectChaScene::_OnClickUpdatePass(CGuiData* pSender, int x, int y, DWORD key) {
	CForm* frmUpdPas = CFormMgr::s_Mgr.Find("frmUpdPas");

	CEdit* edtPassword = dynamic_cast<CEdit*>(frmUpdPas->Find("edtPassword"));
	CEdit* edtPassword2 = dynamic_cast<CEdit*>(frmUpdPas->Find("edtPassword2"));
	CEdit* confirmPIN = dynamic_cast<CEdit*>(frmUpdPas->Find("confirmPIN"));

	if (strlen(edtPassword->GetCaption()) == 0 || strlen(edtPassword2->GetCaption()) == 0 || strlen(confirmPIN->GetCaption()) == 0) {
		NetShowMapCrash("Please fill in all fields.");
		return;
	}

	if (strcmp(edtPassword->GetCaption(), edtPassword2->GetCaption()) != 0) {
		NetShowMapCrash("Passwords do not match.");
		return;
	}

	CS_ChangePass(edtPassword->GetCaption(), confirmPIN->GetCaption());
}

//-----------------------------------------------------------------------
void CSelectChaScene::_SelChaFrmMouseEvent(CCompent* pSender, int nMsgType,
										   int x, int y, DWORD dwKey) {
	string strName = pSender->GetName();

	if (stricmp("frmSelect", pSender->GetForm()->GetName()) != 0) {
		return;
	}

	if (strName == "btnCreate") {
		// ?????????
		stSceneInitParam param;
		param.nTypeID = enumCreateChaScene;
		param.strName = "";
		param.strMapFile = "";
		param.nUITemplete = enumCreateChaForm;
		param.nMaxCha = 20;
		param.nMaxObj = 20;
		param.nMaxItem = 20;
		param.nMaxEff = 20;

		CCreateChaScene* pkScene =
			dynamic_cast<CCreateChaScene*>(g_pGameApp->CreateScene(&param));
		if (!pkScene)
			return;
		CSelectChaScene& rkSelectChaScene = CSelectChaScene::GetCurrScene();

		g_pGameApp->GotoScene(pkScene, false);
		pkScene->setLastScene(&rkSelectChaScene);
	} else if (strName == "btnYes") {
		// ????
		// ?????????????
		GetCurrScene().SendBeginPlayToServer();
		CGameApp::Waiting();
	} else if (strName == "btnDel") {
		if (g_Config.m_IsDoublePwd) {
			// ??????????  modify by Philip.Wu  2006-07-19
			g_stUIDoublePwd.SetType(CDoublePwdMgr::DELETE_CHARACTOR);
			g_stUIDoublePwd.ShowDoublePwdForm();
		} else {
			// ????
			// CBoxMgr::ShowSelectBox(_CheckFrmMouseEvent, RES_STRING(CL_LANGUAGE_MATCH_384), true);
		}
	} else if (strName == "btnNo") {
		if (g_TomServer.bEnable) {
			g_pGameApp->SetIsRun(false);
			return;
		}

		// ??????
		CS_Logout();
		CS_Disconnect(DS_DISCONN);
		g_pGameApp->LoadScriptScene(enumLoginScene);
	} else if (strName == "btnAlter") {
		// ??????
		g_stUIDoublePwd.ShowAlterForm();
	} else if (strName == "btnChangePass") {
		CForm* frmUpdPas = CFormMgr::s_Mgr.Find("frmUpdPas");
		if (frmUpdPas) {
			frmUpdPas->Show();
		}
	}
	// else if(strName == "btnChangePassConf")
	//{
	//	//send packet etc
	// }
	// else if(strName == "btnCancel")
	//{
	//	CForm* frmUpdPas = CFormMgr::s_Mgr.Find( "frmUpdPas");
	//	if(frmUpdPas){
	//		frmUpdPas->Hide();
	//	}
	// }

	return;
}

//-----------------------------------------------------------------------
// ???????????
// void CSelectChaScene::_CheckFrmMouseEvent(CCompent *pSender, int nMsgType,
//                                          int x, int y, DWORD dwKey)
//{
//    if( nMsgType == CForm::mrYes )
//    {
//        //??????????????????????
//        GetCurrScene().SendDelChaToServer();
//        CGameApp::Waiting();
//        return;
//    }
//    return;
//}

// ???????????????????  add by Philip.Wu  2006-07-20
void CSelectChaScene::_evtCreateDoublePwdEvent(CCompent* pSender, int nMsgType, int x, int y, DWORD dwKey) {
	if (nMsgType == CForm::mrYes) {
		// ?????????????????
		g_stUIDoublePwd.ShowCreateForm();
	} else {
		// ?????????????????????
		if (g_TomServer.bEnable) {
			g_pGameApp->SetIsRun(false);
			return;
		}

		// ?????????
		CS_Logout();
		CS_Disconnect(DS_DISCONN);
		g_pGameApp->LoadScriptScene(enumLoginScene);
	}
}

//~ ??????l??? ==========================================================

//-----------------------------------------------------------------------
void CSelectChaScene::DelCurrentSelCha() {
	// ?????????
	m_CharactorPtrs[m_nCurChaIndex]->pCha->SetValid(false);

	// ????
	m_CharactorPtrs[m_nCurChaIndex]->pCha = nullptr;

	m_FreePositions[m_nCurChaIndex] = 0; // ???????

	m_nCurChaIndex = -1;

	// ????UI??
	UpdateButton();
	return;
}

//-----------------------------------------------------------------------
bool CSelectChaScene::CreateCha(const string& sName, int nChaIndex, stNetChangeChaPart* part) {
	if (m_nCurChaIndex >= 0 && m_nCurChaIndex <= 2)
		SetChaDark(m_CharactorPtrs[m_nCurChaIndex]->pCha);

	CCharacter* pCha = this->AddCharacter(part->sTypeID);
	if (!pCha)
		return false;
	pCha->setName(sName.c_str());
	pCha->UpdataFace(*part);

	// ??????????
	int i(0);
	for (; i < 3; i++) {
		if (m_FreePositions[i] == 0)
			break;
	}
	if (i == 3) {
		return false;
	}
	pCha->setPos(m_XPositions[i], m_YPositions[i]);
	pCha->setYaw(m_Yaws[i]);
	m_FreePositions[i] = 1;
	pCha->GetColor(m_ChaColors[i]);
	m_nCurChaIndex = i;
	// SetChaDark(pCha);

	m_CharactorPtrs[m_nCurChaIndex]->pCha = pCha;
	m_CharactorPtrs[m_nCurChaIndex]->iLevel = 1;
	m_CharactorPtrs[m_nCurChaIndex]->sProfession = RES_STRING(CL_LANGUAGE_MATCH_385);
	m_CharactorPtrs[m_nCurChaIndex]->iPos = m_nCurChaIndex;
	m_CharactorPtrs[m_nCurChaIndex]->iFontX = -1;
	m_CharactorPtrs[m_nCurChaIndex]->iFontY = -1;

	m_isCreateCha = true;

	UpdateButton();
	return true;
}

//-----------------------------------------------------------------------
void CSelectChaScene::SendDelChaToServer(const char szPassword2[]) {
	if (m_nCurChaIndex >= 0 && m_nCurChaIndex <= 2)
		// ?????????
		CS_DelCha(m_CharactorPtrs[m_nCurChaIndex]->pCha->getName(), szPassword2);
}

//-----------------------------------------------------------------------
void CSelectChaScene::SendBeginPlayToServer() {
	if (m_nCurChaIndex < 0 && m_nCurChaIndex > 2)
		return;
	if (!m_CharactorPtrs[m_nCurChaIndex]->pCha)
		return;

	CS_BeginPlay(m_CharactorPtrs[m_nCurChaIndex]->pCha->getName());

	// CCharacter* pCha = m_CharactorPtrs[m_nCurChaIndex]->pCha;

	// ????
	// LG("select", "Client Send:%s,%d,%d,%d,%d,%d\n",
	//   pCha->getName(), pCha->GetPartID(0), pCha->GetPartID(1),
	//   pCha->GetPartID(2), pCha->GetPartID(3), pCha->GetPartID(4));
}

bool CSelectChaScene::SelectCharacters(NetChaBehave* chabehave, int num) {
	stNetChangeChaPart* part = nullptr;
	num = min(3, num);
	for (int i = 0; i < num; i++) {
		part = (stNetChangeChaPart*)chabehave[i].sLook;

		CCharacter* pCha = this->AddCharacter(part->sTypeID);
		if (!pCha)
			return false;
		pCha->setName(chabehave[i].sCharName);
		pCha->UpdataFace(*part);
		pCha->SetIsOnMount(false);
		pCha->PlayPose(pCha->GetPose(POSE_WAITING), PLAY_LOOP_SMOOTH);
		if (m_FreePositions[i] == 0) {
			m_FreePositions[i] = 1;
			pCha->setPos(m_XPositions[i], m_YPositions[i]);
			pCha->setYaw(m_Yaws[i]);
		}
		pCha->GetColor(m_ChaColors[i]);
		if (i != 0)
			SetChaDark(pCha);

		m_CharactorPtrs[i]->pCha = pCha;
		m_CharactorPtrs[i]->iLevel = (int)(chabehave[i].iDegree);
		m_CharactorPtrs[i]->sProfession = chabehave[i].sJob;
		m_CharactorPtrs[i]->iPos = i;
		m_CharactorPtrs[i]->iFontX = -1;
		m_CharactorPtrs[i]->iFontY = -1;
	}

	UpdateButton();

	return true;
}

//-----------------------------------------------------------------------
CSelectChaScene& CSelectChaScene::GetCurrScene() {
	CSelectChaScene* pScene =
		dynamic_cast<CSelectChaScene*>(g_pGameApp->GetCurScene());

	if (!pScene)
		throw std::runtime_error("GetCurrScene called when not in CSelectChaScene");

	return *pScene;
}

void CSelectChaScene::SelectChaError(int error_no, const char* error_info) {
	g_pGameApp->MsgBox("%s", g_GetServerError(error_no));
	LG("error", "%s Error, Code:%d, Info: %s", error_info, error_no, g_GetServerError(error_no));
	CGameApp::Waiting(false);
}
void CSelectChaScene::SetChaDark(CCharacter* pCha) {
	pCha->SetColor(129, 121, 114);
}

void CSelectChaScene::UpdateButton() {
	int i(0);
	for (; i < 3; i++) {
		if (m_CharactorPtrs[i]->pCha == nullptr) {
			btnCreate->SetIsEnabled(true);
			break;
		}
	}
	if (i == 3)
		btnCreate->SetIsEnabled(false);

	if (m_nCurChaIndex < 0 || m_nCurChaIndex > 2) {
		btnDel->SetIsEnabled(false);
		btnYes->SetIsEnabled(false);
	} else {
		btnDel->SetIsEnabled(true);
		btnYes->SetIsEnabled(true);
	}

	if (!g_Config.m_IsDoublePwd) {
		btnCreate->SetIsEnabled(false);
		btnAlter->SetIsEnabled(false);
	} else {
		btnAlter->SetIsEnabled(true);
	}
}

int CSelectChaScene::GetChaCount() {
	int nCount = 0;
	for (int i = 0; i < 3; ++i) {
		if (m_FreePositions[i]) {
			++nCount;
		}
	}

	return nCount;
}

void CSelectChaScene::ShowWelcomeNotice(bool bShow) {
	if (frmWelcomeNotice) {
		frmWelcomeNotice->ShowModal();
	}
}

// ??????? ?�?????
void CSelectChaScene::_evtWelcomeNoticeEvent(CCompent* pSender, int nMsgType, int x, int y, DWORD dwKey) {
	string strName = pSender->GetName();
	CSelectChaScene* pSelectChaScene = dynamic_cast<CSelectChaScene*>(g_pGameApp->GetCurScene());

	if (pSelectChaScene) {
		if (strName == "btnYes") {
			pSelectChaScene->frmWelcomeNotice->Close();
		}
	}
}

// ??d???????????????? ?�?????
void CSelectChaScene::_evtCreateOKNoticeEvent(CCompent* pSender, int nMsgType, int x, int y, DWORD dwKey) {
	string strName = pSender->GetName();
	CSelectChaScene* pSelectChaScene = dynamic_cast<CSelectChaScene*>(g_pGameApp->GetCurScene());

	if (pSelectChaScene) {
		if (strName == "btnYes") {
			pSelectChaScene->frmCreateOKNotice->Close();
		}
	}
}

// ??d???????????????? ?�?????
void CSelectChaScene::_evtChaNameAlterMouseEvent(CCompent* pSender, int nMsgType, int x, int y, DWORD dwKey) {
	string strName = pSender->GetName();
	CSelectChaScene* pSelectChaScene = dynamic_cast<CSelectChaScene*>(g_pGameApp->GetCurScene());

	if (pSelectChaScene) {
		if (strName == "btnYes") {
			pSelectChaScene->frmCreateOKNotice->Close();
		}
	}
}

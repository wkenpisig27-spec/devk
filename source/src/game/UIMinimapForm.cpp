#include "StdAfx.h"
#include "uiminimapform.h"
#include "uiForm.h"
#include "uicompent.h"
#include "uilabel.h"
#include "gameapp.h"
#include "SmallMap.h"
#include "uiformmgr.h"
#include "uimemo.h"
#include "ui3dcompent.h"
#include "scene.h"
#include "uiEdit.h"
#include "uiCombo.h"
#include "uiboothform.h"
#include "worldscene.h"
#include "PacketCmd.h"
#include "character.h"
#include "EffectObj.h"
#include "MapSet.h"
#include "UIStoreForm.h"
#include "UIDoublePwdForm.h"
#include "uifont.h"
#include "UIMisLogForm.h" //add by alfred.shi 20080722
#include "UIstartform.h"  //add by alfred.shi 20080711
#include "UIKnowledgeBase.h"

#include "AreaRecord.h" //add by sunny.sun 20080903
#include "MonsterSet.h" //add by mdr
#include <shellapi.h>
#include "ProCirculate.h"
#include "uiBoatForm.h"
#include "UIBossTimerForm.h"

using namespace std;
using namespace GUI;

#define CHECK_FAILED(Status) ((Status) != 0)

//---------------------------------------------------------------------------
// class CTradeMgr
//---------------------------------------------------------------------------

CForm* CMiniMapMgr::frmRadar = 0;
CEdit* CMiniMapMgr::edtX = 0;
CEdit* CMiniMapMgr::edtY = 0;
CCombo* CMiniMapMgr::cboAddr = 0;

//---------------------------------------------------------------------------
bool CMiniMapMgr::Init() {
	// ��ʼ��С��ͼ
	frmMinimap = _FindForm("frmMinimap");
	if (!frmMinimap)
		return false;
	frmMinimap->evtEntrustMouseEvent = _MiniFormMouseEvent;

	MinimatRect = dynamic_cast<CCompent*>(frmMinimap->Find("imgMinimapRect"));
	if (!MinimatRect)
		return Error(RES_STRING(CL_LANGUAGE_MATCH_45), frmMinimap->GetName(), "imgMinimapRect");

	char szName[32] = {0};
	for (int i = 0; i < MAP_POS_MAX; i++) {
		sprintf(szName, "labMapPos%d", i);

		labMapPosRand[i] = dynamic_cast<CLabelEx*>(frmMinimap->Find(szName)); // ��ͼ����
		if (!labMapPosRand[i])
			return Error(RES_STRING(CL_LANGUAGE_MATCH_45), frmMinimap->GetName(), szName);
	}
	// labMapPos = dynamic_cast<CLabelEx*>(frmMinimap->Find( "labMapPos" )); //��ͼ����
	// if( !labMapPos ) return Error("msgui.clu����<%s>���Ҳ����ؼ�<%s>", frmMinimap->GetName(), "labMapPos");
	labMapPos = labMapPosRand[0];

	labMapName = dynamic_cast<CLabelEx*>(frmMinimap->Find("labMapName")); // ��ͼ����
	if (!labMapName)
		return Error(RES_STRING(CL_LANGUAGE_MATCH_45), frmMinimap->GetName(), "labMapName");

	// server timer
	labClock = dynamic_cast<CLabelEx*>(frmMinimap->Find("labClock"));
	if (!labClock)
		return Error(RES_STRING(CL_LANGUAGE_MATCH_45), frmMinimap->GetName(), "labClock");

	// ���ͼ
	frmBigmap = _FindForm("frmBigmap");
	if (!frmBigmap)
		return false;
	frmBigmap->Refresh();
	frmBigmap->evtShow = _evtShowbigmap;
	frmBigmap->evtHide = _evtHidebigmap;

	d3dBigmap = dynamic_cast<C3DCompent*>(frmBigmap->Find("d3dCompent"));
	if (!d3dBigmap)
		return Error(RES_STRING(CL_LANGUAGE_MATCH_45), frmBigmap->GetName(), "d3dCompent");
	d3dBigmap->SetRenderEvent(_RenderBigmapEvent);

	{ // Zoom buttons
		constexpr auto ZOOM_STEP{1.0f};
		if (auto btnZoomIn = dynamic_cast<CTextButton*>(frmBigmap->Find("btnZoomIn")); btnZoomIn) {
			btnZoomIn->evtMouseClick = [](CGuiData* pSender, int x, int y, DWORD key) {
				if (auto scene = dynamic_cast<CWorldScene*>(CGameApp::GetCurScene()); scene) {
					if (auto map = scene->GetLargerMap(); map) {
						map->SetScale(map->GetScale() + ZOOM_STEP);
					}
				}
			};
		}

		if (auto btnZoomOut = dynamic_cast<CTextButton*>(frmBigmap->Find("btnZoomOut")); btnZoomOut) {
			btnZoomOut->evtMouseClick = [](CGuiData* pSender, int x, int y, DWORD key) {
				if (auto scene = dynamic_cast<CWorldScene*>(CGameApp::GetCurScene()); scene) {
					if (auto map = scene->GetLargerMap(); map) {
						map->SetScale(map->GetScale() - ZOOM_STEP);
					}
				}
			};
		}
	}

	if (auto checkFollow = dynamic_cast<CCheckBox*>(frmBigmap->Find("checkFollow")); checkFollow) {
		checkFollow->evtCheckChange = [](CGuiData* pSender) {
			if (auto scene = CGameApp::GetCurScene(); scene) {
				if (auto map = scene->GetLargerMap(); map) {
					map->SetFollow(static_cast<CCheckBox*>(pSender)->GetIsChecked());
				}
			}
		};
	}

	// �״�
	frmRadar = _FindForm("frmSearch");
	if (!frmRadar)
		return false;
	frmRadar->evtEntrustMouseEvent = _RadarFormEvent;
	frmRadar->evtBeforeShow = _RadarFormBeforeShowEvent;

	edtX = dynamic_cast<CEdit*>(frmRadar->Find("edtLeft"));
	if (!edtX)
		return Error(RES_STRING(CL_LANGUAGE_MATCH_719), frmRadar->GetName(), "edtLeft");
	edtX->SetIsDigit(true);
	edtX->evtEnter = _evtRadarEnter;

	edtY = dynamic_cast<CEdit*>(frmRadar->Find("edtRight"));
	if (!edtY)
		return Error(RES_STRING(CL_LANGUAGE_MATCH_719), frmRadar->GetName(), "edtRight");
	edtY->SetIsDigit(true);
	edtY->evtEnter = _evtRadarEnter;

	cboAddr = dynamic_cast<CCombo*>(frmRadar->Find("cboMap"));
	if (!cboAddr)
		return Error(RES_STRING(CL_LANGUAGE_MATCH_719), frmRadar->GetName(), "cboMap");

	btnPosSearch  = dynamic_cast<CTextButton*>(frmMinimap->Find("btnPosSearch"));
		if(!btnPosSearch) return false;
	btnPosSearch->evtMouseClick = _evtShowNPCList;

	btnBossTimer = dynamic_cast<CTextButton*>(frmMinimap->Find("btnBossTimer"));
	if (!btnBossTimer)
		return false;
	// Button click handled via frmMinimap->evtEntrustMouseEvent -> _MiniFormMouseEvent

	chkID = dynamic_cast<CCheckBox*>(frmRadar->Find("chkID"));
	if (chkID) chkID->SetIsChecked(true);

	return true;
}

void CMiniMapMgr::End() {
}

void CMiniMapMgr::FrameMove(DWORD dwTime) {
	static CTimeWork time(1000);
	if (time.IsTimeOut(dwTime)) {
		SetClockStringClientSide();
	}
}

void CMiniMapMgr::_evtShowbigmap(CGuiData* pSender) {
	CWorldScene* pWorldScene = dynamic_cast<CWorldScene*>(CGameApp::GetCurScene());
	if (!pWorldScene)
		return;

	g_pGameApp->Waiting();
	CS_MapMask();

	// Э�鷢�ͻ���ʱ������ʾ
	// if( CGameApp::GetCurScene() && CGameApp::GetCurScene()->GetLargerMap() )
	//{
	//	CGameApp::GetCurScene()->GetLargerMap()->Show( true );
	//}
}

void CMiniMapMgr::_evtHidebigmap(CGuiData* pSender) {
	CWorldScene* pScene = dynamic_cast<CWorldScene*>(CGameApp::GetCurScene());
	if (!pScene)
		return;

	if (CGameApp::GetCurScene() && CGameApp::GetCurScene()->GetLargerMap()) {
		CGameApp::GetCurScene()->GetLargerMap()->Show(false);
	}
}

void CMiniMapMgr::_RenderBigmapEvent(C3DCompent* pSender, int x, int y) {
	if (CGameApp::GetCurScene() && CGameApp::GetCurScene()->GetLargerMap()) {
		CGameApp::GetCurScene()->GetLargerMap()->Render();
	}

	if (g_stUIMap.GetBigmapForm() && g_stUIMap.GetBigmapRect()) {
		g_stUIMap._RenderBigMapHint();
	}
}

void CMiniMapMgr::_evtRadarEnter(CGuiData* pSender) {
	ShowRadar();
}

void CMiniMapMgr::RefreshChaPos(int x, int y) {
	static CTimeWork time(1000);
	if (time.IsTimeOut(CGameApp::GetCurTick())) {
		int n = rand() % 9;
		labMapPos->SetIsShow(false);

		labMapPos = labMapPosRand[n];
		labMapPos->SetIsShow(true);
	}

	static char buf[32] = {0};
	sprintf(buf, "%d,%d", x / 100, y / 100);
	labMapPos->SetCaption(buf);
}

void CMiniMapMgr::RefreshMapName(const char* name) {
	if (labMapName)
		labMapName->SetCaption(name);
}

void CMiniMapMgr::SetClockStringClientSide() {
	// Get current client UTC time and adjust for clock drift
	std::time_t clientUTC = std::time(nullptr);
	std::time_t serverUTC = clientUTC + tServerTimeOffset;
	
	// Convert server UTC to server's local time by adding timezone offset
	std::time_t serverLocalTime = serverUTC + tServerTzOffset;
	
	// Use gmtime since we've already adjusted for timezone
	struct std::tm* ptm = gmtime(&serverLocalTime);
	static char buf[32];
	std::strftime(buf, sizeof(buf), "%T", ptm);
	labClock->SetCaption(buf);
	tServerTime = serverLocalTime;
}

void CMiniMapMgr::SetServerTime(long long serverTimestamp, long serverTzOffset) {
	// Calculate clock drift between server and client
	std::time_t clientTime = std::time(nullptr);
	tServerTimeOffset = static_cast<time_t>(serverTimestamp) - clientTime;
	
	// Store server's timezone offset from UTC
	tServerTzOffset = serverTzOffset;
	
	tServerTime = static_cast<time_t>(serverTimestamp);
	
	// Immediately update the clock display
	SetClockStringClientSide();
}

void CMiniMapMgr::_MiniFormMouseEvent(CCompent* pSender, int nMsgType, int x, int y, DWORD dwKey) {
	string name = pSender->GetName();

	if (name == "btnOpen") {
		// �򿪻�رմ��ͼ
		if (!g_stUIMap.frmBigmap->GetIsShow())
			g_stUIMap.frmBigmap->Show();
		else
			g_stUIMap.frmBigmap->Hide();
	} else if (name == "btnSearch") {

		frmRadar->SetIsShow(!frmRadar->GetIsShow());
	} else if (name == "btnStall") {
		g_stUIBooth.SearchAllStalls();
		// frmRadar->SetIsShow( !frmRadar->GetIsShow() );
	} else if (name == "btnRank") {
		CProCirculateCS* proCir = (CProCirculateCS*)g_NetIF->GetProCir();
		proCir->OpenRankings();
	} else if (name == "btnOpenTempBag") {
		// ����ʱ����
		CUIInterface::MainChaMove();
		// g_stUIStore.ShowTempKitbag();
	} else if (name == "btnOpenStore") {
		// ���̳������������
		g_stUIStore.OpenStoreAsk();
	} else if (name == "btnteam") {
		CS_VolunteerOpen(10);
	} else if (name == "btnBossTimer") {
		// Open Boss Timer form via C++ UI manager
		g_stUIBossTimer.ToggleForm();
	}
	// if( name=="btnClose" || name == "btnNo" )
	//{
	//	CGameApp::GetCurScene()->ShowMinimap( false ) ;
	//	pSender->GetForm()->Close();
	//	return;

	//}
	// float fCurPos = CGameApp::GetCurScene()->GetSmallMap()->GetDist();
	// if (name =="btnSmall")
	//{
	//	fCurPos +=5.0f;
	//	if (fCurPos >=40.0f ) fCurPos = 40.0f ;
	//	CGameApp::GetCurScene()->GetSmallMap()->SetDist( fCurPos );
	//}
	// else if (name == "btnBig" )
	//{
	//	fCurPos -= 5.0f;
	//	if ( fCurPos <= 20.0f ) fCurPos = 20.0f ;
	//	CGameApp::GetCurScene()->GetSmallMap()->SetDist( fCurPos );
	//}
	// else if (name == "btnHelp" )
	//{
	//	CForm* frmHelp = CFormMgr::s_Mgr.Find( "frmHelp" );
	//	if( frmHelp )
	//	{
	//		CMemo* memCtrl = dynamic_cast<CMemo*>(frmHelp->Find( "memCtrl" ));
	//		if( memCtrl )
	//		{
	//			memCtrl->Init();
	//			memCtrl->SetCaption( "��������Online������ָ��_��    �ߣ������������ס���ſɳ�������_ս    �����ڳ���Թ��ﵥ�������ս��_NPC�Ի�������������NPC�ɽ��жԻ�_�ӽ�ת������ס�Ҽ���ˮƽ��ת���������ֿ���_    ����Զ��˫���Ҽ��ص�Ĭ�ϵ��ӽ�_��    �ӣ���������ҵ��Ҽ���ѡ��������ӣ�_    ��Ӻ�ֻ�жӳ���������������Ҽ���_��    �죺Alt+F���������·���ť�ɴ�_     ������������ͷ�񣬻���ֶԻ���_��    �ף���������ҵ���Ҽ���ѡ�����뽻��_     ��Է�ͬ��󣬵�����Ҽ佻�׽���" );
	//			memCtrl->ProcessCaption();
	//		}
	//		frmHelp->Refresh();
	//		frmHelp->Show();
	//	}
	//}
	return;
}

void CMiniMapMgr::_RadarFormEvent(CCompent* pSender, int nMsgType, int x, int y, DWORD dwKey) {
	string name = pSender->GetName();

	if (name == "btnYes") {
		ShowRadar();
	}
}

void CMiniMapMgr::_RadarFormBeforeShowEvent(CForm* pForm, bool& IsShow) {
	if (!CGameApp::GetCurScene())
		return;

	IsShow = true;

	// �õ����Ŀǰ���ڵĵ�ͼ����
	CMapInfo* pMapInfo = GetMapInfo(CGameApp::GetCurScene()->GetInitParam()->strMapFile.c_str());
	if (!pMapInfo)
		cboAddr->GetList()->GetItems()->Select(0);
	else {
		int maxItemNum = cboAddr->GetList()->GetItems()->GetCount();
		CItemRow* pItemRow(0);
		int i(0);
		for (; i < maxItemNum; i++) {
			CItemRow* pItemRow = cboAddr->GetList()->GetItems()->GetItem(i);
			if (pItemRow) {
				if (strcmp(pMapInfo->szName, pItemRow->GetBegin()->GetString()) == 0)
					break;
			}
		}
		if (i < maxItemNum) {
			cboAddr->GetList()->GetItems()->Select(i);
		} else {
			cboAddr->GetList()->GetItems()->Select(0);
		}
	}

	edtX->SetCaption("");
	edtY->SetCaption("");
	edtX->SetActive(edtX);

	if (frmRadar->GetIsShow()) {
		frmRadar->Close();
	}
	if (CNavigationBar::g_cNaviBar.IsShow()) {
		CNavigationBar::g_cNaviBar.Show(false);
	}
}

int CMiniMapMgr::CheckCoordinateEdit(const char* input) {
	if (!(*input))
		return -1;

	while (*input) {
		if (0x30 <= (*input) && (*input) <= 0x39) // �ж�������
		{
			input++;
		} else {
			return -1;
		}
	}
	return 0;
}

void CMiniMapMgr::ClearRadar() {
	edtX->SetCaption("");
	edtY->SetCaption("");
	cboAddr->GetList()->GetItems()->Select(0);
}
void CMiniMapMgr::ShowRadar(const char* szX, const char* szY) {
	if (CHECK_FAILED(CheckCoordinateEdit(szX))) {
		g_pGameApp->MsgBox(RES_STRING(CL_LANGUAGE_MATCH_720));
		edtX->SetCaption("");
		edtX->SetActive(edtX);
		return;
	}
	if (CHECK_FAILED(CheckCoordinateEdit(szY))) {
		g_pGameApp->MsgBox(RES_STRING(CL_LANGUAGE_MATCH_720));
		edtY->SetCaption("");
		edtY->SetActive(edtY);
		return;
	}

	int x = atoi(szX), y = atoi(szY);
	D3DXVECTOR3 target((float)x, (float)y, 0);

	const char* _strName = g_pGameApp->GetCurScene()->GetTerrainName();
	if (strcmp(_strName, "garner") == 0)
		_strName = RES_STRING(CL_LANGUAGE_MATCH_56);
	else if (strcmp(_strName, "magicsea") == 0)
		_strName = RES_STRING(CL_LANGUAGE_MATCH_58);
	else if (strcmp(_strName, "darkblue") == 0)
		_strName = RES_STRING(CL_LANGUAGE_MATCH_59);
	else if (strcmp(_strName, "winterland") == 0)
		_strName = "Winter Isle Archipelago";
	else if (strcmp(_strName, "jialebi") == 0)
		_strName = "Pirate\'s Base";

	CWorldScene* pScene = dynamic_cast<CWorldScene*>(CGameApp::GetCurScene());
	if (!pScene)
		return;
	const char* curMap = pScene->GetCurMapInfo()->szName;

	if ((strcmp(curMap, _strName) == 0) && (!pScene->GetMainCha()->IsBoat())) {
		if (g_stKnowledgeBase.IsAutoPath()) {
			g_cFindPathEx.Reset();
			g_cFindPathEx.ClearDestDirection();
			g_cFindPathEx.SetDestDirection((int)pScene->GetMainCha()->GetPos().x, (int)pScene->GetMainCha()->GetPos().y, x, y);
			g_cFindPathEx.SetTarget((int)pScene->GetMainCha()->GetPos().x, (int)pScene->GetMainCha()->GetPos().y, x, y);
		}
	} else {
		if (pScene->GetMainCha()->IsBoat()) {
			g_stUIBox.ShowMsgBox(nullptr, "No routing on sea.");
		} else {
			g_stUIBox.ShowMsgBox(nullptr, "Not on the same map.");
		}
	}
	CNavigationBar::g_cNaviBar.SetTarget((char*)_strName, target);
	CNavigationBar::g_cNaviBar.Show(true);
}
void CMiniMapMgr::ShowRadar() {
	const char* szX = edtX->GetCaption();
	const char* szY = edtY->GetCaption();
	if (CHECK_FAILED(CheckCoordinateEdit(szX))) {
		g_pGameApp->MsgBox(RES_STRING(CL_LANGUAGE_MATCH_720));
		edtX->SetCaption("");
		edtX->SetActive(edtX);
		return;
	}
	if (CHECK_FAILED(CheckCoordinateEdit(szY))) {
		g_pGameApp->MsgBox(RES_STRING(CL_LANGUAGE_MATCH_720));
		edtY->SetCaption("");
		edtY->SetActive(edtY);
		return;
	}

	edtX->SetActive(edtX);
	int x = atoi(szX), y = atoi(szY);
	D3DXVECTOR3 target((float)x, (float)y, 0);

	const char* szAddress = cboAddr->GetText();

	CWorldScene* pScene = dynamic_cast<CWorldScene*>(CGameApp::GetCurScene());
	if (!pScene)
		return;
	const char* curMap = pScene->GetCurMapInfo()->szName;

	if ((strcmp(curMap, szAddress) == 0) && (!pScene->GetMainCha()->IsBoat())) {
		if (g_stUIMap.chkID->GetIsChecked()) {
			g_cFindPathEx.Reset();
			g_cFindPathEx.ClearDestDirection();
			g_cFindPathEx.SetDestDirection((int)pScene->GetMainCha()->GetPos().x, (int)pScene->GetMainCha()->GetPos().y, x, y);
			g_cFindPathEx.SetTarget((int)pScene->GetMainCha()->GetPos().x, (int)pScene->GetMainCha()->GetPos().y, x, y);
		}
	} else {
		if (pScene->GetMainCha()->IsBoat()) {
			g_stUIBox.ShowMsgBox(nullptr, "No routing on sea.");
		} else {
			g_stUIBox.ShowMsgBox(nullptr, "Not on the same map.");
		}
	}
	CNavigationBar::g_cNaviBar.SetTarget((char*)szAddress, target);
	CNavigationBar::g_cNaviBar.Show(true);
	frmRadar->Hide();
}

void CMiniMapMgr::SwitchMap() {
	frmBigmap->Close();
}

bool CMiniMapMgr::IsShowBigmap() {
	return frmBigmap->GetIsShow();
}

void CMiniMapMgr::CloseRadar() {
	if (frmRadar && frmRadar->GetIsShow()) {
		this->ClearRadar();
		frmRadar->Close();
	}

	if (CNavigationBar::g_cNaviBar.IsShow()) {
		CNavigationBar::g_cNaviBar.Show(false);
	}
}

bool CMiniMapMgr::IsPKSilver() {
	return false;											  // Disables blind CA.
	if (0 == strcmp(GetMapName(), RES_STRING(CL_LANGUAGE_MATCH_900))) // �Ҷ������� //"������"))
		return true;

	return false;
}

bool CMiniMapMgr::IsGuildWar() {
	if (0 == strcmp(GetMapName(), RES_STRING(CL_LANGUAGE_MATCH_934)) || 0 == strcmp(GetMapName(), RES_STRING(CL_LANGUAGE_MATCH_934))) // "ʥս" "Сʥս"
		return true;

	return false;
}
// Add by sunny.sun 20080904
// Begin
void CMiniMapMgr::_RenderBigMapHint(void) {
	// FontModule::Font* pfont = g_pGameApp->GetFont();
	// CMPFont				g_CFont
	if (/*!pfont ||*/ !CGameApp::GetCurScene() || !CGameApp::GetCurScene()->GetLargerMap())
		return;

	POINT ptMouse; // ������꣨�������꣩
	GetCursorPos(&ptMouse);
	ScreenToClient(g_pGameApp->GetHWND(), &ptMouse);

	RECT rcBigMap; // ���ͼ���Σ��������꣩
	rcBigMap.left = GetBigmapForm()->GetLeft() + GetBigmapRect()->GetLeft();
	rcBigMap.top = GetBigmapForm()->GetTop() + GetBigmapRect()->GetTop();
	rcBigMap.right = rcBigMap.left + GetBigmapRect()->GetWidth();
	rcBigMap.bottom = rcBigMap.top + GetBigmapRect()->GetHeight();

	POINT ptBigMapCenter; // ���ͼ�������꣨�������꣩
	ptBigMapCenter.x = (rcBigMap.right - rcBigMap.left) >> 1;
	ptBigMapCenter.y = (rcBigMap.bottom - rcBigMap.top) >> 1;

	POINT ptCenter; // ���ͼ�������꣨��Ϸ���꣩
	ptCenter.x = CGameApp::GetCurScene()->GetLargerMap()->GetCenterX() / 100;
	ptCenter.y = CGameApp::GetCurScene()->GetLargerMap()->GetCenterY() / 100;

	constexpr auto MAP_SCALE = 1.25f; // ���ͼ���ر�����һ���Ʋ�� magic number  :)
	const auto map_scale_modifier = CGameApp::GetCurScene()->GetLargerMap()->GetScale();

	// �������ǺͲ���վ���үҪ��
	if (0 == stricmp(CGameApp::GetCurScene()->GetTerrainName(), "garner") || 0 == stricmp(CGameApp::GetCurScene()->GetTerrainName(), "magicsea") || 0 == stricmp(CGameApp::GetCurScene()->GetTerrainName(), "darkblue")) {
		struct SApplyInfo {
			int nAreaID; // ��Ӧ AreaSet ����� ID
			int x, y;
		};

		SApplyInfo stApply[8];

		//     static SApplyInfo stApply[] =
		//     {
		//{	1,	2218,	2759	},	//������
		//{	55,	1891,	2800	},	//�Ͽ󲹸�վ
		//{	58,	1509,	3093	},	//���￩������վ
		//{	53,	1002,	2972	},	//����ɭ�ֲ���վ
		//{	61,	1114,	2773	},	//�߶�ŵ����վ
		//{	63,	526,	2440	},	//���ò���վ
		//{	8,	736,	1511	},	//������
		//{	13,	855,	3568	},	//ɳ��³�
		//{	57,	781,	3118	},	//ɳȪ����վ
		//{	62,	1204,	3204	},	//�Ͳ�����վ
		//{	20,	1319,	521		},	//���Ǳ��³�
		//{	56,	794,	353		},	//��������վ
		//{	59,	1048,	648		},	//������˹����վ
		//{	60,	2137,	551		},	//����Ӫ�ز���վ
		//{	54,	611,	2097	},	//�����ӵ²���վ
		//     };

		if (0 == stricmp(CGameApp::GetCurScene()->GetTerrainName(), "garner")) {
			stApply[0].nAreaID = 1;
			stApply[0].x = 2218;
			stApply[0].y = 2759;

			stApply[1].nAreaID = 55;
			stApply[1].x = 1891;
			stApply[1].y = 2800;

			stApply[2].nAreaID = 58;
			stApply[2].x = 1509;
			stApply[2].y = 3093;

			stApply[3].nAreaID = 53;
			stApply[3].x = 1002;
			stApply[3].y = 2972;

			stApply[4].nAreaID = 61;
			stApply[4].x = 1114;
			stApply[4].y = 2773;

			stApply[5].nAreaID = 63;
			stApply[5].x = 526;
			stApply[5].y = 2440;

			stApply[6].nAreaID = 8;
			stApply[6].x = 736;
			stApply[6].y = 1511;

			stApply[7].nAreaID = 54;
			stApply[7].x = 611;
			stApply[7].y = 2097;
		}
		if (0 == stricmp(CGameApp::GetCurScene()->GetTerrainName(), "magicsea")) {
			stApply[0].nAreaID = 57;
			stApply[0].x = 781;
			stApply[0].y = 3118;

			stApply[1].nAreaID = 13;
			stApply[1].x = 855;
			stApply[1].y = 3568;

			stApply[2].nAreaID = 62;
			stApply[2].x = 1204;
			stApply[2].y = 3204;

			stApply[3].nAreaID = 0;
			stApply[3].x = 0;
			stApply[3].y = 0;

			stApply[4].nAreaID = 0;
			stApply[4].x = 0;
			stApply[4].y = 0;

			stApply[5].nAreaID = 0;
			stApply[5].x = 0;
			stApply[5].y = 0;

			stApply[6].nAreaID = 0;
			stApply[6].x = 0;
			stApply[6].y = 0;

			stApply[7].nAreaID = 0;
			stApply[7].x = 0;
			stApply[7].y = 0;
		}
		if (0 == stricmp(CGameApp::GetCurScene()->GetTerrainName(), "darkblue")) {
			stApply[0].nAreaID = 20;
			stApply[0].x = 1319;
			stApply[0].y = 521;

			stApply[1].nAreaID = 56;
			stApply[1].x = 794;
			stApply[1].y = 353;

			stApply[2].nAreaID = 59;
			stApply[2].x = 1048;
			stApply[2].y = 648;

			stApply[3].nAreaID = 60;
			stApply[3].x = 2137;
			stApply[3].y = 551;

			stApply[4].nAreaID = 0;
			stApply[4].x = 0;
			stApply[4].y = 0;

			stApply[5].nAreaID = 0;
			stApply[5].x = 0;
			stApply[5].y = 0;

			stApply[6].nAreaID = 0;
			stApply[6].x = 0;
			stApply[6].y = 0;

			stApply[7].nAreaID = 0;
			stApply[7].x = 0;
			stApply[7].y = 0;
		}

		RECT rcBigMapArea; // ���ͼ������Ϸ���꣩
		rcBigMapArea.left = ptCenter.x - ((int)((rcBigMap.right - rcBigMap.left) * MAP_SCALE * map_scale_modifier) >> 1);
		rcBigMapArea.right = ptCenter.x + ((int)((rcBigMap.right - rcBigMap.left) * MAP_SCALE * map_scale_modifier) >> 1);
		rcBigMapArea.top = ptCenter.y - ((int)((rcBigMap.bottom - rcBigMap.top) * MAP_SCALE * map_scale_modifier) >> 1);
		rcBigMapArea.bottom = ptCenter.y + ((int)((rcBigMap.bottom - rcBigMap.top) * MAP_SCALE * map_scale_modifier) >> 1);

		int nCount = sizeof(stApply) / sizeof(stApply[0]);
		for (int i = 0; i < nCount; ++i) {
			if (rcBigMapArea.left < stApply[i].x && stApply[i].x < rcBigMapArea.right &&
				rcBigMapArea.top < stApply[i].y && stApply[i].y < rcBigMapArea.bottom) {
				CAreaInfo* pInfo = GetAreaInfo(stApply[i].nAreaID);
				if (!pInfo)
					continue;

				POINT ptRender; // ���� ����վ�����꣨��Ϸ����->��Ļ���꣩
				ptRender.x = rcBigMap.left + (LONG)((stApply[i].x - rcBigMapArea.left) / MAP_SCALE);
				ptRender.y = rcBigMap.top + (LONG)((stApply[i].y - rcBigMapArea.top) / MAP_SCALE);

				// SIZE size;    // ����������ĵ�ƫ��
				// pfont->GetTextSize(pInfo->szDataName, &size);
				// ptRender.x -= size.cx >> 1;
				// ptRender.y -= size.cy >> 1;

				CGuiFont::s_Font.Render(pInfo->szDataName, ptRender.x, ptRender.y, 0xFFFFFFFF);
			}
		}
	}

	// ���������ڴ��ͼ�����ڣ��������ʾ
	if (rcBigMap.left < ptMouse.x && ptMouse.x < rcBigMap.right &&
		rcBigMap.top < ptMouse.y && ptMouse.y < rcBigMap.bottom) {
		mouse_map_coordinate.x = ptCenter.x + (int)((ptMouse.x - ptBigMapCenter.x - rcBigMap.left) * MAP_SCALE / map_scale_modifier);
		mouse_map_coordinate.y = ptCenter.y + (int)((ptMouse.y - ptBigMapCenter.y - rcBigMap.top) * MAP_SCALE / map_scale_modifier);
		if (mouse_map_coordinate.x < 0 || mouse_map_coordinate.x > defMAP_GARNER_WIDTH ||
			mouse_map_coordinate.y < 0 || mouse_map_coordinate.y > defMAP_GARNER_WIDTH) {
			return;
		}
		/* Removed by Mdr May 2020 FPO Beta
				char szBuf[256] = {0};
				sprintf(szBuf, " Point: %d, %d ", ptCurMouse.x, ptCurMouse.y);

				SIZE size;
				g_pGameApp->GetFont()->GetTextSize(szBuf, &size);

				GetRender().FillFrame(ptMouse.x + 32, ptMouse.y, ptMouse.x + 32 + size.cx, ptMouse.y + size.cy);
			   g_pGameApp->GetFont()->DrawText(szBuf, ptMouse.x + 32, ptMouse.y);

		*/
		CMonsterInfo* pInfo = 0;
		int nID = 1;
		for (;;) {
			pInfo = GetMonsterInfo(nID++);
			if (!pInfo) {
				break;
			}

			if (pInfo->ptStart.x < mouse_map_coordinate.x && mouse_map_coordinate.x < pInfo->ptEnd.x &&
				pInfo->ptStart.y < mouse_map_coordinate.y && mouse_map_coordinate.y < pInfo->ptEnd.y) // && 0 == strcmp(CGameApp::GetCurScene()->GetTerrainName(), pInfo->szArea)
			{
				break;
			}
		}

		if (pInfo) {
			RECT rcMonster; // ˢ�ַ�Χ����Ϸ���꣩
			SetRect(&rcMonster, pInfo->ptStart.x, pInfo->ptStart.y,
					pInfo->ptEnd.x, pInfo->ptEnd.y);

			// �������飨��Ϸ����->��Ļ���꣩
			rcMonster.left = rcBigMap.left + ptBigMapCenter.x + (int)((rcMonster.left - ptCenter.x) / MAP_SCALE);
			rcMonster.top = rcBigMap.top + ptBigMapCenter.y + (int)((rcMonster.top - ptCenter.y) / MAP_SCALE);
			rcMonster.right = rcBigMap.left + ptBigMapCenter.x + (int)((rcMonster.right - ptCenter.x) / MAP_SCALE);
			rcMonster.bottom = rcBigMap.top + ptBigMapCenter.y + (int)((rcMonster.bottom - ptCenter.y) / MAP_SCALE);

			// ������Χ�ü�
			if (rcMonster.left < rcBigMap.left)
				rcMonster.left = rcBigMap.left;
			if (rcMonster.top < rcBigMap.top)
				rcMonster.top = rcBigMap.top;
			if (rcMonster.right > rcBigMap.right)
				rcMonster.right = rcBigMap.right;
			if (rcMonster.bottom > rcBigMap.bottom)
				rcMonster.bottom = rcBigMap.bottom;

			// ��ø����������Ϣ
			CChaRecord* pMonsterInfo[MONSTER_LIST_MAX] = {0};
			string strMonsterName[MONSTER_LIST_MAX];
			int nMonCount = 0;
			int nLevelSum = 0;
			SIZE sizeNameMax = {0};
			for (int i = 0; i < MONSTER_LIST_MAX; ++i) {
				if (0 == pInfo->nMonsterList[i])
					break;

				pMonsterInfo[i] = GetChaRecordInfo(pInfo->nMonsterList[i]);
				if (!pMonsterInfo[i])
					continue;

				nLevelSum += pMonsterInfo[i]->lLv;
				nMonCount++;

				char szBuf[256] = {0};
				sprintf(szBuf, " %d. %s   LV:%2d ", nMonCount, pMonsterInfo[i]->szName, pMonsterInfo[i]->lLv);
				strMonsterName[i] = szBuf;

				int sizeW, sizeH;
				CGuiFont::s_Font.GetSize(szBuf, sizeW, sizeH);
				if (sizeW > sizeNameMax.cx)
					sizeNameMax.cx = sizeW;
				if (sizeH > sizeNameMax.cy)
					sizeNameMax.cy = sizeH;
			}

			if (nMonCount > 0) {
				//	int nLevelCha = g_pGameApp->GetCurScene()->GetMainCha()->getGameAttr()->get(ATTR_LV);
				int nLevelCha = (int)g_pGameApp->GetCurScene()->GetMainCha()->getGameAttr()->get(ATTR_LV);
				int nLevelAvg = nLevelSum / nMonCount; // ������ֵ�ƽ���ȼ�

				DWORD dwAreaColor = 0;
				if (nLevelAvg > nLevelCha + 15)
					dwAreaColor = 0x00FF0000;
				else if (nLevelAvg > nLevelCha + 5)
					dwAreaColor = 0x00FFFF00;
				else if (nLevelAvg > nLevelCha - 5)
					dwAreaColor = 0x0000FF00;
				else if (nLevelAvg > nLevelCha - 15)
					dwAreaColor = 0x0000CCFF;
				else
					dwAreaColor = 0x00666666;

				BYTE byAlpha = 0x80;
				dwAreaColor &= 0x00FFFFFF;
				dwAreaColor |= (byAlpha << 24);

				GetRender().FillFrame(rcMonster.left, rcMonster.top, rcMonster.right, rcMonster.bottom, dwAreaColor);

				// �����б�
				RECT rcRender = rcBigMap;
				GetRender().FillFrame(rcBigMap.left, rcRender.top, rcRender.left + sizeNameMax.cx, rcRender.top + (sizeNameMax.cy * nMonCount)); //*nMonCount
				for (int i = 0; i < nMonCount; ++i) {
					CGuiFont::s_Font.Render(strMonsterName[i].c_str(), rcRender.left + 35, rcRender.top + 50, 0xFFFFFFFF);
					rcRender.top += sizeNameMax.cy;
				}
			}
		}

		//        // �������ָ�������
		char szBuf[256] = {0};
		sprintf(szBuf, " Point: %d, %d ", mouse_map_coordinate.x, mouse_map_coordinate.y);

		int sizeW, sizeH;
		CGuiFont::s_Font.GetSize(szBuf, sizeW, sizeH);

		GetRender().FillFrame(ptMouse.x + 32, ptMouse.y, ptMouse.x + 32 + sizeW, ptMouse.y + sizeH);
		CGuiFont::s_Font.Render(szBuf, ptMouse.x + 32, ptMouse.y, 0xFFFFFFFF);
	}
}
// End
void CMiniMapMgr::_evtShowNPCList(CGuiData* pSender, int x, int y, DWORD key) {
	g_stKnowledgeBase.Toggle();
}
#include "Stdafx.h"
#include "GameApp.h"
#include "Character.h"
#include "Scene.h"
#include "GameConfig.h"
#include "GameAppMsg.h"
#include "packetcmd.h"
#include "MPEditor.h"
#include "MapSet.h"
#include "NetProtocol.h"
#include "worldscene.h"
#include "LoginScene.h"
#include "UIChat.h"
#include "UITeam.h"
#include "uiboxform.h"
#include "cameractrl.h"
#include "uisystemform.h"
#include "UIBoatForm.h"

static void _Disconnect(CCompent* pSender, int nMsgType, int x, int y, DWORD dwKey) {
	// Clear character references before scene change to prevent use-after-free
	g_stUIBoat.Clear();

	CLoginScene* pLogin = dynamic_cast<CLoginScene*>(CGameApp::GetCurScene());
	if (pLogin) {
		pLogin->ShowRegionList();
		// pLogin->ShowLoginForm();
	} else {
		g_pGameApp->LoadScriptScene(enumLoginScene);

		CLoginScene* pLogin = dynamic_cast<CLoginScene*>(CGameApp::GetCurScene());
		if (pLogin) {
			pLogin->ShowRegionList();
			// pLogin->ShowLoginForm();
		}
	}
}

static void _SwitchMapFailed(CCompent* pSender, int nMsgType, int x, int y, DWORD dwKey) {
	CLoginScene* pLogin = dynamic_cast<CLoginScene*>(CGameApp::GetCurScene());
	if (!pLogin) {
		g_pGameApp->LoadScriptScene(enumLoginScene);
		pLogin = dynamic_cast<CLoginScene*>(CGameApp::GetCurScene());
	}

	if (pLogin) {
		if (g_NetIF->IsConnected())
			pLogin->ShowChaList();
		else
			pLogin->ShowRegionList();
		// pLogin->ShowLoginForm();
	}
}

void CGameApp::_HandleMsg(DWORD dwTypeID, WPARAM wParam, LPARAM lParam) {
	switch (dwTypeID) {
	case APP_NET_LOGINRET:
		break;

	case APP_NET_DISCONNECT: {
		Waiting(false);

		if (g_TomServer.bEnable) {
			MessageBox(g_pGameApp->GetHWND(), RES_STRING(CL_LANGUAGE_MATCH_134), "error", 0);
			g_pGameApp->SetIsRun(false);
			return;
		}

		CGameScene* scene = CGameApp::GetCurScene();
		if (!scene)
			return;

		if (wParam == 1000) {
			char szBuf[128] = {0};
			sprintf(szBuf, RES_STRING(CL_LANGUAGE_MATCH_135), WSAGetLastError());
			g_stUIBox.ShowMsgBox(_Disconnect, szBuf);
			return;
		}
		// else if(wParam!=DS_DISCONN && wParam!=DS_SHUTDOWN )
		{
			if (!dynamic_cast<CLoginScene*>(scene)) {
				if (!g_ChaExitOnTime.TimeArrived()) {
					// 非登陆场景断开连接
					char szBuf[256] = {0};
					if (g_NetIF) {
						// modify by Philip.Wu  2006-06-09  非收费系统不提示用户充值
						if (g_Config.m_IsBill) {
							sprintf(szBuf, RES_STRING(CL_LANGUAGE_MATCH_136), RES_STRING(CL_LANGUAGE_MATCH_137), (DWORD)wParam);
						} else {
							// sprintf( szBuf, "您现在已与服务器断开连接,确定后将返回登录界面\n原因:%s[%d]", "您已在其他地方登陆！", wParam );

							const auto reason = g_NetIF->GetDisconnectErrText((int)wParam);
							sprintf(szBuf, reason.c_str());
						}
					} else {
						sprintf(szBuf, RES_STRING(CL_LANGUAGE_MATCH_139), (DWORD)wParam);
					}

					g_stUIBox.ShowMsgBox(_Disconnect, szBuf);

					extern bool g_HaveGameMender;
					if (g_HaveGameMender) {
						g_pGameApp->MsgBox(RES_STRING(CL_LANGUAGE_MATCH_140));
					}
				}
			} else { // 登陆场景内断开连接
				// 判断密码是否错误
				CLoginScene* pkLogin = dynamic_cast<CLoginScene*>(scene);
				if (pkLogin && pkLogin->IsPasswordError()) {
					pkLogin->ShowLoginForm();
					return;
				}
			}
		}
	} break;
	case APP_SWITCH_MAP_FAILED: {
		g_pGameApp->Waiting(false);

		char szBuf[256] = {0};
		sprintf(szBuf, "%s[%d]", g_GetServerError((int)wParam), (int)wParam);
		g_stUIBox.ShowMsgBox(_SwitchMapFailed, szBuf);
	} break;
	case APP_SWITCH_MAP: {
		extern MPEditor g_Editor;
		g_Editor.Init((int)wParam);
		CMapInfo* pInfo = GetMapInfo((int)wParam);
		CCameraCtrl* pCam = g_pGameApp->GetMainCam();
		auto v = D3DXVECTOR3((float)pInfo->nInitX, (float)pInfo->nInitY, 0);
		pCam->SetFollowObj(v);
		// EnableCameraFollow(FALSE);
		break;
	} break;
	}
}

#include "stdafx.h"
#include "GameConfig.h"

#ifdef _LUA_GAME

#include "resource.h"
#include "lua_platform.h"
#include "UIFont.h"

using namespace std;
using namespace GUI;

lua_State* L = nullptr;
HWND g_dlgScript = nullptr;
list<string> g_luaFNList;

int lua_sysAddHelpString(lua_State* L) {
	const char* pszHelp = lua_tostring(L, 1);
	g_luaFNList.push_back(pszHelp);
	return 0;
}

// Native Lua wrapper for UI_CreateFont - required for x64 since CLU_Call uses inline asm
// This function can be called directly from Lua as UI_CreateFont_Native(font, size800, size1024, style)
// or wrapped by font.clu's UI_CreateFont function
int lua_UI_CreateFont(lua_State* L) {
	const char* font = lua_tostring(L, 1);
	int size800 = (int)lua_tonumber(L, 2);
	int size1024 = (int)lua_tonumber(L, 3);
	int nStyle = (int)lua_tonumber(L, 4);
	
	int result = CGuiFont::s_Font.CreateFont((char*)font, size800, size1024, (DWORD)nStyle);
	lua_pushnumber(L, result);
	return 1;
}

void InitLuaPlatform() {
	LG("init", RES_STRING(CL_LANGUAGE_MATCH_182));

	LG("init", "Creating Lua state...\n");
	L = luaL_newstate();
	LG("init", "Lua state created: %p\n", (void*)L);
	
	if (!L) {
		LG("init", "FATAL: luaL_newstate() returned NULL!\n");
		return;
	}
	
	LG("init", "Opening Lua libs...\n");
	luaL_openlibs(L);
	LG("init", "Lua libs opened.\n");

#define REGFN(fn) (lua_pushstring(L, "" #fn ""),      \
				   lua_pushcfunction(L, lua_##fn),    \
				   lua_settable(L, LUA_GLOBALSINDEX), \
				   g_luaFNList.push_back("" #fn ""))

	lua_register(L, "sysAddHelpString", lua_sysAddHelpString);

	// util??????
	g_luaFNList.push_back("[util]");
	REGFN(MsgBox);
	REGFN(GetTickCount);
	REGFN(Rand);
	REGFN(LG);
	REGFN(SysInfo);
	g_luaFNList.push_back("\r");

	// app??????
	g_luaFNList.push_back("[app]");
	REGFN(appGetCurScene);
	REGFN(appSetCaption);
	REGFN(appPlaySound);
	REGFN(appUpdateRender);
	// lua_dofile(L, "scripts/gamesdk/app.lua");
	g_luaFNList.push_back("\r");

	// Melee attack restriction for caster classes
	REGFN(SetDisableMeleeForCasters);
	REGFN(GetDisableMeleeForCasters);

	// scene??????
	g_luaFNList.push_back("[scene]");
	REGFN(sceAddObj);
	REGFN(sceRemoveObj);
	REGFN(sceGetObj);
	REGFN(sceSetMainCha);
	REGFN(sceGetMainCha);
	REGFN(sceGetHoverCha);
	REGFN(sceEnableDefaultMouse);
	// lua_dofile(L, "scripts/gamesdk/scene.lua");
	g_luaFNList.push_back("\r");

	// object??????
	g_luaFNList.push_back("[object]");
	REGFN(objSetPos);
	REGFN(objGetPos);
	REGFN(objSetFaceAngle);
	REGFN(objGetFaceAngle);
	REGFN(objSetAttr);
	REGFN(objGetAttr);
	REGFN(objIsValid);
	REGFN(objGetID);
	REGFN(chaMoveTo);
	REGFN(chaSay);
	REGFN(chaChangePart);
	REGFN(chaPlayPose);
	REGFN(chaStop);
	// lua_dofile(L, "scripts/gamesdk/object.lua");
	g_luaFNList.push_back("\r");

	// ????????
	g_luaFNList.push_back("[camera]");
	REGFN(camGetCenter);
	REGFN(camSetCenter);
	REGFN(camFollow);
	REGFN(camMoveForward);
	REGFN(camMoveLeft);
	REGFN(camMoveUp);
	REGFN(camSetAngle);
	g_luaFNList.push_back("\r");

	// Input??????
	g_luaFNList.push_back("[input]");
	REGFN(IsKeyDown);
	// lua_dofile(L, "scripts/gamesdk/input.lua");
	g_luaFNList.push_back("\r");

	// UI??????
	g_luaFNList.push_back("[UI]");
	REGFN(uiHideAll);
	// Register native UI_CreateFont for x64 compatibility (CLU_Call doesn't work on x64)
	lua_register(L, "UI_CreateFont_Native", lua_UI_CreateFont);
	g_luaFNList.push_back("UI_CreateFont_Native");
	
	// Boss Timer UI functions
	REGFN(uiRequestBossTimerData);
	
	// lua_dofile(L, "scripts/gamesdk/input.lua");
	g_luaFNList.push_back("\r");
	
	LG("init", "InitLuaPlatform() completed successfully.\n");
}

BOOL CALLBACK ScriptDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_INITDIALOG: {
		HWND hEdit1 = GetDlgItem(hwndDlg, IDC_EDIT1);
		// extern CInputBox g_InputBox;
		// g_InputBox.SetEditWindow(hEdit1);

		HWND hEdit = GetDlgItem(hwndDlg, IDC_EDIT2);
		for (list<string>::iterator it = g_luaFNList.begin(); it != g_luaFNList.end(); it++) {
			SendMessage(hEdit, EM_REPLACESEL, 0, (LPARAM)(*it).c_str());
			SendMessage(hEdit, EM_REPLACESEL, 0, (LPARAM) "\r\n");
		}
		ShowWindow(hEdit, SW_HIDE);
		break;
	}
	case WM_KEYDOWN: {
		break;
	}
	case WM_COMMAND: {
		HWND hEdit1 = GetDlgItem(hwndDlg, IDC_EDIT1);
		HWND hEdit2 = GetDlgItem(hwndDlg, IDC_EDIT2);
		HWND hButHelp = GetDlgItem(hwndDlg, IDFNLIST);

		switch (wParam) {
		case IDOK: {
			FILE* fp = fopen("tmp.txt", "wt");
			if (fp == nullptr)
				break;
			char szText[8192];
			int n = GetWindowText(hEdit1, szText, 8192);
			fwrite(szText, n, 1, fp);
			fclose(fp);
			luaL_dofile(L, "tmp.txt");
			break;
		}
		case IDFNLIST: {
			if (SendMessage(hButHelp, BM_GETCHECK, 0, 0) == BST_CHECKED) {
				ShowWindow(hEdit1, SW_HIDE);
				ShowWindow(hEdit2, SW_SHOW);
			} else {
				ShowWindow(hEdit2, SW_HIDE);
				ShowWindow(hEdit1, SW_SHOW);
			}
			break;
		}
		case IDCLEAR: {
			SetWindowText(hEdit1, "");
			break;
		}
		case IDCANCEL: {
			ShowWindow(hwndDlg, SW_HIDE);
		}
		}
		break;
	}
	}
	return FALSE;
}

void CreateScriptDebugWindow(HINSTANCE hInst, HWND hParent) {
	g_dlgScript = CreateDialog(hInst, MAKEINTRESOURCE(IDD_DLG_SCRIPT), hParent, (DLGPROC)ScriptDialogProc);
	if (g_Config.m_bEditor) {
		ShowWindow(g_dlgScript, SW_SHOW);
	} else {
		ShowWindow(g_dlgScript, SW_HIDE);
	}
	RECT rc;
	GetWindowRect(hParent, &rc);
	SetWindowPos(g_dlgScript, nullptr, rc.right - 300, rc.bottom - 300, 0, 0, SWP_NOSIZE);
}

void ToggleScriptDebugWindow() {
	if (IsWindowVisible(g_dlgScript)) {
		ShowWindow(g_dlgScript, SW_HIDE);
	} else {
		ShowWindow(g_dlgScript, SW_SHOW);
	}
}

#endif

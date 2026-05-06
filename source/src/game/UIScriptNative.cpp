//----------------------------------------------------------------------
// UIScriptNative.cpp
//
// Native Lua wrappers for all UI functions on x64
// 
// On x86, CaLua's CLU_Call works via inline assembly.
// On x64, we register native Lua wrappers directly with the Lua state.
// These functions call the same C++ implementations as the CLU functions.
//----------------------------------------------------------------------
#include "stdafx.h"

#ifdef _WIN64

#include "UIScriptNative.h"
#include "caLua.h"
#include "UIScript.h"
#include "uiguidata.h"
#include "UIForm.h"
#include "UIFormMgr.h"
#include "UIFont.h"
#include "UIEdit.h"
#include "UILabel.h"
#include "UITextButton.h"
#include "UIProgressBar.h"
#include "UIScroll.h"
#include "UIList.h"
#include "UICombo.h"
#include "UIImage.h"
#include "UICheckBox.h"
#include "UIGrid.h"
#include "UIListView.h"
#include "UIPage.h"
#include "UITreeView.h"
#include "UI3DCompent.h"
#include "UIPicList.h"
#include "UITextParse.h"
#include "UIMemo.h"
#include "UIGoodsGrid.h"
#include "UIFastCommand.h"
#include "UIHeadSay.h"
#include "UISkillList.h"
#include "UIMenu.h"
#include "UITitle.h"
#include "UIRichEdit.h"
#include "UIChat.h"
#include "StringLib.h"
#include "MPTextureSet.h"
#include "CharacterRecord.h"
#include "MapSet.h"
#include "EffectSet.h"
#include "SceneObjSet.h"

using namespace GUI;

//----------------------------------------------------------------------
// Helper macros for Lua wrappers
//----------------------------------------------------------------------
#define LUA_FUNC(name) static int lua_##name(lua_State* L)
#define GET_INT(n) ((int)lua_tonumber(L, n))
#define GET_UINT(n) ((unsigned int)lua_tonumber(L, n))
#define GET_FLOAT(n) ((float)lua_tonumber(L, n))
#define GET_STRING(n) const_cast<char*>(lua_tostring(L, n))
#define PUSH_INT(v) lua_pushnumber(L, v)
#define PUSH_NIL() lua_pushnil(L)

//----------------------------------------------------------------------
// External declarations (these are defined in UIScript.cpp)
// IMPORTANT: Signatures MUST match exactly - use char* not const char* where UIScript.cpp uses char*
//----------------------------------------------------------------------
extern int UI_CreateForm(char* pszName, int isModal, int w, int h, int x, int y, int isTitle, int isShowFrame);
extern int UI_CreateCompent(int formId, int type, char* pszName, int w, int h, int x, int y);
extern int UI_CreateListView(int formId, char* pszName, int w, int h, int x, int y, int col, int style);

extern int UI_SetFormTempleteMax(int max);
extern int UI_AddAllFormTemplete(int form_id);
extern int UI_AddFormToTemplete(int formid, int nTempleteNo);
extern int UI_SwitchTemplete(int nTempleteNo);

extern int UI_FormSetIsEscClose(int nFormID, int IsEscClose);
extern int UI_FormSetEnterButton(int nFormID, int nButtonID);
extern int UI_FormSetHotKey(int id, int control_key, int key);
extern int UI_LoadFormImage(int id, char* client, int cw, int ch, int tx, int ty, char* file, int w, int h);
extern int UI_ShowForm(int id, int show);
extern int UI_SetFormStyle(int id, int index);
extern int UI_SetFormStyleEx(int id, int index, int offWidth, int offHeight);
extern int UI_LoadFrameImage(int id, const char* client, int cw, int ch, int tx, int ty, const char* file, int w, int h);

extern int UI_SetIsDrag(int id, int isDrag);
extern int UI_SetIsKeyFocus(int id, int IsKeyFocus);
extern int UI_SetCaption(int id, char* caption);  // char* in UIScript.cpp
extern int UI_CopyImage(int targetid, int sourceid);
extern int UI_SetSize(int id, int w, int h);
extern int UI_SetPos(int id, int x, int y);
extern int UI_SetTag(int id, int tag);
extern int UI_SetAlign(int id, int align);
extern int UI_SetHint(int id, char* hint);  // char* in UIScript.cpp
extern int UI_SetIsShow(int id, int isshow);
extern int UI_SetIsEnabled(int id, int isEnabled);
extern int UI_SetMargin(int id, int left, int top, int right, int bottom);
extern int UI_SetAlpha(int id, int alpha);
extern int UI_SetImageAlpha(int id, int alpha);
extern int UI_CopyCompent(int targetid, int sourceid);
extern int UI_AddCompent(int container_id, int compent_id);

extern int UI_LoadImage(int id, char* file, int frame, int w, int h, int tx, int ty);  // char* in UIScript.cpp
extern int UI_LoadScaleImage(int id, char* file, int frame, int w, int h, int tx, int ty, float scalex, float scaley);  // char*
extern int UI_LoadFlashScaleImage(int id, int flash, char* file, int frame, int w, int h, int tx, int ty, float scalex, float scaley);  // char*
extern int UI_SetMaxImage(int id, int max);

extern int UI_LoadButtonImage(int id, char* file, int w, int h, int sx, int sy, int isHorizontal);  // char*
extern int UI_SetButtonModalResult(int id, int modal);
extern int UI_ButtonSetHint(int id, char* hint);  // char*

extern int UI_GetScroll(int id);
extern int UI_GetList(int id);
extern int UI_GetScrollObj(int id, int scrolltype);
extern int UI_SetScrollStyle(int id, int style);

extern int UI_AddListText(int id, char* text);  // char*
extern int UI_ListLoadSelectImage(int id, char* file, int w, int h, int sx, int sy);  // char*
extern int UI_LoadListItemImage(int id, char* file, int w, int h, int sx, int sy, int item_w, int item_h);  // char*
extern int UI_ListSetItemMargin(int id, int left, int top);
extern int UI_ListSetItemImageMargin(int id, int left, int top);
extern int UI_SetListFontColor(int list, DWORD nBackColor, DWORD nSelectColor);
extern int UI_SetListRowHeight(int id, int height);
extern int UI_AddListBarText(int id, char* text, float progress);  // char*
extern int UI_SetListIsMouseFollow(int list, int IsFollow);

extern int UI_ListViewSetTitle(int listviewid, int index, int width, const char* titleimage, int w, int h, int sx, int sy);
extern int UI_ListViewSetTitleHeight(int listviewid, int height);

extern int UI_LoadListFixSelect(int id, const char* imagefile, int w, int h, int sx, int sy);
extern int UI_FixListSetMaxNum(int id, int num);
extern int UI_FixListSetText(int id, int index, const char* text);
extern int UI_FixListSetRowSpace(int id, int height);
extern int UI_CheckFixListSetCheckMargin(int id, int left, int top);
extern int UI_LoadCheckFixListCheck(int id, const char* checkimage, int cw, int ch, int csx, int csy, const char* uncheckimage, int uw, int uh, int usx, int usy);

extern int UI_LoadComboImage(int id, char* edit, int ew, int eh, int ex, int ey, char* button, int bw, int bh, int bx, int by, int isHorizontal);  // char*
extern int UI_ComboSetStyle(int id, int style);  // int not IsDown
extern int UI_ComboSetTextColor(int id, DWORD color);

extern int UI_SetProgressStyle(int id, int style);
extern int UI_SetProgressHintStyle(int id, int style);
extern int UI_SetProgressActiveMouse(int id, int style);

extern int UI_SetEditMaxNum(int id, int num);
extern int UI_SetEditCursorColor(int id, DWORD color);
extern int UI_SetEditEnterButton(int nEditID, int nButtonID);
extern int UI_SetEditMaxNumVisible(int id, int num);

extern int UI_SetTextColor(int id, DWORD color);
extern int UI_SetLabelExFont(int id, int nFontIndex, int IsShadow, DWORD dwShadowColor);
extern int UI_SetLabelExShadowColor(int label_id, DWORD color);

extern int UI_GridLoadSelectImage(int id, char* file, int w, int h, int tx, int ty);  // char*
extern int UI_SetGridIsDragSize(int id, int IsEnabled);
extern int UI_SetGridUnitSize(int id, int w, int h);
extern int UI_AddFaceToGrid(int id, char* file, int w, int h, int sx, int sy, int frame, int nTag);  // char*
extern int UI_SetGridSpace(int id, int x, int y);
extern int UI_SetGridContent(int id, int nRow, int nCol);
extern int UI_GoodGridLoadUnitImage(int id, char* file, int w, int h, int tx, int ty);  // char*

extern int UI_CreatePageItem(int page_id);
extern int UI_GetPageItemObj(int page_item_id, int type);
extern int UI_SetPageButton(int page_id, int button_style, int bw, int bh);

extern int UI_TreeLoadImage(int nTreeID, int nType, char* imagefile, int w, int h, int sx, int sy, int itemw, int itemh);  // char*
extern int UI_TreeSetNodeTextXY(int nTreeID, int textX, int textY);
extern int UI_CreateTextItem(const char* text, int color);
extern int UI_CreateGraphItem(char* file, int w, int h, int sx, int sy, int frame);  // char*
extern int UI_CreateNoteGraphItem(char* file, int w, int h, int sx, int sy, int frame, const char* text, int textx, int texty);  // char* first, const char* second
extern int UI_CreateSingleNode(int treeid, int itemid, int nodeid_parent);
extern int UI_CreateGridNode(int treeid, int itemid, int maxcol, int uw, int uh, int nodeid_parent);
extern int UI_GridNodeAddItem(int nodeid, int itemid);
extern int UI_CreateGraphItemTex(int tx, int ty, int tw, int th, float scale_x, float scale_y, int nTextureID, int nTag);

extern int UI_AddGroupBox(int id, int checkbox);

extern int UI_SetMemoMaxNumPerRow(int id, int num);
extern int UI_SetMemoPageShowNum(int id, int num);
extern int UI_SetMemoRowHeight(int id, int num);

extern int UI_RichSetClipRect(int id, int x0, int y0, int x1, int y1);
extern int UI_RichSetMaxLine(int id, int line);

extern int UI_SetChatColor(DWORD p1, DWORD p2, DWORD p3, DWORD p4, DWORD p5, DWORD p6, DWORD p7, DWORD p8);

extern int UI_LoadHeadSayFaceImage(int num, int maxframe, int w, int h, char* file, int cw, int ch, int tx, int ty);  // char*, different param order!
extern int UI_LoadHeadSayShopImage(int num, int w, int h, char* file, int cw, int ch, int tx, int ty);  // char*, different param order!
extern int UI_LoadHeadSayShopImage2(int num, int w, int h, char* file, int cw, int ch, int tx, int ty);  // char*
extern int UI_LoadHeadSayLifeImage(int w, int h, char* file, int cw, int ch, int tx, int ty, int frame);  // char*, different param order!
extern int UI_LoadHeadSaySayImage(int uw, int uh, char* file, int w, int h, int tx, int ty, int isHorizontal);  // dialog bubble
extern int UI_LoadHeadSayMouseImage(int uw, int uh, char* file, int w, int h, int tx, int ty, int isHorizontal);  // mouse hover
extern int UI_SetHeadSayBkgColor(DWORD color);

extern int UI_SetTextParse(int nIndex, char* file, int w, int h, int sx, int sy, int frame);  // char*
extern int UI_ItemBarLoadImage(char* file, int w, int h, int tx, int ty);  // char*
extern int UI_SetDragSnapToGrid(int w, int h);

extern int UI_MenuLoadSelect(int menuid, char* imagefile, int w, int h, int tx, int ty);  // char*
extern int UI_MenuLoadImage(int id, int IsShowFrame, int IsTitle, char* clientfile, int cw, int ch, int tx, int ty, char* framefile, int w, int h);  // char*
extern int UI_MenuAddText(int id, const char* text);  // const char* in UIScript.cpp

extern int UI_AddFilterTextToNameTable(const char* text);  // const char* in UIScript.cpp
extern int UI_AddFilterTextToDialogTable(const char* text);  // const char* in UIScript.cpp

extern int UI_SetTitleFont(int id, int font, int color, int height);

extern int UI_LoadSkillListButtonImage(int id, char* file, int w, int h, int sx, int sy, int item_w, int item_h);  // char*
extern int UI_LoadSkillActiveImage(char* file, int maxframe, int w, int h, int sx, int sy);  // char*
extern int UI_LoadChargeImage(int link, char* file, int maxframe, int w, int h, int sx, int sy);  // char*

extern int UI_LoadScript(char* file);

// Texture functions (from MPTextureSet.h)
extern int GetChaPhotoTexID(int cha_id);
extern int GetSceneObjPhotoTexID(int obj_id);
extern int GetEffectPhotoTexID(int eff_id);
extern int GetTerrainTextureID(int terrain_id);
extern int GetTerrainTextureType(int terrain_id);
extern int GetSceneObjPhotoTexType(int id);

//----------------------------------------------------------------------
// Native Lua wrapper implementations
//----------------------------------------------------------------------

// Helper: ARGB color construction (pure Lua on x86, native on x64)
LUA_FUNC(ARGB) {
	int a = GET_INT(1);
	int r = GET_INT(2);
	int g = GET_INT(3);
	int b = GET_INT(4);
	DWORD color = ((a & 0xFF) << 24) | ((r & 0xFF) << 16) | ((g & 0xFF) << 8) | (b & 0xFF);
	PUSH_INT((int)color);
	return 1;
}

// Texture functions
LUA_FUNC(GetTextureID) {
	const char* file = GET_STRING(1);
	int result = GetTextureID(file);
	PUSH_INT(result);
	return 1;
}

LUA_FUNC(GetTerrainTextureID) {
	int terrain_id = GET_INT(1);
	int tex_id = GetTerrainTextureID(terrain_id);
	if (tex_id == 0) { PUSH_NIL(); }
	else { PUSH_INT(tex_id); }
	return 1;
}

LUA_FUNC(GetChaPhotoTexID) {
	int cha_id = GET_INT(1);
	int tex = GetChaPhotoTexID(cha_id);
	if (tex == 0) { PUSH_NIL(); }
	else { PUSH_INT(tex); }
	return 1;
}

LUA_FUNC(GetSceneObjPhotoTexID) {
	int obj_id = GET_INT(1);
	int tex = GetSceneObjPhotoTexID(obj_id);
	if (tex == 0) { PUSH_NIL(); }
	else { PUSH_INT(tex); }
	return 1;
}

LUA_FUNC(GetEffectPhotoTexID) {
	int eff_id = GET_INT(1);
	int tex = GetEffectPhotoTexID(eff_id);
	if (tex == 0) { PUSH_NIL(); }
	else { PUSH_INT(tex); }
	return 1;
}

LUA_FUNC(GetTerrainTextureType) {
	int terrain_id = GET_INT(1);
	int tex_id = GetTerrainTextureType(terrain_id);
	if (tex_id == -1) { PUSH_NIL(); }
	else { PUSH_INT(tex_id); }
	return 1;
}

LUA_FUNC(GetSceneObjPhotoTexType) {
	int id = GET_INT(1);
	int tex_id = GetSceneObjPhotoTexType(id);
	if (tex_id == -1) { PUSH_NIL(); }
	else { PUSH_INT(tex_id); }
	return 1;
}

// Script loading
LUA_FUNC(UI_LoadScript) {
	char* file = (char*)GET_STRING(1);
	UI_LoadScript(file);
	return 0;
}

// Form functions
LUA_FUNC(UI_CreateForm) {
	char* name = (char*)GET_STRING(1);
	int isModal = GET_INT(2);
	int w = GET_INT(3);
	int h = GET_INT(4);
	int x = GET_INT(5);
	int y = GET_INT(6);
	int isTitle = GET_INT(7);
	int isShowFrame = GET_INT(8);
	int id = UI_CreateForm(name, isModal, w, h, x, y, isTitle, isShowFrame);
	if (id == -1) { PUSH_NIL(); }
	else { PUSH_INT(id); }
	return 1;
}

LUA_FUNC(UI_FormSetIsEscClose) {
	UI_FormSetIsEscClose(GET_INT(1), GET_INT(2));
	return 0;
}

LUA_FUNC(UI_FormSetEnterButton) {
	UI_FormSetEnterButton(GET_INT(1), GET_INT(2));
	return 0;
}

LUA_FUNC(UI_FormSetHotKey) {
	UI_FormSetHotKey(GET_INT(1), GET_INT(2), GET_INT(3));
	return 0;
}

LUA_FUNC(UI_SetFormTempleteMax) {
	UI_SetFormTempleteMax(GET_INT(1));
	return 0;
}

LUA_FUNC(UI_AddFormToTemplete) {
	UI_AddFormToTemplete(GET_INT(1), GET_INT(2));
	return 0;
}

LUA_FUNC(UI_AddAllFormTemplete) {
	UI_AddAllFormTemplete(GET_INT(1));
	return 0;
}

LUA_FUNC(UI_SwitchTemplete) {
	UI_SwitchTemplete(GET_INT(1));
	return 0;
}

LUA_FUNC(UI_LoadFormImage) {
	UI_LoadFormImage(GET_INT(1), (char*)GET_STRING(2), GET_INT(3), GET_INT(4), GET_INT(5), GET_INT(6), (char*)GET_STRING(7), GET_INT(8), GET_INT(9));
	return 0;
}

LUA_FUNC(UI_ShowForm) {
	UI_ShowForm(GET_INT(1), GET_INT(2));
	return 0;
}

LUA_FUNC(UI_SetFormStyle) {
	UI_SetFormStyle(GET_INT(1), GET_INT(2));
	return 0;
}

LUA_FUNC(UI_SetFormStyleEx) {
	UI_SetFormStyleEx(GET_INT(1), GET_INT(2), GET_INT(3), GET_INT(4));
	return 0;
}

LUA_FUNC(UI_LoadFrameImage) {
	UI_LoadFrameImage(GET_INT(1), GET_STRING(2), GET_INT(3), GET_INT(4), GET_INT(5), GET_INT(6), GET_STRING(7), GET_INT(8), GET_INT(9));
	return 0;
}

// Component functions
LUA_FUNC(UI_CreateCompent) {
	int formId = GET_INT(1);
	int type = GET_INT(2);
	char* name = (char*)GET_STRING(3);
	int w = GET_INT(4);
	int h = GET_INT(5);
	int x = GET_INT(6);
	int y = GET_INT(7);
	int ret = UI_CreateCompent(formId, type, name, w, h, x, y);
	if (ret == -1) { PUSH_NIL(); }
	else { PUSH_INT(ret); }
	return 1;
}

LUA_FUNC(UI_SetIsDrag) {
	UI_SetIsDrag(GET_INT(1), GET_INT(2));
	return 0;
}

LUA_FUNC(UI_SetHint) {
	UI_SetHint(GET_INT(1), GET_STRING(2));
	return 0;
}

LUA_FUNC(UI_CreateListView) {
	int formId = GET_INT(1);
	char* name = (char*)GET_STRING(2);
	int w = GET_INT(3);
	int h = GET_INT(4);
	int x = GET_INT(5);
	int y = GET_INT(6);
	int col = GET_INT(7);
	int style = GET_INT(8);
	int ret = UI_CreateListView(formId, name, w, h, x, y, col, style);
	if (ret == -1) { PUSH_NIL(); }
	else { PUSH_INT(ret); }
	return 1;
}

LUA_FUNC(UI_ListViewSetTitle) {
	UI_ListViewSetTitle(GET_INT(1), GET_INT(2), GET_INT(3), GET_STRING(4), GET_INT(5), GET_INT(6), GET_INT(7), GET_INT(8));
	return 0;
}

LUA_FUNC(UI_ListViewSetTitleHeight) {
	UI_ListViewSetTitleHeight(GET_INT(1), GET_INT(2));
	return 0;
}

LUA_FUNC(UI_SetListIsMouseFollow) {
	UI_SetListIsMouseFollow(GET_INT(1), GET_INT(2));
	return 0;
}

LUA_FUNC(UI_SetTag) {
	UI_SetTag(GET_INT(1), GET_INT(2));
	return 0;
}

LUA_FUNC(UI_SetSize) {
	UI_SetSize(GET_INT(1), GET_INT(2), GET_INT(3));
	return 0;
}

LUA_FUNC(UI_SetPos) {
	UI_SetPos(GET_INT(1), GET_INT(2), GET_INT(3));
	return 0;
}

LUA_FUNC(UI_SetIsKeyFocus) {
	UI_SetIsKeyFocus(GET_INT(1), GET_INT(2));
	return 0;
}

LUA_FUNC(UI_SetCaption) {
	UI_SetCaption(GET_INT(1), GET_STRING(2));
	return 0;
}

LUA_FUNC(UI_CopyImage) {
	UI_CopyImage(GET_INT(1), GET_INT(2));
	return 0;
}

LUA_FUNC(UI_SetAlpha) {
	UI_SetAlpha(GET_INT(1), GET_INT(2));
	return 0;
}

LUA_FUNC(UI_SetImageAlpha) {
	UI_SetImageAlpha(GET_INT(1), GET_INT(2));
	return 0;
}

LUA_FUNC(UI_SetAlign) {
	UI_SetAlign(GET_INT(1), GET_INT(2));
	return 0;
}

LUA_FUNC(UI_SetIsShow) {
	UI_SetIsShow(GET_INT(1), GET_INT(2));
	return 0;
}

LUA_FUNC(UI_SetIsEnabled) {
	UI_SetIsEnabled(GET_INT(1), GET_INT(2));
	return 0;
}

LUA_FUNC(UI_SetMargin) {
	UI_SetMargin(GET_INT(1), GET_INT(2), GET_INT(3), GET_INT(4), GET_INT(5));
	return 0;
}

LUA_FUNC(UI_SetChatColor) {
	UI_SetChatColor(GET_UINT(1), GET_UINT(2), GET_UINT(3), GET_UINT(4), GET_UINT(5), GET_UINT(6), GET_UINT(7), GET_UINT(8));
	return 0;
}

// Image functions
LUA_FUNC(UI_LoadImage) {
	UI_LoadImage(GET_INT(1), GET_STRING(2), GET_INT(3), GET_INT(4), GET_INT(5), GET_INT(6), GET_INT(7));
	return 0;
}

LUA_FUNC(UI_SetMaxImage) {
	UI_SetMaxImage(GET_INT(1), GET_INT(2));
	return 0;
}

LUA_FUNC(UI_LoadScaleImage) {
	UI_LoadScaleImage(GET_INT(1), GET_STRING(2), GET_INT(3), GET_INT(4), GET_INT(5), GET_INT(6), GET_INT(7), GET_FLOAT(8), GET_FLOAT(9));
	return 0;
}

LUA_FUNC(UI_LoadFlashScaleImage) {
	UI_LoadFlashScaleImage(GET_INT(1), GET_INT(2), GET_STRING(3), GET_INT(4), GET_INT(5), GET_INT(6), GET_INT(7), GET_INT(8), GET_FLOAT(9), GET_FLOAT(10));
	return 0;
}

LUA_FUNC(UI_LoadSkillListButtonImage) {
	UI_LoadSkillListButtonImage(GET_INT(1), GET_STRING(2), GET_INT(3), GET_INT(4), GET_INT(5), GET_INT(6), GET_INT(7), GET_INT(8));
	return 0;
}

// Button functions
LUA_FUNC(UI_LoadButtonImage) {
	UI_LoadButtonImage(GET_INT(1), GET_STRING(2), GET_INT(3), GET_INT(4), GET_INT(5), GET_INT(6), GET_INT(7));
	return 0;
}

LUA_FUNC(UI_SetButtonModalResult) {
	UI_SetButtonModalResult(GET_INT(1), GET_INT(2));
	return 0;
}

LUA_FUNC(UI_ButtonSetHint) {
	UI_ButtonSetHint(GET_INT(1), GET_STRING(2));
	return 0;
}

// Scroll functions
LUA_FUNC(UI_GetScroll) {
	int ret = UI_GetScroll(GET_INT(1));
	if (ret == -1) { PUSH_NIL(); }
	else { PUSH_INT(ret); }
	return 1;
}

LUA_FUNC(UI_GetList) {
	int ret = UI_GetList(GET_INT(1));
	if (ret == -1) { PUSH_NIL(); }
	else { PUSH_INT(ret); }
	return 1;
}

LUA_FUNC(UI_GetScrollObj) {
	int ret = UI_GetScrollObj(GET_INT(1), GET_INT(2));
	if (ret == -1) { PUSH_NIL(); }
	else { PUSH_INT(ret); }
	return 1;
}

LUA_FUNC(UI_SetScrollStyle) {
	UI_SetScrollStyle(GET_INT(1), GET_INT(2));
	return 0;
}

// Grid functions
LUA_FUNC(UI_GridLoadSelectImage) {
	UI_GridLoadSelectImage(GET_INT(1), GET_STRING(2), GET_INT(3), GET_INT(4), GET_INT(5), GET_INT(6));
	return 0;
}

LUA_FUNC(UI_SetGridIsDragSize) {
	UI_SetGridIsDragSize(GET_INT(1), GET_INT(2));
	return 0;
}

// Combo functions
LUA_FUNC(UI_LoadComboImage) {
	UI_LoadComboImage(GET_INT(1), GET_STRING(2), GET_INT(3), GET_INT(4), GET_INT(5), GET_INT(6), GET_STRING(7), GET_INT(8), GET_INT(9), GET_INT(10), GET_INT(11), GET_INT(12));
	return 0;
}

LUA_FUNC(UI_ComboSetStyle) {
	UI_ComboSetStyle(GET_INT(1), GET_INT(2));
	return 0;
}

LUA_FUNC(UI_ComboSetTextColor) {
	UI_ComboSetTextColor(GET_INT(1), GET_UINT(2));
	return 0;
}

// FixList functions
LUA_FUNC(UI_LoadListFixSelect) {
	UI_LoadListFixSelect(GET_INT(1), GET_STRING(2), GET_INT(3), GET_INT(4), GET_INT(5), GET_INT(6));
	return 0;
}

LUA_FUNC(UI_CopyCompent) {
	UI_CopyCompent(GET_INT(1), GET_INT(2));
	return 0;
}

// List functions
LUA_FUNC(UI_AddListText) {
	UI_AddListText(GET_INT(1), GET_STRING(2));
	return 0;
}

LUA_FUNC(UI_ListLoadSelectImage) {
	UI_ListLoadSelectImage(GET_INT(1), GET_STRING(2), GET_INT(3), GET_INT(4), GET_INT(5), GET_INT(6));
	return 0;
}

LUA_FUNC(UI_LoadListItemImage) {
	UI_LoadListItemImage(GET_INT(1), GET_STRING(2), GET_INT(3), GET_INT(4), GET_INT(5), GET_INT(6), GET_INT(7), GET_INT(8));
	return 0;
}

LUA_FUNC(UI_ListSetItemMargin) {
	UI_ListSetItemMargin(GET_INT(1), GET_INT(2), GET_INT(3));
	return 0;
}

LUA_FUNC(UI_ListSetItemImageMargin) {
	UI_ListSetItemImageMargin(GET_INT(1), GET_INT(2), GET_INT(3));
	return 0;
}

LUA_FUNC(UI_AddListBarText) {
	UI_AddListBarText(GET_INT(1), GET_STRING(2), GET_FLOAT(3));
	return 0;
}

LUA_FUNC(UI_SetListFontColor) {
	UI_SetListFontColor(GET_INT(1), GET_UINT(2), GET_UINT(3));
	return 0;
}

LUA_FUNC(UI_SetListRowHeight) {
	UI_SetListRowHeight(GET_INT(1), GET_INT(2));
	return 0;
}

LUA_FUNC(UI_AddGroupBox) {
	UI_AddGroupBox(GET_INT(1), GET_INT(2));
	return 0;
}

// Progress functions
LUA_FUNC(UI_SetProgressStyle) {
	UI_SetProgressStyle(GET_INT(1), GET_INT(2));
	return 0;
}

LUA_FUNC(UI_SetProgressHintStyle) {
	UI_SetProgressHintStyle(GET_INT(1), GET_INT(2));
	return 0;
}

LUA_FUNC(UI_SetProgressActiveMouse) {
	UI_SetProgressActiveMouse(GET_INT(1), GET_INT(2));
	return 0;
}

// Edit functions
LUA_FUNC(UI_SetEditMaxNum) {
	UI_SetEditMaxNum(GET_INT(1), GET_INT(2));
	return 0;
}

LUA_FUNC(UI_SetEditCursorColor) {
	UI_SetEditCursorColor(GET_INT(1), GET_UINT(2));
	return 0;
}

LUA_FUNC(UI_SetEditEnterButton) {
	UI_SetEditEnterButton(GET_INT(1), GET_INT(2));
	return 0;
}

LUA_FUNC(UI_SetEditMaxNumVisible) {
	UI_SetEditMaxNumVisible(GET_INT(1), GET_INT(2));
	return 0;
}

// Text functions
LUA_FUNC(UI_SetTextColor) {
	UI_SetTextColor(GET_INT(1), GET_UINT(2));
	return 0;
}

LUA_FUNC(UI_SetGridSpace) {
	UI_SetGridSpace(GET_INT(1), GET_INT(2), GET_INT(3));
	return 0;
}

LUA_FUNC(UI_SetGridContent) {
	UI_SetGridContent(GET_INT(1), GET_INT(2), GET_INT(3));
	return 0;
}

LUA_FUNC(UI_GoodGridLoadUnitImage) {
	UI_GoodGridLoadUnitImage(GET_INT(1), GET_STRING(2), GET_INT(3), GET_INT(4), GET_INT(5), GET_INT(6));
	return 0;
}

LUA_FUNC(UI_SetGridUnitSize) {
	UI_SetGridUnitSize(GET_INT(1), GET_INT(2), GET_INT(3));
	return 0;
}

LUA_FUNC(UI_AddFaceToGrid) {
	UI_AddFaceToGrid(GET_INT(1), GET_STRING(2), GET_INT(3), GET_INT(4), GET_INT(5), GET_INT(6), GET_INT(7), GET_INT(8));
	return 0;
}

LUA_FUNC(UI_FixListSetMaxNum) {
	UI_FixListSetMaxNum(GET_INT(1), GET_INT(2));
	return 0;
}

// Memo functions
LUA_FUNC(UI_SetMemoMaxNumPerRow) {
	UI_SetMemoMaxNumPerRow(GET_INT(1), GET_INT(2));
	return 0;
}

LUA_FUNC(UI_SetMemoPageShowNum) {
	UI_SetMemoPageShowNum(GET_INT(1), GET_INT(2));
	return 0;
}

LUA_FUNC(UI_SetMemoRowHeight) {
	UI_SetMemoRowHeight(GET_INT(1), GET_INT(2));
	return 0;
}

// Rich functions
LUA_FUNC(UI_RichSetClipRect) {
	UI_RichSetClipRect(GET_INT(1), GET_INT(2), GET_INT(3), GET_INT(4), GET_INT(5));
	return 0;
}

LUA_FUNC(UI_RichSetMaxLine) {
	UI_RichSetMaxLine(GET_INT(1), GET_INT(2));
	return 0;
}

LUA_FUNC(UI_FixListSetText) {
	UI_FixListSetText(GET_INT(1), GET_INT(2), GET_STRING(3));
	return 0;
}

LUA_FUNC(UI_FixListSetRowSpace) {
	UI_FixListSetRowSpace(GET_INT(1), GET_INT(2));
	return 0;
}

LUA_FUNC(UI_CheckFixListSetCheckMargin) {
	UI_CheckFixListSetCheckMargin(GET_INT(1), GET_INT(2), GET_INT(3));
	return 0;
}

LUA_FUNC(UI_LoadCheckFixListCheck) {
	UI_LoadCheckFixListCheck(GET_INT(1), GET_STRING(2), GET_INT(3), GET_INT(4), GET_INT(5), GET_INT(6), GET_STRING(7), GET_INT(8), GET_INT(9), GET_INT(10), GET_INT(11));
	return 0;
}

// Tree functions
LUA_FUNC(UI_TreeLoadImage) {
	UI_TreeLoadImage(GET_INT(1), GET_INT(2), GET_STRING(3), GET_INT(4), GET_INT(5), GET_INT(6), GET_INT(7), GET_INT(8), GET_INT(9));
	return 0;
}

LUA_FUNC(UI_TreeSetNodeTextXY) {
	UI_TreeSetNodeTextXY(GET_INT(1), GET_INT(2), GET_INT(3));
	return 0;
}

LUA_FUNC(UI_CreateTextItem) {
	int ret = UI_CreateTextItem(GET_STRING(1), GET_INT(2));
	if (ret == -1) { PUSH_NIL(); }
	else { PUSH_INT(ret); }
	return 1;
}

LUA_FUNC(UI_CreateGraphItem) {
	int ret = UI_CreateGraphItem(GET_STRING(1), GET_INT(2), GET_INT(3), GET_INT(4), GET_INT(5), GET_INT(6));
	if (ret == -1) { PUSH_NIL(); }
	else { PUSH_INT(ret); }
	return 1;
}

LUA_FUNC(UI_CreateNoteGraphItem) {
	int ret = UI_CreateNoteGraphItem(GET_STRING(1), GET_INT(2), GET_INT(3), GET_INT(4), GET_INT(5), GET_INT(6), GET_STRING(7), GET_INT(8), GET_INT(9));
	if (ret == -1) { PUSH_NIL(); }
	else { PUSH_INT(ret); }
	return 1;
}

LUA_FUNC(UI_CreateSingleNode) {
	int ret = UI_CreateSingleNode(GET_INT(1), GET_INT(2), GET_INT(3));
	if (ret == -1) { PUSH_NIL(); }
	else { PUSH_INT(ret); }
	return 1;
}

LUA_FUNC(UI_CreateGridNode) {
	int ret = UI_CreateGridNode(GET_INT(1), GET_INT(2), GET_INT(3), GET_INT(4), GET_INT(5), GET_INT(6));
	if (ret == -1) { PUSH_NIL(); }
	else { PUSH_INT(ret); }
	return 1;
}

LUA_FUNC(UI_GridNodeAddItem) {
	UI_GridNodeAddItem(GET_INT(1), GET_INT(2));
	return 0;
}

LUA_FUNC(UI_CreateGraphItemTex) {
	int ret = UI_CreateGraphItemTex(GET_INT(1), GET_INT(2), GET_INT(3), GET_INT(4), GET_FLOAT(5), GET_FLOAT(6), GET_INT(7), GET_INT(8));
	if (ret == -1) { PUSH_NIL(); }
	else { PUSH_INT(ret); }
	return 1;
}

// Page functions
LUA_FUNC(UI_CreatePageItem) {
	int ret = UI_CreatePageItem(GET_INT(1));
	if (ret == -1) { PUSH_NIL(); }
	else { PUSH_INT(ret); }
	return 1;
}

LUA_FUNC(UI_GetPageItemObj) {
	int ret = UI_GetPageItemObj(GET_INT(1), GET_INT(2));
	if (ret == -1) { PUSH_NIL(); }
	else { PUSH_INT(ret); }
	return 1;
}

LUA_FUNC(UI_AddCompent) {
	UI_AddCompent(GET_INT(1), GET_INT(2));
	return 0;
}

LUA_FUNC(UI_SetLabelExShadowColor) {
	UI_SetLabelExShadowColor(GET_INT(1), GET_UINT(2));
	return 0;
}

LUA_FUNC(UI_SetPageButton) {
	UI_SetPageButton(GET_INT(1), GET_INT(2), GET_INT(3), GET_INT(4));
	return 0;
}

LUA_FUNC(UI_SetDragSnapToGrid) {
	UI_SetDragSnapToGrid(GET_INT(1), GET_INT(2));
	return 0;
}

LUA_FUNC(UI_SetTextParse) {
	UI_SetTextParse(GET_INT(1), GET_STRING(2), GET_INT(3), GET_INT(4), GET_INT(5), GET_INT(6), GET_INT(7));
	return 0;
}

LUA_FUNC(UI_ItemBarLoadImage) {
	UI_ItemBarLoadImage(GET_STRING(1), GET_INT(2), GET_INT(3), GET_INT(4), GET_INT(5));
	return 0;
}

// Menu functions
LUA_FUNC(UI_MenuLoadImage) {
	UI_MenuLoadImage(GET_INT(1), GET_INT(2), GET_INT(3), GET_STRING(4), GET_INT(5), GET_INT(6), GET_INT(7), GET_INT(8), GET_STRING(9), GET_INT(10), GET_INT(11));
	return 0;
}

LUA_FUNC(UI_MenuLoadSelect) {
	UI_MenuLoadSelect(GET_INT(1), GET_STRING(2), GET_INT(3), GET_INT(4), GET_INT(5), GET_INT(6));
	return 0;
}

LUA_FUNC(UI_MenuAddText) {
	UI_MenuAddText(GET_INT(1), GET_STRING(2));
	return 0;
}

LUA_FUNC(UI_AddFilterTextToNameTable) {
	UI_AddFilterTextToNameTable(GET_STRING(1));
	return 0;
}

LUA_FUNC(UI_AddFilterTextToDialogTable) {
	UI_AddFilterTextToDialogTable(GET_STRING(1));
	return 0;
}

LUA_FUNC(UI_SetHeadSayBkgColor) {
	UI_SetHeadSayBkgColor(GET_UINT(1));
	return 0;
}

LUA_FUNC(UI_SetTitleFont) {
	UI_SetTitleFont(GET_INT(1), GET_INT(2), GET_INT(3), GET_INT(4));
	return 0;
}

LUA_FUNC(UI_SetLabelExFont) {
	UI_SetLabelExFont(GET_INT(1), GET_INT(2), GET_INT(3), GET_UINT(4));
	return 0;
}

LUA_FUNC(UI_LoadSkillActiveImage) {
	UI_LoadSkillActiveImage(GET_STRING(1), GET_INT(2), GET_INT(3), GET_INT(4), GET_INT(5), GET_INT(6));
	return 0;
}

LUA_FUNC(UI_LoadChargeImage) {
	UI_LoadChargeImage(GET_INT(1), GET_STRING(2), GET_INT(3), GET_INT(4), GET_INT(5), GET_INT(6), GET_INT(7));
	return 0;
}

// HeadSay functions
LUA_FUNC(UI_LoadHeadSayFaceImage) {
	UI_LoadHeadSayFaceImage(GET_INT(1), GET_INT(2), GET_INT(3), GET_INT(4), GET_STRING(5), GET_INT(6), GET_INT(7), GET_INT(8), GET_INT(9));
	return 0;
}

LUA_FUNC(UI_LoadHeadSayShopImage) {
	UI_LoadHeadSayShopImage(GET_INT(1), GET_INT(2), GET_INT(3), GET_STRING(4), GET_INT(5), GET_INT(6), GET_INT(7), GET_INT(8));
	return 0;
}

LUA_FUNC(UI_LoadHeadSayShopImage2) {
	UI_LoadHeadSayShopImage2(GET_INT(1), GET_INT(2), GET_INT(3), GET_STRING(4), GET_INT(5), GET_INT(6), GET_INT(7), GET_INT(8));
	return 0;
}

LUA_FUNC(UI_LoadHeadSayLifeImage) {
	UI_LoadHeadSayLifeImage(GET_INT(1), GET_INT(2), GET_STRING(3), GET_INT(4), GET_INT(5), GET_INT(6), GET_INT(7), GET_INT(8));
	return 0;
}

LUA_FUNC(UI_LoadHeadSaySayImage) {
	UI_LoadHeadSaySayImage(GET_INT(1), GET_INT(2), GET_STRING(3), GET_INT(4), GET_INT(5), GET_INT(6), GET_INT(7), GET_INT(8));
	return 0;
}

LUA_FUNC(UI_LoadHeadSayMouseImage) {
	UI_LoadHeadSayMouseImage(GET_INT(1), GET_INT(2), GET_STRING(3), GET_INT(4), GET_INT(5), GET_INT(6), GET_INT(7), GET_INT(8));
	return 0;
}

// Font function
LUA_FUNC(UI_CreateFont) {
	const char* font = GET_STRING(1);
	int size800 = GET_INT(2);
	int size1024 = GET_INT(3);
	int nStyle = GET_INT(4);
	int result = CGuiFont::s_Font.CreateFont((char*)font, size800, size1024, (DWORD)nStyle);
	PUSH_INT(result);
	return 1;
}

//----------------------------------------------------------------------
// Scene/Camera functions (from SceneScript.cpp, AppScript.cpp, ChaScript.cpp)
//----------------------------------------------------------------------
extern int SN_SetAttackChaColor(int r, int g, int b);
extern int SN_CreateScene(int type, char* name, char* map_name, int ui, int max_cha, int max_obj, int max_item, int max_eff);
extern int SN_SetIsShow3DCursor(int isShow);
extern int SN_SetIsShowMinimap(int isShow);
extern int SN_SetTerrainShowCenter(int sceneid, int x, int y);
extern int CHA_SetClientAttr(int nScriptID, int nAngle, float fDis, float fHei);
extern int CameraRangeXY(int nMode, float fMin, float fMax);
extern int CameraRangeZ(int nMode, float fMin, float fMax);
extern int CameraRangeFOV(int nMode, float fMin, float fMax);
extern int CameraEnableRotate(int nMode, int nEnable);
extern int CameraShowSize(int nMode, int w, int h);

// Character script functions (from ChaScript.cpp)
extern int CH_Create(int sceneid, int type);
extern int CH_SetPos(int id, int x, int y);
extern int CH_SetYaw(int id, int yaw);
extern int CH_PlayPos(int id, int pose, int posetype);

// App/Game script functions (from AppScript.cpp)
extern int GP_SetCameraPos(float ex, float ey, float ez, float rx, float ry, float rz);
extern int GP_GotoScene(int sceneid);

LUA_FUNC(SN_SetAttackChaColor) {
	SN_SetAttackChaColor(GET_INT(1), GET_INT(2), GET_INT(3));
	return 0;
}

LUA_FUNC(SN_CreateScene) {
	int result = SN_CreateScene(GET_INT(1), GET_STRING(2), GET_STRING(3), GET_INT(4), 
	                            GET_INT(5), GET_INT(6), GET_INT(7), GET_INT(8));
	PUSH_INT(result);
	return 1;
}

LUA_FUNC(SN_SetIsShow3DCursor) {
	int result = SN_SetIsShow3DCursor(GET_INT(1));
	PUSH_INT(result);
	return 1;
}

LUA_FUNC(SN_SetIsShowMinimap) {
	int result = SN_SetIsShowMinimap(GET_INT(1));
	PUSH_INT(result);
	return 1;
}

LUA_FUNC(CHA_SetClientAttr) {
	CHA_SetClientAttr(GET_INT(1), GET_INT(2), GET_FLOAT(3), GET_FLOAT(4));
	return 0;
}

LUA_FUNC(CameraRangeXY) {
	CameraRangeXY(GET_INT(1), GET_FLOAT(2), GET_FLOAT(3));
	return 0;
}

LUA_FUNC(CameraRangeZ) {
	CameraRangeZ(GET_INT(1), GET_FLOAT(2), GET_FLOAT(3));
	return 0;
}

LUA_FUNC(CameraRangeFOV) {
	CameraRangeFOV(GET_INT(1), GET_FLOAT(2), GET_FLOAT(3));
	return 0;
}

LUA_FUNC(CameraEnableRotate) {
	CameraEnableRotate(GET_INT(1), GET_INT(2));
	return 0;
}

LUA_FUNC(CameraShowSize) {
	CameraShowSize(GET_INT(1), GET_INT(2), GET_INT(3));
	return 0;
}

// Scene functions
LUA_FUNC(SN_SetTerrainShowCenter) {
	SN_SetTerrainShowCenter(GET_INT(1), GET_INT(2), GET_INT(3));
	return 0;
}

// Character functions
LUA_FUNC(CH_Create) {
	int result = CH_Create(GET_INT(1), GET_INT(2));
	PUSH_INT(result);
	return 1;
}

LUA_FUNC(CH_SetPos) {
	CH_SetPos(GET_INT(1), GET_INT(2), GET_INT(3));
	return 0;
}

LUA_FUNC(CH_SetYaw) {
	CH_SetYaw(GET_INT(1), GET_INT(2));
	return 0;
}

LUA_FUNC(CH_PlayPos) {
	CH_PlayPos(GET_INT(1), GET_INT(2), GET_INT(3));
	return 0;
}

// App/Game functions
LUA_FUNC(GP_SetCameraPos) {
	GP_SetCameraPos(GET_FLOAT(1), GET_FLOAT(2), GET_FLOAT(3), GET_FLOAT(4), GET_FLOAT(5), GET_FLOAT(6));
	return 0;
}

LUA_FUNC(GP_GotoScene) {
	int result = GP_GotoScene(GET_INT(1));
	PUSH_INT(result);
	return 1;
}

//----------------------------------------------------------------------
// Registration function - called during initialization
//----------------------------------------------------------------------
void RegisterNativeUIFunctions(lua_State* L) {
	// Register all native UI functions directly with the Lua state
	// This replaces CLU_Call which doesn't work on x64
	// Functions are registered WITHOUT the _native suffix so they can be
	// called directly from Lua. The gui.clu wrappers are no longer needed.
	
	// IMPORTANT: These native functions MUST be registered AFTER gui.clu is loaded
	// if we want to override the CLU_Call wrappers. OR, gui.clu must be modified
	// to not define wrappers when native functions are available.
	
	// We use a flag to signal that native mode is active
	lua_pushboolean(L, 1);
	lua_setglobal(L, "_NATIVE_UI_MODE");
	
	// NOTE: We do NOT override CLU_Call globally because:
	// 1. scene.clu loads before gui.clu, and uses CLU_Call for functions we haven't wrapped yet
	// 2. Overriding causes infinite recursion when Lua wrappers call CLU_Call
	// 
	// Instead, we just register native implementations directly with their names,
	// and gui.clu checks _NATIVE_UI_MODE to decide whether to use native or CLU_Call
	
	OutputDebugStringA("[x64] Native UI functions will be registered directly\n");
	
	#define REG(name) lua_register(L, #name, lua_##name)
	
	// Helper functions
	REG(ARGB);
	
	// Texture functions
	REG(GetTextureID);
	REG(GetTerrainTextureID);
	REG(GetChaPhotoTexID);
	REG(GetSceneObjPhotoTexID);
	REG(GetEffectPhotoTexID);
	REG(GetTerrainTextureType);
	REG(GetSceneObjPhotoTexType);
	
	// Script
	REG(UI_LoadScript);
	
	// Form functions
	REG(UI_CreateForm);
	REG(UI_FormSetIsEscClose);
	REG(UI_FormSetEnterButton);
	REG(UI_FormSetHotKey);
	REG(UI_SetFormTempleteMax);
	REG(UI_AddFormToTemplete);
	REG(UI_AddAllFormTemplete);
	REG(UI_SwitchTemplete);
	REG(UI_LoadFormImage);
	REG(UI_ShowForm);
	REG(UI_SetFormStyle);
	REG(UI_SetFormStyleEx);
	REG(UI_LoadFrameImage);
	
	// Component functions
	REG(UI_CreateCompent);
	REG(UI_SetIsDrag);
	REG(UI_SetHint);
	REG(UI_SetTag);
	REG(UI_SetSize);
	REG(UI_SetPos);
	REG(UI_SetIsKeyFocus);
	REG(UI_SetCaption);
	REG(UI_CopyImage);
	REG(UI_SetAlpha);
	REG(UI_SetImageAlpha);
	REG(UI_SetAlign);
	REG(UI_SetIsShow);
	REG(UI_SetIsEnabled);
	REG(UI_SetMargin);
	REG(UI_CopyCompent);
	REG(UI_AddCompent);
	
	// Image functions
	REG(UI_LoadImage);
	REG(UI_SetMaxImage);
	REG(UI_LoadScaleImage);
	REG(UI_LoadFlashScaleImage);
	REG(UI_LoadSkillListButtonImage);
	
	// Button functions
	REG(UI_LoadButtonImage);
	REG(UI_SetButtonModalResult);
	REG(UI_ButtonSetHint);
	
	// List functions
	REG(UI_CreateListView);
	REG(UI_ListViewSetTitle);
	REG(UI_ListViewSetTitleHeight);
	REG(UI_SetListIsMouseFollow);
	REG(UI_AddListText);
	REG(UI_ListLoadSelectImage);
	REG(UI_LoadListItemImage);
	REG(UI_ListSetItemMargin);
	REG(UI_ListSetItemImageMargin);
	REG(UI_AddListBarText);
	REG(UI_SetListFontColor);
	REG(UI_SetListRowHeight);
	REG(UI_GetList);
	
	// Scroll functions
	REG(UI_GetScroll);
	REG(UI_GetScrollObj);
	REG(UI_SetScrollStyle);
	
	// Grid functions
	REG(UI_GridLoadSelectImage);
	REG(UI_SetGridIsDragSize);
	REG(UI_SetGridSpace);
	REG(UI_SetGridContent);
	REG(UI_GoodGridLoadUnitImage);
	REG(UI_SetGridUnitSize);
	REG(UI_AddFaceToGrid);
	
	// Combo functions
	REG(UI_LoadComboImage);
	REG(UI_ComboSetStyle);
	REG(UI_ComboSetTextColor);
	
	// FixList functions
	REG(UI_LoadListFixSelect);
	REG(UI_FixListSetMaxNum);
	REG(UI_FixListSetText);
	REG(UI_FixListSetRowSpace);
	REG(UI_CheckFixListSetCheckMargin);
	REG(UI_LoadCheckFixListCheck);
	
	// Progress functions
	REG(UI_SetProgressStyle);
	REG(UI_SetProgressHintStyle);
	REG(UI_SetProgressActiveMouse);
	
	// Edit functions
	REG(UI_SetEditMaxNum);
	REG(UI_SetEditCursorColor);
	REG(UI_SetEditEnterButton);
	REG(UI_SetEditMaxNumVisible);
	
	// Text/Label functions
	REG(UI_SetTextColor);
	REG(UI_SetLabelExFont);
	REG(UI_SetLabelExShadowColor);
	
	// GroupBox
	REG(UI_AddGroupBox);
	
	// Memo functions
	REG(UI_SetMemoMaxNumPerRow);
	REG(UI_SetMemoPageShowNum);
	REG(UI_SetMemoRowHeight);
	
	// Rich functions
	REG(UI_RichSetClipRect);
	REG(UI_RichSetMaxLine);
	
	// Tree functions
	REG(UI_TreeLoadImage);
	REG(UI_TreeSetNodeTextXY);
	REG(UI_CreateTextItem);
	REG(UI_CreateGraphItem);
	REG(UI_CreateNoteGraphItem);
	REG(UI_CreateSingleNode);
	REG(UI_CreateGridNode);
	REG(UI_GridNodeAddItem);
	REG(UI_CreateGraphItemTex);
	
	// Page functions
	REG(UI_CreatePageItem);
	REG(UI_GetPageItemObj);
	REG(UI_SetPageButton);
	
	// Chat
	REG(UI_SetChatColor);
	
	// Text parse
	REG(UI_SetDragSnapToGrid);
	REG(UI_SetTextParse);
	REG(UI_ItemBarLoadImage);
	
	// Menu functions
	REG(UI_MenuLoadImage);
	REG(UI_MenuLoadSelect);
	REG(UI_MenuAddText);
	
	// Filter functions
	REG(UI_AddFilterTextToNameTable);
	REG(UI_AddFilterTextToDialogTable);
	
	// HeadSay functions
	REG(UI_SetHeadSayBkgColor);
	REG(UI_LoadHeadSayFaceImage);
	REG(UI_LoadHeadSayShopImage);
	REG(UI_LoadHeadSayShopImage2);
	REG(UI_LoadHeadSayLifeImage);
	REG(UI_LoadHeadSaySayImage);
	REG(UI_LoadHeadSayMouseImage);
	
	// Title
	REG(UI_SetTitleFont);
	
	// Skill functions
	REG(UI_LoadSkillActiveImage);
	REG(UI_LoadChargeImage);
	
	// Font - register under both names for compatibility
	REG(UI_CreateFont);
	lua_register(L, "UI_CreateFont_Native", lua_UI_CreateFont);  // Alias for font.clu compatibility
	
	// Scene/Camera functions
	REG(SN_SetAttackChaColor);
	REG(SN_CreateScene);
	REG(SN_SetIsShow3DCursor);
	REG(SN_SetIsShowMinimap);
	REG(SN_SetTerrainShowCenter);
	REG(CHA_SetClientAttr);
	REG(CameraRangeXY);
	REG(CameraRangeZ);
	REG(CameraRangeFOV);
	REG(CameraEnableRotate);
	REG(CameraShowSize);
	
	// Character functions
	REG(CH_Create);
	REG(CH_SetPos);
	REG(CH_SetYaw);
	REG(CH_PlayPos);
	
	// App/Game functions
	REG(GP_SetCameraPos);
	REG(GP_GotoScene);
	
	#undef REG
	
	OutputDebugStringA("[x64] Registered all native UI functions with Lua state\n");
}

#endif // _WIN64

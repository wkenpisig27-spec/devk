#include "Stdafx.h"

#include "GameApp.h"

#include "SceneObjSet.h"
#include "EffectSet.h"
#include "MusicSet.h"
#include "CharacterPoseSet.h"
#include "MapSet.h"
#include "ChaCreateSet.h"
#include "EventSoundSet.h"
#include "AreaRecord.h"
#include "ServerSet.h"

#include "MPEditor.h"
#include "FindPath.h"
#include "MPFont.h"

#include "EffectObj.h"
#include "notifyset.h"
#include "ChatIconSet.h"
#include "ItemTypeSet.h"
#include "InputBox.h"
#include "ItemPreSet.h"
#include "ItemRefineSet.h"
#include "ItemRefineEffectSet.h"
#include "StoneSet.h"
#include "ElfSkillSet.h"
#include "GameWG.h"
#include "GameMovie.h"
#include "MonsterSet.h"
#include "helpinfoset.h"
#include "ResourceBundleManage.h"
#include "pi_Alloc.h"
#include "MountRecord.h"
#include "LootFilter.h"

#ifndef USE_DSOUND
#include "AudioThread.h"
CAudioThread g_AudioThread;
#endif

// Language string system - using CResourceBundleManage (.res files)
CResourceBundleManage g_ResourceBundleManage("Game.loc");
pi_LeakReporter pi_leakReporter("gameleak.log");

bool volatile g_bLoadRes = FALSE;
CGameApp* g_pGameApp = nullptr;

CEffectSet* CEffectSet::_Instance = nullptr;
CShadeSet* CShadeSet::_Instance = nullptr;
CMusicSet* CMusicSet::_Instance = nullptr;
CPoseSet* CPoseSet::_Instance = nullptr;
CMapSet* CMapSet::_Instance = nullptr;
CChaCreateSet* CChaCreateSet::_Instance = nullptr;
CEventSoundSet* CEventSoundSet::_Instance = nullptr;
CAreaSet* CAreaSet::_Instance = nullptr;
CServerSet* CServerSet::_Instance = nullptr;
CNotifySet* CNotifySet::_Instance = nullptr;
CChatIconSet* CChatIconSet::_Instance = nullptr;
CItemTypeSet* CItemTypeSet::_Instance = nullptr;
CItemPreSet* CItemPreSet::_Instance = nullptr;
CItemRefineSet* CItemRefineSet::_Instance = nullptr;
CItemRefineEffectSet* CItemRefineEffectSet::_Instance = nullptr;
CStoneSet* CStoneSet::_Instance = nullptr;
CElfSkillSet* CElfSkillSet::_Instance = nullptr;

CMonsterSet* CMonsterSet::_Instance = nullptr; // Add by sunny.sun 20080903
CHelpInfoSet* CHelpInfoSet::_Instance = nullptr;
// Add by Mdr
CMountSet* CMountSet::_Instance = nullptr;

MPEditor g_Editor;
CFindPath g_cFindPath(128, 38);
CFindPathEx g_cFindPathEx; // Add by mdr
CInputBox g_InputBox;

CGameWG g_oGameWG;
CGameMovie g_GameMovie;

LootFilter* g_lootFilter = nullptr;

// 锟酵伙拷锟剿版本锟斤拷, 锟斤拷GateServer锟斤拷锟斤拷证
short g_sClientVer = 32144;
DWORD g_dwCurMusicID = 0;

// CLightEff plight;

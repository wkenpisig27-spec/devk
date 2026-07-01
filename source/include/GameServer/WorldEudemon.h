// WorldEudemon.h Created by knight-gongjian 2005.3.9.
//---------------------------------------------------------
#pragma once

#ifndef _WORLD_EUDEMON_H_
#define _WORLD_EUDEMON_H_

#include "Npc.h"
//---------------------------------------------------------

namespace mission {
class CWorldEudemon : public CNpc {
public:
	CWorldEudemon();
	virtual ~CWorldEudemon();

	virtual void SetType() { m_byType = EUDEMON; }

	// 网络消息处理函数
	virtual HRESULT MsgProc(CCharacter& character, RPACKET& packet);

	// 装载世界守护神配置信息
	virtual BOOL Load(const char szMsgProc[], const char szName[], dbc::uLong ulID);

private:
	// 装载脚本信息
	virtual BOOL InitScript(const char szFunc[], const char szName[]);
};

extern CWorldEudemon g_WorldEudemon;
} // namespace mission

//---------------------------------------------------------

#endif // _WORLD_EUDEMON_H_
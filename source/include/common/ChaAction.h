//=============================================================================
// FileName: ChaAction.h
// Creater: ZhangXuedong
// Date: 2005.09.15
// Comment: Character Action Control Type
//=============================================================================

#ifndef CHAACTION_H
#define CHAACTION_H

const int ACTCONTROL_MOVE = 0;		   // 移动位
const int ACTCONTROL_USE_GSKILL = 1;  // 使用普通技能位
const int ACTCONTROL_USE_MSKILL = 2;  // 使用魔法技能位
const int ACTCONTROL_BEUSE_SKILL = 3; // 被使用技能位
const int ACTCONTROL_TRADE = 4;	   // 交易位
const int ACTCONTROL_USE_ITEM = 5;	   // 使用物品位
const int ACTCONTROL_BEUSE_ITEM = 6;  // 被使用物品位
const int ACTCONTROL_INVINCIBLE = 7;  // 无敌位
const int ACTCONTROL_EYESHOT = 8;	   // 视野位（可以看到视野内可见的实体）
const int ACTCONTROL_NOHIDE = 9;	   // 不隐形（可以被看见）
const int ACTCONTROL_NOSHOW = 10;	   // 不被强制现形（如果有强制现形，则隐形被屏蔽）
const int ACTCONTROL_ITEM_OPT = 11;   // 道具操作位
const int ACTCONTROL_TALKTO_NPC = 12; // 和NPC对话位

const int ACTCONTROL_MAX = 13;

#endif // CHAACTION_H
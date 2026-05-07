#include "stdafx.h"
#include "lua_bridge.h"
#include "ChaAttr.h"
#include "SubMap.h"

// -----------------------------------------------------------------------
// CCharacter LuaBridge class registration
//
// Exposes a subset of CCharacter methods to Lua as OOP-style calls:
//   local hp  = cha:GetAttr(ATTR_HP)
//   local lv  = cha:GetLevel()
//   local nm  = cha:GetName()
//   local dbid= cha:GetDBID()
//   local x,y = cha:GetPosX(), cha:GetPosY()
//
// All existing global functions (QueryChaAttr, GetChaName, ...) continue to
// work unchanged — this only ADDS the OOP interface on top.
// -----------------------------------------------------------------------

void RegisterCCharacterClass(lua_State* L)
{
    luabridge::getGlobalNamespace(L)
        .beginClass<CCharacter>("Character")
            // Identity / info
            .addFunction("GetName",    &CCharacter::GetName)
            .addFunction("GetLevel",   &CCharacter::GetLevel)
            .addFunction("IsLiveing",  &CCharacter::IsLiveing)
            .addFunction("IsPlayer",   &CCharacter::IsPlayerCha)
            .addFunction("IsBoat",     &CCharacter::IsBoat)
            .addFunction("IsHide",     &CCharacter::IsHide)
            .addFunction("IsPKSilver", &CCharacter::IsPKSilver)

            // Database ID  (m_CChaAttr.m_lID is dbc::Long = int32_t)
            .addFunction("GetDBID", [](CCharacter* c) -> int {
                return static_cast<int>(c->m_CChaAttr.m_lID);
            })

            // General attribute query — wraps getAttr(int) which returns dbc::LLong (__int64)
            // Cast to long long for portable LuaBridge marshaling
            .addFunction("GetAttr", [](CCharacter* c, int idx) -> long long {
                return static_cast<long long>(c->getAttr(idx));
            })
            .addFunction("SetAttr", [](CCharacter* c, int idx, long long val) {
                c->setAttr(idx, static_cast<LONG64>(val));
            })

            // Position — GetPos() returns const Point& which LuaBridge can't expose
            // directly without binding the Point type, so expose as X/Y accessors
            .addFunction("GetPosX", [](CCharacter* c) -> int {
                return static_cast<int>(c->GetPos().x);
            })
            .addFunction("GetPosY", [](CCharacter* c) -> int {
                return static_cast<int>(c->GetPos().y);
            })

            // Block count (used by NPC AI)
            .addFunction("GetBlockCnt", &CCharacter::GetBlockCnt)
            .addFunction("SetBlockCnt", &CCharacter::SetBlockCnt)

            // Guild
            .addFunction("GetGuildID",   &CCharacter::GetGuildID)
            .addFunction("GetGuildName", &CCharacter::GetGuildName)
            .addFunction("HasGuild",     &CCharacter::HasGuild)

            // Motto / icon
            .addFunction("GetMotto", &CCharacter::GetMotto)
            .addFunction("GetIcon",  &CCharacter::GetIcon)

            // Map info
            .addFunction("GetSubMapName", [](CCharacter* c) -> const char* {
                auto* sm = c->GetSubMap();
                return sm ? sm->GetName() : "";
            })
        .endClass();
}

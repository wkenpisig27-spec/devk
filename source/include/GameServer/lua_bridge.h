#pragma once

// LuaBridge3 backwards-compatible integration for CCharacter*.
//
// Provides:
//   - Stack<CCharacter*> specialization: push as full userdata, get from EITHER
//     lightuserdata (legacy scripts) or full userdata (new LuaBridge path)
//   - LB_GetCha(L, idx)  — drop-in for (CCharacter*)lua_touserdata(L, idx)
//   - LB_PushCha(L, p)   — drop-in for lua_pushlightuserdata that enables OOP
//   - RegisterCCharacterClass(L) — call once at server startup

#include "Character.h"

// LuaBridge requires Lua headers included before it.
#include "lua.hpp"

// Windows headers define min/max as macros; suppress them so LuaBridge can use
// std::numeric_limits<T>::min() / max() without syntax errors.
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#include "../luabridge/LuaBridge.h"

namespace luabridge {

// Custom Stack specialization for CCharacter*.
// push: full userdata (enables cha:GetHP() etc. in Lua)
// get:  accepts both lightuserdata (old scripts) and full userdata (new)
template <>
struct Stack<CCharacter*>
{
    using ReturnType = TypeResult<CCharacter*>;

    [[nodiscard]] static Result push(lua_State* L, CCharacter* p)
    {
        return detail::UserdataPtr::push(L, p);
    }

    [[nodiscard]] static ReturnType get(lua_State* L, int index)
    {
        if (lua_islightuserdata(L, index))
            return static_cast<CCharacter*>(lua_touserdata(L, index));

        auto result = detail::Userdata::get<CCharacter>(L, index, false);
        if (!result)
            return result.error();
        return *result;
    }

    [[nodiscard]] static bool isInstance(lua_State* L, int index)
    {
        return lua_isuserdata(L, index) != 0;
    }
};

} // namespace luabridge

// Get CCharacter* from Lua stack at index — handles lightuserdata (old) and full userdata (new).
// Use everywhere you previously wrote: (CCharacter*)lua_touserdata(L, idx)
inline CCharacter* LB_GetCha(lua_State* L, int idx)
{
    if (lua_islightuserdata(L, idx))
        return static_cast<CCharacter*>(lua_touserdata(L, idx));
    if (lua_isuserdata(L, idx)) {
        auto result = luabridge::detail::Userdata::get<CCharacter>(L, idx, false);
        if (result) return *result;
    }
    return nullptr;
}

// Push CCharacter* to Lua stack as LuaBridge full userdata.
// Use everywhere you previously wrote: lua_pushlightuserdata(L, (void*)pCha)
inline void LB_PushCha(lua_State* L, CCharacter* p)
{
    if (!p) {
        lua_pushnil(L);
        return;
    }
    auto err = luabridge::detail::UserdataPtr::push(L, p);
    if (err) {
        // Class not registered yet — fall back to lightuserdata so scripts still work
        lua_pushlightuserdata(L, static_cast<void*>(p));
    }
}

// Register CCharacter as a LuaBridge class.  Call once from RegisterLuaAI().
void RegisterCCharacterClass(lua_State* L);

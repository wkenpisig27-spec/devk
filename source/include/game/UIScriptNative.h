//----------------------------------------------------------------------
// UIScriptNative.h
//
// Native Lua wrappers for all UI functions on x64
// 
// On x86, CaLua's CLU_Call works via inline assembly.
// On x64, we register native Lua wrappers directly with the Lua state.
//----------------------------------------------------------------------
#pragma once

#ifdef _WIN64

struct lua_State;

// Register all native UI functions with the given Lua state
// Call this during initialization after CLU_Init() and before loading any .clu scripts
void RegisterNativeUIFunctions(lua_State* L);

#endif // _WIN64

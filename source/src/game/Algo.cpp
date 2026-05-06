
#include "stdafx.h"
#include "Algo.h"
#include "NetIF.h"

#include <cstdint>
#include <intrin.h>

// x64-compatible version of big_apple (float to int64 conversion)
static inline int64_t big_apple_x64(double val) {
	return static_cast<int64_t>(val);
}

#pragma warning(disable : 4800)
int lua_fox_boff(lua_State* L) {
	if ((lua_gettop(L) == 3) && lua_isuserdata(L, 1) && lua_isnumber(L, 2) && lua_isnumber(L, 3)) {
		unsigned char* ptr = (unsigned char*)lua_touserdata(L, 1);
		int64_t offset = big_apple_x64(lua_tonumber(L, 2));
		int64_t xorval = big_apple_x64(lua_tonumber(L, 3));
		ptr[offset] ^= (unsigned char)xorval;
	}
	return 0;
}

int lua_dog_blog(lua_State* L) {
	if ((lua_gettop(L) == 3) && lua_isuserdata(L, 1) && lua_isnumber(L, 2) && lua_isnumber(L, 3)) {
		unsigned char* ptr = (unsigned char*)lua_touserdata(L, 1);
		int64_t offset = big_apple_x64(lua_tonumber(L, 2));
		int64_t rotval = big_apple_x64(lua_tonumber(L, 3));
		ptr[offset] = _rotl8(ptr[offset], (unsigned char)rotval);
	}
	return 0;
}

int lua_dog_brog(lua_State* L) {
	if ((lua_gettop(L) == 3) && lua_isuserdata(L, 1) && lua_isnumber(L, 2) && lua_isnumber(L, 3)) {
		unsigned char* ptr = (unsigned char*)lua_touserdata(L, 1);
		int64_t offset = big_apple_x64(lua_tonumber(L, 2));
		int64_t rotval = big_apple_x64(lua_tonumber(L, 3));
		ptr[offset] = _rotr8(ptr[offset], (unsigned char)rotval);
	}
	return 0;
}

int lua_cat_fish(lua_State* L) {
	if ((lua_gettop(L) == 2) && lua_isnumber(L, 1) && lua_isnumber(L, 2)) {
		unsigned int a = (unsigned int)lua_tonumber(L, 1);
		unsigned int b = (unsigned int)lua_tonumber(L, 2);
		lua_pushnumber(L, a / b);
		lua_pushnumber(L, a % b);
		return 2;
	} else
		return 0;
}

#define REGFN(fn)                          \
	{                                      \
		lua_pushstring(L, "" #fn "");      \
		lua_pushcfunction(L, lua_##fn);    \
		lua_settable(L, LUA_GLOBALSINDEX); \
	}

lua_State* init_lua() {
	lua_State* L = luaL_newstate();
	luaL_openlibs(L);
	lua_register(L, "lua_fox_boff", lua_fox_boff);
	lua_register(L, "lua_dog_blog", lua_dog_blog);
	lua_register(L, "lua_dog_brog", lua_dog_brog);
	lua_register(L, "lua_cat_fish", lua_cat_fish);
	return L;
}

void exit_lua(lua_State* L) {
	if (L) {
		lua_close(L);
	}
}

static void callalert(lua_State* L, int status) {
	if (status != 0) {
		lua_getglobal(L, "_ALERT");
		if (lua_isfunction(L, -1)) {
			lua_insert(L, -2);
			lua_call(L, 1, 0);
		} else { // no _ALERT function; print it on stderr
			fprintf(stderr, "%s\n", lua_tolstring(L, -2, nullptr));
			lua_pop(L, 2);
		}
	}
}

int load_luc(lua_State* L, char const* fname) {
	int status = luaL_loadfile(L, fname);
	if (status != 0) {
		printf("load luc file %s error\n", fname);
		return -1;
	}
	status = lua_pcall(L, 0, LUA_MULTRET, 0);
	callalert(L, status);
	return status;
}

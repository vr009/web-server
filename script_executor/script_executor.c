#include </usr/local/include/lua.h>
#include </usr/local/include/lualib.h>
#include </usr/local/include/lauxlib.h>

#include "script_executor.h"

int execute_script(char * script_path) {
	if (script_path == NULL) {
		return -1;
	}
	lua_State *L = luaL_newstate();
	luaL_openlibs(L);
	fprintf(stdout, "OK");

	if (luaL_dofile(L, script_path) == LUA_OK) {
		lua_pop(L, lua_gettop(L));
	}

	lua_close(L);
	return 0;
}

int execute_2() {
	lua_State *L = luaL_newstate();
	luaL_openlibs(L);

	if (luaL_dofile(L, "hello.lua") == LUA_OK) {
		lua_pop(L, lua_gettop(L));
	}

	lua_getglobal(L, "message");

	if (lua_isstring(L, -1)) {
		const char * message = lua_tostring(L, -1);
		lua_pop(L, 1);
		printf("Message from lua: %s\n", message);
	}

	lua_close(L);
	return 0;
}

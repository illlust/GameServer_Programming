#include <iostream>

extern "C"
{
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

#pragma comment (lib, "lua53.lib")
using namespace std;

int add_two_c(lua_State* L)
{
	int a = (int)lua_tonumber(L, -2);
	int b = (int)lua_tonumber(L, -1);
	lua_pop(L, 3); //2 해야하는데 3 했다. 이제 이 스택에 들어가 있는 함수들 필요 없어서.
	lua_pushnumber(L, a + b);
	return 1;
}

int main()
{
	lua_State *L = luaL_newstate();					//가상 머신을 생성하는 함수. 루아를 연다. 
	luaL_openlibs(L);								//루아 표준 라이브러리를 연다
	
	luaL_loadfile(L, "TEST.LUA");

	int error = lua_pcall(L, 0, 0, 0);
	if (error)
	{
		cout << "Error : " << lua_tostring(L, -1);
		lua_pop(L, 1);
	}
	
	lua_register(L, "c_add_two", add_two_c);
	//스택에 놓는 순서 
	lua_getglobal(L, "add_two"); //함수를 놓고 
	lua_pushnumber(L, 100); //파라미터를 놓는다.
	lua_pushnumber(L, 200); //파라미터를 놓는다.
	lua_pcall(L, 2, 1, 0);

	int result = (int)lua_tonumber(L, -1);
	lua_pop(L, 1);

	cout << "result : " << result << "\n";

	lua_close(L);

	system("pause");
}


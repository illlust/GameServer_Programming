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
	lua_pop(L, 3); //2 �ؾ��ϴµ� 3 �ߴ�. ���� �� ���ÿ� �� �ִ� �Լ��� �ʿ� ���.
	lua_pushnumber(L, a + b);
	return 1;
}

int main()
{
	lua_State *L = luaL_newstate();					//���� �ӽ��� �����ϴ� �Լ�. ��Ƹ� ����. 
	luaL_openlibs(L);								//��� ǥ�� ���̺귯���� ����
	
	luaL_loadfile(L, "TEST.LUA");

	int error = lua_pcall(L, 0, 0, 0);
	if (error)
	{
		cout << "Error : " << lua_tostring(L, -1);
		lua_pop(L, 1);
	}
	
	lua_register(L, "c_add_two", add_two_c);
	//���ÿ� ���� ���� 
	lua_getglobal(L, "add_two"); //�Լ��� ���� 
	lua_pushnumber(L, 100); //�Ķ���͸� ���´�.
	lua_pushnumber(L, 200); //�Ķ���͸� ���´�.
	lua_pcall(L, 2, 1, 0);

	int result = (int)lua_tonumber(L, -1);
	lua_pop(L, 1);

	cout << "result : " << result << "\n";

	lua_close(L);

	system("pause");
}


#pragma once

#define SERVER_PORT		9000
#define NPC_ID_START	5000
#define NUM_NPC			2000

constexpr int MAX_ID_LEN = 50;
constexpr int MAX_STR_LEN = 80;

constexpr int MAX_PACKET_SIZE = 255;
constexpr auto MAX_BUF_SIZE = 1024;
constexpr auto MAX_USER = NPC_ID_START;
constexpr auto VIEW_RADIUS = 7;

#define WORLD_WIDTH		800
#define WORLD_HEIGHT	800

#define MAX_STR_SIZE  100

//받는 입장에서는 패킷 type이 날아온다. 보내는 입장에서는 이걸 담아 보내야 알려줄 수 있다.
#define C2S_LOGIN	1
#define C2S_MOVE	2
#define C2S_ATTACK  3
#define C2S_CHAT    4
#define C2S_LOGOUT  5

#define S2C_LOGIN_OK		1
#define S2C_LOGIN_FAIL		2
#define S2C_MOVE			3
#define S2C_ENTER			4
#define S2C_LEAVE			5
#define S2C_CHAT			6
#define S2C_STAT_CHANGE     7

#define NPC_PEACE 0
#define NPC_WAR 1

#define NPC_FIX 0
#define NPC_RANDOM_MOVE 1

enum GRID_TYPE
{
	eBLANK,
	eSTART,
	eEND,
	eBLOCKED,
	eOPEN,
	eCLOSE,
};

struct MAP
{
	GRID_TYPE type; //0-땅바닥, 3-장애물, 10-몬스터, 11-플레이어, 12-다른플레이어, 
};

#pragma pack(push ,1)

struct sc_packet_chat {
	char size;
	char type;
	int	 id;
	//wchar_t mess[MAX_STR_LEN];
	char mess[MAX_STR_LEN];
	int chatType; //0-개인 / 1-시스템
};

struct sc_packet_login_ok {
	char size;
	char type;
	int id;
	short x, y;
	short hp;
	short level;
	int	exp;
};

struct sc_packet_login_fail {
	char size;
	char type;
};

struct sc_packet_move {
	char size;
	char type;
	int id;
	short x, y;
	unsigned move_time;
};


//OBJECT TYPE
constexpr unsigned char O_PLAYER = 0;
constexpr unsigned char O_NPC_PEACE = 1;
constexpr unsigned char O_NPC_WAR = 2;

struct sc_packet_enter {
	char size;
	char type;
	int id;
	char name[MAX_ID_LEN];
	char o_type;
	short x, y;

	bool npcCharacterType; //0-peace / 1-war
	bool npcMoveType; //0-고정 / 1-로밍

};

struct sc_packet_leave {
	char size;
	char type;
	int id;
};

struct cs_packet_login {
	char	size;
	char	type;
	char	name[MAX_ID_LEN];
};

struct sc_packet_stat_change {
	char size;
	char type;
	int id; //내가 추가
	short hp;
	short level;
	int	exp;
};

constexpr unsigned char D_UP = 0;
constexpr unsigned char D_DOWN = 1;
constexpr unsigned char D_LEFT = 2;
constexpr unsigned char D_RIGHT = 3;

struct cs_packet_move {
	char	size;
	char	type;
	char	direction;
	unsigned move_time;
};

struct cs_packet_attack {
	char size;
	char type;
};

struct cs_packet_chat {
	char size;
	char type;
	wchar_t message[MAX_STR_LEN];
};

struct cs_packet_logout {
	char size;
	char type;
};

#pragma pack (pop)
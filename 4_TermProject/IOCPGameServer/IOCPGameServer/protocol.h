#pragma once

#define SERVER_PORT		9000
#define NPC_ID_START	20000
#define NUM_NPC			2000

constexpr int MAX_ID_LEN = 50;
constexpr int MAX_STR_LEN = 80;

constexpr int MAX_PACKET_SIZE = 255;
constexpr auto MAX_BUF_SIZE = 1024;
constexpr auto MAX_USER = NPC_ID_START;
constexpr auto VIEW_RADIUS = 6;

#define WORLD_WIDTH		400
#define WORLD_HEIGHT	400

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


#pragma pack(push ,1)

struct sc_packet_chat {
	char size;
	char type;
	int	 id;
	//wchar_t mess[MAX_STR_LEN];
	char mess[MAX_STR_LEN];
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
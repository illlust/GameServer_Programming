#define _CRT_SECURE_NO_WARNINGS
#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <windows.h>
#include <iostream>
#include <unordered_map>
#include <chrono>
using namespace std;
using namespace chrono;

#include "..\..\IOCPGameServer\IOCPGameServer\protocol.h"

//#pragma comment(lib, "sfml_network.lib")

sf::TcpSocket g_socket;

constexpr auto SCREEN_WIDTH = 20;
constexpr auto SCREEN_HEIGHT = 20;

constexpr auto TILE_WIDTH = 65;
constexpr auto WINDOW_WIDTH = TILE_WIDTH * SCREEN_WIDTH / 2 + 10;   // size of window
constexpr auto WINDOW_HEIGHT = TILE_WIDTH * SCREEN_WIDTH / 2 + 100;
constexpr auto BUF_SIZE = 200;


// 추후 확장용.
//int NPC_ID_START = 10000;

int g_left_x;
int g_top_y;
int g_myid;

sf::RenderWindow* g_window;
sf::Font g_font;

sf::Text m_worldText[3];
high_resolution_clock::time_point m_worldtime_out[3];

sf::Text m_responText;
high_resolution_clock::time_point m_time_out_responText;

sf::RectangleShape rectangle_EXPbox;
sf::RectangleShape rectangle_EXP;

class OBJECT {
private:
	bool m_showing;
	sf::Sprite m_sprite;

	char m_mess[MAX_STR_LEN];
	high_resolution_clock::time_point m_time_out;
	sf::Text m_text;
	sf::Text m_name;
	sf::Text m_level;
	sf::Text m_hp;

	sf::RectangleShape rectangle_HPbox;
	sf::RectangleShape rectangle_HP;

	sf::CircleShape circle_levelBox;

public:
	sf::Text m_exp;
	int m_x, m_y;
	int m_id;
	char name[MAX_ID_LEN];
	short level;
	int exp;
	short hp;

	bool npcCharacterType; //0-peace / 1-war
	bool npcMoveType; //0-고정 / 1-로밍

	OBJECT(sf::Texture& t, int x, int y, int x2, int y2) {
		m_showing = false;
		m_sprite.setTexture(t);
		m_sprite.setTextureRect(sf::IntRect(x, y, x2, y2));
		m_time_out = high_resolution_clock::now();
	}
	OBJECT() {
		m_showing = false;
		m_time_out = high_resolution_clock::now();
	}
	void show()
	{
		m_showing = true;
	}
	void hide()
	{
		m_showing = false;
	}

	void a_move(int x, int y) {
		m_sprite.setPosition((float)x, (float)y);
	}

	void a_draw() {
		g_window->draw(m_sprite);
	}

	void move(int x, int y) {
		m_x = x;
		m_y = y;
	}
	void draw() {
		
		if (false == m_showing) return;
		float rx = (m_x - g_left_x) * 65.0f + 8;
		float ry = (m_y - g_top_y) * 65.0f + 8;
		
		m_sprite.setPosition(rx, ry);
		g_window->draw(m_sprite);

		m_name.setPosition(rx , ry - 65);
		g_window->draw(m_name);
		
		//level
		circle_levelBox.setPosition(sf::Vector2f(rx - 50, ry - 25));
		circle_levelBox.setOutlineThickness(3.0f);
		circle_levelBox.setOutlineColor(sf::Color::White);
		circle_levelBox.setFillColor(sf::Color::Black);
		circle_levelBox.setRadius(18);
		g_window->draw(circle_levelBox);

		m_level.setPosition(rx - 40, ry - 25);
		g_window->draw(m_level);

		//exp
		//rectangle_EXPbox.setPosition(sf::Vector2f(100, WINDOW_HEIGHT + 550));
		//rectangle_EXPbox.setOutlineThickness(3.0f);
		//rectangle_EXPbox.setOutlineColor(sf::Color::White);
		//rectangle_EXPbox.setFillColor(sf::Color::Black);
		//rectangle_EXPbox.setSize(sf::Vector2f(WINDOW_WIDTH + 500, 20));
		//g_window->draw(rectangle_EXPbox);

		//rectangle_EXP.setPosition(sf::Vector2f(100, WINDOW_HEIGHT + 550));
		//rectangle_EXP.setFillColor(sf::Color::Blue);
		//rectangle_EXP.setSize(sf::Vector2f(exp, 20));
		//g_window->draw(rectangle_EXP);

		////total exp level * level * 100 

		//m_exp.setPosition(10, WINDOW_HEIGHT + 530);
		//g_window->draw(m_exp);

		//hp
		rectangle_HPbox.setPosition(sf::Vector2f(rx, ry - 25));
		rectangle_HPbox.setOutlineThickness(3.0f);
		rectangle_HPbox.setOutlineColor(sf::Color::White);
		rectangle_HPbox.setFillColor(sf::Color::Red);
		rectangle_HPbox.setSize(sf::Vector2f(100, 30));
		g_window->draw(rectangle_HPbox);

		rectangle_HP.setPosition(sf::Vector2f(rx, ry - 25));
		rectangle_HP.setFillColor(sf::Color::Green);
		rectangle_HP.setSize(sf::Vector2f(hp, 30));
		g_window->draw(rectangle_HP);
		
		m_hp.setPosition(rx + 10, ry - 25);
		g_window->draw(m_hp);



		if (high_resolution_clock::now() < m_time_out) {
			m_text.setPosition(rx - 10, ry + 15);
			g_window->draw(m_text);
		}

		if (high_resolution_clock::now() < m_worldtime_out[0]) {
			m_worldText[0].setPosition(10, WINDOW_HEIGHT + 640);
			g_window->draw(m_worldText[0]);
		}
		if (high_resolution_clock::now() < m_worldtime_out[1]) {
			m_worldText[1].setPosition(10, WINDOW_HEIGHT + 600);
			g_window->draw(m_worldText[1]);
		}
		if (high_resolution_clock::now() < m_worldtime_out[2]) {
			m_worldText[2].setPosition(10, WINDOW_HEIGHT + 560);
			g_window->draw(m_worldText[2]);
		}
		if (high_resolution_clock::now() < m_time_out_responText) {
			m_responText.setPosition(200, WINDOW_HEIGHT - 300);
			g_window->draw(m_responText);
		}
	}
	void set_name(char str[]) {
		m_name.setFont(g_font);
		m_name.setString(str);
		m_name.setCharacterSize(30);
		m_name.setFillColor(sf::Color(255, 255, 0));
		m_name.setStyle(sf::Text::Bold);
	}

	void set_level(char str[]) {
		m_level.setFont(g_font);
		m_level.setString(str);
		m_level.setCharacterSize(30);
		m_level.setFillColor(sf::Color(255, 255, 255));
		m_level.setStyle(sf::Text::Bold);
	}

	void set_exp(char str[]) {
		m_exp.setFont(g_font);
		m_exp.setString(str);
		m_exp.setCharacterSize(40);
		m_exp.setOutlineThickness(3.0f);
		m_exp.setOutlineColor(sf::Color::White);
		m_exp.setFillColor(sf::Color(255, 0, 0));
		m_exp.setStyle(sf::Text::Bold);
	}

	void set_hp(char str[]) {
		m_hp.setFont(g_font);
		m_hp.setString(str);
		m_hp.setCharacterSize(20);
		m_hp.setFillColor(sf::Color(255, 255, 255));
		m_hp.setStyle(sf::Text::Bold);
	}



	void add_chat(char chat[]) {
		m_text.setFont(g_font);
		m_text.setString(chat);
		m_time_out = high_resolution_clock::now() + 1s;
	}

	void add_chat_world(char chat[])
	{
		for (int i = 0; i < 3; ++i)
		{
			m_worldText[i].setFont(g_font);
			m_worldText[i].setCharacterSize(40);
			m_worldText[i].setFillColor(sf::Color(255, 255, 255));
		}
		m_worldText[2].setString(m_worldText[1].getString());
		m_worldText[1].setString(m_worldText[0].getString());
		m_worldText[0].setString(chat);

		m_worldtime_out[2] = m_worldtime_out[1];
		m_worldtime_out[1] = m_worldtime_out[0];
		m_worldtime_out[0] = high_resolution_clock::now() + 5s;
	}

	void add_chat_respown(char chat[]) {
		m_responText.setFont(g_font);
		m_responText.setString(chat);
		m_responText.setCharacterSize(100);
		m_responText.setFillColor(sf::Color(255, 255, 255));
		m_time_out_responText = high_resolution_clock::now() + 2s;
	}
};

OBJECT avatar;
unordered_map <int, OBJECT> npcs;

OBJECT blocked_tile;
OBJECT blank_tile;

sf::Texture* board;
sf::Texture* pieces;

char g_Map[WORLD_HEIGHT][WORLD_WIDTH];

void init_map()
{
	char data;
	FILE* fp = fopen("mapData.txt", "rb");

	int count = 0;


	while (fscanf(fp, "%c", &data) != EOF) {
		//printf("%c", data);
		switch (data)
		{
		case '0':
			g_Map[count / 800][count % 800] = eBLANK;
			count++;
			break;
		case '3':
			g_Map[count / 800][count % 800] = eBLOCKED;
			count++;
			break;
		default:
			break;
		}
	}
}

void client_initialize()
{
	board = new sf::Texture;
	pieces = new sf::Texture;
	if (false == g_font.loadFromFile("NanumGothicBold.ttf")) {
		cout << "Font Loading Error!\n";
		while (true);
	}
	board->loadFromFile("mapTile.bmp");
	//board->loadFromFile("chessmap.bmp");
	pieces->loadFromFile("chess2.png");
	blank_tile = OBJECT{ *board, 0, 0, TILE_WIDTH, TILE_WIDTH };
	blocked_tile = OBJECT{ *board, 65, 0, TILE_WIDTH, TILE_WIDTH };
	avatar = OBJECT{ *pieces, 128, 0, 64, 64 };
	avatar.move(4, 4);
}

void client_finish()
{
	delete board;
	delete pieces;
}

void ProcessPacket(char* ptr)
{
	static bool first_time = true;
	switch (ptr[1])
	{
	case S2C_LOGIN_OK:
	{
		sc_packet_login_ok* my_packet = reinterpret_cast<sc_packet_login_ok*>(ptr);
		g_myid = my_packet->id;
		avatar.move(my_packet->x, my_packet->y);
		g_left_x = my_packet->x - (SCREEN_WIDTH / 2);
		g_top_y = my_packet->y - (SCREEN_HEIGHT / 2);
		avatar.show();
	}
	break;

	case S2C_ENTER:
	{
		sc_packet_enter* my_packet = reinterpret_cast<sc_packet_enter*>(ptr);
		int id = my_packet->id;

		if (id == g_myid) {
			avatar.move(my_packet->x, my_packet->y);
			g_left_x = my_packet->x - (SCREEN_WIDTH / 2);
			g_top_y = my_packet->y - (SCREEN_HEIGHT / 2);
			avatar.show();
		}
		else {
			if (id < NPC_ID_START)
				npcs[id] = OBJECT{ *pieces, 64, 0, 64, 64 };
			else
			{
				if(my_packet->npcCharacterType == NPC_WAR)
					npcs[id] = OBJECT{ *pieces, 0, 0, 64, 64 };
				else
					npcs[id] = OBJECT{ *pieces, 192, 0, 64, 64 };
			}
			npcs[id].npcCharacterType = my_packet->npcCharacterType;
			npcs[id].npcMoveType = my_packet->npcMoveType;

			strcpy_s(npcs[id].name, my_packet->name);
			npcs[id].set_name(my_packet->name);
			npcs[id].move(my_packet->x, my_packet->y);
			npcs[id].show();
		}
	}
	break;
	case S2C_MOVE:
	{
		sc_packet_move* my_packet = reinterpret_cast<sc_packet_move*>(ptr);
		int other_id = my_packet->id;
		if (other_id == g_myid) {
			avatar.move(my_packet->x, my_packet->y);
			g_left_x = my_packet->x - (SCREEN_WIDTH / 2);
			g_top_y = my_packet->y - (SCREEN_HEIGHT / 2);
		}
		else {
			if (0 != npcs.count(other_id))
				npcs[other_id].move(my_packet->x, my_packet->y);
		}
	}
	break;

	case S2C_LEAVE:
	{
		sc_packet_leave* my_packet = reinterpret_cast<sc_packet_leave*>(ptr);
		int other_id = my_packet->id;
		if (other_id == g_myid) {
			avatar.hide();
		}
		else {
			if (0 != npcs.count(other_id))
			{
				npcs[other_id].hide();
			}
		}
	}
	break;

	case S2C_CHAT:
	{	
		sc_packet_chat* my_packet = reinterpret_cast<sc_packet_chat*>(ptr);
		int o_id = my_packet->id;
		int chatType = my_packet->chatType;
		if (chatType == 0)
		{
			if (0 != npcs.count(o_id))
			{
				npcs[o_id].add_chat(my_packet->mess);
			}
		}
		else if (chatType == 1)
		{
			npcs[o_id].add_chat_world(my_packet->mess);
		}
		else if (chatType == 2)
		{
			npcs[o_id].add_chat_respown(my_packet->mess);
		}

	}
	break;

	case S2C_STAT_CHANGE:
	{
		sc_packet_stat_change* my_packet = reinterpret_cast<sc_packet_stat_change*>(ptr);
		int id = my_packet->id;

		if (id == g_myid) {
			avatar.hp = my_packet->hp;
			avatar.level = my_packet->level;
			avatar.exp = my_packet->exp;

			char str[20];
			sprintf(str, "%d", my_packet->level); 
			avatar.set_level(str);
			
			sprintf(str, "EXP "); 
			avatar.set_exp(str);
			
			sprintf(str, "%d/100", my_packet->hp); 
			avatar.set_hp(str);

			avatar.show();
		}
		else {
			npcs[id].exp = my_packet->exp;
			npcs[id].hp = my_packet->hp;
			npcs[id].level = my_packet->level;

			char str[20];
			sprintf(str, "%d", my_packet->level);
			npcs[id].set_level(str);

			sprintf(str, "%d/100", my_packet->hp);
			npcs[id].set_hp(str);

			npcs[id].show();
		}


	}
	break;
	
	default:
		printf("Unknown PACKET type [%d]\n", ptr[1]);

	}
}

void process_data(char* net_buf, size_t io_byte)
{
	char* ptr = net_buf;
	static size_t in_packet_size = 0;
	static size_t saved_packet_size = 0;
	static char packet_buffer[BUF_SIZE];

	while (0 != io_byte) {
		if (0 == in_packet_size) in_packet_size = ptr[0];
		if (io_byte + saved_packet_size >= in_packet_size) {
			memcpy(packet_buffer + saved_packet_size, ptr, in_packet_size - saved_packet_size);
			ProcessPacket(packet_buffer);
			ptr += in_packet_size - saved_packet_size;
			io_byte -= in_packet_size - saved_packet_size;
			in_packet_size = 0;
			saved_packet_size = 0;
		}
		else {
			memcpy(packet_buffer + saved_packet_size, ptr, io_byte);
			saved_packet_size += io_byte;
			io_byte = 0;
		}
	}
}

void client_main()
{
	char net_buf[BUF_SIZE];
	size_t	received;

	auto recv_result = g_socket.receive(net_buf, BUF_SIZE, received);
	if (recv_result == sf::Socket::Error)
	{
		wcout << L"Recv 에러!";
		while (true);
	}

	if (recv_result == sf::Socket::Disconnected)
	{
		wcout << L"서버 접속 종료.";
		g_window->close();
	}

	if (recv_result != sf::Socket::NotReady)
		if (received > 0) process_data(net_buf, received);

	for (int i = 0; i < SCREEN_WIDTH; ++i)
	{
		for (int j = 0; j < SCREEN_HEIGHT; ++j)
		{
			int tile_x = i + g_left_x;
			int tile_y = j + g_top_y;
			if ((tile_x < 0) || (tile_y < 0)) continue;

			if (g_Map[tile_y][tile_x] == eBLANK)
			{
				blank_tile.a_move(TILE_WIDTH * i, TILE_WIDTH * j);
				blank_tile.a_draw();
			}
			else if (g_Map[tile_y][tile_x] == eBLOCKED)
			{
				blocked_tile.a_move(TILE_WIDTH * i, TILE_WIDTH * j);
				blocked_tile.a_draw();
			}

			//if (((tile_x + tile_y) % 2) == 0) {
			//	if (((tile_x / 3 + tile_y / 3) % 2) == 0) {
			//		blank_tile.a_move(TILE_WIDTH * i + 7, TILE_WIDTH * j + 7);
			//		blank_tile.a_draw();
			//	}
			//	else
			//	{
			//		blocked_tile.a_move(TILE_WIDTH * i + 7, TILE_WIDTH * j + 7);
			//		blocked_tile.a_draw();
			//	}
			//}
		}
	}
	avatar.draw();
	//	for (auto &pl : players) pl.draw();
	for (auto& npc : npcs) npc.second.draw();
	sf::Text text;
	text.setFont(g_font);
	char buf[100];
	sprintf_s(buf, "(%d, %d)", avatar.m_x, avatar.m_y);
	text.setPosition(0, -50);
	text.setCharacterSize(40);
	text.setString(buf);
	g_window->draw(text);

	sprintf_s(buf, "LV : %d ## EXP : %d/%d ## HP : %d/100", avatar.level, avatar.exp, (int)(100 * pow(2, (avatar.level - 1))), avatar.hp);
	text.setString(buf);
	text.setPosition(300, -50);
	text.setCharacterSize(40);
	text.setOutlineColor(sf::Color::Green);
	text.setStyle(sf::Text::Underlined);
	g_window->draw(text);

	//exp
	rectangle_EXPbox.setPosition(sf::Vector2f(100, WINDOW_HEIGHT + 550));
	rectangle_EXPbox.setOutlineThickness(3.0f);
	rectangle_EXPbox.setOutlineColor(sf::Color::White);
	rectangle_EXPbox.setFillColor(sf::Color::Black);
	rectangle_EXPbox.setSize(sf::Vector2f(WINDOW_WIDTH + 500, 25));
	g_window->draw(rectangle_EXPbox);

	rectangle_EXP.setPosition(sf::Vector2f(100, WINDOW_HEIGHT + 550));
	rectangle_EXP.setFillColor(sf::Color::Blue);
	rectangle_EXP.setSize(sf::Vector2f((avatar.exp * (WINDOW_WIDTH + 500)) / (int)(100 * pow(2, (avatar.level - 1))), 25));
	g_window->draw(rectangle_EXP);

	//total exp level * level * 100 

	avatar.m_exp.setPosition(10, WINDOW_HEIGHT + 530);
	g_window->draw(avatar.m_exp);

}
	

void send_packet(void* packet)
{
	char* p = reinterpret_cast<char*>(packet);
	size_t sent;
	g_socket.send(p, p[0], sent);
}

void send_move_packet(unsigned char dir)
{
	cs_packet_move m_packet;
	m_packet.type = C2S_MOVE;
	m_packet.size = sizeof(m_packet);
	m_packet.direction = dir;
	send_packet(&m_packet);
}

void send_attack_packet()
{
	cs_packet_attack m_packet;
	m_packet.type = C2S_ATTACK;
	m_packet.size = sizeof(m_packet);
	send_packet(&m_packet);
}


int main()
{
	wcout.imbue(locale("korean"));
	sf::Socket::Status status = g_socket.connect("127.0.0.1", SERVER_PORT);
	g_socket.setBlocking(false);

	if (status != sf::Socket::Done) {
		wcout << L"서버와 연결할 수 없습니다.\n";
		while (true);
	}

	init_map();
	client_initialize();

	cs_packet_login l_packet;
	l_packet.size = sizeof(l_packet);
	l_packet.type = C2S_LOGIN;
	int t_id = GetCurrentProcessId();

	char id[20];
	printf("id 입력 : ");
	scanf_s("%s", id, 10);
	strcpy_s(l_packet.name, id);
	avatar.set_name(id);
	send_packet(&l_packet);

	//sprintf_s(l_packet.name, "P%03d", t_id % 1000);
	//strcpy_s(avatar.name, l_packet.name);
	//avatar.set_name(l_packet.name);
	//send_packet(&l_packet);

	sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH , WINDOW_HEIGHT), "2D CLIENT");
	g_window = &window;

	sf::View view = g_window->getView();
	view.zoom(2.0f);
	view.move(SCREEN_WIDTH * TILE_WIDTH / 4, SCREEN_HEIGHT * TILE_WIDTH / 4);
	g_window->setView(view);

	while (window.isOpen())
	{
		sf::Event event;
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
				window.close();
			if (event.type == sf::Event::KeyPressed) {
				int p_type = -1;
				switch (event.key.code) {
				case sf::Keyboard::Left:
					send_move_packet(D_LEFT);
					break;
				case sf::Keyboard::Right:
					send_move_packet(D_RIGHT);
					break;
				case sf::Keyboard::Up:
					send_move_packet(D_UP);
					break;
				case sf::Keyboard::Down:
					send_move_packet(D_DOWN);
					break;
				case sf::Keyboard::Space:
					send_attack_packet();
					break;
				case sf::Keyboard::Escape:
					window.close();
					break;
				}
			}
		}

		window.clear();
		client_main();
		window.display();
	}
	client_finish();

	return 0;
}
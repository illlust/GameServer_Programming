#include <iostream>
#include <WS2tcpip.h>
#include <MSWSock.h>
#include <vector>
#include <thread>
#include <mutex>
#include <unordered_set>
#include <atomic>
#include <chrono>
#include <queue>

#include "protocol.h"
#pragma comment (lib, "WS2_32.lib")
#pragma comment (lib, "mswsock.lib")
#pragma comment (lib, "lua53.lib")

extern "C"
{
#include "lua.h"
#include "luaconf.h"
#include "lauxlib.h"
#include "lualib.h"
}

using namespace std;
using namespace chrono;


enum ENUMOP { OP_RECV , OP_SEND, OP_ACCEPT, OP_RANDOM_MOVE , OP_PLAYER_MOVE};

enum C_STATUS {ST_FREE, ST_ALLOCATED, ST_ACTIVE, ST_SLEEP};


struct event_type
{
	int obj_id;
	ENUMOP event_id; //����, �̵� ...
	high_resolution_clock::time_point wakeup_time;
	int target_id;

	constexpr bool operator < (const event_type& left) const
	{
		return (wakeup_time > left.wakeup_time);
	}
};
priority_queue<event_type> timer_queue;
mutex timer_lock;

//Ȯ�� overlapped ����ü
struct EXOVER
{
	WSAOVERLAPPED	over;
	ENUMOP			op;						//send, recv, accpet �� �������� 
	char			io_buf[MAX_BUF_SIZE];	//������ ��ġ ����
	
	union {
		WSABUF			wsabuf;					//������ ���� �ٿ��� ���� ���� ��ü�� ����. �ѱ��� ���� �θ� Ȯ�屸��ü�� �����ϸ� ��ü�� ���� �ȴ�.
		SOCKET			c_socket;
		int				p_id; //������ player�� id
	};
};

//Ŭ���̾�Ʈ ���� ���� ����ü
struct CLIENT
{
	mutex	m_cLock;
	SOCKET	m_socket;			//lock���� ��ȣ
	int		m_id;				//lock���� ��ȣ
	EXOVER	m_recv_over;
	int		m_prev_size; 
	char	m_packet_buf[MAX_PACKET_SIZE];		//������ �� �޾Ƶα� ���� ����
	
	atomic<C_STATUS> m_status;

	//���� ������ 
	short x, y;
	char m_name[MAX_ID_LEN + 1];			//lock���� ��ȣ

	unsigned  m_move_time;

	//high_resolution_clock::time_point m_last_move_time;

	unordered_set<int> view_list; //������ ������� �� unordered ���°� �� �ӵ��� ������ 

	lua_State* L;
	mutex lua_l;
};


CLIENT g_clients[NPC_ID_START + NUM_NPC];		//Ŭ���̾�Ʈ ������ŭ �����ϴ� �����̳� �ʿ�

HANDLE g_iocp;					//iocp �ڵ�
SOCKET listenSocket;			//���� ��ü�� �ϳ�. �ѹ� �������� �ȹٲ�� �����ͷ��̽� �ƴ�. 

void add_timer(int obj_id, ENUMOP op_type, int duration)
{
	timer_lock.lock();
	event_type ev{ obj_id, op_type,high_resolution_clock::now() + milliseconds(duration), 0 };
	timer_queue.emplace(ev);
	timer_lock.unlock();
}

//lock���� ��ȣ�ް��ִ� �Լ�
void send_packet(int user_id, void *p)
{
	char* buf = reinterpret_cast<char*>(p);

	CLIENT &user = g_clients[user_id];

	//WSASend�� �ι�° ������ over�� recv���̶� ���� �ȵȴ�. ���� ������ �Ѵ�.
	EXOVER *exover = new EXOVER;
	exover->op = OP_SEND;
	ZeroMemory(&exover->over, sizeof(exover->over));
	exover->wsabuf.buf = exover->io_buf;
	exover->wsabuf.len = buf[0];
	memcpy(exover->io_buf, buf, buf[0]);

	WSASend(user.m_socket, &exover->wsabuf, 1, NULL, 0, &exover->over, NULL);
}
void send_login_ok_packet(int user_id)
{
	sc_packet_login_ok p;
	p.exp = 0;
	p.hp = 0;
	p.id = user_id;
	p.level = 0;
	p.size = sizeof(p);
	p.type = S2C_LOGIN_OK;
	p.x = g_clients[user_id].x;
	p.y = g_clients[user_id].y;

	send_packet(user_id, &p); //&p�� ���� ������ ����Ǿ ���󰡴ϱ� ���ɿ� ������. 
}

void send_login_fail_packet()
{

}

//���̵𿡰�, ���� �̵��ߴ��� �˷����
void send_move_packet(int user_id, int mover)
{
	sc_packet_move p;
	p.id = mover;
	p.size = sizeof(p);
	p.type = S2C_MOVE;
	p.x = g_clients[mover].x;
	p.y = g_clients[mover].y;
	p.move_time = g_clients[mover].m_move_time;

	send_packet(user_id, &p); //&p�� ���� ������ ����Ǿ ���󰡴ϱ� ���ɿ� ������. 
}

void send_chat_packet(int user_id, int chatter, char message[])
{
	sc_packet_chat p;
	p.id = chatter;
	p.size = sizeof(p);
	p.type = S2C_CHAT;
	strcpy_s(p.message, message);
	send_packet(user_id, &p);
}

void send_enter_packet(int user_id, int o_id)
{
	sc_packet_enter p;
	p.id = o_id;
	p.size = sizeof(p);
	p.type = S2C_ENTER;
	p.x = g_clients[o_id].x;
	p.y = g_clients[o_id].y;
	strcpy_s(p.name, g_clients[o_id].m_name);
	p.o_type = O_PLAYER;

	g_clients[user_id].m_cLock.lock();
	g_clients[user_id].view_list.insert(o_id);
	g_clients[user_id].m_cLock.unlock();

	send_packet(user_id, &p); //&p�� ���� ������ ����Ǿ ���󰡴ϱ� ���ɿ� ������. 
}


void send_near_packet(int client, int new_id)
{
	// new_id�� ���԰� �� new id�� ������ client���� ������ ��
	sc_packet_near packet;
	packet.id = new_id;
	packet.size = sizeof(packet);
	packet.type = S2C_NEAR_PLAYER;
	packet.x = g_clients[new_id].x;
	packet.y = g_clients[new_id].y;
	send_packet(client, &packet);
}
void send_leave_packet(int user_id, int o_id)
{
	sc_packet_move p;
	p.id = o_id;
	p.size = sizeof(p);
	p.type = S2C_LEAVE;

	g_clients[user_id].m_cLock.lock();
	g_clients[user_id].view_list.erase(o_id);
	g_clients[user_id].m_cLock.unlock();

	send_packet(user_id, &p); //&p�� ���� ������ ����Ǿ ���󰡴ϱ� ���ɿ� ������. 
}

//a �� b�� ���� �þ߿� �ֳ�? 
bool is_near(int a, int b)
{
	if (abs(g_clients[a].x - g_clients[b].x) > VIEW_RADIUS) return false;
	if (abs(g_clients[a].y - g_clients[b].y) > VIEW_RADIUS) return false;

	return true;
}

bool is_player(int id)
{
	return id < NPC_ID_START;
}

//SLEEP���� ACTIVE�� �ٲ۰Ϳ� ������ ��츸 add_timer. �ѹ��� ���� ������ 2�� �ӵ��� �̵�
void activate_npc(int id)
{
	g_clients[id].m_status = ST_ACTIVE;
	C_STATUS old_status = ST_SLEEP;
	if (true == atomic_compare_exchange_strong(&g_clients[id].m_status, &old_status, ST_ACTIVE)) 
		add_timer(id, OP_RANDOM_MOVE, 1000);
}

void do_move(int user_id, int direction)
{
	int x = g_clients[user_id].x;
	int y = g_clients[user_id].y;

	switch (direction)
	{
	case D_UP:
		if (y > 0)	y--;	break;
	case D_DOWN:
		if (y < WORLD_HEIGHT - 1)	y++;	break;
	case D_LEFT:
		if (x > 0)	x--;	break;
	case D_RIGHT:
		if (x < WORLD_WIDTH - 1)	x++;	break;
	default:
		cout << "unknown direction from client move packet \n";
		DebugBreak();
	}

	g_clients[user_id].x = x;
	g_clients[user_id].y = y;

	//�����صΰ� ����. �ٵ� �� ���Ŀ� ��߳����� �װ� �����ؾ��Ѵ�. ������ �������� ��������
	g_clients[user_id].m_cLock.lock();
	unordered_set<int> old_vl = g_clients[user_id].view_list;
	g_clients[user_id].m_cLock.unlock();
	
	unordered_set<int> new_vl;
	for (auto &cl : g_clients)
	{
		if (false == is_near(cl.m_id, user_id)) continue;		//��� Ŭ�� �� ����°� �ƴ϶� ��ó�� �ִ� �ָ� ����� 
		if (ST_SLEEP == cl.m_status) activate_npc(cl.m_id);		//���� �ִ� npc�� active�� �ٲٱ�
		if (ST_ACTIVE != cl.m_status) continue;					//�� �� ���´� ����������
		if (cl.m_id == user_id) continue;						//�� �ڽ��̸� ����������
		
		//ncp��� �÷��̾ �̵��ߴٰ� OP_PLAYER_MOVE�� �����ش�. 
		if (false == is_player(cl.m_id))
		{
			EXOVER* over = new EXOVER();
			over->p_id = user_id;
			over->op = OP_PLAYER_MOVE;
			PostQueuedCompletionStatus(g_iocp, 1, cl.m_id, &over->over);
		}
		new_vl.insert(cl.m_id);									//���ο� view list�� ����
	}

	//�ؿ��� view list�� �˷��ִϱ� �����״� �� �˷���. �׷��� ���� ���� ������ �̵��� �˷���� ��.
	send_move_packet(user_id, user_id);

	//�þ߿� ���� ���� �÷��̾� 
	for (auto newPlayer : new_vl)
	{
		//old_vl�� �����µ� new_vl�� �ִٸ� ���� ���� ��.
		if (0 == old_vl.count(newPlayer))
		{
			send_enter_packet(user_id, newPlayer);

			if (false == is_player(newPlayer))
				continue;

			g_clients[newPlayer].m_cLock.lock();
			if (0 == g_clients[newPlayer].view_list.count(user_id)) //��Ƽ������ ���α׷��̴ϱ�, �ٸ� �����忡�� �̸� �þ� ó���� ���� �� �ִ�.
			{
				g_clients[newPlayer].m_cLock.unlock();
				send_enter_packet(newPlayer, user_id);
			}
			else
			{
				g_clients[newPlayer].m_cLock.unlock();
				send_move_packet(newPlayer, user_id);
			}
		}
		//���� ���� �÷��̾ �ƴ϶� �������� ������ �ֶ�� �̵��� �˷���
		else
		{
			if (false == is_player(newPlayer)) continue;

			//������ �̵��ϸ鼭 �þ߿��� ���� ���� �� �ִ�. �׷��ϱ� ���� viewlist�� Ȯ���ϰ� ������ �Ѵ�.
			g_clients[newPlayer].m_cLock.lock();
			if (0 != g_clients[newPlayer].view_list.count(user_id))
			{
				g_clients[newPlayer].m_cLock.unlock();
				send_move_packet(newPlayer, user_id);
			}
			else
			{
				g_clients[newPlayer].m_cLock.unlock();
				send_enter_packet(newPlayer, user_id);
			}
		}
	}


	//�þ߿��� ��� �÷��̾�
	for (auto oldPlayer : old_vl)
	{

		if (0 == new_vl.count(oldPlayer))
		{
			send_leave_packet(user_id, oldPlayer);

			if (false == is_player(oldPlayer)) continue;
	
			g_clients[oldPlayer].m_cLock.lock();
			if (0 != g_clients[oldPlayer].view_list.count(user_id))
			{
				g_clients[oldPlayer].m_cLock.unlock();
				send_leave_packet(oldPlayer, user_id);
			}
			else
				g_clients[oldPlayer].m_cLock.unlock();
		}
	}
}

void random_move_npc(int npc_id)
{
	int x = g_clients[npc_id].x;
	int y = g_clients[npc_id].y;

	switch (rand() % 4)
	{
	case 0:
		if (x < WORLD_WIDTH - 1) x++; break;
	case 1:
		if (x > 0) x--; break;
	case 2:
		if (y < WORLD_HEIGHT - 1) y++; break;
	case 3:
		if (y > 0 ) y--; break;
	}

	g_clients[npc_id].x = x;
	g_clients[npc_id].y = y;

	for (int i = 0; i < NPC_ID_START; ++i)
	{
		if (g_clients[i].m_status != ST_ACTIVE) continue;
		if (true == is_near(i, npc_id))
		{
			g_clients[i].m_cLock.lock();
			if (0 != g_clients[i].view_list.count(npc_id))
			{
				g_clients[i].m_cLock.unlock();
				send_move_packet(i, npc_id);
			}
			else
			{
				g_clients[i].m_cLock.unlock();
				send_enter_packet(i, npc_id);
			}
		}
		else
		{
			g_clients[i].m_cLock.lock();
			if (0 != g_clients[i].view_list.count(npc_id))
			{
				g_clients[i].m_cLock.unlock();
				send_leave_packet(i, npc_id);
			}
			else
				g_clients[i].m_cLock.unlock();
		}
	}

}



void enter_game(int user_id, char name[])
{
	g_clients[user_id].m_cLock.lock();

	strcpy_s(g_clients[user_id].m_name, name);
	g_clients[user_id].m_name[MAX_ID_LEN] = NULL;
	send_login_ok_packet(user_id);
	g_clients[user_id].m_status = ST_ACTIVE;

	g_clients[user_id].m_cLock.unlock();

	for (auto& cl : g_clients)
	{
		int i = cl.m_id;
		if (user_id == i) continue;
		//�þ߸� ������� ó������ ���ƶ� (���� �� �þ�ó��)
		if (true == is_near(user_id, i))
		{
			//npc�� �ڰ��ִٸ� �����
			if (ST_SLEEP == g_clients[i].m_status)
				activate_npc(i);
		
			//g_clients[i].m_cLock.lock(); //i�� user_id�� �������� ��� 2�߶����� ���� ���� �߻� (�����)
			//�ٸ� ������ status�� �ٲ� �� ������, �ǽ����� ����������� ���⵵�� �����ؾ� �ϱ� ������ �ϴ� ���α�� �Ѵ�. 
			//�׷��� �ϴ� �����Ϸ� ������ �޸� ������ �ذ��ؾ� �ϱ� ������ status ������ atomic���� �ٲ۴�. 
			if (ST_ACTIVE == g_clients[i].m_status)
			{
				send_enter_packet(user_id, i);
				if(true == is_player(i)) //�÷��̾��� ��츸 send. �ȱ׷��� ������ ��� ������
					send_enter_packet(i, user_id); //�ϰ� ���� ���� ���� �ʸ� ����
			}
			//g_clients[i].m_cLock.unlock();
		}
	}
}

void process_packet(int user_id, char* buf)
{
	switch (buf[1]) //[0]�� size
	{
	case C2S_LOGIN:
	{	
		cs_packet_login *packet = reinterpret_cast<cs_packet_login*>(buf);
		enter_game(user_id, packet->name);
		break;
	}
	case C2S_MOVE:
	{	cs_packet_move *packet = reinterpret_cast<cs_packet_move*>(buf);
		g_clients[user_id].m_move_time = packet->move_time;
		do_move(user_id, packet->direction);
		break;
	}
	default:
		cout << "unknown packet type error \n";
		DebugBreak(); 
		exit(-1);
	}
}

void initialize_clients()
{
	for (int i = 0; i < MAX_USER; ++i)
	{
		//�̰� ��Ƽ������� ���� ����, �̱۾������ ���ư��� �Լ����� lock�� �Ŵ� �ǹ̰� ����. 
		g_clients[i].m_status = ST_FREE;
		g_clients[i].m_id = i;
	}
}


void disconnect(int user_id)
{
	send_leave_packet(user_id, user_id); //���� ������ ������
	
	g_clients[user_id].m_cLock.lock();
	g_clients[user_id].m_status = ST_ALLOCATED;	//ó�� �Ǳ� ���� FREE�ϸ� ���� ������ ��ó���� �ȵƴµ� �� ������ ���� �� ����

	closesocket(g_clients[user_id].m_socket);

	for (int i = 0; i < NPC_ID_START; ++i)
	{
		CLIENT& cl = g_clients[i];
		if (cl.m_id == user_id) continue;
		//cl.m_cLock.lock();
		if (ST_ACTIVE == cl.m_status)
			send_leave_packet(cl.m_id, user_id);
		//cl.m_cLock.unlock();
	}
	g_clients[user_id].m_status = ST_FREE;	//�� ó�������� FREE
	g_clients[user_id].m_cLock.unlock();
}

//��Ŷ ������ - ��������, ����Ʈ��ŭ�� �����Ͱ� �Դ�
void packet_construct(int user_id, int io_byte)
{
	CLIENT &curr_user = g_clients[user_id];
	EXOVER &recv_over = curr_user.m_recv_over;

	int rest_byte = io_byte;		//�̸�ŭ ������ ó������� �Ѵ�
	char *p = recv_over.io_buf;		//ó���ؾ��� �������� �����Ͱ� �ʿ��ϴ�
	int packet_size = 0;			//�̰� 0�̶�� ���� ������ ó���ϴ� ��Ŷ�� ���ٴ� �� 
	
	if (curr_user.m_prev_size != 0)	
		packet_size = curr_user.m_packet_buf[0]; //�������� ��ٱ�� ��Ŷ ������
	
	while (rest_byte > 0)	//ó���ؾ��� �����Ͱ� ���������� ó���ؾ��Ѵ�.
	{
		if (0 == packet_size)	packet_size = p[0];

		//������ �����ͷ� ��Ŷ�� ���� �� �ֳ� ���� Ȯ��
		if (packet_size <= rest_byte + curr_user.m_prev_size)
		{
			memcpy(curr_user.m_packet_buf + curr_user.m_prev_size, p, packet_size - curr_user.m_prev_size);		//���� ó���� ������ ũ�⸸ŭ ��Ŷ ������� ���ֱ�
			
			p += packet_size - curr_user.m_prev_size;
			rest_byte -= packet_size - curr_user.m_prev_size;
			packet_size = 0;														//�� ��Ŷ�� �̹� ó���� �߰� ���� ��Ŷ ������� ��.

			process_packet(user_id, curr_user.m_packet_buf);
			
			curr_user.m_prev_size = 0;

		}
		else	//��Ŷ �ϳ��� ���� �� ���ٸ� ���ۿ� �����صΰ� �����Ϳ� ������ ����
		{
			memcpy(curr_user.m_packet_buf + curr_user.m_prev_size, p, rest_byte); //���� ������ ���� �޴µ�, �������� ���� �����Ͱ� �������� ��찡 ������, �� �ڿ� �޾ƾ��Ѵ�.
			curr_user.m_prev_size += rest_byte;
			rest_byte = 0;
			p += rest_byte;
		}
	}
}

void worker_Thread()
{
	while (true) {

		DWORD io_byte;
		ULONG_PTR key;
		WSAOVERLAPPED* over;
		GetQueuedCompletionStatus(g_iocp, &io_byte, &key, &over, INFINITE);

		EXOVER *exover = reinterpret_cast<EXOVER*>(over);
		int user_id = static_cast<int>(key);

		CLIENT& cl = g_clients[user_id]; //Ÿ���� ���̱� ����

		switch (exover->op)
		{
		case OP_RECV:			//���� ��Ŷ ó�� -> overlapped����ü �ʱ�ȭ -> recv
		{
			if (0 == io_byte)
			{
				disconnect(user_id);
				if (OP_SEND == exover->op)
					delete exover;
			}
			else
			{
				packet_construct(user_id, io_byte);
				ZeroMemory(&cl.m_recv_over.over, 0, sizeof(cl.m_recv_over.over));
				DWORD flags = 0;
				WSARecv(cl.m_socket, &cl.m_recv_over.wsabuf, 1, NULL, &flags, &cl.m_recv_over.over, NULL);;
			}
			break;
		}
		case OP_SEND:			//����ü delete
			if (0 == io_byte)
				disconnect(user_id);

			delete exover;
			break;

		case OP_ACCEPT:			//CreateIoCompletionPort���� Ŭ����� iocp�� ��� -> �ʱ�ȭ -> recv -> accept �ٽ�(��������)
		{
			int user_id = -1;
			for (int i = 0; i < MAX_USER; ++i)
			{
				lock_guard<mutex> gl{ g_clients[i].m_cLock }; //�̷��� �ϸ� unlock�� �ʿ� ����. �� ��Ͽ��� ���������� unlock�� �ڵ����� ���ش�.
				if (ST_FREE == g_clients[i].m_status )
				{
					g_clients[i].m_status = ST_ALLOCATED;
					user_id = i;
					break;
				}
			}

			//main���� ������ worker������� �Űܿ��� ���� listen������ ����������, client������ ����� �����Դ�.
			SOCKET clientSocket = exover->c_socket;
			
			if (-1 == user_id)
				closesocket(clientSocket); // send_login_fail_packet();
			else
			{
				CreateIoCompletionPort(reinterpret_cast<HANDLE>(clientSocket), g_iocp, user_id, 0);

				//g_clients[user_id].m_id = user_id; �־����� �ϴ°� �ƴϰ� �ʱ�ȭ �Ҷ� �ѹ� ����� �� ó���� �ѹ�.
				g_clients[user_id].m_prev_size = 0; //������ �޾Ƶ� ������ ������ 0
				g_clients[user_id].m_socket = clientSocket;

				ZeroMemory(&g_clients[user_id].m_recv_over.over, 0, sizeof(g_clients[user_id].m_recv_over.over));
				g_clients[user_id].m_recv_over.op = OP_RECV;
				g_clients[user_id].m_recv_over.wsabuf.buf = g_clients[user_id].m_recv_over.io_buf;
				g_clients[user_id].m_recv_over.wsabuf.len = MAX_BUF_SIZE;

				g_clients[user_id].x = rand() % WORLD_WIDTH;
				g_clients[user_id].y = rand() % WORLD_HEIGHT;

				g_clients[user_id].view_list.clear();

				DWORD flags = 0;
				WSARecv(clientSocket, &g_clients[user_id].m_recv_over.wsabuf, 1, NULL, &flags, &g_clients[user_id].m_recv_over.over, NULL);
			}

			//���� �ʱ�ȭ �� �ٽ� accept
			clientSocket = WSASocketW(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
			exover->c_socket = clientSocket; //���� �޴� ������ �־��ش�. �ȱ׷��� Ŭ����� ���� ������ �����Ѵ�.
			ZeroMemory(&exover->over, sizeof(exover->over)); //accept_over�� exover��� �̸����� �޾����� exover�� ����
			AcceptEx(listenSocket, clientSocket, exover->io_buf, NULL, sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16, NULL, &exover->over);

		}
			break;

		case OP_RANDOM_MOVE:
		{
			random_move_npc(user_id);
			
			bool keep_alive = false;

			//active�� �÷��̾ �ֺ��� ������ ��� �����α�
			for(int i=0; i<NPC_ID_START; ++i)
				if(true == is_near(user_id, i))
					if (ST_ACTIVE == g_clients[i].m_status)
					{
						keep_alive = true;
						break;
					}
			if (true == keep_alive) add_timer(user_id, OP_RANDOM_MOVE, 1000);
			else g_clients[user_id].m_status = ST_SLEEP; //������ ���� �ƹ��� ������ SLEEP���� ����α�

			delete exover;
		}
			break;

		case OP_PLAYER_MOVE:
		{
			g_clients[user_id].lua_l.lock();
			lua_State* L = g_clients[user_id].L;
			lua_getglobal(L, "event_player_move");
			lua_pushnumber(L, exover->p_id);//���� ���������� �ޱ� ���� exover�� union�� �̿��Ѵ�.
			int err = lua_pcall(L, 1, 0, 0);
			g_clients[user_id].lua_l.unlock();
			delete exover;
		}
			break;
		default:
			cout << "unknown operation in worker_thread\n";
			while (true);
		}
	}

}

int API_SendMessage(lua_State* L)
{
	int my_id = (int)lua_tointeger(L, -3);
	int user_id = (int)lua_tointeger(L, -2);
	char* message = (char*)lua_tostring(L, -1);

	send_chat_packet(user_id, my_id, message);
	lua_pop(L, 3);
	return 0; //���� �� ���� 0��
}

int API_get_x(lua_State* L)
{
	int obj_id = (int)lua_tointeger(L, -1);
	lua_pop(L, 2);
	int x = g_clients[obj_id].x;
	lua_pushnumber(L, x);
	return 1;
}

int API_get_y(lua_State* L)
{
	int obj_id = (int)lua_tointeger(L, -1);
	lua_pop(L, 2);
	int y = g_clients[obj_id].y;
	lua_pushnumber(L, y);
	return 1;
}

void init_npc()
{
	for (int i = NPC_ID_START; i < NUM_NPC; ++i)
	{
		g_clients[i].m_socket = 0;
		g_clients[i].m_id = i;
		sprintf_s(g_clients[i].m_name, "NPC%d", i);
		g_clients[i].m_status = ST_SLEEP;
		g_clients[i].x = rand() % WORLD_WIDTH;
		g_clients[i].y = rand() % WORLD_HEIGHT;
		//g_clients[i].m_last_move_time = high_resolution_clock::now();
		//add_timer(i, OP_RANDOM_MOVE, 1000); �׳� �����̴°� �ƴ϶� �÷��̾ ���������� �þ߿� ������ ������ ��

		lua_State *L = g_clients[i].L = luaL_newstate();
		luaL_openlibs(L);
		luaL_loadfile(L, "NPC.LUA");
		lua_pcall(L, 0, 0, 0);
		lua_getglobal(L, "set_uid");
		lua_pushnumber(L, i);
		lua_pcall(L, 1, 0, 0);
		lua_pop(L, 1);

		//API ���
		lua_register(L, "API_SendMessage", API_SendMessage);
		lua_register(L, "API_get_x", API_get_x);
		lua_register(L, "API_get_y", API_get_y);
	}
}

//void do_ai()
//{
//	while (true)
//	{
//		auto ai_start_time = high_resolution_clock::now();
//		for (int i = NPC_ID_START; i < NPC_ID_START + NUM_NPC; ++i)
//		{
//			if (high_resolution_clock::now() - g_clients[i].m_last_move_time > 1s)
//			{
//				random_move_npc(i);
//				g_clients[i].m_last_move_time = high_resolution_clock::now();
//			}
//		}
//		auto ai_time = high_resolution_clock::now() - ai_start_time;
//		cout << "AI exec Time = " << duration_cast<milliseconds>(ai_time).count() << "ms \n";
//	}
//}

void do_timer()
{
	while (true)
	{
		this_thread::sleep_for(1ms); //Sleep(1);
		while (true)
		{
			timer_lock.lock();
			if (timer_queue.empty() == true)
			{
				timer_lock.unlock();
				break;
			}
			if (timer_queue.top().wakeup_time > high_resolution_clock::now())
			{
				timer_lock.unlock();
				break;
			}
			event_type ev = timer_queue.top();
			timer_queue.pop();
			timer_lock.unlock();

			switch (ev.event_id)
			{
			case OP_RANDOM_MOVE:
				EXOVER* over = new EXOVER();
				over->op = ev.event_id;
				PostQueuedCompletionStatus(g_iocp, 1, ev.obj_id, &over->over);
				//random_move_npc(ev.obj_id);
				//add_timer(ev.obj_id, ev.event_id, 1000);
				break;
			}
		}
	}
}




void main()
{
	//��Ʈ��ũ �ʱ�ȭ
	WSADATA WSAData;
	WSAStartup(MAKEWORD(2, 2), &WSAData);

	cout << "npc initialization start \n";
	init_npc();
	cout << "npc initialization finished \n";


	//�� �ڿ� flag�� overlapped�� �� ��� ����� ����
	listenSocket = WSASocketW(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);

	//bind�� ����� server address
	SOCKADDR_IN serverAddr;
	memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(SERVER_PORT); //host to network �ؼ� �־�� �Ѵ�
	serverAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY); //��� Ŭ��κ��� ������ �޾ƾ� �Ѵ�
	::bind(listenSocket, reinterpret_cast<SOCKADDR *>(&serverAddr), sizeof(serverAddr)); //�׳� bind�� ���� c++11�� �ִ� Ű����� ����ȴ�. ���� �տ� ::�� ���δ�.

	listen(listenSocket, SOMAXCONN);

	//IOCP �ڵ� �Ҵ�, IOCP��ü �����
	g_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 0);

	initialize_clients();


	//listen���� ���
	CreateIoCompletionPort(reinterpret_cast<HANDLE>(listenSocket), g_iocp, 999, 0);

	//���� accept�� ������ �����ϴµ�, �� �Լ��� �̸� ������ ����� �ΰ� �� ������ Ŭ��� ��������ش�. ������ �� �ٸ�.
	//�񵿱�� acceptExȣ��. 
	SOCKET clientSocket = WSASocketW(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	EXOVER accept_over; 
	ZeroMemory(&accept_over, sizeof(accept_over.over));
	accept_over.op = OP_ACCEPT;
	accept_over.c_socket = clientSocket;
	AcceptEx(listenSocket, clientSocket, accept_over.io_buf , NULL, sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16, NULL, &accept_over.over);
	
	//������ �����
	vector <thread> worker_threads;
	for (int i = 0; i < 5; ++i)
		worker_threads.emplace_back(worker_Thread);

	thread timer_thread{ do_timer };

	//���� ���� �� ��� ������ ���� ��ٸ���
	for (auto &th : worker_threads) th.join();
	timer_thread.join();
}
#include <iostream>
#include <WS2tcpip.h>
#include <MSWSock.h>
#include <vector>
#include <thread>
#include <mutex>
#include <set>
#include "protocol.h"
#pragma comment (lib, "WS2_32.lib")
#pragma comment (lib, "mswsock.lib")

using namespace std;

enum ENUMOP { OP_RECV , OP_SEND, OP_ACCEPT };

enum C_STATUS {ST_FREE, ST_ALLOCATED, ST_ACTIVE};

//Ȯ�� overlapped ����ü
struct EXOVER
{
	WSAOVERLAPPED	over;
	ENUMOP			op;						//send, recv, accpet �� �������� 
	char			io_buf[MAX_BUF_SIZE];	//������ ��ġ ����
	
	union {
		WSABUF			wsabuf;					//������ ���� �ٿ��� ���� ���� ��ü�� ����. �ѱ��� ���� �θ� Ȯ�屸��ü�� �����ϸ� ��ü�� ���� �ȴ�.
		SOCKET			c_socket;
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
	C_STATUS m_status;

	//���� ������ 
	short x, y;
	char m_name[MAX_ID_LEN + 1];			//lock���� ��ȣ

	unsigned  m_move_time;

	set<int> viewlist;
};


CLIENT g_clients[MAX_USER];		//Ŭ���̾�Ʈ ������ŭ �����ϴ� �����̳� �ʿ�

HANDLE g_iocp;					//iocp �ڵ�
SOCKET listenSocket;			//���� ��ü�� �ϳ�. �ѹ� �������� �ȹٲ�� �����ͷ��̽� �ƴ�. 

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

	g_clients[mover].viewlist.insert(user_id);
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

	send_packet(user_id, &p); //&p�� ���� ������ ����Ǿ ���󰡴ϱ� ���ɿ� ������. 
}

bool is_near_object(int a, int b)
{
	return 5 >= abs(g_clients[a].x - g_clients[b].x) + abs(g_clients[a].y - g_clients[b].y);
}

bool is_near_id(int player, int other)
{
	//lock_guard<mutex> gl{ g_clients[player].m_cLock};
	return (0 != g_clients[player].viewlist.count(other));
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

	//lock_guard<mutex> lg{ g_clients[client].m_cLock};
	g_clients[client].viewlist.insert(new_id);
}
void send_leave_packet(int user_id, int o_id)
{
	sc_packet_move p;
	p.id = o_id;
	p.size = sizeof(p);
	p.type = S2C_LEAVE;

	send_packet(user_id, &p); //&p�� ���� ������ ����Ǿ ���󰡴ϱ� ���ɿ� ������. 
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


	//for (int i = 0; i < MAX_USER; ++i) {
	//	if ((ST_ACTIVE == g_clients[i].m_status) &&
	//		(true == is_near_object(user_id, i)) &&
	//		(i != user_id))
	//		new_vl.push_back(i);
	//}


	for (auto &cl : g_clients) {
		cl.m_cLock.lock();
		if (ST_ACTIVE == cl.m_status)
		{
			//��ó�� �ִ�
			if (true == is_near_object(cl.m_id, user_id))
			{
				send_move_packet(cl.m_id, user_id);
	
				//viewlist�� ������
				if (true == is_near_id(user_id, cl.m_id))
				{
					send_move_packet(cl.m_id, user_id);
					send_move_packet(user_id, cl.m_id);
				}
			}
			//��ó�� ���� 
			else
			{
				//viewlist�� ������
				if (is_near_id(user_id, cl.m_id))
				{
					send_leave_packet(cl.m_id, user_id);
					send_leave_packet(user_id, cl.m_id );
					g_clients[cl.m_id].viewlist.erase(user_id);
					g_clients[user_id].viewlist.erase(cl.m_id);
				}
			}
		}
		cl.m_cLock.unlock();
	}
}

void enter_game(int user_id, char name[])
{
	g_clients[user_id].m_cLock.lock();

	strcpy_s(g_clients[user_id].m_name, name);
	g_clients[user_id].m_name[MAX_ID_LEN] = NULL;
	send_login_ok_packet(user_id);

	for (int i = 0; i < MAX_USER; i++)
	{
		if (user_id == i) continue;
		g_clients[i].m_cLock.lock(); //i�� user_id�� �������� ��� 2�߶����� ���� ���� �߻� (�����)
		if (ST_ACTIVE == g_clients[i].m_status)
		{
			if (i != user_id)		//�����Դ� ������ �ʴ´�.
			{
				send_enter_packet(user_id, i);
				send_enter_packet(i, user_id); //�ϰ� ���� ���� ���� �ʸ� ����
			}
		}
		g_clients[i].m_cLock.unlock();
	}
	
	g_clients[user_id].m_status = ST_ACTIVE;
	g_clients[user_id].m_cLock.unlock();
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
		g_clients[i].viewlist.clear();
	}
}


void disconnect(int user_id)
{
	g_clients[user_id].m_cLock.lock();
	g_clients[user_id].m_status = ST_ALLOCATED;	//ó�� �Ǳ� ���� FREE�ϸ� ���� ������ ��ó���� �ȵƴµ� �� ������ ���� �� ����

	send_leave_packet(user_id, user_id); //���� ������ ������
	closesocket(g_clients[user_id].m_socket);

	for (auto &cl : g_clients)
	{
		if (cl.m_id == user_id) continue;
		cl.m_cLock.lock();
		if (ST_ACTIVE == cl.m_status)
			send_leave_packet(cl.m_id, user_id);
		cl.m_cLock.unlock();
	}
	g_clients[user_id].m_status = ST_FREE;	//�� ó�������� FREE
	g_clients[user_id].viewlist.clear();
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

				for (auto &cl : g_clients) {
					cl.m_cLock.lock();
					if (ST_ACTIVE == cl.m_status)
					{
						//��ó�� �ִ�
						if (true == is_near_object(cl.m_id, user_id))
						{
							send_move_packet(cl.m_id, user_id);

							//viewlist�� ������
							if (false == is_near_id(user_id, cl.m_id))
							{
								send_near_packet(user_id, cl.m_id);
								send_near_packet(cl.m_id, user_id);
							}
						}
						//��ó�� ���� 
						else
						{
							//viewlist�� ������
							if (is_near_id(user_id, cl.m_id))
							{
								send_leave_packet(cl.m_id, user_id);
								send_leave_packet(user_id, cl.m_id);
								g_clients[cl.m_id].viewlist.erase(user_id);
								g_clients[user_id].viewlist.erase(cl.m_id);
							}
						}
					}
					cl.m_cLock.unlock();
				}

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
		}
	}

}

void main()
{
	//��Ʈ��ũ �ʱ�ȭ
	WSADATA WSAData;
	WSAStartup(MAKEWORD(2, 2), &WSAData);

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

	//���� ���� �� ��� ������ ���� ��ٸ���
	for (auto &th : worker_threads) th.join();
}
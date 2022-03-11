#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "stdfx.h"
#include "protocol.h"
using namespace std;
#pragma comment (lib, "WS2_32.LIB")
#pragma comment (lib, "MSWSock.LIB")


bool g_shutdown = false;

void error_display(int err_num)
{
	WCHAR* lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		nullptr, err_num,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		reinterpret_cast<LPTSTR>(&lpMsgBuf), 0, 0);
	wcout << "[Error] " << lpMsgBuf << endl;
	while (true);
	LocalFree(lpMsgBuf);
}

enum COMP_OP { OP_RECV, OP_SEND, OP_ACCEPT };

class EXP_OVER {
public:
	WSAOVERLAPPED	_wsa_over;
	COMP_OP			_comp_op;
	WSABUF			_wsa_buf;
	unsigned char	_net_buf[BUF_SIZE];
public:
	EXP_OVER(COMP_OP comp_op, char num_bytes, void* mess) : _comp_op(comp_op)
	{
		ZeroMemory(&_wsa_over, sizeof(_wsa_over));
		_wsa_buf.buf = reinterpret_cast<char*>(_net_buf);
		_wsa_buf.len = num_bytes;
		memcpy(_net_buf, mess, num_bytes);
	}

	EXP_OVER(COMP_OP comp_op) : _comp_op(comp_op) {}

	EXP_OVER()
	{
		_comp_op = OP_RECV;
	}

	~EXP_OVER()
	{
	}
};

class CLIENT {
public:
	char name[MAX_NAME_SIZE];
	int	   _id;
	short  x, y;

	bool in_use;

	EXP_OVER _recv_over;
	SOCKET _socket;
	int		_prev_size;
public:
	CLIENT()
	{
		in_use = false;
		x = 0;
		y = 0;
		_prev_size = 0;
	}

	~CLIENT()
	{
		closesocket(_socket);
	}

	void do_recv()
	{
		DWORD recv_flag = 0;
		ZeroMemory(&_recv_over._wsa_over, sizeof(_recv_over._wsa_over));
		_recv_over._wsa_buf.buf = reinterpret_cast<char*>(_recv_over._net_buf + _prev_size);
		_recv_over._wsa_buf.len = sizeof(_recv_over._net_buf) - _prev_size;
		int ret = WSARecv(_socket, &_recv_over._wsa_buf, 1, 0, &recv_flag, &_recv_over._wsa_over, NULL);
		if (SOCKET_ERROR == ret) {
			int error_num = WSAGetLastError();
			if (ERROR_IO_PENDING != error_num)
				error_display(error_num);
		}
	}

	void do_send(int num_bytes, void* mess)
	{
		EXP_OVER* ex_over = new EXP_OVER(OP_SEND, num_bytes, mess);
		WSASend(_socket, &ex_over->_wsa_buf, 1, 0, 0, &ex_over->_wsa_over, NULL);
	}
};

array <CLIENT, MAX_USER> clients;

int get_new_id()
{
	static int g_id = 0;

	for (int i = 0; i < MAX_USER; ++i)
		if (false == clients[i].in_use) {
			clients[i].in_use = true;
			return i;
		}
	cout << "Maximum Number of Clients Overflow!!\n";
	return -1;
}

void send_login_ok_packet(int c_id)
{
	sc_packet_login_ok packet;
	packet.id = c_id;
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_LOGIN_OK;
	packet.x = clients[c_id].x;
	packet.y = clients[c_id].y;

	clients[c_id].do_send(sizeof(packet), &packet);
}

void send_move_packet(int c_id, int mover)
{
	sc_packet_move_object packet;
	packet.id = mover;
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_MOVE_OBJECT;
	packet.x = clients[mover].x;
	packet.y = clients[mover].y;

	clients[c_id].do_send(sizeof(packet), &packet);
}

void send_remove_object(int c_id, int victim)
{
	sc_packet_remove_object packet;
	packet.id = victim;
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_REMOVE_OBJECT;

	clients[c_id].do_send(sizeof(packet), &packet);
}

void Disconnect(int c_id)
{
	clients[c_id].in_use = false;
	for (auto& cl : clients) {
		if (false == cl.in_use) continue;
		send_remove_object(cl._id, c_id);
	}
	closesocket(clients[c_id]._socket);
}

void process_packet(int client_id, unsigned char* p)
{
	unsigned char packet_type = p[1];
	CLIENT& cl = clients[client_id];

	switch (packet_type)
	{
	case CS_PACKET_LOGIN:
	{
		cs_packet_login* packet = reinterpret_cast<cs_packet_login*>(p);
		strcpy_s(cl.name, packet->name);

		send_login_ok_packet(client_id);

		for (auto& other : clients) {
			if (other._id == client_id) continue;
			if (false == other.in_use) continue;

			sc_packet_put_object packet;
			packet.id = client_id;
			strcpy_s(packet.name, cl.name);
			packet.object_type = 0;
			packet.size = sizeof(packet);
			packet.type = SC_PACKET_PUT_OBJECT;
			packet.x = cl.x;
			packet.y = cl.y;

			other.do_send(sizeof(packet), &packet);
		}

		for (auto& other : clients) {
			if (other._id == client_id) continue;
			if (false == other.in_use) continue;

			sc_packet_put_object packet;
			packet.id = other._id;
			strcpy_s(packet.name, other.name);
			packet.object_type = 0;
			packet.size = sizeof(packet);
			packet.type = SC_PACKET_PUT_OBJECT;
			packet.x = other.x;
			packet.y = other.y;

			cl.do_send(sizeof(packet), &packet);
		}
	}
	break;
	case CS_PACKET_MOVE:
	{
		cs_packet_move* packet = reinterpret_cast<cs_packet_move*>(p);
		short& x = cl.x;
		short& y = cl.y;
		switch (packet->direction)
		{
		case 0: if (y > 0) y--; break;
		case 1: if (y < (WORLD_HEIGHT - 1)) y++; break;
		case 2: if (x > 0) x--; break;
		case 3: if (x < (WORLD_WIDTH - 1)) x++; break;
		default:
			cout << "Invalid move in client " << client_id << endl;
			exit(-1);
			break;
		}
		//cl.x = x;
		//cl.y = y;
		for (auto& cl : clients) {
			if (true == cl.in_use)
				send_move_packet(cl._id, client_id);
		}
	}
	break;
	default:
		break;
	}
}

void Update(float elapsed);

int main()
{
	wcout.imbue(locale("korean"));

	WSADATA WSAData;
	WSAStartup(MAKEWORD(2, 2), &WSAData);

	SOCKET s_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, 0);

	//bind
	SOCKADDR_IN server_addr;
	ZeroMemory(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(SERVER_PORT);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	bind(s_socket, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr));

	//listen
	listen(s_socket, SOMAXCONN);

	INT addr_size = sizeof(server_addr);
	SOCKET c_socket = WSAAccept(s_socket, reinterpret_cast<sockaddr*>(&server_addr), &addr_size, 0, 0);

	cout << "\n[Server] Client connect: IP Address = " << inet_ntoa(server_addr.sin_addr) << " , Port Number = " << ntohs(server_addr.sin_port) << endl;

	auto pre_t = chrono::system_clock::now();
	for (;;) {
		auto cur_t = chrono::system_clock::now();
		float elapsed = chrono::duration_cast<chrono::milliseconds>(cur_t - pre_t).count() / float(1000);
		Update(elapsed);

		pre_t = cur_t;
		if (chrono::system_clock::now() - pre_t < 32ms) {//30프레임
			this_thread::sleep_for(32ms - (chrono::system_clock::now() - cur_t));
		}
	}
	for (auto& cl : clients) {
		if (true == cl.in_use)
			Disconnect(cl._id);
	}
	closesocket(s_socket);
	WSACleanup();
}

void Update(float elapsed)
{
	cout << "초속 30회 sex" << endl;
}


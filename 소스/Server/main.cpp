#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "stdfx.h"
#include "protocol.h"
using namespace std;
#pragma comment (lib, "WS2_32.LIB")
#pragma comment (lib, "MSWSock.LIB")

bool g_shutdown = false;

void Update(float elapsed);
void error_display(const int err_num, const char* msg);
void CALLBACK recv_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED over, DWORD flags);

class CLIENT {
public:
	c_data			_data;
	SOCKET			_socket;
	char			_client_buf[BUF_SIZE];
	//WSABUF		_wsabuf;
	//WSAOVERLAPPED	_recv_over;
public:
	CLIENT()
	{
		_data._in_use = false;
	}

	~CLIENT()
	{
		closesocket(_socket);
	}

	//void do_recv()
	//{
	//	DWORD recv_flag = 0;
	//	ZeroMemory(&_recv_over, sizeof(_recv_over));
	//	int ret = WSARecv(_socket, &_wsabuf, 1, 0, &recv_flag, &_recv_over, recv_callback);
	//	if (SOCKET_ERROR == ret) {
	//		int error_num = WSAGetLastError();
	//		if (ERROR_IO_PENDING != error_num)
	//			error_display(WSAGetLastError(), "Recv");
	//	}
	//}

	//void do_send(int num_bytes, void* mess)
	//{
	//	EXP_OVER* ex_over = new EXP_OVER(OP_SEND, num_bytes, mess);
	//	WSASend(_socket, &ex_over->_wsa_buf, 1, 0, 0, &ex_over->_wsa_over, NULL);
	//}
};

//void CALLBACK recv_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED over, DWORD flags)
//{
//	if (0 == num_bytes) return;
//	cout << "Client sent: " << c_mess << endl;
//	c_wsabuf[0].len = num_bytes;
//	memset(&c_over, 0, sizeof(c_over));
//	WSASend(c_socket, c_wsabuf, 1, 0, 0, &c_over, send_callback);
//}

array <CLIENT, MAX_USER> clients;
vector<std::thread> threads;

int get_new_id()
{
	for (int i = 0; i < MAX_USER; ++i)
		if (false == clients[i]._data._in_use) {
			clients[i]._data._in_use = true;
			return i;
		}
	cout << "Maximum Number of Clients Overflow!!\n";
	return -1;
}

void send_login_ok_packet(int id)
{
	CLIENT& cl = clients[id];
	sc_packet_login_ok packet;
	packet.data._id = id;
	packet.data._in_use = true;
	packet.data._state = legs_state::IDLE;
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_LOGIN_OK;
	char buf[BUF_SIZE];
	memcpy(buf, reinterpret_cast<char*>(&packet), sizeof(packet));
	WSABUF wsabuf;
	wsabuf.buf = buf;
	wsabuf.len = sizeof(buf);
	DWORD sent_byte;
	int error_code = WSASend(cl._socket, &wsabuf, 1, &sent_byte, 0, nullptr, nullptr);
	if (error_code == SOCKET_ERROR) error_display(WSAGetLastError(), "Recv");
}

//void send_move_packet(int c_id, int mover)
//{
//	sc_packet_move_object packet;
//	packet.id = mover;
//	packet.size = sizeof(packet);
//	packet.type = SC_PACKET_MOVE_OBJECT;
//	packet.x = clients[mover].x;
//	packet.y = clients[mover].y;
//
//	clients[c_id].do_send(sizeof(packet), &packet);
//}
//
//void send_remove_object(int c_id, int victim)
//{
//	sc_packet_remove_object packet;
//	packet.id = victim;
//	packet.size = sizeof(packet);
//	packet.type = SC_PACKET_REMOVE_OBJECT;
//
//	clients[c_id].do_send(sizeof(packet), &packet);
//}

//void Disconnect(int c_id)
//{
//	clients[c_id].in_use = false;
//	for (auto& cl : clients) {
//		if (false == cl.in_use) continue;
//		send_remove_object(cl._id, c_id);
//	}
//	closesocket(clients[c_id]._socket);
//}

//void process_packet(int client_id, unsigned char* p)
//{
//	unsigned char packet_type = p[1];
//	CLIENT& cl = clients[client_id];
//
//	switch (packet_type)
//	{
//	case CS_PACKET_LOGIN:
//	{
//		cs_packet_login* packet = reinterpret_cast<cs_packet_login*>(p);
//		strcpy_s(cl.name, packet->name);
//
//		send_login_ok_packet(client_id);
//
//		for (auto& other : clients) {
//			if (other._id == client_id) continue;
//			if (false == other.in_use) continue;
//
//			sc_packet_put_object packet;
//			packet.id = client_id;
//			strcpy_s(packet.name, cl.name);
//			packet.object_type = 0;
//			packet.size = sizeof(packet);
//			packet.type = SC_PACKET_PUT_OBJECT;
//			packet.x = cl.x;
//			packet.y = cl.y;
//
//			other.do_send(sizeof(packet), &packet);
//		}
//
//		for (auto& other : clients) {
//			if (other._id == client_id) continue;
//			if (false == other.in_use) continue;
//
//			sc_packet_put_object packet;
//			packet.id = other._id;
//			strcpy_s(packet.name, other.name);
//			packet.object_type = 0;
//			packet.size = sizeof(packet);
//			packet.type = SC_PACKET_PUT_OBJECT;
//			packet.x = other.x;
//			packet.y = other.y;
//
//			cl.do_send(sizeof(packet), &packet);
//		}
//	}
//	break;
//	case CS_PACKET_MOVE:
//	{
//		cs_packet_move* packet = reinterpret_cast<cs_packet_move*>(p);
//		short& x = cl.x;
//		short& y = cl.y;
//		switch (packet->direction)
//		{
//		case 0: if (y > 0) y--; break;
//		case 1: if (y < (WORLD_HEIGHT - 1)) y++; break;
//		case 2: if (x > 0) x--; break;
//		case 3: if (x < (WORLD_WIDTH - 1)) x++; break;
//		default:
//			cout << "Invalid move in c_data " << client_id << endl;
//			exit(-1);
//			break;
//		}
//		//cl.x = x;
//		//cl.y = y;
//		for (auto& cl : clients) {
//			if (true == cl.in_use)
//				send_move_packet(cl._id, client_id);
//		}
//	}
//	break;
//	default:
//		break;
//	}
//}

void recv_process(int id)
{
	CLIENT& cl = clients[id];
	for (;;)
	{
		char buf[BUF_SIZE];
		WSABUF recv_buf;
		recv_buf.buf = buf;
		recv_buf.len = BUF_SIZE;
		DWORD recvd_byte;
		DWORD flag = 0;
		int error_code = WSARecv(cl._socket, &recv_buf, 1, &recvd_byte, &flag, nullptr, nullptr);
		if (error_code == SOCKET_ERROR) error_display(WSAGetLastError(), "Recv");
		switch (static_cast<int>(buf[1]))
		{
		case CS_PACKET_UPDATE_LEGS:
			cl._data._state = static_cast<legs_state>(buf[2]);
			cout << (int)cl._data._state << endl;
			break;
 		default:
			cout << "Server Received Unknown Packet" << endl;
			break;
		}
	}
}

int main()
{
	wcout.imbue(locale("korean"));
	cout << sizeof(legs_state) << endl;
	WSADATA WSAData;
	WSAStartup(MAKEWORD(2, 2), &WSAData);

	SOCKET s_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);

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
	for (int i = 0; i < MAX_USER; ++i)
	{
		int new_id = get_new_id();
		CLIENT& cl = clients[new_id];
		cl._socket = WSAAccept(s_socket, reinterpret_cast<sockaddr*>(&server_addr), &addr_size, 0, 0);
		cl._data._id = new_id;
		cl._data._state = legs_state::IDLE;
		send_login_ok_packet(new_id);
		cout << "\n[" << cl._data._id << " Client connect] IP: " << inet_ntoa(server_addr.sin_addr) << endl;
		threads.emplace_back(recv_process, new_id);
	}

	cout << "process start" << endl;

	auto pre_t = chrono::system_clock::now();
	for (;;) {
		auto cur_t = chrono::system_clock::now();
		float elapsed = chrono::duration_cast<chrono::milliseconds>(cur_t - pre_t).count() / float(1000);

		// process update packet
		for (auto& cl : clients)
		{
			sc_packet_update_client packet;
			packet.data = cl._data;
			packet.size = sizeof(packet);
			packet.type = SC_PACKET_UPDATE_CLIENT;
			char buf[BUF_SIZE];
			memcpy(buf, reinterpret_cast<char*>(&packet), sizeof(packet));
			WSABUF wsabuf;
			wsabuf.buf = buf;
			wsabuf.len = sizeof(buf);
			DWORD sent_byte;
			int error_code = WSASend(cl._socket, &wsabuf, 1, &sent_byte, 0, nullptr, nullptr);
			if (error_code == SOCKET_ERROR) error_display(WSAGetLastError(), "Send");

		}

		pre_t = cur_t;
		if (chrono::system_clock::now() - pre_t < 32ms) {
			this_thread::sleep_for(32ms - (chrono::system_clock::now() - cur_t));
		}
	}
	
	//for (auto& cl : clients) {
	//	if (true == cl.in_use)
	//		//Disconnect(cl._id)
	//}

	//closesocket(c_socket);
	closesocket(s_socket);
	WSACleanup();
}

void Update(float elapsed)
{
}

void error_display(const int err_num, const char* msg)
{
	WCHAR* lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		nullptr, err_num,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		reinterpret_cast<LPTSTR>(&lpMsgBuf), 0, nullptr);
	wcout << "[" << msg << " Error] " << lpMsgBuf << endl;
	while (true);
	LocalFree(lpMsgBuf);
}
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "stdafx.h"
#include "protocol.h"
#pragma comment (lib, "WS2_32.LIB")
#pragma comment (lib, "MSWSock.LIB")

//bool g_shutdown = false;

void errorDisplay(const int errNum, const char* msg);
//void CALLBACK recv_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED over, DWORD flags);

class Client {
public:
	PlayerData		m_data;
	SOCKET			m_socket;
	char			m_clientBuf[BUF_SIZE];
	std::mutex		m_lock;
	//WSABUF		_wsabuf;
	//WSAOVERLAPPED	_recv_over;
public:
	Client() : m_data{ 0, false, eAnimationType::IDLE }, m_socket{}, m_clientBuf{}
	{

	}

	~Client()
	{
		closesocket(m_socket);
	}

	//void do_recv()
	//{
	//	DWORD recv_flag = 0;
	//	ZeroMemory(&_recv_over, sizeof(_recv_over));
	//	int ret = WSARecv(m_socket, &_wsabuf, 1, 0, &recv_flag, &_recv_over, recv_callback);
	//	if (SOCKET_ERROR == ret) {
	//		int error_num = WSAGetLastError();
	//		if (ERROR_IO_PENDING != error_num)
	//			errorDisplay(WSAGetLastError(), "Recv");
	//	}
	//}

	//void do_send(int num_bytes, void* mess)
	//{
	//	EXP_OVER* ex_over = new EXP_OVER(OP_SEND, num_bytes, mess);
	//	WSASend(m_socket, &ex_over->_wsa_buf, 1, 0, 0, &ex_over->_wsa_over, NULL);
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

std::array <Client, MAX_USER> clients;
std::vector<std::thread> threads;

CHAR GetNewId()
{
	for (int i = 0; i < MAX_USER; ++i) {
		std::cout << "id 검색중: " << i << std::endl;
		if (false == clients[i].m_data.isActive)
		{
			std::cout << "찾았다: " << i << std::endl;
			return i;
		}
	}
	std::cout << "Maximum Number of Clients Overflow!!\n";
	return -1;
}

void SendLoginOkPacket(int id)
{
	Client& cl = clients[id];

	sc_packet_login_ok packet;
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_LOGIN_OK;
	packet.data.id = id;
	packet.data.isActive = true;
	packet.data.aniType = eAnimationType::IDLE;
	packet.data.upperAniType = eUpperAnimationType::NONE;

	char buf[sizeof(packet)];
	memcpy(buf, reinterpret_cast<char*>(&packet), sizeof(packet));
	WSABUF wsabuf;
	wsabuf.buf = buf;
	wsabuf.len = sizeof(buf);
	DWORD sentByte;
	std::cout << "login pacet: " << static_cast<int>(buf[0]) << " : " << static_cast<int>(buf[1]) << " : " << static_cast<int>(buf[2]) << std::endl;
	const int retVal = WSASend(cl.m_socket, &wsabuf, 1, &sentByte, 0, nullptr, nullptr);
	if (retVal == SOCKET_ERROR) errorDisplay(WSAGetLastError(), "Login Send");
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

void Disconnect(int id)
{
	Client& cl = clients[id];
	cl.m_lock.lock();
	cl.m_data.id = 0;
	cl.m_data.isActive = false;
	cl.m_data.aniType = eAnimationType::IDLE;
	cl.m_data.upperAniType = eUpperAnimationType::NONE;
	cl.m_data.pos = {};
	cl.m_data.velocity = {};
	cl.m_data.yaw = 0.0f;
	closesocket(cl.m_socket);
	cl.m_lock.unlock();

}

//void process_packet(int client_id, unsigned char* p)
//{
//	unsigned char packet_type = p[1];
//	client& cl = clients[client_id];
//
//	switch (packet_type)
//	{
//	case CS_PACKET_LOGIN:
//	{
//		cs_packet_login* packet = reinterpret_cast<cs_packet_login*>(p);
//		strcpy_s(cl.name, packet->name);
//
//		SendLoginOkPacket(client_id);
//
//		for (auto& other : clients) {
//			if (other.id == client_id) continue;
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
//			if (other.id == client_id) continue;
//			if (false == other.in_use) continue;
//
//			sc_packet_put_object packet;
//			packet.id = other.id;
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
//			cout << "Invalid move in playerData " << client_id << endl;
//			exit(-1);
//			break;
//		}
//		//cl.x = x;
//		//cl.y = y;
//		for (auto& cl : clients) {
//			if (true == cl.in_use)
//				send_move_packet(cl.id, client_id);
//		}
//	}
//	break;
//	default:
//		break;
//	}
//}

void ProcessRecvPacket(int id)
{
	Client& cl = clients[id];
	for (;;)
	{
		// cs_packet_update_legs
		// size, type, aniType, upperAniType, pos, velocity, yaw
		char buf[1 + 1 + 1 + 1 + 12 + 12 + 4]{};
		WSABUF wsabuf;
		wsabuf.buf = buf;
		wsabuf.len = sizeof(buf);
		DWORD recvd_byte;
		DWORD flag{ 0 };
		int retVal = WSARecv(cl.m_socket, &wsabuf, 1, &recvd_byte, &flag, nullptr, nullptr);
		if (retVal == SOCKET_ERROR)
		{
			if (WSAGetLastError() == WSAECONNRESET)
			{
				std::cout << "Disconnect " << static_cast<int>(cl.m_data.id) << std::endl;
				Disconnect(cl.m_data.id);
				break;
			}
			errorDisplay(WSAGetLastError(), "Recv");
		}
		if (recvd_byte == 0)
		{
			std::cout << "Disconnect " << static_cast<int>(cl.m_data.id) << std::endl;
			Disconnect(cl.m_data.id);
			break;
		}
		switch (static_cast<int>(buf[1]))
		{
		case CS_PACKET_UPDATE_LEGS:
			cl.m_data.aniType = static_cast<eAnimationType>(buf[2]);
			cl.m_data.upperAniType = static_cast<eUpperAnimationType>(buf[3]);
			memcpy(&cl.m_data.pos, &buf[4], sizeof(cl.m_data.pos));
			memcpy(&cl.m_data.velocity, &buf[16], sizeof(cl.m_data.velocity));
			memcpy(&cl.m_data.yaw, &buf[28], sizeof(cl.m_data.yaw));
			
			// 애니메이션 리시브 확인
			std::cout << id << "PLAYER : " << static_cast<int>(cl.m_data.aniType) << ", " << static_cast<int>(cl.m_data.upperAniType) << std::endl;

			break;
 		default:
	        std::cout << "Server Received Unknown Packet" << std::endl;
			break;
		}
	}
}

bool g_isAccept = false;
void AcceptThread(SOCKET socket)
{
	SOCKADDR_IN serverAddr;
	INT addrSize = sizeof(serverAddr);
	while (true)
	{
		SOCKET cSocket = WSAAccept(socket, reinterpret_cast<sockaddr*>(&serverAddr), &addrSize, nullptr, 0);
		if (cSocket == INVALID_SOCKET) errorDisplay(WSAGetLastError(), "accept");

		CHAR id{ GetNewId() };
		Client& cl = clients[id];

		cl.m_lock.lock();
		cl.m_data.id = id;
		cl.m_data.isActive = true;
		cl.m_data.aniType = eAnimationType::IDLE;
		cl.m_data.upperAniType = eUpperAnimationType::NONE;
		cl.m_socket = cSocket;
		SendLoginOkPacket(id);
		cl.m_lock.unlock();

		std::cout << "\n[" << static_cast<int>(cl.m_data.id) << " Client connect] IP: " << inet_ntoa(serverAddr.sin_addr) << std::endl;
		threads.emplace_back(ProcessRecvPacket, id);
		g_isAccept = true;
	}
}

int main()
{
	std::wcout.imbue(std::locale("korean"));

	WSADATA WSAData;
	if (WSAStartup(MAKEWORD(2, 2), &WSAData) != 0) return 1;

	const SOCKET socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);
	if (socket == INVALID_SOCKET) errorDisplay(WSAGetLastError(), "socket");

	//bind
	SOCKADDR_IN serverAddr;
	ZeroMemory(&serverAddr, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(SERVER_PORT);
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	int retVal = bind(socket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr));
	if(retVal == SOCKET_ERROR) errorDisplay(WSAGetLastError(), "bind");

	//listen
	retVal = listen(socket, SOMAXCONN);
	if (retVal == SOCKET_ERROR) errorDisplay(WSAGetLastError(), "listen");
	
	threads.emplace_back(AcceptThread, socket);

	std::cout << "process start" << std::endl;

	auto time_point = std::chrono::system_clock::now();
	for (;;)
	{
		using namespace std;
		auto point = chrono::system_clock::now();
		float elapsed = chrono::duration_cast<chrono::milliseconds>(point - time_point).count() / static_cast<float>(1000);
		if (g_isAccept)
		{
			// process update packet
			for (const auto& cl : clients)
			{
				if (!cl.m_data.isActive) continue;

				sc_packet_update_client packet;
				packet.size = sizeof(packet);
				packet.type = SC_PACKET_UPDATE_CLIENT;
				packet.data = cl.m_data;

				char buf[sizeof(packet)];
				memcpy(buf, reinterpret_cast<char*>(&packet), sizeof(packet));

				WSABUF wsabuf;
				wsabuf.buf = buf;
				wsabuf.len = sizeof(buf);

				DWORD sent_byte;
				//for (const auto& other : clients)
				//{
				for (int i = 0; i < MAX_USER; ++i)
				{
					if (!clients[i].m_data.isActive) continue;
					retVal = WSASend(clients[i].m_socket, &wsabuf, 1, &sent_byte, 0, nullptr, nullptr);
					if (retVal == SOCKET_ERROR)
					{
						if (WSAGetLastError() == WSAECONNRESET)
						{
							std::cout << "Disconnect " << static_cast<int>(cl.m_data.id)  << " " << clients[i].m_data.id << std::endl;
							//Disconnect(cl.m_data.id);
						}
						else
						{
							cout << "ERROR SEND" << i << ", " << static_cast<int>(clients[i].m_data.id) << ", " << clients[i].m_data.isActive << endl;
							cout << static_cast<int>(packet.data.id) << ", " << packet.data.isActive << endl;
							errorDisplay(WSAGetLastError(), "Send");
						}
					}

					// 플레이어의 상체 애니메이션은 한 번 보내고 나면 NONE 상태로 초기화
					clients[i].m_data.upperAniType = eUpperAnimationType::NONE;
				}
			}
		}
		time_point = point;
		if (chrono::system_clock::now() - time_point < 32ms)
		{
			this_thread::sleep_for(32ms - (chrono::system_clock::now() - point));
		}
	}
	
	for (auto& cl : clients)
	{
		if (true == cl.m_data.isActive)
			Disconnect(cl.m_data.id);
	}
	
	closesocket(socket);
	WSACleanup();
}

void errorDisplay(const int errNum, const char* msg)
{
	WCHAR* lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		nullptr, errNum,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		reinterpret_cast<LPTSTR>(&lpMsgBuf), 0, nullptr);
	std::wcout << errNum <<" [" << msg << " Error] " << lpMsgBuf << std::endl;
	while (true);
	LocalFree(lpMsgBuf);
}
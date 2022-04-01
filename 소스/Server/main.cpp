#include "main.h"

NetFramework       g_netFramework{};

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
	//std::thread lobbyThread = std::thread(&NetFramework::AcceptThread, ,socket);
	//g_netFramework.threads.emplace_back(g_netFramework.AcceptThread, socket);
	g_netFramework.threads.emplace_back(&NetFramework::AcceptThread, NetFramework::GetInstance(), socket);

	std::cout << "main process start" << std::endl;

	// 1초에 60회 동작하는 루프
	using frame = std::chrono::duration<int32_t, std::ratio<1, 60>>;
	using ms = std::chrono::duration<float, std::milli>;
	std::chrono::time_point<std::chrono::steady_clock> fpsTimer(std::chrono::steady_clock::now());
	frame FPS{};
	frame frameNumber{};
	while (true)
	{
		FPS = std::chrono::duration_cast<frame>(std::chrono::steady_clock::now() - fpsTimer);
		if (FPS.count() >= 1)
		{
			frameNumber = std::chrono::duration_cast<frame>(frameNumber + FPS);
			fpsTimer = std::chrono::steady_clock::now();
			//std::cout << "LastFrame: " << duration_cast<ms>(FPS).count() << "ms  |  FPS: " << FPS.count() * 60 << " |  FrameNumber: " << frameNumber.count() << std::endl;
			if (1 & frameNumber.count()) // even FrameNumber
			{
				// playerData Send
				if (g_netFramework.m_isAccept) g_netFramework.SendPlayerDataPacket();
			}
			else // odd FrameNumber
			{
				// playerData Send
			}
			if (frameNumber.count() >= 60) frameNumber = std::chrono::duration_cast<frame>(frameNumber - frameNumber);
		}
	
	}
	
	for (auto& cl : g_netFramework.clients)
	{
		if (true == cl.m_data.isActive)
			g_netFramework.Disconnect(cl.m_data.id);
	}
	
	closesocket(socket);
	WSACleanup();
}

//void CALLBACK recv_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED over, DWORD flags)
//{
//	if (0 == num_bytes) return;
//	cout << "Session sent: " << c_mess << endl;
//	c_wsabuf[0].len = num_bytes;
//	memset(&c_over, 0, sizeof(c_over));
//	WSASend(c_socket, c_wsabuf, 1, 0, 0, &c_over, send_callback);
//}

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
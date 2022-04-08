#include "main.h"

NetworkFramework	g_networkFramework{};
SOCKET				g_socket{};

int main()
{
	std::wcout.imbue(std::locale("korean"));
	
	if (g_networkFramework.OnInit(g_socket)) return 1;
	std::cout << "main process start" << std::endl;

	// test
	g_networkFramework.clients[0].data.pos.x = 0;
	g_networkFramework.clients[0].data.pos.z = 0;
	g_networkFramework.monsters[0].id = 0;
	g_networkFramework.monsters[0].type = 8;
	g_networkFramework.monsters[0].pos = {5, 0, 5};
	g_networkFramework.monsters[1].id = 1;
	g_networkFramework.monsters[1].type = 5;

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
				if (g_networkFramework.isAccept) g_networkFramework.SendPlayerDataPacket();
			}
			else // odd FrameNumber
			{
				// MonsterData Send
				if (g_networkFramework.isAccept) g_networkFramework.SendMonsterDataPacket();
			}
			// temp monster timer
			if (g_networkFramework.isAccept)
			{
				// speedFps는 1/60 초당가는 거리 만큼 좁아지는 타이머
				// 한명이라도 들어오면 실행, 0번 몬스터가 0번 클라이언트를 향해 무작정 다가간다.
				// 변수 초기화, 오버헤드가 큰 실수 제곱근 연산은 해당 플레이어가 움직일때(30fps로 Send할때)만 작동하게 바꿀예정
				constexpr float fpsSpeed = 1.0f;
				const float dis_x = g_networkFramework.clients[0].data.pos.x;
				const float dis_z = g_networkFramework.clients[0].data.pos.z;
				const float start_x = g_networkFramework.monsters[0].pos.x;
				const float start_z = g_networkFramework.monsters[0].pos.z;
				// a: 방향, (x, z): 목적지까지 도달하면 이번 프레임에 움직여야하는 좌표
				const float a = abs((dis_z - start_z) / (dis_x - start_x)); 
				const float x = fpsSpeed / (sqrt(pow(a, 2) + 1));
				const float z = a * x;
				if ((dis_x - start_x) < 0) g_networkFramework.monsters[0].pos.x -= x;
				else g_networkFramework.monsters[0].pos.x += x;
				if ((dis_z - start_z) < 0) g_networkFramework.monsters[0].pos.z -= z;
				else g_networkFramework.monsters[0].pos.z += z;

				//std::cout << "player: " << g_networkFramework.clients[0].data.pos.x << ", " << g_networkFramework.clients[0].data.pos.z << std::endl;
				//std::cout << "monster: " << g_networkFramework.monsters[0].pos.x << ", " << g_networkFramework.monsters[0].pos.z << std::endl;
				
			}

			if (frameNumber.count() >= 60)
			{
				frameNumber = std::chrono::duration_cast<frame>(frameNumber - frameNumber);
			}
		}
	}

	for (const auto& c : g_networkFramework.clients)
	{
		if (c.data.isActive)
			g_networkFramework.Disconnect(c.data.id);
	}
	
	closesocket(g_socket);
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
//}	LocalFree(lpMsgBuf);
//}

#include "main.h"

int main()
{
	std::wcout.imbue(std::locale("korean"));

	if (g_networkFramework.OnInit(g_socket)) return 1;
	std::cout << "main process start" << std::endl;

	// test
	Monster monster{};
	monster.SetId(0);
	monster.SetType(0);
	monster.SetAnimationType(eMobAnimationType::IDLE);
	monster.SetPosition(DirectX::XMFLOAT3{ 0.0f, 0.0f, 50.0f });
	g_networkFramework.monsters.push_back(std::move(monster));

	// 1초에 60회 동작하는 루프
	using frame = std::chrono::duration<int32_t, std::ratio<1, 60>>;
	using ms = std::chrono::duration<float, std::milli>;
	std::chrono::time_point<std::chrono::steady_clock> fpsTimer{ std::chrono::steady_clock::now() };

	frame fps{}, frameCount{};
	while (true)
	{
		// 이전 사이클에 얼마나 시간이 걸렸는지 계산
		fps = duration_cast<frame>(std::chrono::steady_clock::now() - fpsTimer);

		// 아무도 서버에 접속하지 않았으면 패스
		if (!g_networkFramework.isAccept) continue;

		// 아직 1/60초가 안지났으면 패스
		if (fps.count() < 1) continue;

		if (frameCount.count() & 1) // even FrameNumber
		{
			// playerData Send
			g_networkFramework.SendPlayerDataPacket();
		}
		else // odd FrameNumber
		{
			// MonsterData Send
			g_networkFramework.SendMonsterDataPacket();
		}

		// 서버에서 해야할 모든 계산 실행
		g_networkFramework.Update(duration_cast<ms>(fps).count() / 1000.0f);

		// 이번 프레임 계산이 끝난 시각 저장
		frameCount = duration_cast<frame>(frameCount + fps);
		if (frameCount.count() >= 60)
			frameCount = frame::zero();
		fpsTimer = std::chrono::steady_clock::now();
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

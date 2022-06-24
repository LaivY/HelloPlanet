#include "stdafx.h"
#include "framework.h"
using namespace DirectX;

NetworkFramework::NetworkFramework() : isAccept{ false }, isClearStage1{ false }, isInGame{ false }, readyCount{ 0 },
bulletHits{}, m_spawnCooldown{ g_spawnCooldown }, m_lastMobId{ 0 }, m_killScore{ 0 }
{
	// m_spawnCooldown 2.0f은 임의의 상수
}

int NetworkFramework::OnInit(SOCKET socket)
{
	std::wcout.imbue(std::locale("korean"));
	WSADATA WSAData;
	if (WSAStartup(MAKEWORD(2, 2), &WSAData) != 0) return 1;

	socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);
	if (socket == INVALID_SOCKET) errorDisplay(WSAGetLastError(), "Socket");

	// bind
	SOCKADDR_IN serverAddr{};
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(SERVER_PORT);
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	int retVal = bind(socket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr));
	if (retVal == SOCKET_ERROR) errorDisplay(WSAGetLastError(), "Bind");

	// listen
	retVal = listen(socket, SOMAXCONN);
	if (retVal == SOCKET_ERROR) errorDisplay(WSAGetLastError(), "Listen");

	threads.emplace_back(&NetworkFramework::AcceptThread, this, socket);
	return 0;
}

int NetworkFramework::OnInit_iocp()
{
	WSADATA WSAData;
	if (WSAStartup(MAKEWORD(2, 2), &WSAData) != 0) return 1;

	g_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);
	if (g_socket == INVALID_SOCKET) errorDisplay(WSAGetLastError(), "Socket");

	// bind
	SOCKADDR_IN server_addr;
	ZeroMemory(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(SERVER_PORT);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	int retVal = bind(g_socket, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr));
	if (retVal == SOCKET_ERROR) errorDisplay(WSAGetLastError(), "Bind");

	// listen
	retVal = listen(g_socket, SOMAXCONN);
	if (retVal == SOCKET_ERROR) errorDisplay(WSAGetLastError(), "Listen");

	g_h_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 0);
	CreateIoCompletionPort(reinterpret_cast<HANDLE>(g_socket), g_h_iocp, 0, 0);

	SOCKET c_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);
	char accept_buf[sizeof(SOCKADDR_IN) * 2 + 32 + 100];
	EXP_OVER accept_ex;
	*(reinterpret_cast<SOCKET*>(&accept_ex._net_buf)) = c_socket;
	ZeroMemory(&accept_ex._wsa_over, sizeof(accept_ex._wsa_over));
	accept_ex._comp_op = OP_ACCEPT;

	bool ret = AcceptEx(g_socket, c_socket, accept_buf, 0, sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, 0, &accept_ex._wsa_over);
	
	std::cout << "Accept Called " << std::endl;;

	for (int i = 0; i < MAX_USER; ++i)
		clients[i].data.id = i;

	for (int i = 0; i < 6; ++i)
		worker_threads.emplace_back(&NetworkFramework::WorkThreads, this);
	for (auto& th : worker_threads)
		th.detach();
	std::cout << "main process start" << std::endl;

	// 1초에 60회 동작하는 루프
	using frame = std::chrono::duration<int32_t, std::ratio<1, 60>>;
	using ms = std::chrono::duration<float, std::milli>;
	std::chrono::time_point<std::chrono::steady_clock> fpsTimer{ std::chrono::steady_clock::now() };

	frame fps{}, frameCount{};
	while (true)
	{
		// 아무도 서버에 접속하지 않았으면 패스
		if (!isAccept)
		{
			// 이 부분이 없다면 첫 프레임 때 deltaTime이 '클라에서 처음 접속한 시각 - 서버를 켠 시각' 이 된다.
			fpsTimer = std::chrono::steady_clock::now();
			continue;
		}

		// 이전 사이클에 얼마나 시간이 걸렸는지 계산
		fps = duration_cast<frame>(std::chrono::steady_clock::now() - fpsTimer);

		// 아직 1/60초가 안지났으면 패스
		if (fps.count() < 1) continue;

		if (frameCount.count() & 1) // even FrameNumber
		{
			// playerData Send
			SendPlayerDataPacket();
			SendBulletHitPacket();
		}
		else // odd FrameNumber
		{
			// MonsterData Send
			SendMonsterDataPacket();
		}

		// 서버에서 해야할 모든 계산 실행
		Update(duration_cast<ms>(fps).count() / 1000.0f);

		// 이번 프레임 계산이 끝난 시각 저장
		frameCount = duration_cast<frame>(frameCount + fps);
		if (frameCount.count() >= 60)
			frameCount = frame::zero();
		fpsTimer = std::chrono::steady_clock::now();
	}

	for (const auto& c : clients)
	{
		if (c.data.isActive)
			Disconnect(c.data.id);
	}
	for (auto& th : worker_threads)
		th.join();

	closesocket(g_socket);
	WSACleanup();
	return 0;
}

void NetworkFramework::AcceptThread(SOCKET socket)
{
	std::cout << "AcceptThread start" << std::endl;
	SOCKADDR_IN clientAddr;
	INT addrSize = sizeof(clientAddr);
	while (!isAccept)
	{
		//if (clients[0].data.isActive && clients[1].data.isActive && clients[2].data.isActive) continue;

		SOCKET cSocket = WSAAccept(socket, reinterpret_cast<sockaddr*>(&clientAddr), &addrSize, nullptr, 0);
		if (cSocket == INVALID_SOCKET) errorDisplay(WSAGetLastError(), "Accept()");

		CHAR id{ GetNewId() };
		Session& player = clients[id];

		player.lock.lock();
		player.data.id = id;
		player.data.isActive = true;
		player.data.aniType = eAnimationType::IDLE;
		player.data.upperAniType = eUpperAnimationType::NONE;
		player.socket = cSocket;
		player.isReady = false;
		constexpr char dummyName[10] = "unknown\0";
		strcpy_s(player.name, sizeof(dummyName), dummyName);
		player.weaponType = eWeaponType::AR;
		player.lock.unlock();

		char ipInfo[20]{};
		inet_ntop(AF_INET, &clientAddr.sin_addr, ipInfo, sizeof(ipInfo));
		std::cout << "[" << static_cast<int>(player.data.id) << " Session] connect IP: " << ipInfo << std::endl;
		threads.emplace_back(&NetworkFramework::ProcessRecvPacket, this, id);
		//isAccept = true;
	}
}

void NetworkFramework::WorkThreads()
{
	std::cout << "worker_threads is start" << std::endl;
	for (;;) {
		DWORD num_byte;
		LONG64 iocp_key;
		WSAOVERLAPPED* p_over;
		BOOL ret = GetQueuedCompletionStatus(g_h_iocp, &num_byte, (PULONG_PTR)&iocp_key, &p_over, INFINITE);
		//std::cout << iocp_key << std::endl;
		int client_id = static_cast<int>(iocp_key);
		EXP_OVER* exp_over = reinterpret_cast<EXP_OVER*>(p_over);
		if (FALSE == ret)
		{
			int err_no = WSAGetLastError();
			errorDisplay(err_no, "GQCS");
			Disconnect(client_id);
			if (exp_over->_comp_op == OP_SEND) delete exp_over;
			continue;
		}

		switch (exp_over->_comp_op) {
		case OP_RECV: {
			if (num_byte == 0) {
				Disconnect(client_id);
				continue;
			}
			Session& cl = clients[client_id];
			int remain_data = num_byte + cl.prev_size;
			char* packet_start = exp_over->_net_buf;
			int packet_size = packet_start[0];

			while (packet_size <= remain_data) {
				ProcessRecvPacket_iocp(client_id, packet_start);
				remain_data -= packet_size;
				packet_start += packet_size;
				if (remain_data > 0) packet_size = packet_start[0];
				else break;
			}

			if (0 < remain_data) {
				cl.prev_size = remain_data;
				memcpy(&exp_over->_net_buf, packet_start, remain_data);
			}
			cl.do_recv();
			break;
		}
		case OP_SEND: {
			if (num_byte != exp_over->_wsa_buf.len) {
				Disconnect(client_id);
			}
			delete exp_over;
			break;
		}
		case OP_ACCEPT: {
			std::cout << "Accept Completed." << std::endl;
			SOCKET c_socket = *(reinterpret_cast<SOCKET*>(exp_over->_net_buf));
			CHAR new_id{ GetNewId() };
			if (-1 == new_id) {
				std::cout << "Maxmum user overflow. Accept aborted." << std::endl;
			}
			else {
				Session& cl = clients[new_id];
				cl.lock.lock();
				cl.data.id = new_id;
				cl.data.isActive = true;
				cl.data.aniType = eAnimationType::IDLE;
				cl.data.upperAniType = eUpperAnimationType::NONE;
				cl.socket = c_socket;
				cl.isReady = false;
				constexpr char dummyName[10] = "unknown\0";
				strcpy_s(cl.name, sizeof(dummyName), dummyName);
				cl.weaponType = eWeaponType::AR;
				cl.prev_size = 0;
				cl.recv_over._comp_op = OP_RECV;
				cl.recv_over._wsa_buf.buf = reinterpret_cast<char*>(cl.recv_over._net_buf);
				cl.recv_over._wsa_buf.len = sizeof(cl.recv_over._net_buf);
				ZeroMemory(&cl.recv_over._wsa_over, sizeof(cl.recv_over._wsa_over));
				cl.lock.unlock();

				CreateIoCompletionPort(reinterpret_cast<HANDLE>(c_socket), g_h_iocp, new_id, 0);
				cl.do_recv();
			}

			ZeroMemory(&exp_over->_wsa_over, sizeof(exp_over->_wsa_over));
			c_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);
			*(reinterpret_cast<SOCKET*>(exp_over->_net_buf)) = c_socket;
			AcceptEx(g_socket, c_socket, exp_over->_net_buf + 8, 0, sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, 0, &exp_over->_wsa_over);
		}
		break;
		}
	}
}

void NetworkFramework::SendPacket2AllPlayer(const void* packet, int bufSize) const
{
	char buf[BUF_SIZE]{};
	memcpy(buf, packet, bufSize);
	WSABUF wsaBuf = {  static_cast<ULONG>(bufSize), buf};
	DWORD sentByte;

	for (const auto& cl : clients)
	{
		if (!cl.data.isActive) continue;
		const int retVal = WSASend(cl.socket, &wsaBuf, 1, &sentByte, 0, nullptr, nullptr);
		if (retVal == SOCKET_ERROR) 
		{
			errorDisplay(WSAGetLastError(), "SendPacket2AllPlayer");
			std::cout << "Error Num: " << static_cast<int>(buf[1]) << std::endl;
		}
	}
}

void NetworkFramework::SendLoginOkPacket(const Session& player) const
{
	sc_packet_login_confirm packet{};
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_LOGIN_CONFIRM;
	packet.data.id = player.data.id;
	packet.data.isActive = true;
	packet.data.aniType = eAnimationType::IDLE;
	packet.data.upperAniType = eUpperAnimationType::NONE;
	packet.data.pos = DirectX::XMFLOAT3{ 0.0f, 0.0f, 0.0f };
	strcpy_s(packet.name, sizeof(packet.name), player.name);
	packet.isReady = player.isReady;
	packet.weaponType = player.weaponType;

	char buf[sizeof(packet)];
	memcpy(buf, reinterpret_cast<char*>(&packet), sizeof(packet));
	WSABUF wsabuf{ sizeof(buf), buf };
	DWORD sentByte;

	// 모든 클라이언트에게 로그인한 클라이언트의 정보 전송
	for (const auto& other : clients)
	{
		if (!other.data.isActive) continue;
		const int retVal = WSASend(other.socket, &wsabuf, 1, &sentByte, 0, nullptr, nullptr);
		if (retVal == SOCKET_ERROR) errorDisplay(WSAGetLastError(), "Send(SC_PACKET_LOGIN_CONFIRM)");
	}

	// 로그인한 클라이언트에게 이미 접속해있는 클라이언트들의 정보 전송
	for (const auto& other : clients)
	{
		if (!other.data.isActive) continue;
		if (static_cast<int>(other.data.id) == player.data.id) continue;

		sc_packet_login_confirm subPacket{};
		subPacket.size = sizeof(subPacket);
		subPacket.type = SC_PACKET_LOGIN_CONFIRM;
		subPacket.data = other.data;
		strcpy_s(subPacket.name, sizeof(subPacket.name), other.name);
		subPacket.isReady = other.isReady;
		subPacket.weaponType = other.weaponType;

		memcpy(buf, reinterpret_cast<char*>(&subPacket), sizeof(subPacket));
		const int retVal = WSASend(player.socket, &wsabuf, 1, &sentByte, 0, nullptr, nullptr);
		if (retVal == SOCKET_ERROR) errorDisplay(WSAGetLastError(), "Recv(SC_PACKET_LOGIN_CONFIRM)");
	}

	std::cout << "[" << static_cast<int>(player.data.id) << " Session] Login Packet Received" << std::endl;
}

void NetworkFramework::SendSelectWeaponPacket(const Session& player) const
{
	sc_packet_select_weapon packet{};
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_SELECT_WEAPON;
	packet.id = player.data.id;
	packet.weaponType = player.weaponType;

	SendPacket2AllPlayer(reinterpret_cast<void*>(&packet), packet.size);

	std::cout << "[" << static_cast<int>(player.data.id) << " Session] Select Weapon Packet Received" << std::endl;
}

void NetworkFramework::SendReadyCheckPacket(const Session& player) const
{
	sc_packet_ready_check packet{};
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_READY_CHECK;
	packet.id = player.data.id;
	packet.isReady = player.isReady;

	SendPacket2AllPlayer(&packet, packet.size);
	
	std::cout << "[" << static_cast<int>(player.data.id) << " Session] Ready Packet Received" << std::endl;
}

void NetworkFramework::SendChangeScenePacket(const eSceneType sceneType) const
{
	// 모두에게 보내는 패킷전송함수
	sc_packet_change_scene packet{};
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_CHANGE_SCENE;
	packet.sceneType = sceneType;

	SendPacket2AllPlayer(&packet, packet.size);

	std::cout << "[All Session] Ready To Play" << std::endl;
}

void NetworkFramework::SendPlayerDataPacket()
{
	sc_packet_update_client packet{};
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_UPDATE_CLIENT;
	for (int i = 0; i < MAX_USER; ++i)
		packet.data[i] = clients[i].data;

	char buf[sizeof(packet)];
	memcpy(buf, reinterpret_cast<char*>(&packet), sizeof(packet));
	WSABUF wsabuf{ sizeof(buf), buf };
	DWORD sent_byte;

	for (const auto& cl : clients)
	{
		if (!cl.data.isActive) continue;
		const int retVal = WSASend(cl.socket, &wsabuf, 1, &sent_byte, 0, nullptr, nullptr);
		if (retVal == SOCKET_ERROR)
		{
			if (WSAGetLastError() == WSAECONNRESET)
				std::cout << "[" << static_cast<int>(cl.data.id) << " Session] Disconnect" << std::endl;
			else errorDisplay(WSAGetLastError(), "Send(SC_PACKET_UPDATE_CLIENT)");
		}
	}

	// 플레이어의 상체 애니메이션은 한 번 보내고 나면 NONE 상태로 초기화
	for (auto& c : clients)
		c.data.upperAniType = eUpperAnimationType::NONE;
}

void NetworkFramework::SendBulletHitPacket()
{
	sc_packet_bullet_hit packet{};
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_BULLET_HIT;

	char buf[sizeof(packet)]{};
	WSABUF wsabuf{ sizeof(buf), buf };
	DWORD sent_byte;

	for (const auto& data : bulletHits)
	{
		packet.data = data;
		memcpy(buf, reinterpret_cast<char*>(&packet), sizeof(packet));
		const int retVal = WSASend(clients[static_cast<int>(data.bullet.playerId)].socket, &wsabuf, 1, &sent_byte, 0, nullptr, nullptr);
		if (retVal == SOCKET_ERROR)
		{
			if (WSAGetLastError() == WSAECONNRESET)
				std::cout << "[" << static_cast<int>(data.bullet.playerId) << " Session] Disconnect" << std::endl;
			else errorDisplay(WSAGetLastError(), "Send(SC_PACKET_BULLET_HIT)");
		}
	}
	bulletHits.clear();
}

void NetworkFramework::SendMonsterDataPacket()
{
	sc_packet_update_monsters packet{};
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_UPDATE_MONSTER;
	for (int i = 0; i < monsters.size(); ++i)
		packet.data[i] = monsters[i]->GetData();
	for (int i = monsters.size(); i < MAX_MONSTER; ++i)
		packet.data[i] = MonsterData{ .id = -1 };

	char buf[sizeof(packet)];
	memcpy(buf, reinterpret_cast<char*>(&packet), sizeof(packet));
	WSABUF wsabuf{ sizeof(buf), buf};
	DWORD sent_byte;

	for (const auto& player : clients)
	{
		if (!player.data.isActive) continue;
		const int retVal = WSASend(player.socket, &wsabuf, 1, &sent_byte, 0, nullptr, nullptr);
		if (retVal == SOCKET_ERROR) errorDisplay(WSAGetLastError(), "Send(SC_PACKET_UPDATE_MONSTER)");
	}

	// 죽은 몬스터는 서버에서 삭제
	erase_if(monsters, [](const std::unique_ptr<Monster>& m) { return m->GetHp() <= 0; });
}

void NetworkFramework::SendMonsterAttackPacket(const int id, const int mobId, const int damage) const
{
	sc_packet_monster_attack packet{};
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_MONSTER_ATTACK;
	packet.data.id = static_cast<CHAR>(id);
	packet.data.mobId = static_cast<CHAR>(mobId);
	packet.data.damage = static_cast<CHAR>(damage);
	SendPacket2AllPlayer(&packet, packet.size);
}

void NetworkFramework::SendRoundResultPacket(const eRoundResult result) const
{
	sc_packet_round_result packet{};
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_ROUND_RESULT;
	packet.result = result;

	SendPacket2AllPlayer(&packet, packet.size);

	std::cout << "[All Session] Ready To Play" << std::endl;
}

void NetworkFramework::ProcessRecvPacket(const int id)
{
	Session& cl = clients[id];
	for (;;)
	{
		char buf[2]; // size, type
		WSABUF wsabuf{ sizeof(buf), buf };
		DWORD recvd_byte{ 0 }, flag{ 0 };
		int retVal = WSARecv(cl.socket, &wsabuf, 1, &recvd_byte, &flag, nullptr, nullptr);
		if (retVal == SOCKET_ERROR)
		{
			if (WSAGetLastError() == WSAECONNRESET)
			{
				std::cout << "[" << id << " Session] Abortive Close " << std::endl;
				Disconnect(cl.data.id);
				break;
			}
			errorDisplay(WSAGetLastError(), "Recv()");
		}
		if (recvd_byte == 0)
		{
			std::cout << "[" << id << " Session] Graceful Close " << std::endl;
			Disconnect(cl.data.id);
			break;
		}

		UCHAR size{ static_cast<UCHAR>(buf[0]) };
		UCHAR type{ static_cast<UCHAR>(buf[1]) };

		switch (type)
		{
		case CS_PACKET_LOGIN:
		{
			// name[MAX_NAME_SIZE]
			char subBuf[MAX_NAME_SIZE]{};
			wsabuf = { sizeof(subBuf), subBuf };
			retVal = WSARecv(cl.socket, &wsabuf, 1, &recvd_byte, &flag, nullptr, nullptr);
			if (retVal == SOCKET_ERROR) errorDisplay(WSAGetLastError(), "Recv(CS_PACKET_LOGIN)");
			memcpy(&cl.name, subBuf, sizeof(cl.name));
			SendLoginOkPacket(cl);
			break;
		}
		case CS_PACKET_SELECT_WEAPON:
		{
			// 무기 타입
			char subBuf[1]{};
			wsabuf = { sizeof(subBuf), subBuf };
			retVal = WSARecv(cl.socket, &wsabuf, 1, &recvd_byte, &flag, nullptr, nullptr);
			if (retVal == SOCKET_ERROR) errorDisplay(WSAGetLastError(), "Recv(CS_PACKET_SELECT_WEAPON)");
			cl.weaponType = static_cast<eWeaponType>(subBuf[0]);
			SendSelectWeaponPacket(cl);
			break;
		}
		case CS_PACKET_READY:
		{
			// 준비 상태
			char subBuf[1]{};
			wsabuf = { sizeof(subBuf), subBuf };
			retVal = WSARecv(cl.socket, &wsabuf, 1, &recvd_byte, &flag, nullptr, nullptr);
			if (retVal == SOCKET_ERROR) errorDisplay(WSAGetLastError(), "Recv(CS_PACKET_READY)");
			cl.isReady = subBuf[0];
			SendReadyCheckPacket(cl);
			
			// 3명 다 레디한다면 게임 시작
			for (const auto& c : clients)
			{
				if (!c.data.isActive) continue;
				if (c.isReady) readyCount++;
			}
			if (readyCount >= MAX_USER)
			{
				isAccept = true;
				SendChangeScenePacket(eSceneType::GAME);
			}
			readyCount = 0;
			break;
		}
		case CS_PACKET_UPDATE_PLAYER:
		{
			// aniType, upperAniType, pos, velocity, yaw
			char subBuf[1 + 1 + 12 + 12 + 4];
			wsabuf = { sizeof(subBuf), subBuf };
			retVal = WSARecv(cl.socket, &wsabuf, 1, &recvd_byte, &flag, nullptr, nullptr);
			if (retVal == SOCKET_ERROR) errorDisplay(WSAGetLastError(), "Recv(CS_PACKET_UPDATE_PLAYER)");
			cl.data.aniType = static_cast<eAnimationType>(subBuf[0]);
			cl.data.upperAniType = static_cast<eUpperAnimationType>(subBuf[1]);
			memcpy(&cl.data.pos, &subBuf[2], sizeof(cl.data.pos));
			memcpy(&cl.data.velocity, &subBuf[14], sizeof(cl.data.velocity));
			memcpy(&cl.data.yaw, &subBuf[26], sizeof(cl.data.yaw));
			break;
		}
		case CS_PACKET_BULLET_FIRE:
		{
			char subBuf[sizeof(BulletData)]{};
			wsabuf = { sizeof(subBuf), subBuf };
			retVal = WSARecv(cl.socket, &wsabuf, 1, &recvd_byte, &flag, nullptr, nullptr);
			if (retVal == SOCKET_ERROR) errorDisplay(WSAGetLastError(), "Recv(CS_PACKET_BULLET_FIRE)");
			sc_packet_bullet_fire packet{};
			packet.size = sizeof(packet);
			packet.type = SC_PACKET_BULLET_FIRE;
			memcpy(&packet.data, subBuf, sizeof(BulletData));

			char sendBuf[sizeof(packet)];
			wsabuf = { sizeof(sendBuf), sendBuf };
			memcpy(sendBuf, &packet, sizeof(packet));
			DWORD sent_byte;

			// 총알은 수신하자마자 모든 클라이언트들에게 송신
			for (const auto& c : clients)
			{
				if (!c.data.isActive) continue;
				retVal = WSASend(c.socket, &wsabuf, 1, &sent_byte, 0, nullptr, nullptr);
				if (retVal == SOCKET_ERROR) errorDisplay(WSAGetLastError(), "Send(SC_PACKET_BULLET_FIRE)");
			}

			// 총알 정보를 추가해두고 Update함수 때 피격 판정한다.
			bullets.push_back(packet.data);
			break;
		}
		case CS_PACKET_SELECT_REWARD: // 플레이어가 보상을 선택했을 때
		{
			// 플레이어 아이디
			char subBuf[1]{};
			wsabuf = { sizeof(subBuf), subBuf };
			retVal = WSARecv(cl.socket, &wsabuf, 1, &recvd_byte, &flag, nullptr, nullptr);

			/*
			이 패킷이 3번 수신되면 다음 단계를 시작하는 코드
			*/

			break;
		}
		case CS_PACKET_LOGOUT:
		{
			sc_packet_logout_ok packet{};
			packet.size = sizeof(packet);
			packet.type = SC_PACKET_LOGOUT_OK;
			packet.id = id;

			char subBuf[sizeof(packet)]{};
			wsabuf = { sizeof(subBuf), subBuf };
			memcpy(subBuf, &packet, sizeof(packet));
			DWORD sent_byte;

			// 모든 클라이언트에게 이 플레이어가 나갔다고 송신
			for (const auto& c : clients)
			{
				if (!c.data.isActive) continue;
				retVal = WSASend(c.socket, &wsabuf, 1, &sent_byte, 0, nullptr, nullptr);
				if (retVal == SOCKET_ERROR) errorDisplay(WSAGetLastError(), "Send(SC_PACKET_LOGOUT_OK)");
			}

			std::cout << "[" << id << " Session] Logout" << std::endl;
			break;
		}
		default:
			std::cout << "[" << static_cast<int>(cl.data.id) << " Session] Server Received Unknown Packet (type : " << static_cast<int>(type) << ")" << std::endl;
			break;
		}
	}
}

void NetworkFramework::ProcessRecvPacket_iocp(const int id, char* p)
{
	char packet_type = p[1];
	Session& cl = clients[id];

	switch (packet_type) {
	case CS_PACKET_LOGIN: {
		cs_packet_login* packet = reinterpret_cast<cs_packet_login*>(p);
#ifdef DB_MODE
		bool retval = try_login_db(packet->name, id);
		if (retval == 1) {
			Disconnect(id);
			return;
		}
#else 
		strcpy_s(cl.name, packet->name);
#endif
		SendLoginOkPacket(cl);
		cl.lock.lock();
		cl.state = STATE::ST_INGAME;
		cl.lock.unlock();
		std::cout << packet->name << " is connect" << std::endl;
		break;
	}
	case CS_PACKET_SELECT_WEAPON:
	{
		// 무기 타입
		cs_packet_select_weapon* packet = reinterpret_cast<cs_packet_select_weapon*>(p);
		cl.weaponType = packet->weaponType;
		SendSelectWeaponPacket(cl);
		std::cout << id << " client is select" << std::endl;

		break;
	}
	case CS_PACKET_READY:
	{
		// 준비 상태
		cs_packet_ready* packet = reinterpret_cast<cs_packet_ready*>(p);

		cl.isReady = packet->isReady;
		SendReadyCheckPacket(cl);

		// 3명 다 레디한다면 게임 시작
		for (const auto& c : clients)
		{
			if (!c.data.isActive) continue;
			if (c.isReady) readyCount++;
		}
		if (readyCount >= MAX_USER)
		{
			isAccept = true;
			SendChangeScenePacket(eSceneType::GAME);
		}

		std::cout << id << " client is ready" << std::endl;

		readyCount = 0;
		break;
	}
	case CS_PACKET_UPDATE_PLAYER: {
		cs_packet_update_player* packet = reinterpret_cast<cs_packet_update_player*>(p);
		int x = cl.data.pos.x;
		int y = cl.data.pos.y;
		cl.data.aniType = packet->aniType;
		cl.data.upperAniType = packet->upperAniType;
		cl.data.pos = packet->pos;
		cl.data.velocity = packet->velocity;
		cl.data.yaw = packet->yaw;
		break;
	}
	case CS_PACKET_BULLET_FIRE:
	{
		cs_packet_bullet_fire* packet = reinterpret_cast<cs_packet_bullet_fire*>(p);
		sc_packet_bullet_fire send_packet{};
		send_packet.size = sizeof(packet);
		send_packet.type = SC_PACKET_BULLET_FIRE;
		send_packet.data = packet->data;

		char sendBuf[sizeof(send_packet)];
		WSABUF wsabuf = { sizeof(sendBuf), sendBuf };
		memcpy(sendBuf, &send_packet, sizeof(send_packet));
		DWORD sent_byte;

		// 총알은 수신하자마자 모든 클라이언트들에게 송신
		for (const auto& c : clients)
		{
			if (!c.data.isActive) continue;
			int retVal = WSASend(c.socket, &wsabuf, 1, &sent_byte, 0, nullptr, nullptr);
			if (retVal == SOCKET_ERROR) errorDisplay(WSAGetLastError(), "Send(SC_PACKET_BULLET_FIRE)");
		}

		// 총알 정보를 추가해두고 Update함수 때 피격 판정한다.
		bullets.push_back(send_packet.data);
		break;
	}
	case CS_PACKET_LOGOUT:
	{
		sc_packet_logout_ok send_packet{};
		send_packet.size = sizeof(send_packet);
		send_packet.type = SC_PACKET_LOGOUT_OK;
		send_packet.id = id;

		char subBuf[sizeof(send_packet)]{};
		WSABUF wsabuf = { sizeof(subBuf), subBuf };
		memcpy(subBuf, &send_packet, sizeof(send_packet));
		DWORD sent_byte;

		// 모든 클라이언트에게 이 플레이어가 나갔다고 송신
		for (const auto& c : clients)
		{
			if (!c.data.isActive) continue;
			int retVal = WSASend(c.socket, &wsabuf, 1, &sent_byte, 0, nullptr, nullptr);
			if (retVal == SOCKET_ERROR) errorDisplay(WSAGetLastError(), "Send(SC_PACKET_LOGOUT_OK)");
		}

		std::cout << "[" << id << " Session] Logout" << std::endl;
		break;
	}
	default:
		std::cout << "[" << static_cast<int>(cl.data.id) << " Session] Server Received Unknown Packet (size : " << 
			static_cast<int>(p[0]) << ", type : "<< static_cast<int>(p[1]) <<")" << std::endl;
		break;
	}
}

void NetworkFramework::Update(const FLOAT deltaTime)
{
	// 몬스터 스폰
	if (monsters.size() < MAX_MONSTER && isClearStage1 == false)
		SpawnMonsters(deltaTime);

	// 몬스터 업데이트
	for (auto& m : monsters)
		m->Update(deltaTime);

	// 충돌체크
	CollisionCheck();

	// 종료체크
	if (m_killScore >= stage1Goal && isClearStage1 == false)
	{
		for (auto& mob : monsters)
		{
			mob->SetHp(0);
			mob->SetAnimationType(eMobAnimationType::DIE);
		}
		std::cout << "[system] Stage1 Clear!" << std::endl;
		SendRoundResultPacket(eRoundResult::CLEAR);
		//SendChangeScenePacket(eSceneType::ENDING);
		isAccept = false;
		isClearStage1 = false;
		readyCount = 0;
		m_spawnCooldown = g_spawnCooldown;
		m_lastMobId = 0;
		m_killScore = 0;
		erase_if(monsters, [](const std::unique_ptr<Monster>& m) { return m->GetHp() >= -100; });
	}
}

void NetworkFramework::SpawnMonsters(const FLOAT deltaTime)
{
	// 쿨타임 때마다 생성
	m_spawnCooldown -= deltaTime;
	if (m_spawnCooldown <= 0.0f)
	{
		auto m{ std::make_unique<GarooMonster>() };
		m->SetId(m_lastMobId);
		m->SetRandomPosition();
		m->SetTargetId(DetectPlayer(m->GetPosition()));
		std::cout << static_cast<int>(m->GetId()) << " is generated, capacity: " << monsters.size() << " / " << MAX_MONSTER << std::endl;
		monsters.push_back(std::move(m));

		m_spawnCooldown = g_spawnCooldown;
		m_lastMobId++;
	}
}

UCHAR NetworkFramework::DetectPlayer(const XMFLOAT3& pos) const
{
	UCHAR index{ 0 };
	float length{ FLT_MAX };		// 가까운 플레이어의 거리
	for (const auto& cl : clients)
	{
		const float l{ Vector3::Length(Vector3::Sub(cl.data.pos, pos)) };
		if (l < length)
		{
			length = l;
			index = cl.data.id;
		}
	}
	return index;
}

void NetworkFramework::CollisionCheck()
{
	// 총알과 충돌한 몬스터들 중 총알과 가장 가까운 몬스터와
	// 충돌한 것으로 처리해야되기 때문에 총알을 바깥 for문으로 돌아야한다.
	for (const BulletData& b : bullets)
	{
		Monster* hitMonster{ nullptr };	// 피격된 몹
		float length{ FLT_MAX };		// 피격된 몹과 총알 간의 거리

		for (auto& m : monsters)
		{
			if (m->GetHp() <= 0) continue;

			BoundingOrientedBox boundingBox{ m->GetBoundingBox() };
			XMVECTOR origin{ XMLoadFloat3(&b.pos) };
			XMVECTOR direction{ XMLoadFloat3(&b.dir) };
			float dist{ 3000.0f };
			if (boundingBox.Intersects(origin, direction, dist))
			{
				// 기존 피격된 몹과 총알 간의 거리보다
				// 새로 피격된 몹과 총알 간의 거리가 더 짧으면 피격 당한 몹을 바꿈
				float l{ Vector3::Length(Vector3::Sub(b.pos, m->GetPosition())) };
				if (l < length)
				{
					hitMonster = m.get();
					length = l;
					//std::cout << static_cast<int>(m.GetId()) << " is hit" << std::endl;
				}

				// 해당 플레이어가 총알을 맞췄다는 것을 저장
				BulletHitData hitData{};
				hitData.bullet = b;
				hitData.mobId = m->GetId();
				bulletHits.push_back(std::move(hitData));
			}
		}
		if (hitMonster) hitMonster->OnHit(b);
	}

	// 충돌체크가 완료된 총알들은 삭제
	bullets.clear();
}

void NetworkFramework::Disconnect(const int id)
{
	Session& cl = clients[id];
	cl.lock.lock();
	cl.data.id = 0;
	cl.data.isActive = false;
	cl.data.aniType = eAnimationType::IDLE;
	cl.data.upperAniType = eUpperAnimationType::NONE;
	cl.data.pos = {};
	cl.data.velocity = {};
	cl.data.yaw = 0.0f;
	cl.isReady = false;
	cl.weaponType = eWeaponType::AR;
	closesocket(cl.socket);
	cl.lock.unlock();
}

CHAR NetworkFramework::GetNewId() const
{
	for (int i = 0; i < MAX_USER; ++i) {
		if (false == clients[i].data.isActive)
		{
			return i;
		}
	}
	std::cout << "Maximum Number of Clients" << std::endl;
	return -1;
}

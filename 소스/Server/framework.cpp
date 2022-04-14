#include "framework.h"
using namespace DirectX;

int NetworkFramework::OnInit(SOCKET socket)
{
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

void NetworkFramework::AcceptThread(SOCKET socket)
{
	std::cout << "AcceptThread start" << std::endl;
	SOCKADDR_IN clientAddr;
	INT addrSize = sizeof(clientAddr);
	while (true)
	{
		SOCKET cSocket = WSAAccept(socket, reinterpret_cast<sockaddr*>(&clientAddr), &addrSize, nullptr, 0);
		if (cSocket == INVALID_SOCKET) errorDisplay(WSAGetLastError(), "Accept");

		CHAR id{ GetNewId() };
		Session& player = clients[id];

		player.lock.lock();
		player.data.id = id;
		player.data.isActive = true;
		player.data.aniType = eAnimationType::IDLE;
		player.data.upperAniType = eUpperAnimationType::NONE;
		player.socket = cSocket;
		SendLoginOkPacket(id);
		player.lock.unlock();
		char ipInfo[20]{};
		inet_ntop(AF_INET, &clientAddr.sin_addr, ipInfo, sizeof(ipInfo));
		std::cout << "[" << static_cast<int>(player.data.id) << " Session] connect IP: " << ipInfo << std::endl;
		//m_networkThread = thread{ &GameFramework::ProcessClient, this, reinterpret_cast<LPVOID>(g_c_socket) };
		threads.emplace_back(&NetworkFramework::ProcessRecvPacket, this, id);
		isAccept = true;
	}
}

void NetworkFramework::SendLoginOkPacket(int id)
{
	sc_packet_login_ok packet{};
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_LOGIN_OK;
	packet.data.id = id;
	packet.data.isActive = true;
	packet.data.aniType = eAnimationType::IDLE;
	packet.data.upperAniType = eUpperAnimationType::NONE;
	packet.data.pos = DirectX::XMFLOAT3{ 0.0f, 0.0f, 0.0f };

	char buf[sizeof(packet)];
	memcpy(buf, reinterpret_cast<char*>(&packet), sizeof(packet));
	WSABUF wsabuf{ sizeof(buf), buf };
	DWORD sentByte;

	std::cout << "[" << static_cast<int>(buf[2]) << " Session] Login Packet Received" << std::endl;
	const int retVal = WSASend(clients[id].socket, &wsabuf, 1, &sentByte, 0, nullptr, nullptr);
	if (retVal == SOCKET_ERROR) errorDisplay(WSAGetLastError(), "LoginSend");
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
				std::cout << "[" << static_cast<int>(cl.data.id) << " SESSION] Disconnect" << std::endl;
			else errorDisplay(WSAGetLastError(), "SendPlayerData");
		}
	}
	/*
	for (const auto& player : clients)
	{
		if (!player.data.isActive) continue;
		sc_packet_update_client packet{};
		for (int i = 0; i < MAX_USER; ++i)	{
			packet.data[i] = clients[i].data;
		}
		packet.size = sizeof(packet);
		packet.type = SC_PACKET_UPDATE_CLIENT;
		//packet.data = player.data;
		char buf[sizeof(packet)];
		memcpy(buf, reinterpret_cast<char*>(&packet), sizeof(packet));
		WSABUF wsabuf{ sizeof(buf), buf };
		DWORD sent_byte;
		for (const auto& cl : clients)
		{
			if (!cl.data.isActive) continue;
			int retVal = WSASend(cl.socket, &wsabuf, 1, &sent_byte, 0, nullptr, nullptr);
			if (retVal == SOCKET_ERROR)
			{
				if (WSAGetLastError() == WSAECONNRESET)
					std::cout <<  "[" << static_cast<int>(cl.data.id) << " SESSION] Disconnect" << std::endl;
				else errorDisplay(WSAGetLastError(), "SendPlayerData");
			}
		}
	}*/
	// 플레이어의 상체 애니메이션은 한 번 보내고 나면 NONE 상태로 초기화
	for (auto& c : clients)
		c.data.upperAniType = eUpperAnimationType::NONE;
}

void NetworkFramework::SendMonsterDataPacket()
{
	sc_packet_update_monsters packet{};
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_UPDATE_MONSTER;
	//for (int i = 0; i < MAX_MONSTER; ++i)
	//	packet.data[i] = monsters[i].GetData();
	for (int i = 0; i < monsters.size(); ++i)
		packet.data[i] = monsters[i].GetData();
	if (monsters.size() < MAX_MONSTER)
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
		if (retVal == SOCKET_ERROR)
		{
			if (WSAGetLastError() == WSAECONNRESET)
				std::cout << "[" << static_cast<int>(player.data.id) << " SESSION] Disconnect" << std::endl;
			else errorDisplay(WSAGetLastError(), "SendMonsterData");
		}
	}
}

void NetworkFramework::ProcessRecvPacket(int id)
{
	Session& cl = clients[id];
	for (;;)
	{
		char buf[2]; // size, type
		WSABUF wsabuf{ sizeof(buf), buf };
		DWORD recvd_byte{ 0 }, flag{ 0 };
		const int retVal = WSARecv(cl.socket, &wsabuf, 1, &recvd_byte, &flag, nullptr, nullptr);
		//std::cout << "[" << static_cast<int>(buf[0]) << " type] : " << static_cast<int>(buf[1]) << "byte received" << std::endl;
		if (retVal == SOCKET_ERROR)
		{
			if (WSAGetLastError() == WSAECONNRESET)
			{
				std::cout << "[" << static_cast<int>(cl.data.id) << " client] Abortive Close " << std::endl;
				Disconnect(cl.data.id);
				break;
			}
			errorDisplay(WSAGetLastError(), "Recv");
		}
		if (recvd_byte == 0)
		{
			std::cout << "[" << static_cast<int>(cl.data.id) << " client] Graceful Close " << std::endl;
			Disconnect(cl.data.id);
			break;
		}

		UCHAR size{ static_cast<UCHAR>(buf[0]) };
		UCHAR type{ static_cast<UCHAR>(buf[1]) };

		switch (type)
		{
		case CS_PACKET_UPDATE_LEGS:
		{
			// aniType, upperAniType, pos, velocity, yaw
			char subBuf[1 + 1 + 12 + 12 + 4];
			wsabuf = { sizeof(subBuf), subBuf };
			WSARecv(cl.socket, &wsabuf, 1, &recvd_byte, &flag, nullptr, nullptr);

			cl.data.aniType = static_cast<eAnimationType>(subBuf[0]);
			cl.data.upperAniType = static_cast<eUpperAnimationType>(subBuf[1]);
			memcpy(&cl.data.pos, &subBuf[2], sizeof(cl.data.pos));
			memcpy(&cl.data.velocity, &subBuf[14], sizeof(cl.data.velocity));
			memcpy(&cl.data.yaw, &subBuf[26], sizeof(cl.data.yaw));
			break;
		}
		case CS_PACKET_BULLET_FIRE:
		{
			// pos, dir
			char subBuf[12 + 12];
			wsabuf = { sizeof(subBuf), subBuf };
			WSARecv(cl.socket, &wsabuf, 1, &recvd_byte, &flag, nullptr, nullptr);

			sc_packet_bullet_fire packet{};
			packet.size = sizeof(packet);
			packet.type = SC_PACKET_BULLET_FIRE;
			memcpy(&packet.data.pos, &subBuf[0], sizeof(packet.data.pos));
			memcpy(&packet.data.dir, &subBuf[12], sizeof(packet.data.dir));

			char sendBuf[sizeof(packet)];
			wsabuf = { sizeof(sendBuf), sendBuf };
			memcpy(sendBuf, &packet, sizeof(packet));
			DWORD sent_byte;

			// 총알은 수신하자마자 다른 클라이언트들에게 송신
			for (const auto& c : clients)
			{
				if (c.data.id == cl.data.id) continue; // 총을 쏜 사람에겐 보내지않음
				WSASend(c.socket, &wsabuf, 1, &sent_byte, 0, nullptr, nullptr);
			}

			// 총알 정보를 추가해두고 Update함수 때 피격 판정한다.
			bullets.push_back(packet.data);
			break;
		}
		default:
			std::cout << "[" << static_cast<int>(cl.data.id) << " client] Server Received Unknown Packet" << std::endl;
			break;
		}
	}
}

void NetworkFramework::Update(FLOAT deltaTime)
{
	// 몬스터 스폰
	if (monsters.size() < MAX_MONSTER)
		SpawnMonsters(deltaTime);

	// 몬스터 업데이트
	for (auto& m : monsters)
		m.Update(deltaTime);

	// 충돌체크
	CollisionCheck();
}

void NetworkFramework::SpawnMonsters(FLOAT deltaTime)
{
	// 쿨타임 때마다 생성
	m_spawnCooldown -= deltaTime;
	if (m_spawnCooldown <= 0.0f) {
		const auto m = new Monster();
		m->SetId(monsters.size());
		m->SetType(0);
		m->SetAnimationType(eMobAnimationType::IDLE);
		m->SetPosition(DirectX::XMFLOAT3{ 0.0f, 0.0f, 400.0f });
		g_networkFramework.monsters.push_back(std::move(*m));
		m_spawnCooldown = 5.0f;
	}
}


void NetworkFramework::CollisionCheck()
{
	// 총알과 충돌한 몬스터들 중 총알과 가장 가까운 몬스터와
	// 충돌한 것으로 처리해야되기 때문에 총알을 바깥 for문으로 돌아야한다.
	for (const BulletData& b : bullets)
	{
		Monster* hitMonsters{ nullptr };	// 피격된 몹
		float length{ FLT_MAX };			// 피격된 몹과 총알 간의 거리

		for (Monster& m : monsters)
		{
			BoundingOrientedBox boundingBox{ m.GetBoundingBox() };
			XMVECTOR origin{ XMLoadFloat3(&b.pos) };
			XMVECTOR direction{ XMLoadFloat3(&b.dir) };
			float dist{ 3000.0f };
			if (boundingBox.Intersects(origin, direction, dist))
			{
				// 기존 피격된 몹과 총알 간의 거리보다
				// 새로 피격된 몹과 총알 간의 거리가 더 짧으면 피격 당한 몹을 바꿈
				float l{ Vector3::Length(Vector3::Sub(b.pos, m.GetPosition())) };
				if (l < length)
				{
					hitMonsters = &m;
					length = l;
				}
			}
		}
		if (hitMonsters)
			hitMonsters->SetAnimationType(eMobAnimationType::HIT);
	}

	// 충돌체크가 완료된 총알들은 삭제
	bullets.clear();
}

void NetworkFramework::Disconnect(int id)
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
	closesocket(cl.socket);
	cl.lock.unlock();
}

CHAR NetworkFramework::GetNewId()
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

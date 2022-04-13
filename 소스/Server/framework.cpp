#include "framework.h"

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
		int retVal = WSASend(cl.socket, &wsabuf, 1, &sent_byte, 0, nullptr, nullptr);
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
	for (int i = 0; i < MAX_MONSTER; ++i)
		packet.data[i] = monsters[i];

	char buf[sizeof(packet)];
	memcpy(buf, reinterpret_cast<char*>(&packet), sizeof(packet));
	WSABUF wsabuf{ sizeof(buf), buf};
	DWORD sent_byte;

	for (const auto& player : clients)
	{
		if (!player.data.isActive) continue;
		int retVal = WSASend(player.socket, &wsabuf, 1, &sent_byte, 0, nullptr, nullptr);
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
		int retVal = WSARecv(cl.socket, &wsabuf, 1, &recvd_byte, &flag, nullptr, nullptr);
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
	using namespace DirectX;
	constexpr int TARGET_CLIENT_INDEX{ 0 };	// 몬스터가 쫓아갈 플레이어 인덱스
	constexpr float MOB_SPEED{ 50.0f };		// 1초당 움직일 거리

	// 모든 몬스터 업데이트
	// 다만 현재 구조로는 모든 몬스터를 세팅해주지 않으면 쓰레기값으로 업데이트할 수도 있다.
	// 나중에 array<unique_ptr<MonsterData>, MAX_MONSTER>로 바꿔야할 것 같다.
	for (int i = 0; i < MAX_MONSTER; ++i)
	{
		XMVECTOR playerPos{ XMLoadFloat3(&clients[TARGET_CLIENT_INDEX].data.pos) };	// 플레이어 위치
		XMVECTOR mobPos{ XMLoadFloat3(&monsters[i].pos) };							// 몬스터 위치
		XMVECTOR dir{ XMVector3Normalize(playerPos - mobPos) };						// 몬스터 -> 플레이어 방향 벡터

		// 몹 이동속도
		XMFLOAT3 velocity{};
		XMStoreFloat3(&velocity, dir * MOB_SPEED);

		// 몹 위치
		monsters[i].pos.x += velocity.x * deltaTime;
		monsters[i].pos.y += velocity.y * deltaTime;
		monsters[i].pos.z += velocity.z * deltaTime;

		// 몹 속도
		// 클라이언트에서 속도는 로컬좌표계 기준이다.
		// 객체 기준 x값만큼 오른쪽으로, y값만큼 위로, z값만큼 앞으로 간다.
		// 몹은 플레이어를 향해 앞으로만 가므로 x, y는 0으로 바꾸고 z는 몹의 이동속도로 설정한다.
		monsters[i].velocity.x = 0.0f;
		monsters[i].velocity.y = 0.0f;
		monsters[i].velocity.z = MOB_SPEED;

		// 몹 각도
		XMVECTOR zAxis{ XMVectorSet(0.0f, 0.0f, 1.0f, 1.0f) };			// +z축
		XMVECTOR radian{ XMVector3AngleBetweenNormals(zAxis, dir) };	// +z축과 몬스터 -> 플레이어 방향 벡터 간의 각도
		XMFLOAT3 angle{}; XMStoreFloat3(&angle, radian);
		angle.x = XMConvertToDegrees(angle.x);

		// x가 0보다 작다는 것은 반시계 방향으로 회전한 것
		XMFLOAT3 direction{}; XMStoreFloat3(&direction, dir);
		monsters[i].yaw = direction.x < 0 ? -angle.x : angle.x;

		// 몹 애니메이션
		monsters[i].aniType = eMobAnimationType::RUNNING;

		// 피격 계산
		BoundingOrientedBox boundingBox{ Utility::GetBoundingBox(monsters[i]) };
		for (const auto& bullet : bullets)
		{
			XMVECTOR origin{ XMLoadFloat3(&bullet.pos) };
			XMVECTOR direction{ XMLoadFloat3(&bullet.dir) };
			float dist{ 3000.0f };
			if (boundingBox.Intersects(origin, direction, dist))
				std::cout << "HIT!" << std::endl;
			else
				std::cout << "NO HIT!" << std::endl;
		}

		// 피격 계산을 완료한 총알은 삭제
		bullets.clear();
	}
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
		//std::cout << "id 검색중: " << i << std::endl;
		if (false == clients[i].data.isActive)
		{
			//std::cout << "찾았다: " << i << std::endl;
			return i;
		}
	}
	std::cout << "Maximum Number of Clients" << std::endl;
	return -1;
}

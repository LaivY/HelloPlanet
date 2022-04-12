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
	Session& player = clients[id];

	sc_packet_login_ok packet;
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_LOGIN_OK;
	packet.data.id = id;
	packet.data.isActive = true;
	packet.data.aniType = eAnimationType::IDLE;
	packet.data.upperAniType = eUpperAnimationType::NONE;
	packet.data.pos = { 0.0f, 0.0f, 0.0f };
	char buf[sizeof(packet)];
	memcpy(buf, reinterpret_cast<char*>(&packet), sizeof(packet));
	WSABUF wsabuf{ sizeof(buf), buf };
	DWORD sentByte;
	std::cout << "[" << static_cast<int>(buf[2]) << " Session] Login Packet Received" << std::endl;
	const int retVal = WSASend(player.socket, &wsabuf, 1, &sentByte, 0, nullptr, nullptr);
	if (retVal == SOCKET_ERROR) errorDisplay(WSAGetLastError(), "LoginSend");
}

void NetworkFramework::SendPlayerDataPacket()
{
	sc_packet_update_client packet{};
	for (int i = 0; i < MAX_USER; ++i) {
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
	for (int i = 0; i < MAX_MONSTER; ++i)
	{
		packet.data[i] = monsters[i];
	}
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_UPDATE_MONSTER;
	char buf[sizeof(packet)];
	memcpy(buf, reinterpret_cast<char*>(&packet), sizeof(packet));
	WSABUF wsabuf{ buf[0], buf};
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
			for (const auto& c : clients)
			{
				if (c.data.id == cl.data.id) continue; // 총을 쏜 사람에겐 보내지않음
				WSASend(c.socket, &wsabuf, 1, &sent_byte, 0, nullptr, nullptr);
			}
			break;
		}
		default:
			std::cout << "[" << static_cast<int>(cl.data.id) << " client] Server Received Unknown Packet" << std::endl;
			break;
		}
	}
}

void NetworkFramework::MonsterTimer(int mobId)
{
	// speedFps는 1/60 초당가는 거리 만큼 좁아지는 타이머
	// 한명이라도 들어오면 실행, 0번 몬스터가 0번 클라이언트를 향해 무작정 다가간다.
	// 변수 초기화, 오버헤드가 큰 실수 제곱근 연산은 해당 플레이어가 움직일때(30fps로 Send할때)만 작동하게 바꿀예정
	//constexpr float fpsSpeed = 0.5f;
	//monsters[mobId].velocity = fpsSpeed;
	//monsters[mobId].state = eMobAnimationType::RUNNING;
	//const float dis_x = clients[mobId].data.pos.x;
	//const float dis_z = clients[mobId].data.pos.z;
	//const float start_x = monsters[mobId].pos.x;
	//const float start_z = monsters[mobId].pos.z;
	//// a: 방향, (x, z): 목적지까지 도달하면 이번 프레임에 움직여야하는 좌표
	//const float a = abs((dis_z - start_z) / (dis_x - start_x));
	//const float x = fpsSpeed / (sqrt(pow(a, 2) + 1));
	//const float z = a * x;
	//if ((dis_x - start_x) < 0) monsters[mobId].pos.x -= x;
	//else monsters[mobId].pos.x += x;
	//if ((dis_z - start_z) < 0) monsters[mobId].pos.z -= z;
	//else monsters[mobId].pos.z += z;

	//if (start_x <= dis_x && start_z <= dis_z)
	//	monsters[mobId].yaw = DirectX::XMConvertToDegrees(atan(a));
	//else if (start_x > dis_x && start_z <= dis_z)
	//	monsters[mobId].yaw = DirectX::XMConvertToDegrees(atan(a)) + 90;
	//else if (start_x > dis_x && start_z > dis_z)
	//	monsters[mobId].yaw = DirectX::XMConvertToDegrees(atan(a)) + 180;
	//else if (start_x <= dis_x && start_z > dis_z)
	//	monsters[mobId].yaw = DirectX::XMConvertToDegrees(atan(a)) + 270;

	//std::cout << "monster: " << g_networkFramework.monsters[0].pos.x << ", " << g_networkFramework.monsters[0].pos.z << ", " << g_networkFramework.monsters[0].yaw << std::endl;

	using namespace DirectX;
	XMVECTOR playerPos{ XMLoadFloat3(&clients[0].data.pos) };		// 플레이어 위치
	XMVECTOR mobPos{ XMLoadFloat3(&monsters[mobId].pos) };			// 몬스터 위치
	XMVECTOR dir{ XMVector3Normalize(playerPos - mobPos) };			// 몬스터 -> 플레이어 방향 벡터
	XMVECTOR zAxis{ XMVectorSet(0.0f, 0.0f, 1.0f, 1.0f) };			// +z축
	XMVECTOR radian{ XMVector3AngleBetweenNormals(zAxis, dir) };	// z축과 몬스터 -> 플레이어 방향 벡터 간의 각도

	XMFLOAT3 direction{};
	XMStoreFloat3(&direction, dir);

	// 몹 이동속도
	XMStoreFloat3(&monsters[mobId].velocity, dir * 0.5f);

	// 몹 위치
	monsters[mobId].pos.x += monsters[mobId].velocity.x;
	monsters[mobId].pos.y += monsters[mobId].velocity.y;
	monsters[mobId].pos.z += monsters[mobId].velocity.z;

	// 몹 각도
	XMFLOAT3 angle{};
	XMStoreFloat3(&angle, radian);
	angle.x = XMConvertToDegrees(angle.x);
	if (direction.x < 0) angle.x *= -1;
	monsters[mobId].yaw = angle.x;

	// 몹 애니메이션
	monsters[mobId].aniType = eMobAnimationType::RUNNING;
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

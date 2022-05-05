﻿#include "framework.h"
using namespace DirectX;

NetworkFramework::NetworkFramework() : isAccept{ false }, bulletHits{}, m_spawnCooldown{ 5.0f }, m_lastMobId{ 0 }
{

}

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
		if (retVal == SOCKET_ERROR) errorDisplay(WSAGetLastError(), "Recv(SC_PACKET_LOGIN_CONFIRM)");
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

	char buf[sizeof(packet)];
	memcpy(buf, reinterpret_cast<char*>(&packet), sizeof(packet));
	WSABUF wsabuf{ sizeof(buf), buf };
	DWORD sentByte;

	// 모든 클라이언트에게 클라이언트의 무기상태 전송
	for (const auto& c : clients)
	{
		if (c.data.id == player.data.id) continue;
		if (!c.data.isActive) continue;
		const int retVal = WSASend(c.socket, &wsabuf, 1, &sentByte, 0, nullptr, nullptr);
		if (retVal == SOCKET_ERROR) errorDisplay(WSAGetLastError(), "Recv(SC_PACKET_SELECT_WEAPON)");
	}

	std::cout << "[" << static_cast<int>(player.data.id) << " Session] Select Weapon Packet Received" << std::endl;
}

void NetworkFramework::SendReadyCheckPacket(const Session& player) const
{
	sc_packet_ready_check packet{};
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_READY_CHECK;
	packet.id = player.data.id;
	packet.isReady = player.isReady;

	char buf[sizeof(packet)];
	memcpy(buf, reinterpret_cast<char*>(&packet), sizeof(packet));
	WSABUF wsabuf{ sizeof(buf), buf };
	DWORD sentByte;

	// 모든 클라이언트에게 이 클라이언트의 준비상태 전송
	for (const auto& c : clients)
	{
		if (c.data.id == player.data.id) continue;
		if (!c.data.isActive) continue;
		const int retVal = WSASend(c.socket, &wsabuf, 1, &sentByte, 0, nullptr, nullptr);
		if (retVal == SOCKET_ERROR) errorDisplay(WSAGetLastError(), "Recv(SC_PACKET_READY_CHECK)");
	}

	std::cout << "[" << static_cast<int>(player.data.id) << " Session] Ready Packet Received" << std::endl;
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
			else errorDisplay(WSAGetLastError(), "SendPlayerData");
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
			else errorDisplay(WSAGetLastError(), "SendBulletHitData");
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
		packet.data[i] = monsters[i].GetData();
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
				std::cout << "[" << static_cast<int>(player.data.id) << " Session] Disconnect" << std::endl;
			else errorDisplay(WSAGetLastError(), "SendMonsterData");
		}
	}

	// 죽은 몬스터는 서버에서 삭제
	erase_if(monsters, [](const Monster& m) { return m.GetHp() <= 0; });
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
			
			// 게임 시작 확인
			for (const auto& c : clients)
			{
				if (!c.data.isActive) continue;
				if (c.isReady) readyCount++;
			}
			if (readyCount >= 2)
			{
				isAccept = true;
				sc_packet_change_scene sendPacket{};
				sendPacket.size = sizeof(sendPacket);
				sendPacket.type = SC_PACKET_CHANGE_SCENE;
				sendPacket.sceneType = eSceneType::GAME;

				char sendBuf[sizeof(sendPacket)]{};
				WSABUF sendWsabuf = { sizeof(sendBuf), sendBuf };
				memcpy(sendBuf, &sendPacket, sizeof(sendPacket));
				DWORD sent_byte;

				for (const auto& cl : clients)
				{
					if (!cl.data.isActive) continue;
					retVal = WSASend(cl.socket, &sendWsabuf, 1, &sent_byte, 0, nullptr, nullptr);
					if (retVal == SOCKET_ERROR) errorDisplay(WSAGetLastError(), "Send(SC_PACKET_CHANGE_SCENE)");
				}

				std::cout << "[Send All Client] Ready To Play" << std::endl;
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
			// pos, dir, playerId
			char subBuf[12 + 12 + 1]{};
			wsabuf = { sizeof(subBuf), subBuf };
			retVal = WSARecv(cl.socket, &wsabuf, 1, &recvd_byte, &flag, nullptr, nullptr);
			if (retVal == SOCKET_ERROR) errorDisplay(WSAGetLastError(), "Recv(CS_PACKET_BULLET_FIRE)");
			sc_packet_bullet_fire packet{};
			packet.size = sizeof(packet);
			packet.type = SC_PACKET_BULLET_FIRE;
			memcpy(&packet.data.pos, &subBuf[0], sizeof(packet.data.pos));
			memcpy(&packet.data.dir, &subBuf[12], sizeof(packet.data.dir));
			memcpy(&packet.data.playerId, &subBuf[24], sizeof(packet.data.playerId));

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

void NetworkFramework::Update(const FLOAT deltaTime)
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

void NetworkFramework::SpawnMonsters(const FLOAT deltaTime)
{
	// 쿨타임 때마다 생성
	m_spawnCooldown -= deltaTime;
	if (m_spawnCooldown <= 0.0f)
	{
		Monster m;
		m.SetId(m_lastMobId);
		m.SetHp(100);
		m.SetType(0);
		m.SetAnimationType(eMobAnimationType::IDLE);
		m.SetPosition(DirectX::XMFLOAT3{ 0.0f, 0.0f, -400.0f });
		m.SetTargetId(DetectPlayer(m.GetPosition()));
		std::cout << static_cast<int>(m.GetId()) << " is generated, capacity: " << monsters.size() << " / " << MAX_MONSTER << std::endl;
		monsters.push_back(std::move(m));

		m_spawnCooldown = 5.0f;
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

		for (Monster& m : monsters)
		{
			if (m.GetHp() <= 0) continue;

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
					hitMonster = &m;
					length = l;
					std::cout << static_cast<int>(m.GetId()) << " is hit" << std::endl;
				}

				// 해당 플레이어가 총알을 맞췄다는 것을 저장
				BulletHitData hitData{};
				hitData.bullet = b;
				hitData.mobId = m.GetId();
				bulletHits.push_back(std::move(hitData));
			}
		}
		if (hitMonster) hitMonster->OnHit(b);
	}

	// 충돌체크가 완료된 총알들은 삭제
	bullets.clear();

	// 몬스터 공격 
	//Monster* attackMonster{ nullptr };	// 다가온 몹
	
	for (Monster& mob : monsters)
	{
		/*if (mob.GetHp() <= 0) continue;
		std::cout << mob.GetAtkTimer() << "   " << static_cast<int>(mob.GetAnimationType()) << std::endl;
		if (mob.GetAnimationType() == eMobAnimationType::ATTACK)
		{
			if (mob.GetAtkTimer() >= 0.3f && mob.tmp == false)
			{
				float range{ Vector3::Length(Vector3::Sub(clients[mob.GetTargetId()].data.pos, mob.GetPosition())) };
				if (range < 27.0f) std::cout << static_cast<int>(mob.GetTargetId()) << " is under attack!" << std::endl;
				else std::cout << static_cast<int>(mob.GetTargetId()) << " is dodge!" << std::endl;
				mob.tmp = true;
			}
			continue;
		}
		float range{ Vector3::Length(Vector3::Sub(clients[mob.GetTargetId()].data.pos, mob.GetPosition()))};

		if (range < 25.0f)
		{
			std::cout << static_cast<int>(mob.GetTargetId()) << " is close!" << std::endl;
			mob.Attack(mob.GetTargetId());
		}*/
	}
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

#include "stdafx.h"
#include "framework.h"

void NetworkFramework::SendPacket2AllPlayer(void* packet, int bufSize) const
{
	//char buf[BUF_SIZE]{};
	//memcpy(buf, packet, bufSize);
	auto buf = static_cast<char*>(packet);
	WSABUF wsaBuf = { static_cast<ULONG>(bufSize), buf };
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
	// + 피격 애니메이션은 한 번 보내고 나면 NONE 상태로 초기화
	for (auto& c : clients)
	{
		c.data.upperAniType = eUpperAnimationType::NONE;
		if (c.data.aniType == eAnimationType::HIT)
			c.data.aniType = eAnimationType::NONE;
	}
}

void NetworkFramework::SendBulletDataPacket()
{
	for (auto& bl : bullets)
	{
		if (bl.isBulletCast) continue;
		sc_packet_bullet_fire send_packet{};
		send_packet.size = sizeof(send_packet);
		send_packet.type = SC_PACKET_BULLET_FIRE;
		send_packet.data = bl.data;
		char sendBuf[sizeof(send_packet)];
		WSABUF wsabuf = { sizeof(sendBuf), sendBuf };
		memcpy(sendBuf, &send_packet, sizeof(send_packet));
		DWORD sent_byte;

		// 총알은 수신하자마자 모든 클라이언트들에게 송신
		for (const auto& c : clients)
		{
			if (!c.data.isActive) continue;
			int retVal = WSASend(c.socket, &wsabuf, 1, &sent_byte, 0, nullptr, nullptr);
			if (retVal == SOCKET_ERROR)
			{
				if (WSAGetLastError() == WSAECONNRESET)
					std::cout << "[" << static_cast<int>(c.data.id) << " Session] Disconnect" << std::endl;
				else errorDisplay(WSAGetLastError(), "Send(SC_PACKET_BULLET_FIRE)");
			}
			
		}
		bl.isBulletCast = TRUE;
	}

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
	packet.size = static_cast<UCHAR>(sizeof(packet)); // UCHAR의 최대값을 넘어감.. 하지만 size값을 사용하지 않을거라면 상관없음
	packet.type = SC_PACKET_UPDATE_MONSTER;
	for (size_t i = 0; i < monsters.size(); ++i)
		packet.data[i] = monsters[i]->GetData();
	for (size_t i = monsters.size(); i < MAX_MONSTER; ++i)
		packet.data[i] = MonsterData{ .id = -1 };

	char buf[sizeof(packet)];
	memcpy(buf, reinterpret_cast<char*>(&packet), sizeof(packet));
	WSABUF wsabuf{ sizeof(buf), buf };
	DWORD sent_byte;

	for (const auto& player : clients)
	{
		if (!player.data.isActive) continue;
		const int retVal = WSASend(player.socket, &wsabuf, 1, &sent_byte, 0, nullptr, nullptr);
		if (retVal == SOCKET_ERROR)
		{
			if (WSAGetLastError() == WSAECONNRESET)
				std::cout << "[" << static_cast<int>(player.data.id) << " Session] Disconnect" << std::endl;
			else errorDisplay(WSAGetLastError(), "Send(SC_PACKET_UPDATE_MONSTER)");
		}
	}

	// 죽은 몬스터는 서버에서 삭제
	std::unique_lock<std::mutex> lock{ g_mutex };
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
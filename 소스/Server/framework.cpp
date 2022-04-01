#include "framework.h"

NetFramework* NetFramework::instance = nullptr;
NetFramework* NetFramework::GetInstance()
{
	return instance;
}
CHAR NetFramework::GetNewId()
{
	for (int i = 0; i < MAX_USER; ++i) {
		//std::cout << "id 검색중: " << i << std::endl;
		if (false == clients[i].m_data.isActive)
		{
			//std::cout << "찾았다: " << i << std::endl;
			return i;
		}
	}
	std::cout << "Maximum Number of Clients" << std::endl;
	return -1;
}

void NetFramework::SendLoginOkPacket(int id)
{
	Session& cl = clients[id];

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
	std::cout << "login pacet: " << static_cast<int>(buf[0]) << " : " << static_cast<int>(buf[1]) << " : " << static_cast<int>(buf[2]) << std::endl;
	const int retVal = WSASend(cl.m_socket, &wsabuf, 1, &sentByte, 0, nullptr, nullptr);
	if (retVal == SOCKET_ERROR) errorDisplay(WSAGetLastError(), "Login Send");
}

void NetFramework::Disconnect(int id)
{
	Session& cl = clients[id];
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

void NetFramework::ProcessRecvPacket(int id)
{
	Session& cl = clients[id];
	for (;;)
	{
		char buf[2]; // size, type
		WSABUF wsabuf{ sizeof(buf), buf };
		DWORD recvd_byte{ 0 }, flag{ 0 };
		int retVal = WSARecv(cl.m_socket, &wsabuf, 1, &recvd_byte, &flag, nullptr, nullptr);
		std::cout << "[" << static_cast<int>(buf[0]) << " type] : " << static_cast<int>(buf[1]) << "byte received" << std::endl;
		if (retVal == SOCKET_ERROR)
		{
			if (WSAGetLastError() == WSAECONNRESET)
			{
				std::cout << "[" << static_cast<int>(cl.m_data.id) << " client] Abortive Close " << std::endl;
				Disconnect(cl.m_data.id);
				break;
			}
			errorDisplay(WSAGetLastError(), "Recv");
		}
		if (recvd_byte == 0)
		{
			std::cout << "[" << static_cast<int>(cl.m_data.id) << " client] Graceful Close " << std::endl;
			Disconnect(cl.m_data.id);
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
			WSARecv(cl.m_socket, &wsabuf, 1, &recvd_byte, &flag, nullptr, nullptr);

			cl.m_data.aniType = static_cast<eAnimationType>(subBuf[0]);
			cl.m_data.upperAniType = static_cast<eUpperAnimationType>(subBuf[1]);
			memcpy(&cl.m_data.pos, &subBuf[2], sizeof(cl.m_data.pos));
			memcpy(&cl.m_data.velocity, &subBuf[14], sizeof(cl.m_data.velocity));
			memcpy(&cl.m_data.yaw, &subBuf[26], sizeof(cl.m_data.yaw));
			break;
		}
		case CS_PACKET_BULLET_FIRE:
		{
			// pos, dir
			char subBuf[12 + 12];
			wsabuf = { sizeof(subBuf), subBuf };
			WSARecv(cl.m_socket, &wsabuf, 1, &recvd_byte, &flag, nullptr, nullptr);

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
				if (c.m_data.id == cl.m_data.id) continue; // 총을 쏜 사람에겐 보내지않음
				WSASend(c.m_socket, &wsabuf, 1, &sent_byte, 0, nullptr, nullptr);
			}
			break;
		}
		default:
			std::cout << "[" << static_cast<int>(cl.m_data.id) << " client] Server Received Unknown Packet" << std::endl;
			break;
		}
	}
}
void NetFramework::AcceptThread(SOCKET socket)
{
	std::cout << "AcceptThread start" << std::endl;
	SOCKADDR_IN clientAddr;
	INT addrSize = sizeof(clientAddr);
	while (true)
	{
		SOCKET cSocket = WSAAccept(socket, reinterpret_cast<sockaddr*>(&clientAddr), &addrSize, nullptr, 0);
		if (cSocket == INVALID_SOCKET) errorDisplay(WSAGetLastError(), "accept");

		CHAR id{ GetNewId() };
		Session& cl = clients[id];

		cl.m_lock.lock();
		cl.m_data.id = id;
		cl.m_data.isActive = true;
		cl.m_data.aniType = eAnimationType::IDLE;
		cl.m_data.upperAniType = eUpperAnimationType::NONE;
		cl.m_socket = cSocket;
		SendLoginOkPacket(id);
		cl.m_lock.unlock();
		char ipInfo[20]{};
		inet_ntop(AF_INET, &clientAddr.sin_addr, ipInfo, sizeof(ipInfo));
		std::cout << "[" << static_cast<int>(cl.m_data.id) << " Session] connect IP: " << ipInfo << std::endl;
		//m_networkThread = thread{ &GameFramework::ProcessClient, this, reinterpret_cast<LPVOID>(g_c_socket) };
		threads.emplace_back( &NetFramework::ProcessRecvPacket, NetFramework::GetInstance(), id);
		m_isAccept = true;
	}
}
void NetFramework::SendPlayerDataPacket()
{
	for (const auto& player : clients)
	{
		if (!player.m_data.isActive) continue;
		sc_packet_update_client packet;
		packet.size = sizeof(packet);
		packet.type = SC_PACKET_UPDATE_CLIENT;
		packet.data = player.m_data;
		char buf[sizeof(packet)];
		memcpy(buf, reinterpret_cast<char*>(&packet), sizeof(packet));
		WSABUF wsabuf{ sizeof(buf), buf };
		DWORD sent_byte;
		for (const auto& other : clients)
		{
			if (!other.m_data.isActive) continue;
			int retVal = WSASend(other.m_socket, &wsabuf, 1, &sent_byte, 0, nullptr, nullptr);
			if (retVal == SOCKET_ERROR)
			{
				if (WSAGetLastError() == WSAECONNRESET)
					std::cout << "Disconnect " << static_cast<int>(other.m_data.id) << " " << other.m_data.id << std::endl;
				else errorDisplay(WSAGetLastError(), "Send");
			}
		}
	}
	// 플레이어의 상체 애니메이션은 한 번 보내고 나면 NONE 상태로 초기화
	for (auto& c : clients)
		c.m_data.upperAniType = eUpperAnimationType::NONE;
}
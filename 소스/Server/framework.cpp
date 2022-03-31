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
		// cs_packet_update_legs
		// size, type, aniType, upperAniType, pos, velocity, yaw
		char buf[1 + 1 + 1 + 1 + 12 + 12 + 4]{};
		WSABUF wsabuf{ sizeof(buf), buf };
		DWORD recvd_byte;
		DWORD flag{ 0 };
		int retVal = WSARecv(cl.m_socket, &wsabuf, 1, &recvd_byte, &flag, nullptr, nullptr);
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
		switch (static_cast<int>(buf[1]))
		{
		case CS_PACKET_UPDATE_LEGS:
			cl.m_data.aniType = static_cast<eAnimationType>(buf[2]);
			cl.m_data.upperAniType = static_cast<eUpperAnimationType>(buf[3]);
			memcpy(&cl.m_data.pos, &buf[4], sizeof(cl.m_data.pos));
			memcpy(&cl.m_data.velocity, &buf[16], sizeof(cl.m_data.velocity));
			memcpy(&cl.m_data.yaw, &buf[28], sizeof(cl.m_data.yaw));
			// 애니메이션 리시브 확인
			// std::cout << id << "PLAYER : " << static_cast<int>(cl.m_data.aniType) << ", " << static_cast<int>(cl.m_data.upperAniType) << std::endl;
			break;
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
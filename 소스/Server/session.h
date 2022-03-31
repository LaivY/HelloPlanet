#pragma once

#include "stdafx.h"
#include "protocol.h"

class Session {
public:
	PlayerData		m_data;
	SOCKET			m_socket;
	char			m_clientBuf[BUF_SIZE];
	std::mutex		m_lock;
	//WSABUF		_wsabuf;
	//WSAOVERLAPPED	_recv_over;
public:
	Session();
	~Session();
};

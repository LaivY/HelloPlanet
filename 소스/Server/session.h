#pragma once

#include "stdafx.h"
#include "protocol.h"

class Session
{
public:
	Session();
	~Session();

public:
	SOCKET		socket;
	PlayerData	data;
	std::mutex	lock;
	//WSABUF		_wsabuf;
	//WSAOVERLAPPED	_recv_over;
};

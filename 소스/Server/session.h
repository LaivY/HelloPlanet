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
	std::mutex	lock;
	PlayerData	data;
	eWeaponType weaponType;
	BOOL		isReady;
	CHAR		name[MAX_NAME_SIZE];
	//WSABUF		_wsabuf;
	//WSAOVERLAPPED	_recv_over;
};

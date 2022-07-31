#pragma once
#include "stdafx.h"

enum COMP_OP { OP_RECV, OP_SEND, OP_ACCEPT };
class EXP_OVER 
{
public:
	WSAOVERLAPPED	_wsa_over;
	COMP_OP			_comp_op;
	WSABUF			_wsa_buf;
	char			_net_buf[BUF_SIZE];
	int				_target;

public:
	EXP_OVER(COMP_OP comp_op, char num_bytes, void* mess) : _comp_op{ comp_op }
	{
		ZeroMemory(&_wsa_over, sizeof(_wsa_over));
		_wsa_buf.buf = reinterpret_cast<char*>(_net_buf);
		_wsa_buf.len = num_bytes;
		memcpy(_net_buf, mess, num_bytes);
	}

	EXP_OVER(COMP_OP comp_op) : _comp_op{ comp_op } { }
	EXP_OVER() : _comp_op{ OP_RECV } { }
	~EXP_OVER() { }
};

enum class STATE { ST_FREE, ST_ACCEPT, ST_INGAME };
class Session
{
public:
	Session();
	~Session();

	void do_recv();
	void do_send(int num_bytes, void* mess);

public:
	// 통신 관련 변수
	SOCKET				socket;
	std::mutex			lock;
	STATE				state;
	EXP_OVER			recv_over;
	INT					prev_size;

	// 게임 관련 변수
	PlayerData			data;					// 클라이언트로 보낼 데이터 구조체
	CHAR				name[MAX_NAME_SIZE];	// 닉네임
	eWeaponType			weaponType;				// 무기 종류
	std::atomic_bool	isReady;				// 로비에서 준비 여부
	BOOL				isAlive;				// 생존 여부
};
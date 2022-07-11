﻿#include "stdafx.h"
#include "session.h"
#include "framework.h"

Session::Session() : socket{}, data{ 0, false, eAnimationType::IDLE, eUpperAnimationType::NONE, {}, {}, {} }, weaponType{ eWeaponType::AR }, isReady{ FALSE }, isAlive{ TRUE },
                     state{ STATE::ST_FREE }, prev_size{ 0 }
{
	strcpy_s(name, "Player\0");
}

Session::~Session()
{
	closesocket(socket);
}

void Session::do_recv()
{
	DWORD recv_flag = 0;
	ZeroMemory(&recv_over._wsa_over, sizeof(recv_over._wsa_over));
	recv_over._wsa_buf.buf = reinterpret_cast<char*>(recv_over._net_buf + prev_size);
	recv_over._wsa_buf.len = sizeof(recv_over._net_buf) - prev_size;
	int ret = WSARecv(socket, &recv_over._wsa_buf, 1, 0, &recv_flag, &recv_over._wsa_over, NULL);
	if (SOCKET_ERROR == ret) 
	{
		int error_num = WSAGetLastError();
		if (ERROR_IO_PENDING != error_num)
		{
			g_networkFramework.Disconnect(data.id);
			errorDisplay(error_num, "do_recv");
		}
	}
}

void Session::do_send(int num_bytes, void* mess)
{
	EXP_OVER* ex_over = new EXP_OVER(OP_SEND, num_bytes, mess);
	int ret = WSASend(socket, &ex_over->_wsa_buf, 1, 0, 0, &ex_over->_wsa_over, NULL);
	if (SOCKET_ERROR == ret) 
	{
		int error_num = WSAGetLastError();
		if (ERROR_IO_PENDING != error_num) errorDisplay(error_num, "do_send");
	}
}

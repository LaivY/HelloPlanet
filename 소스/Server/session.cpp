#include "stdafx.h"
#include "session.h"

Session::Session() : socket{}, data{ 0, false, eAnimationType::IDLE, eUpperAnimationType::NONE, {}, {}, {} }, weaponType{ eWeaponType::AR }, isReady{ FALSE }
{
	strcpy_s(name, "Player\0");
}

Session::~Session()
{
	closesocket(socket);
}

//void do_recv()
//{
//	DWORD recv_flag = 0;
//	ZeroMemory(&_recv_over, sizeof(_recv_over));
//	int ret = WSARecv(m_socket, &_wsabuf, 1, 0, &recv_flag, &_recv_over, recv_callback);
//	if (SOCKET_ERROR == ret) {
//		int error_num = WSAGetLastError();
//		if (ERROR_IO_PENDING != error_num)
//			errorDisplay(WSAGetLastError(), "Recv");
//	}
//}

//void do_send(int num_bytes, void* mess)
//{
//	EXP_OVER* ex_over = new EXP_OVER(OP_SEND, num_bytes, mess);
//	WSASend(m_socket, &ex_over->_wsa_buf, 1, 0, 0, &ex_over->_wsa_over, NULL);
//}

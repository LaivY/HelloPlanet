#pragma once
#include <WS2tcpip.h>
#include <MSWSock.h>
#include <DirectXCollision.h>
#include <DirectXMath.h>
#include <array>
#include <chrono>
#include <iostream>
#include <random>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <fstream>
#include <atomic>
#include <sqlext.h>
#include "protocol.h"
#pragma comment (lib, "WS2_32.LIB")
#pragma comment (lib, "MSWSock.LIB")

//#define DB_MODE
#define IOCP_MODE

void errorDisplay(const int errNum, const char* msg);

class NetworkFramework;
extern NetworkFramework	g_networkFramework;
extern SOCKET			g_socket;
extern HANDLE           g_h_iocp;
extern std::mt19937		g_randomEngine;

namespace Vector3
{
    using namespace DirectX;

    inline XMFLOAT3 Add(const XMFLOAT3& a, const XMFLOAT3& b)
    {
        return XMFLOAT3{ a.x + b.x, a.y + b.y, a.z + b.z };
    }
    inline XMFLOAT3 Sub(const XMFLOAT3& a, const XMFLOAT3& b)
    {
        return XMFLOAT3{ a.x - b.x, a.y - b.y, a.z - b.z };
    }
    inline XMFLOAT3 Mul(const XMFLOAT3& a, const FLOAT& scalar)
    {
        return XMFLOAT3{ a.x * scalar, a.y * scalar, a.z * scalar };
    }
	inline XMFLOAT3 Normalize(const XMFLOAT3& a)
	{
		XMFLOAT3 result;
		XMStoreFloat3(&result, XMVector3Normalize(XMLoadFloat3(&a)));
		return result;
	}
    inline FLOAT Length(const XMFLOAT3& a)
    {
        XMFLOAT3 result;
        XMVECTOR v{ XMVector3Length(XMLoadFloat3(&a)) };
        XMStoreFloat3(&result, v);
        return result.x;
    }
}

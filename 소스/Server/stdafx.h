#pragma once

#include <WS2tcpip.h>
#include <MSWSock.h>
#include <DirectXCollision.h>
#include <DirectXMath.h>
#include <array>
#include <chrono>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include "protocol.h"
#pragma comment (lib, "WS2_32.LIB")
#pragma comment (lib, "MSWSock.LIB")

void errorDisplay(const int errNum, const char* msg);

namespace Utility
{
	using namespace DirectX;
	BoundingOrientedBox GetBoundingBox(const MonsterData& monsterData);
}
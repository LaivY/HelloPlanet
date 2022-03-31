#pragma once

#include <WS2tcpip.h>
#include <MSWSock.h>
#include <DirectXMath.h>
#include <array>
#include <chrono>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#pragma comment (lib, "WS2_32.LIB")
#pragma comment (lib, "MSWSock.LIB")



void errorDisplay(const int errNum, const char* msg);
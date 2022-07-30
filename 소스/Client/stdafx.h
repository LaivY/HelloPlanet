#pragma once
#define FIRSTVIEW
//#define RENDER_HITBOX
#define NETWORK

#include "targetver.h"
#define WIN32_LEAN_AND_MEAN

// C/C++
#include <xaudio2.h>
#include <tchar.h>
#include <windows.h>
#include <wrl.h>
#include <algorithm>
#include <array>
#include <deque>
#include <exception>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <mutex>
#include <random>
#include <ranges>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>
using namespace std;
using Microsoft::WRL::ComPtr;

// DirectX 12
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxguid.lib")
#include <dxgi1_6.h>
#include <dwrite.h>
#include <d2d1_3.h>
#include <d3d11on12.h>
#include <d3d12.h>
#include <d3d12sdklayers.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <DirectXCollision.h>
#include "d3dx12.h"
using namespace DirectX;

// Network
#pragma comment (lib, "WS2_32.LIB")
#pragma comment (lib, "MSWSock.LIB")
#include <WS2tcpip.h>
#include <MSWSock.h>
#include "../Server/protocol.h"

class GameFramework;
class AudioEngine;

extern GameFramework        g_gameFramework;                    // 게임프레임워크
extern AudioEngine          g_audioEngine;                      // 사운드 출력 엔진
extern UINT                 g_maxWidth;                         // 화면 전체 너비
extern UINT                 g_maxHeight;                        // 화면 전체 높이
extern UINT                 g_width;                            // 현재 화면 너비
extern UINT                 g_height;                           // 현재 화면 높이
extern mt19937              g_randomEngine;                     // 랜덤 값 생성에 필요한 엔진

extern ComPtr<ID3D12Device> g_device;                           // DirectX 디바이스
extern UINT                 g_cbvSrvDescriptorIncrementSize;    // 상수버퍼뷰, 셰이더리소스뷰 서술자 힙 크기
extern UINT                 g_dsvDescriptorIncrementSize;       // 깊이스텐실뷰 서술자 힙 크기

extern SOCKET               g_socket;                           // 소켓
extern BOOL                 g_isConnected;                      // 서버 연결 상태
extern thread               g_networkThread;                    // 네트워크 쓰레드
extern string				g_serverIP;							// 서버 아이피
extern mutex                g_mutex;                            // 쓰레드 동기화 뮤텍스

namespace DX
{
	// Helper class for COM exceptions
	class com_exception : public std::exception
	{
	public:
		com_exception(HRESULT hr) : result(hr) {}

		const char* what() const override
		{
			static char s_str[64] = {};
			sprintf_s(s_str, "Failure with HRESULT of %08X",
				static_cast<unsigned int>(result));
			return s_str;
		}

	private:
		HRESULT result;
	};

	// Helper utility converts D3D API failures into exceptions.
	inline void ThrowIfFailed(HRESULT hr)
	{
		if (FAILED(hr))
		{
			throw com_exception(hr);
		}
	}
}

namespace Vector3
{
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
	inline FLOAT Dot(const XMFLOAT3& a, const XMFLOAT3& b)
	{
		return a.x * b.x + a.y * b.y + a.z * b.z;
	}
	inline XMFLOAT3 Cross(const XMFLOAT3& a, const XMFLOAT3& b)
	{
		XMFLOAT3 result;
		XMStoreFloat3(&result, XMVector3Cross(XMLoadFloat3(&a), XMLoadFloat3(&b)));
		return result;
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
	inline XMFLOAT3 TransformCoord(const XMFLOAT3& a, const XMFLOAT4X4& b)
	{
		XMFLOAT3 result;
		XMStoreFloat3(&result, XMVector3TransformCoord(XMLoadFloat3(&a), XMLoadFloat4x4(&b)));
		return result;
	}
	inline XMFLOAT3 TransformNormal(const XMFLOAT3& a, const XMFLOAT4X4& b)
	{
		XMFLOAT3 result;
		XMStoreFloat3(&result, XMVector3TransformNormal(XMLoadFloat3(&a), XMLoadFloat4x4(&b)));
		return result;
	}
	inline XMFLOAT3 Interpolate(const XMFLOAT3& a, const XMFLOAT3& b, const FLOAT& t)
	{
		return XMFLOAT3{ lerp(a.x, b.x, t), lerp(a.y, b.y, t), lerp(a.z, b.z, t) };
	}
	inline void Print(const XMFLOAT3& a, BOOL newLine=TRUE)
	{
		cout << a.x << ", " << a.y << ", " << a.z;
		if (newLine) cout << endl;
	}
}

namespace Matrix
{
	inline XMFLOAT4X4 Mul(const XMFLOAT4X4& a, const XMFLOAT4X4& b)
	{
		XMFLOAT4X4 result;
		XMMATRIX x{ XMLoadFloat4x4(&a) };
		XMMATRIX y{ XMLoadFloat4x4(&b) };
		XMStoreFloat4x4(&result, x * y);
		return result;
	}
	inline XMFLOAT4X4 Transpose(const XMFLOAT4X4& a)
	{
		XMFLOAT4X4 result;
		XMMATRIX m{ XMMatrixTranspose(XMLoadFloat4x4(&a)) };
		XMStoreFloat4x4(&result, m);
		return result;
	}
	inline XMFLOAT4X4 Inverse(const XMFLOAT4X4& a)
	{
		XMFLOAT4X4 result;
		XMMATRIX m{ XMMatrixInverse(NULL, XMLoadFloat4x4(&a)) };
		XMStoreFloat4x4(&result, m);
		return result;
	}
	inline XMFLOAT4X4 Interpolate(const XMFLOAT4X4& a, const XMFLOAT4X4& b, const FLOAT& t)
	{
		XMFLOAT4X4 result;
		for (int i = 0; i < 4; ++i)
			for (int j = 0; j < 4; ++j)
				result.m[i][j] = lerp(a.m[i][j], b.m[i][j], t);
		return result;
	}
	inline XMFLOAT4X4 Identity()
	{
		XMFLOAT4X4 result;
		XMStoreFloat4x4(&result, XMMatrixIdentity());
		return result;
	}
}

namespace Utile
{
	constexpr auto RESOURCE = 1;
	constexpr auto SHADER   = 2;

	ComPtr<ID3D12Resource> CreateBufferResource(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList,
												const void* data, UINT sizePerData, UINT dataCount, D3D12_HEAP_TYPE heapType, D3D12_RESOURCE_STATES resourceState, ID3D12Resource** uploadBuffer = nullptr);
	string PATH(const string& file);
	wstring PATH(const wstring& file);
	int Random(int min, int max);
	float Random(float min, float max);

	template <typename T>
	UINT GetConstantBufferSize()
	{ 
		return (sizeof(T) + 255) & ~255; 
	}
}

namespace Setting
{
	constexpr auto SCREEN_WIDTH     = 1280; // 화면 가로 길이
	constexpr auto SCREEN_HEIGHT    = 720;  // 화면 세로 길이
	constexpr auto MAX_PLAYERS      = 2;    // 최대 플레이어 수(본인 제외)
	constexpr auto MAX_LIGHTS       = 3;    // 씬 조명 최대 개수
	constexpr auto MAX_MATERIALS    = 10;   // 메쉬 재질 최대 개수
	constexpr auto MAX_JOINTS       = 50;   // 메쉬 뼈 최대 개수
	constexpr auto BLENDING_FRAMES  = 5;    // 메쉬 애니메이션 블렌딩에 걸리는 프레임
	constexpr auto CAMERA_MIN_PITCH = -80;  // 카메라 위아래 최소 각도
	constexpr auto CAMERA_MAX_PITCH = 80;   // 카메라 위아래 최대 각도
	constexpr auto SHADOWMAP_COUNT  = 4;    // 케스케이드 그림자맵 개수
}

void error_quit(const char* msg);
void error_display(const char* msg);
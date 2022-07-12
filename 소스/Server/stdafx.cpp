#include "stdafx.h"
#include "framework.h"

NetworkFramework	g_networkFramework{};
SOCKET				g_socket{};
HANDLE				g_h_iocp;
std::mt19937		g_randomEngine{ std::random_device{}() };

void errorDisplay(const int errNum, const char* msg)
{
	WCHAR* lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		nullptr, errNum,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		reinterpret_cast<LPTSTR>(&lpMsgBuf), 0, nullptr);
	std::wcout << errNum << " [" << msg << " Error] " << lpMsgBuf << std::endl;
	//while (true);
	LocalFree(lpMsgBuf);
}

namespace Utility
{
	using namespace DirectX;

	BoundingOrientedBox GetBoundingBox(const MonsterData& monsterData)
	{
		// look벡터는 +z축을 yaw만큼 회전시킨 벡터임
		XMVECTOR look{ XMVectorSet(0.0f, 0.0f, 1.0f, 1.0f) };
		XMMATRIX rotateMatrix{ XMMatrixRotationRollPitchYaw(0.0f, monsterData.yaw, 0.0f) };
		look = XMVector3Transform(look, rotateMatrix);

		// up벡터는 고정
		XMVECTOR up{ XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f) };

		// right 벡터는 up벡터와 look벡터의 외적
		XMVECTOR right{ XMVector3Cross(up, look) };

		// 월드변환행렬은 1열이 right, 2열이 up, 3열이 look, 4열이 위치. 모두 정규화 되어있어야함.
		XMMATRIX worldMatrix{ XMMatrixIdentity() };
		worldMatrix.r[0] = XMVector3Normalize(right);
		worldMatrix.r[1] = up;
		worldMatrix.r[2] = XMVector3Normalize(look);
		worldMatrix.r[3] = XMVectorSet(monsterData.pos.x, monsterData.pos.y, monsterData.pos.z, 1.0f);

		// 중심이 (0, 8, 0)이고 대각선이 (7, 7, 10)인 바운딩박스라고 가정함
		BoundingOrientedBox boundingBox{ XMFLOAT3{ 0.0f, 8.0f, 0.0f }, XMFLOAT3{ 7.0f, 7.0f, 10.0f }, XMFLOAT4{ 0.0f, 0.0f, 0.0f, 1.0f } };
		boundingBox.Transform(boundingBox, worldMatrix);
		return boundingBox;
	}
}
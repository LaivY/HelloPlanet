#include "stdafx.h"
#include "main.h"

int main()
{
	std::wcout.imbue(std::locale("korean"));

	if (g_networkFramework.OnInit_iocp()) return 1;
	//std::cout << "main process start" << std::endl;

	//std::vector<std::thread> worker_threads;
	//for (int i = 0; i < 10; ++i)
	//	worker_threads.emplace_back(&NetworkFramework::WorkThreads);


	//// 1초에 60회 동작하는 루프
	//using frame = std::chrono::duration<int32_t, std::ratio<1, 60>>;
	//using ms = std::chrono::duration<float, std::milli>;
	//std::chrono::time_point<std::chrono::steady_clock> fpsTimer{ std::chrono::steady_clock::now() };

	//frame fps{}, frameCount{};
	//while (true)
	//{
	//	// 아무도 서버에 접속하지 않았으면 패스
	//	if (!g_networkFramework.isAccept)
	//	{
	//		// 이 부분이 없다면 첫 프레임 때 deltaTime이 '클라에서 처음 접속한 시각 - 서버를 켠 시각' 이 된다.
	//		fpsTimer = std::chrono::steady_clock::now();
	//		continue;
	//	}

	//	// 이전 사이클에 얼마나 시간이 걸렸는지 계산
	//	fps = duration_cast<frame>(std::chrono::steady_clock::now() - fpsTimer);

	//	// 아직 1/60초가 안지났으면 패스
	//	if (fps.count() < 1) continue;

	//	if (frameCount.count() & 1) // even FrameNumber
	//	{
	//		// playerData Send
	//		g_networkFramework.SendPlayerDataPacket();
	//		g_networkFramework.SendBulletHitPacket();
	//	}
	//	else // odd FrameNumber
	//	{
	//		// MonsterData Send
	//		g_networkFramework.SendMonsterDataPacket();
	//	}

	//	// 서버에서 해야할 모든 계산 실행
	//	g_networkFramework.Update(duration_cast<ms>(fps).count() / 1000.0f);

	//	// 이번 프레임 계산이 끝난 시각 저장
	//	frameCount = duration_cast<frame>(frameCount + fps);
	//	if (frameCount.count() >= 60)
	//		frameCount = frame::zero();
	//	fpsTimer = std::chrono::steady_clock::now();
	//}

	//for (const auto& c : g_networkFramework.clients)
	//{
	//	if (c.data.isActive)
	//		g_networkFramework.Disconnect(c.data.id);
	//}
	//for (auto& th : worker_threads)
	//	th.join();

	//closesocket(g_socket);
	//WSACleanup();
}

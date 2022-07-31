#include "stdafx.h"
#include "framework.h"
using namespace DirectX;

NetworkFramework::NetworkFramework() :
	isAccept{ false }, readyCount{ 0 }, disconnectCount{ 0 },
	isGameOver{ FALSE }, round{ 1 }, roundGoal{ 20, 20, 20, 1 }, doSpawnMonster{ TRUE }, spawnCooldown{ g_spawnCooldown }, lastMobId{ 0 }, killCount{ 0 }
{
	std::ranges::copy(roundGoal, roundMobCount);
	LoadMapObjects("map.txt");
}

int NetworkFramework::OnInit()
{
	WSADATA WSAData;
	if (WSAStartup(MAKEWORD(2, 2), &WSAData) != 0) return 1;

	g_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);
	if (g_socket == INVALID_SOCKET) errorDisplay(WSAGetLastError(), "Socket");

	// bind
	SOCKADDR_IN server_addr;
	ZeroMemory(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(SERVER_PORT);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	int retVal = bind(g_socket, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr));
	if (retVal == SOCKET_ERROR) errorDisplay(WSAGetLastError(), "Bind");

	// listen
	retVal = listen(g_socket, SOMAXCONN);
	if (retVal == SOCKET_ERROR) errorDisplay(WSAGetLastError(), "Listen");

	g_h_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 0);
	CreateIoCompletionPort(reinterpret_cast<HANDLE>(g_socket), g_h_iocp, 0, 0);

	SOCKET c_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);
	char accept_buf[sizeof(SOCKADDR_IN) * 2 + 32 + 100];
	EXP_OVER accept_ex;
	*(reinterpret_cast<SOCKET*>(&accept_ex._net_buf)) = c_socket;
	ZeroMemory(&accept_ex._wsa_over, sizeof(accept_ex._wsa_over));
	accept_ex._comp_op = OP_ACCEPT;

	bool ret = AcceptEx(g_socket, c_socket, accept_buf, 0, sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, 0, &accept_ex._wsa_over);
	
	std::cout << "Accept Called " << std::endl;;

	for (int i = 0; i < MAX_USER; ++i)
		clients[i].data.id = i;

	for (int i = 0; i < 6; ++i)
		worker_threads.emplace_back(&NetworkFramework::WorkThreads, this);
	for (auto& th : worker_threads)
		th.detach();
	std::cout << "main process start" << std::endl;

	// 1초에 60회 동작하는 루프
	using frame = std::chrono::duration<int32_t, std::ratio<1, 60>>;
	using ms = std::chrono::duration<float, std::milli>;
	std::chrono::time_point<std::chrono::steady_clock> fpsTimer{ std::chrono::steady_clock::now() };

	frame fps{}, frameCount{};
	while (true)
	{
		// 아무도 서버에 접속하지 않았으면 패스
		if (!isAccept)
		{
			// 이 부분이 없다면 첫 프레임 때 deltaTime이 '클라에서 처음 접속한 시각 - 서버를 켠 시각' 이 된다.
			fpsTimer = std::chrono::steady_clock::now();
			continue;
		}

		// 이전 사이클에 얼마나 시간이 걸렸는지 계산
		fps = duration_cast<frame>(std::chrono::steady_clock::now() - fpsTimer);

		// 아직 1/60초가 안지났으면 패스
		if (fps.count() < 1) continue;

		if (frameCount.count() & 1) // even FrameNumber
		{
			// playerData Send
			SendPlayerDataPacket();
			SendBulletDataPacket();
			SendBulletHitPacket();
		}
		else // odd FrameNumber
		{
			// MonsterData Send
			SendMonsterDataPacket();
		}

		// 서버에서 해야할 모든 계산 실행
		Update(duration_cast<ms>(fps).count() / 1000.0f);

		// 이번 프레임 계산이 끝난 시각 저장
		frameCount = duration_cast<frame>(frameCount + fps);
		if (frameCount.count() >= 60)
			frameCount = frame::zero();
		fpsTimer = std::chrono::steady_clock::now();
	}

	for (const auto& c : clients)
	{
		if (c.data.isActive)
			Disconnect(c.data.id);
	}
	for (auto& th : worker_threads)
		th.join();

	closesocket(g_socket);
	WSACleanup();
	return 0;
}

void NetworkFramework::WorkThreads()
{
	std::cout << "worker_threads is start" << std::endl;
	for (;;) {
		DWORD num_byte{};
		LONG64 iocp_key{};
		WSAOVERLAPPED* p_over{};
		BOOL ret = GetQueuedCompletionStatus(g_h_iocp, &num_byte, reinterpret_cast<PULONG_PTR>(&iocp_key), &p_over, INFINITE);
		//std::cout << iocp_key << std::endl;
		int client_id = static_cast<int>(iocp_key);
		EXP_OVER* exp_over = reinterpret_cast<EXP_OVER*>(p_over);
		if (FALSE == ret)
		{
			int err_no = WSAGetLastError();
			errorDisplay(err_no, "GQCS");
			Disconnect(client_id);
			if (exp_over->_comp_op == OP_SEND) delete exp_over;
			continue;
		}

		switch (exp_over->_comp_op) {
		case OP_RECV: {
			if (num_byte == 0) {
				Disconnect(client_id);
				continue;
			}
			Session& cl = clients[client_id];
			int remain_data = num_byte + cl.prev_size;
			char* packet_start = exp_over->_net_buf;
			int packet_size = packet_start[0];

			while (packet_size <= remain_data) {
				ProcessRecvPacket(client_id, packet_start);
				remain_data -= packet_size;
				packet_start += packet_size;
				if (remain_data > 0) packet_size = packet_start[0];
				else break;
			}

			if (0 < remain_data) {
				cl.prev_size = remain_data;
				memcpy(&exp_over->_net_buf, packet_start, remain_data);
			}
			cl.do_recv();
			break;
		}
		case OP_SEND: {
			if (num_byte != exp_over->_wsa_buf.len) {
				Disconnect(client_id);
			}
			delete exp_over;
			break;
		}
		case OP_ACCEPT: {
			std::cout << "Accept Completed." << std::endl;
			SOCKET c_socket = *(reinterpret_cast<SOCKET*>(exp_over->_net_buf));
			CHAR new_id{ GetNewId() };
			if (-1 == new_id) {
				std::cout << "Maxmum user overflow. Accept aborted." << std::endl;
			}
			else {
				Session& cl = clients[new_id];
				cl.lock.lock();
				cl.data.id = new_id;
				cl.data.isActive = true;
				cl.data.aniType = eAnimationType::IDLE;
				cl.data.upperAniType = eUpperAnimationType::NONE;
				cl.socket = c_socket;
				cl.isReady = false;
				constexpr char dummyName[10] = "unknown\0";
				strcpy_s(cl.name, sizeof(dummyName), dummyName);
				cl.weaponType = eWeaponType::AR;
				cl.prev_size = 0;
				cl.recv_over._comp_op = OP_RECV;
				cl.recv_over._wsa_buf.buf = reinterpret_cast<char*>(cl.recv_over._net_buf);
				cl.recv_over._wsa_buf.len = sizeof(cl.recv_over._net_buf);
				ZeroMemory(&cl.recv_over._wsa_over, sizeof(cl.recv_over._wsa_over));
				cl.lock.unlock();

				CreateIoCompletionPort(reinterpret_cast<HANDLE>(c_socket), g_h_iocp, new_id, 0);
				cl.do_recv();
			}

			ZeroMemory(&exp_over->_wsa_over, sizeof(exp_over->_wsa_over));
			c_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);
			*(reinterpret_cast<SOCKET*>(exp_over->_net_buf)) = c_socket;
			AcceptEx(g_socket, c_socket, exp_over->_net_buf + 8, 0, 
				sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, 0, &exp_over->_wsa_over);
		}
		break;
		}
	}
}

void NetworkFramework::ProcessRecvPacket(const int id, char* p)
{
	char packet_type = p[1];
	Session& cl = clients[id];

	switch (packet_type)
	{
	case CS_PACKET_LOGIN:
	{
		cs_packet_login* packet = reinterpret_cast<cs_packet_login*>(p);
#ifdef DB_MODE
		bool retval = try_login_db(packet->name, id);
		if (retval == 1) {
			Disconnect(id);
			return;
		}
#else 
		strcpy_s(cl.name, packet->name);
#endif
		SendLoginOkPacket(cl);
		cl.lock.lock();
		cl.state = STATE::ST_INGAME;
		cl.lock.unlock();
		std::cout << packet->name << " is connect" << std::endl;

		// 재접속 시 disconnectCount를 감소시켜야함
		disconnectCount = max(0, disconnectCount - 1);
		break;
	}
	case CS_PACKET_SELECT_WEAPON:
	{
		// 무기 타입
		cs_packet_select_weapon* packet = reinterpret_cast<cs_packet_select_weapon*>(p);
		cl.weaponType = packet->weaponType;
		SendSelectWeaponPacket(cl);
		std::cout << id << " client is select" << std::endl;
		break;
	}
	case CS_PACKET_READY:
	{
		// 준비 상태
		cs_packet_ready* packet = reinterpret_cast<cs_packet_ready*>(p);
		cl.isReady = packet->isReady;
		SendReadyCheckPacket(cl);

		// 모두 준비되면 게임시작
		for (const auto& c : clients)
		{
			if (!c.data.isActive) continue;
			if (c.isReady) readyCount++;
		}
		if (readyCount >= MAX_USER)
		{
			isAccept = true;		
			SendChangeScenePacket(eSceneType::GAME);
		}
		readyCount = 0;
		std::cout << id << " client is ready" << std::endl;
		break;
	}
	case CS_PACKET_UPDATE_PLAYER:
	{
		cs_packet_update_player* packet = reinterpret_cast<cs_packet_update_player*>(p);
		//int x = cl.data.pos.x;
		//int y = cl.data.pos.y;
		cl.data.aniType = packet->aniType;
		cl.data.upperAniType = packet->upperAniType;
		cl.data.pos = packet->pos;
		cl.data.velocity = packet->velocity;
		cl.data.yaw = packet->yaw;
		break;
	}
	case CS_PACKET_BULLET_FIRE:
	{
		// 총알 정보를 추가해두고 Update함수 때 피격 판정한다.
		auto packet{ reinterpret_cast<cs_packet_bullet_fire*>(p) };
		bullets.emplace_back(packet->data, FALSE, FALSE);
		break;
	}
	case CS_PACKET_SELECT_REWARD:
	{
		auto packet{ reinterpret_cast<cs_packet_select_reward*>(p) };
		++readyCount;

		// 모든 플레이어가 보상 선택을 완료했다면 다음 라운드 진행
		if (readyCount >= MAX_USER)
		{
			doSpawnMonster = TRUE;
			readyCount = 0;
		}
		break;
	}
	case CS_PACKET_PLAYER_STATE:
	{
		auto packet{ reinterpret_cast<cs_packet_player_state*>(p) };
		if (packet->playerState == ePlayerState::DIE)
			clients[static_cast<int>(packet->playerId)].isAlive = FALSE;
		break;
	}
	case CS_PACKET_LOGOUT:
	{
		sc_packet_logout_ok send_packet{};
		send_packet.size = sizeof(send_packet);
		send_packet.type = SC_PACKET_LOGOUT_OK;
		send_packet.id = id;

		char subBuf[sizeof(send_packet)]{};
		WSABUF wsabuf = { sizeof(subBuf), subBuf };
		memcpy(subBuf, &send_packet, sizeof(send_packet));
		DWORD sent_byte;

		// 모든 클라이언트에게 이 플레이어가 나갔다고 송신
		for (const auto& c : clients)
		{
			if (!c.data.isActive) continue;
			int retVal = WSASend(c.socket, &wsabuf, 1, &sent_byte, 0, nullptr, nullptr);
			if (retVal == SOCKET_ERROR)
			{
				if (WSAGetLastError() == WSAECONNRESET)
					std::cout << "[" << static_cast<int>(c.data.id) << " Session] Disconnect(Send(SC_PACKET_LOGOUT_OK))" << std::endl;
				else errorDisplay(WSAGetLastError(), "Send(SC_PACKET_LOGOUT_OK)");
			}
			
		}
		std::cout << "[" << id << " Session] Logout" << std::endl;

		if (++disconnectCount == MAX_USER)
		{
			Reset();
			std::cout << "─RESET─" << std::endl;
		}
		break;
	}
	case CS_PACKET_DEBUG:
	{
		auto packet{ reinterpret_cast<cs_packet_debug*>(p) };
		switch (packet->debugType)
		{
		case eDebugType::KILLALL:
			for (auto& m : monsters)
			{
				BulletData bullet{};
				bullet.damage = m->GetHp();
				m->OnHit(bullet);
			}
			killCount = roundGoal[round - 1];
			break;
		}
		break;
	}
	default:
		std::cout << "[" << static_cast<int>(cl.data.id) << " Session] Server Received Unknown Packet (size : " << 
			static_cast<int>(p[0]) << ", type : "<< static_cast<int>(p[1]) <<")" << std::endl;
		break;
	}
}

void NetworkFramework::Reset()
{
	isAccept = FALSE;
	readyCount = 0;
	disconnectCount = 0;

	isGameOver = FALSE;
	doSpawnMonster = TRUE;

	round = 1;
	std::ranges::copy(roundGoal, roundMobCount);
	spawnCooldown = 0.0f;
	lastMobId = 0;
	killCount = 0;

	for (auto& c : clients)
	{

		c.isReady = FALSE;
		c.isAlive = TRUE;
	}
	bullets.clear();
	bulletHits.clear();
	monsters.clear();
}

void NetworkFramework::Update(const FLOAT deltaTime)
{
	// 몬스터 스폰
	if (monsters.size() < MAX_MONSTER && doSpawnMonster)
		SpawnMonsters(deltaTime);

	// 몬스터 업데이트
	for (auto& m : monsters)
		m->Update(deltaTime);

	// 충돌 체크
	CollisionCheck();

	// 종료 체크
	RoundClearCheck();

	// 게임오버 체크
	GameOverCheck();
}

void NetworkFramework::SpawnMonsters(const FLOAT deltaTime)
{
	// 해당 라운드의 몬스터를 모두 스폰했다면 더 이상 스폰하지 않음
	if (roundMobCount[round - 1] <= 0)
	{
		spawnCooldown = g_spawnCooldown;
		return;
	}

	// 쿨타임 때마다 생성
	if (spawnCooldown <= 0.0f)
	{
		std::unique_ptr<Monster> m;
		switch (round)
		{
		case 1:
			m = std::make_unique<GarooMonster>();
			m->SetRandomPosition();
			break;
		case 2:
			m = std::make_unique<SerpentMonster>();
			m->SetRandomPosition();
			break;
		case 3:
			m = std::make_unique<HorrorMonster>();
			m->SetRandomPosition();
			break;
		case 4:
			m = std::make_unique<UlifoMonster>();
			m->SetPosition(XMFLOAT3{ 0.0f, 1000.0f, 300.0f });
			m->SetYaw(180.0f);
			break;
		}
		m->SetId(lastMobId++);
		m->SetTargetId(DetectPlayer(m->GetPosition()));
		std::cout << static_cast<int>(m->GetId()) << " is generated, capacity: " << monsters.size() << " / " << MAX_MONSTER << std::endl;
		monsters.push_back(std::move(m));

		spawnCooldown = g_spawnCooldown;
		--roundMobCount[round - 1];
	}
	spawnCooldown -= deltaTime;
}

UCHAR NetworkFramework::DetectPlayer(const XMFLOAT3& pos) const
{
	UCHAR index{ 0 };
	float length{ FLT_MAX };
	for (const auto& cl : clients)
	{
		if (!cl.isAlive) continue;
		const float l{ Vector3::Length(Vector3::Sub(cl.data.pos, pos)) };
		if (l < length)
		{
			length = l;
			index = cl.data.id;
		}
	}
	return index;
}

void NetworkFramework::CollisionCheck()
{
	// 총알과 충돌한 몬스터들 중 총알과 가장 가까운 몬스터와
	// 충돌한 것으로 처리해야되기 때문에 총알을 바깥 for문으로 돌아야한다.
	for (auto& b : bullets)
	{
		// 이미 충돌 체크 했으면 패스
		if (b.isCollisionCheck) continue;

		// 충돌 체크 확인
		b.isCollisionCheck = TRUE;

		Monster* hitMonster{ nullptr };	// 피격된 몹
		float length{ FLT_MAX };		// 피격된 몹과 총알 간의 거리

		for (auto& m : monsters)
		{
			if (m->GetHp() <= 0) continue;

			BoundingOrientedBox boundingBox{ m->GetBoundingBox() };
			XMVECTOR origin{ XMLoadFloat3(&b.data.pos) };
			XMVECTOR direction{ XMLoadFloat3(&b.data.dir) };
			float dist{ 3000.0f };
			if (boundingBox.Intersects(origin, direction, dist))
			{
				// 기존 피격된 몹과 총알 간의 거리보다
				// 새로 피격된 몹과 총알 간의 거리가 더 짧으면 피격 당한 몹을 바꿈
				float l{ Vector3::Length(Vector3::Sub(b.data.pos, m->GetPosition())) };
				if (l < length)
				{
					hitMonster = m.get();
					length = l;
					//std::cout << static_cast<int>(m.GetId()) << " is hit" << std::endl;
				}
			}
		}
		if (hitMonster)
		{
			hitMonster->OnHit(b.data);

			// 해당 플레이어가 총알을 맞췄다는 것을 저장
			BulletHitData hitData{};
			hitData.bullet = b.data;
			hitData.mobId = hitMonster->GetId();
			bulletHits.push_back(std::move(hitData));
		} 
	}

	// 충돌체크가 완료된 총알들은 삭제
	std::unique_lock<std::mutex> lock{ g_mutex };
	erase_if(bullets, [](const BulletDataFrame& b) { return b.isCollisionCheck && b.isBulletCast; });
}

void NetworkFramework::RoundClearCheck()
{
	if (killCount >= roundGoal[round - 1])
	{
		std::cout << "[system] Stage" << round << " Clear!" << std::endl;
		for (auto& mob : monsters)
		{
			mob->SetHp(0);
			mob->SetAnimationType(eMobAnimationType::DIE);
		}

		// 라운드 클리어 시 관련 변수 초기화
		for (auto& c : clients)
			c.isAlive = TRUE;
		spawnCooldown = g_spawnCooldown;
		lastMobId = 0;
		killCount = 0;

		// 마지막 라운드 클리어
		if (round == 4)
		{
			SendRoundResultPacket(eRoundResult::ENDING);
			return;
		}

		// 나머지 라운드 클리어
		doSpawnMonster = FALSE;
		SendRoundResultPacket(eRoundResult::CLEAR);
		++round;
	}
}

void NetworkFramework::GameOverCheck()
{
	if (std::ranges::all_of(clients, [](const Session& c) { return !c.isAlive; }) && !isGameOver)
	{
		isGameOver = TRUE;
		SendRoundResultPacket(eRoundResult::OVER);
	}
}

void NetworkFramework::Disconnect(const int id)
{
	Session& cl = clients[id];
	cl.lock.lock();
	cl.data.id = 0;
	cl.data.isActive = false;
	cl.data.aniType = eAnimationType::IDLE;
	cl.data.upperAniType = eUpperAnimationType::NONE;
	cl.data.pos = {};
	cl.data.velocity = {};
	cl.data.yaw = 0.0f;
	cl.isReady = false;
	cl.weaponType = eWeaponType::AR;
	closesocket(cl.socket);
	cl.lock.unlock();
}

CHAR NetworkFramework::GetNewId() const
{
	for (int i = 0; i < MAX_USER; ++i) {
		if (false == clients[i].data.isActive)
		{
			return i;
		}
	}
	std::cout << "Maximum Number of Clients" << std::endl;
	return -1;
}

void NetworkFramework::LoadMapObjects(const std::string& mapFile)
{
	std::ifstream file{ mapFile };

	// 오브젝트 개수
	int count{};
	file >> count;

	for (int i = 0; i < count; ++i)
	{
		int type{};			// 종류
		XMFLOAT3 scale{};	// 크기
		XMFLOAT3 rotat{};	// 회전
		XMFLOAT3 trans{};	// 이동

		file >> type;
		file >> scale.x >> scale.y >> scale.z;
		file >> rotat.x >> rotat.y >> rotat.z;
		file >> trans.x >> trans.y >> trans.z;

		// 서버에서는 히트박스(12)만 필요함
		if (type != 12) continue;

		// 저장된 값은 미터 단위기 때문에 센티미터 단위로 변경
		scale = Vector3::Mul(scale, 100.0f);
		trans = Vector3::Mul(trans, 100.0f);

		// 크기, 회전, 이동 순서로 적용해야 제대로된 히트박스가 생성됨
		// 파일에 저장되어 있는 회전각은 유니티 좌표계이므로 x와 z를 바꿔야 DirectX 좌표계랑 동일함. 따라서 roll, pitch, yaw는 z, x, y가 됨
		XMMATRIX matrix{ XMMatrixIdentity() };
		matrix *= XMMatrixRotationRollPitchYaw(XMConvertToRadians(rotat.x), XMConvertToRadians(rotat.y), XMConvertToRadians(rotat.z));
		matrix *= XMMatrixTranslation(trans.x, trans.y, trans.z);

		// 크기는 바운딩박스를 만들때 Extends에 scale의 절반 값을 넘겨줌. 그 이후 회전, 이동 행렬을 곱함.
		DirectX::BoundingOrientedBox hitbox{ XMFLOAT3{}, Vector3::Mul(scale, 0.5f), XMFLOAT4{ 0.0f, 0.0f, 0.0f, 1.0f } };
		hitbox.Transform(hitbox, matrix);
		hitboxes.push_back(std::move(hitbox));
	}
}

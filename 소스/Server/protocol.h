#pragma once

constexpr short SERVER_PORT = 9000;
constexpr const char* SERVER_IP = "127.0.0.1";
//constexpr const char* SERVER_IP = "59.16.199.246";

constexpr int  BUF_SIZE = 256;
constexpr int  MAX_USER = 3;

constexpr char CS_PACKET_LOGIN = 1;
constexpr char CS_PACKET_UPDATE_LEGS = 2;
constexpr char CS_PACKET_BULLET_FIRE = 3;

constexpr char SC_PACKET_LOGIN_OK = 1;
constexpr char SC_PACKET_UPDATE_CLIENT = 2;

enum class eAnimationType : char
{
	IDLE,
	RUNNING,
	WALKING,
	WALKLEFT,
	WALKRIGHT,
	WALKBACK
};

enum class eUpperAnimationType : char
{
	NONE,
	RELOAD,
	FIRING
};

enum class eMobAnimationType : char
{ // 임시
	IDLE,
	ACK,
	RUNNING,
	DIE
};

#pragma pack (push, 1)
struct PlayerData
{
	CHAR				id;				// 플레이어 고유 번호
	bool				isActive;		// 유효 여부
	eAnimationType		aniType;		// 애니메이션 종류
	eUpperAnimationType upperAniType;	// 상체 애니메이션 종류
	DirectX::XMFLOAT3	pos;			// 위치
	DirectX::XMFLOAT3	velocity;		// 속도
	FLOAT				yaw;			// 회전각
};

struct BulletData
{
	DirectX::XMFLOAT3 pos; // 위치
	DirectX::XMFLOAT3 dir; // 방향
};

struct MonsterData
{
	CHAR				id;				// 몬스터 고유 번호
	CHAR				type;			// 몬스터 타입
	eMobAnimationType	state;			// 애니메이션 종류
	DirectX::XMFLOAT3	pos;			// 위치
	DirectX::XMFLOAT3	velocity;		// 속도
	FLOAT				yaw;			// 회전각
};

struct cs_packet_login
{
	UCHAR			size;
	UCHAR			type;
};

struct cs_packet_update_legs
{
	UCHAR				size;
	UCHAR				type;
	eAnimationType		aniType;
	eUpperAnimationType	upperAniType;
	DirectX::XMFLOAT3	pos;
	DirectX::XMFLOAT3	velocity;
	FLOAT				yaw;
};

struct cs_packet_bullet_fire
{
	UCHAR				size;
	UCHAR				type;
	BulletData			data;
};

struct sc_packet_login_ok
{
	UCHAR			size;
	UCHAR			type;
	PlayerData		data;
};

struct sc_packet_update_client
{
	UCHAR			size;
	UCHAR			type;
	PlayerData		data;
};
#pragma pack(pop)

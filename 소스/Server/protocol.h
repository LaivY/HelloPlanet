﻿#pragma once

constexpr short SERVER_PORT = 9000;
constexpr const char* SERVER_IP = "127.0.0.1";
//constexpr const char* SERVER_IP = "121.173.248.190";

constexpr int  BUF_SIZE = 256;
constexpr int  MAX_USER = 3;
constexpr int  MAX_MONSTER = 10;
constexpr int  MAX_NAME_SIZE = 10;

constexpr char CS_PACKET_LOGIN = 1;
constexpr char CS_PACKET_SELECT_WEAPON = 2;
constexpr char CS_PACKET_READY = 3;
constexpr char CS_PACKET_UPDATE_LEGS = 4;
constexpr char CS_PACKET_BULLET_FIRE = 5;
constexpr char CS_PACKET_BULLET_HIT = 6;
constexpr char CS_PACKET_LOGOUT = 127;

constexpr char SC_PACKET_LOGIN_OK = 1;
constexpr char SC_PACKET_SELECT_WEAPON = 2;
constexpr char SC_PACKET_READY = 3;
constexpr char SC_PACKET_CHANGE_SCENE = 4;
constexpr char SC_PACKET_UPDATE_CLIENT = 5;
constexpr char SC_PACKET_BULLET_FIRE = 6;
constexpr char SC_PACKET_BULLET_HIT = 7;
constexpr char SC_PACKET_UPDATE_MONSTER = 8;
constexpr char SC_PACKET_LOGOUT_OK = 127;

enum class eAnimationType : char
{
	NONE,
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
{
	IDLE,
	WALKING,
	RUNNING,
	ATTACK,
	HIT,
	DIE
};

enum class eClientSceneType : char
{
	LOBBY,
	INGAME,
	ENDING
};

enum class eClientWeaponType : char
{
	AR,
	SG,
	MG
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
	DirectX::XMFLOAT3 pos;		// 위치
	DirectX::XMFLOAT3 dir;		// 방향
	CHAR			  playerId; // 쏜 사람
};

struct BulletHitData
{
	BulletData		bullet;		// 총알
	CHAR			mobId;		// 맞춘 몬스터
};

struct MonsterData
{
	CHAR				id;			// 몬스터 고유 번호
	CHAR				type;		// 몬스터 타입
	eMobAnimationType	aniType;	// 애니메이션 종류
	DirectX::XMFLOAT3	pos;		// 위치
	DirectX::XMFLOAT3	velocity;	// 속도
	FLOAT				yaw;		// 회전각
};

// ---------------------------------

struct cs_packet_login
{
	UCHAR	size;
	UCHAR	type;
	CHAR	name[MAX_NAME_SIZE];
};

struct cs_packet_logout
{
	UCHAR size;
	UCHAR type;
};

struct cs_packet_select_weapon
{
	UCHAR		size;
	UCHAR		type;
	eClientWeaponType	weaponType;
};

struct cs_packet_ready
{
	UCHAR		size;
	UCHAR		type;
	bool		isReady; // true : 준비완료
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
	UCHAR		size;
	UCHAR		type;
	BulletData	data;
};

// ---------------------------------

struct sc_packet_login_ok
{
	UCHAR		size;
	UCHAR		type;
	PlayerData	data;
	CHAR		name[MAX_NAME_SIZE];
};

struct sc_packet_logout_ok
{
	UCHAR	size;
	UCHAR	type;
	CHAR	id;
};

struct sc_packet_ready
{
	UCHAR		size;
	UCHAR		type;
	CHAR		id;
	bool		isReady;
};

struct sc_packet_select_weapon
{
	UCHAR		size;
	UCHAR		type;
	CHAR		id;
	eClientWeaponType	weaponType;
};

struct sc_packet_change_scene
{
	UCHAR		size;
	UCHAR		type;
	eClientSceneType	sceneType;
};

struct sc_packet_update_client
{
	UCHAR		size;
	UCHAR		type;
	PlayerData	data[MAX_USER];
};

struct sc_packet_bullet_fire
{
	UCHAR		size;
	UCHAR		type;
	BulletData	data;
};

struct sc_packet_bullet_hit
{
	UCHAR			size;
	UCHAR			type;
	BulletHitData	data;
};

struct sc_packet_update_monsters
{
	UCHAR		size;
	UCHAR		type;
	MonsterData	data[MAX_MONSTER];
};

#pragma pack(pop)

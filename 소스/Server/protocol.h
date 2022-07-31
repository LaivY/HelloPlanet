#pragma once

constexpr short SERVER_PORT = 9000;
constexpr const char* SERVER_IP = "127.0.0.1";
//constexpr const char* SERVER_IP = "211.216.149.203";

constexpr int  BUF_SIZE = 1600;
constexpr int  MAX_USER = 3;
constexpr int  MAX_MONSTER = 20;
constexpr int  MAX_NAME_SIZE = 10;

constexpr char CS_PACKET_LOGIN = 1;
constexpr char CS_PACKET_SELECT_WEAPON = 2;
constexpr char CS_PACKET_READY = 3;
constexpr char CS_PACKET_UPDATE_PLAYER = 4;
constexpr char CS_PACKET_BULLET_FIRE = 5;
constexpr char CS_PACKET_BULLET_HIT = 6;
constexpr char CS_PACKET_SELECT_REWARD = 7;
constexpr char CS_PACKET_PLAYER_STATE = 8;
constexpr char CS_PACKET_DEBUG = 9;
constexpr char CS_PACKET_LOGOUT = 127;

constexpr char SC_PACKET_LOGIN_CONFIRM = 1;
constexpr char SC_PACKET_SELECT_WEAPON = 2;
constexpr char SC_PACKET_READY_CHECK = 3;
constexpr char SC_PACKET_CHANGE_SCENE = 4;
constexpr char SC_PACKET_UPDATE_CLIENT = 5;
constexpr char SC_PACKET_BULLET_FIRE = 6;
constexpr char SC_PACKET_BULLET_HIT = 7;
constexpr char SC_PACKET_UPDATE_MONSTER = 8;
constexpr char SC_PACKET_MONSTER_ATTACK = 9;
constexpr char SC_PACKET_ROUND_RESULT = 10;
constexpr char SC_PACKET_LOGOUT_OK = 127;

enum class eAnimationType : char
{
	NONE, IDLE,	RUNNING, WALKING, WALKLEFT,	WALKRIGHT, WALKBACK, HIT, DIE
};

enum class eUpperAnimationType : char
{
	NONE, RELOAD, FIRING
};

enum class eMobType : char
{
	GAROO, SERPENT, HORROR, ULIFO
};

enum class eMobAnimationType : char
{
	NONE, IDLE, WALKING, RUNNING, ATTACK, HIT, DIE,
	DOWN, STANDUP, JUMPATK, LEGATK, REST, ROAR  // 보스 애니메이션
};

enum class eSceneType : char
{
	NONE, LOADING, MAIN, LOBBY,	GAME
};

enum class eWeaponType : char
{
	AR,	SG,	MG
};

enum class eRoundResult : char
{
	CLEAR, OVER, ENDING
};

enum class ePlayerState : char
{
	DIE
};

enum class eDebugType : char
{
	KILLALL
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
	DirectX::XMFLOAT3	pos;		// 위치
	DirectX::XMFLOAT3	dir;		// 방향
	INT					damage;		// 데미지
	CHAR				playerId;	// 쏜 사람
};

struct BulletHitData
{
	BulletData		bullet;		// 총알
	CHAR			mobId;		// 맞춘 몬스터
};

struct MonsterData
{
	CHAR				id;			// 몬스터 고유 번호
	eMobType			type;		// 몬스터 타입
	eMobAnimationType	aniType;	// 애니메이션 종류
	DirectX::XMFLOAT3	pos;		// 위치
	DirectX::XMFLOAT3	velocity;	// 속도
	FLOAT				yaw;		// 회전각
};

struct MonsterAttackData
{
	CHAR		id;			// 맞은 사람
	CHAR		mobId;		// 때린 몬스터
	CHAR		damage;		// 데미지
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
	eWeaponType	weaponType;
};

struct cs_packet_ready
{
	UCHAR	size;
	UCHAR	type;
	bool	isReady; // true : 준비완료
};

struct cs_packet_update_player
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

struct cs_packet_select_reward
{
	UCHAR	size;
	UCHAR	type;
	CHAR	playerId;
};

struct cs_packet_player_state
{
	UCHAR			size;
	UCHAR			type;
	CHAR			playerId;
	ePlayerState	playerState;
};

struct cs_packet_debug
{
	UCHAR		size;
	UCHAR		type;
	eDebugType	debugType;
};

// ---------------------------------

struct sc_packet_login_confirm
{
	UCHAR		size;
	UCHAR		type;
	PlayerData	data;
	CHAR		name[MAX_NAME_SIZE];
	bool		isReady;
	eWeaponType	weaponType;
};

struct sc_packet_logout_ok
{
	UCHAR	size;
	UCHAR	type;
	CHAR	id;
};

struct sc_packet_ready_check
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
	eWeaponType	weaponType;
};

struct sc_packet_change_scene
{
	UCHAR		size;
	UCHAR		type;
	eSceneType	sceneType;
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

struct sc_packet_monster_attack
{
	UCHAR				size;
	UCHAR				type;
	MonsterAttackData	data;
};

struct sc_packet_round_result
{
	UCHAR				size;
	UCHAR				type;
	eRoundResult		result;
};
#pragma pack(pop)

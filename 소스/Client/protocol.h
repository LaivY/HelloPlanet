#pragma once

constexpr short SERVER_PORT = 9000;
constexpr const char* SERVER_IP = "127.0.0.1";

constexpr int  BUF_SIZE = 256;
constexpr int  MAX_USER = 1;

constexpr char CS_PACKET_LOGIN = 1;
constexpr char CS_PACKET_UPDATE_LEGS = 2;

constexpr char SC_PACKET_LOGIN_OK = 1;
constexpr char SC_PACKET_UPDATE_CLIENT = 2;

enum class legState : char
{
	IDLE,
	RUNNING,
	WALKING,
	WALKLEFT,
	WALKRIGHT,
	WALKBACK
};

#pragma pack (push, 1)
struct playerData
{
	CHAR			id;
	bool			isActive;
	legState		state;
	XMFLOAT3		pos;
};

struct cs_packet_login
{
	UCHAR			size;
	UCHAR			type;
};

struct cs_packet_update_legs
{
	UCHAR			size;
	UCHAR			type;
	legState		state;
	XMFLOAT3		pos;
};

struct sc_packet_login_ok
{
	UCHAR			size;
	UCHAR			type;
	playerData		data;
};

struct sc_packet_update_client
{
	UCHAR			size;
	UCHAR			type;
	playerData		data;
};
#pragma pack(pop)

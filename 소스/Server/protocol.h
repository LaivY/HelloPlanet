#pragma once

constexpr short SERVER_PORT = 9000;
constexpr const char* SERVER_IP = "127.0.0.1";

constexpr int  BUF_SIZE = 256;
constexpr int  MAX_USER = 1;

constexpr char CS_PACKET_LOGIN = 1;
constexpr char CS_PACKET_UPDATE_LEGS = 2;

constexpr char SC_PACKET_LOGIN_OK = 1;
constexpr char SC_PACKET_UPDATE_CLIENT = 2;

enum class legs_state : char
{
	IDLE,
	RUNNING,
	WALKING,
	WALKLEFT,
	WALKRIGHT,
	WALKBACK
};


#pragma pack (push, 1)
struct c_data
{
	char		_id;
	bool		_in_use;
	legs_state	_state;
};

struct cs_packet_login
{
	unsigned char	size;
	char			type;
};

struct cs_packet_update_legs
{
	unsigned char	size;
	char			type;
	legs_state		state;
};

struct sc_packet_login_ok
{
	unsigned char	size;
	char			type;
	c_data			data;
};

struct sc_packet_update_client
{
	unsigned char	size;
	char			type;
	c_data			data;
};
#pragma pack(pop)

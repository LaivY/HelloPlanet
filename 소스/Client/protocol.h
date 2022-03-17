#pragma once

constexpr short SERVER_PORT = 9000;
constexpr const char* SERVER_IP = "127.0.0.1";

constexpr int  BUF_SIZE = 256;

constexpr int  WORLD_HEIGHT = 8;
constexpr int  WORLD_WIDTH = 8;
constexpr int  MAX_NAME_SIZE = 20;
constexpr int  MAX_USER = 10;

constexpr char CS_PACKET_LOGIN = 1;
constexpr char CS_PACKET_MOVE = 2;

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
struct cs_packet_login {
	unsigned char	size;
	char			type;
	char			name[MAX_NAME_SIZE];
};

struct cs_packet_move {
	unsigned char	size;
	char			type;
	legs_state		state;
};

struct sc_packet_login_ok
{
	unsigned char	size;
	char			type;
};

struct sc_packet_update_client
{
	unsigned char	size;
	char			type;
	legs_state	state;
};
#pragma pack(pop)

#pragma once

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
constexpr char SC_PACKET_MOVE_OBJECT = 2;
constexpr char SC_PACKET_PUT_OBJECT = 3;
constexpr char SC_PACKET_REMOVE_OBJECT = 4;


#pragma pack (push, 1)
struct cs_packet_login {
	unsigned char	size;
	char			type;
	char			name[MAX_NAME_SIZE];
};

struct cs_packet_move {
	unsigned char	size;
	char			type;
	char			direction;			// 0:up, 1:down, 2:left, 3:right
};

struct sc_packet_login_ok {
	unsigned char	size;
	char			type;
	int				id;
	short			x, y;
};

struct sc_packet_move_object {
	unsigned char	size;
	char			type;
	int				id;
	short			x, y;
};

struct sc_packet_put_object {
	unsigned char	size;
	char			type;
	int				id;
	short			x, y;
	char			object_type;
	char			name[MAX_NAME_SIZE];
};

struct sc_packet_remove_object {
	unsigned char	size;
	char			type;
	int				id;
};
#pragma pack(pop)

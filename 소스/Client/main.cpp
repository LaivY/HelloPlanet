#include "main.h"
#define MAX_LOADSTRING 100

// 콘솔
//#ifdef UNICODE
//#pragma comment(linker, "/entry:wWinMainCRTStartup /subsystem:console")
//#else
//#pragma comment(linker, "/entry:WinMainCRTStartup /subsystem:console")
//#endif

// 전역 변수:
HINSTANCE           hInst;                                          // 현재 인스턴스입니다.
WCHAR               szTitle[MAX_LOADSTRING];                        // 제목 표시줄 텍스트입니다.
WCHAR               szWindowClass[MAX_LOADSTRING];                  // 기본 창 클래스 이름입니다.
GameFramework       g_GameFramework(SCREEN_WIDTH, SCREEN_HEIGHT);   // 게임프레임워크
SOCKET              g_c_socket;

// 이 코드 모듈에 포함된 함수의 선언을 전달합니다:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);

// 통신 함수
void ConnectServer();
DWORD APIENTRY ProcessClient(LPVOID arg);
void RecvPacket();
void SendPacket(LPVOID lp_packet);
void error_quit(const char* msg);
void error_display(const char* msg);

int APIENTRY wWinMain(_In_      HINSTANCE hInstance,
	_In_opt_   HINSTANCE hPrevInstance,
	_In_       LPWSTR    lpCmdLine,
	_In_       int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// TODO: 여기에 코드를 입력합니다.

	// 전역 문자열을 초기화합니다.
	LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadStringW(hInstance, IDC_CLIENT, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// 애플리케이션 초기화를 수행합니다:
	if (!InitInstance(hInstance, nCmdShow))
	{
		return FALSE;
	}

	HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_CLIENT));


	MSG msg;

	// 기본 메시지 루프입니다:
	while (TRUE)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT) break;
			if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		else
		{
			g_GameFramework.GameLoop();
		}
	}

	g_GameFramework.OnDestroy();
	return (int)msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEXW wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDC_CLIENT));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = nullptr;
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
	return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	hInst = hInstance; // 인스턴스 핸들을 전역 변수에 저장합니다.

	// 윈도우 사이즈를 프레임워크 생성할 때 정한 너비와 높이로 설정
	RECT rect{ 0, 0, g_GameFramework.GetWindowWidth(), g_GameFramework.GetWindowHeight() };
	DWORD dwStyle{ WS_OVERLAPPED | WS_SYSMENU | WS_BORDER };
	AdjustWindowRect(&rect, dwStyle, FALSE);

	HWND hWnd = CreateWindowW(szWindowClass, szTitle, dwStyle, CW_USEDEFAULT, CW_USEDEFAULT, rect.right - rect.left, rect.bottom - rect.top, nullptr, nullptr, hInstance, nullptr);

	if (!hWnd)
	{
		return FALSE;
	}

	// 프레임워크 초기화
	g_GameFramework.OnInit(hInst, hWnd);

#ifdef NETWORK
	// 서버와 연결
	ConnectServer();
#endif

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_ACTIVATE:
		g_GameFramework.SetIsActive((BOOL)wParam);
		break;
	case WM_MOUSEWHEEL:
	case WM_LBUTTONDOWN:
		g_GameFramework.OnMouseEvent(hWnd, message, wParam, lParam);
		break;
	case WM_KEYDOWN:
	case WM_KEYUP:
		g_GameFramework.OnKeyboardEvent(hWnd, message, wParam, lParam);
		if (wParam == 'w' || wParam == 'W')
		{
			const char test[] = "[KeyUP] : W";
			int send_result = send(g_c_socket, test, sizeof(test), 0);
		}
		if (wParam == 'a' || wParam == 'A')
		{
			const char test[] = "[KeyUP] : A";
			int send_result = send(g_c_socket, test, sizeof(test), 0);
		}
		if (wParam == 'd' || wParam == 'D')
		{
			const char test[] = "[KeyUP] : D";
			int send_result = send(g_c_socket, test, sizeof(test), 0);
		}
		if (wParam == 's' || wParam == 'S')
		{
			const char test[] = "[KeyUP] : S";
			int send_result = send(g_c_socket, test, sizeof(test), 0);
		}
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

void ConnectServer()
{
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return;

	// socket 생성
	g_c_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (g_c_socket == INVALID_SOCKET) error_quit("socket()");

	// connect
	SOCKADDR_IN server_address;
	ZeroMemory(&server_address, sizeof(server_address));
	server_address.sin_family = AF_INET;
	inet_pton(AF_INET, SERVER_IP, &(server_address.sin_addr.s_addr));
	//server_address.sin_addr.s_addr = inet_addr(SERVER_IP);
	server_address.sin_port = htons(SERVER_PORT);
	const int return_value = connect(g_c_socket, reinterpret_cast<SOCKADDR*>(&server_address), sizeof(server_address));
	if (return_value == SOCKET_ERROR) error_quit("connect()");

	// 스레드 생성
	const HANDLE h_thread = CreateThread(nullptr, 0, ProcessClient, reinterpret_cast<LPVOID>(g_c_socket), 0, nullptr);
	if (h_thread == nullptr) closesocket(g_c_socket);
}

DWORD APIENTRY ProcessClient(LPVOID arg)
{
	while (true)
	{
		RecvPacket();
	}
	return 0;
}

void RecvPacket()
{
	constexpr int head_size = 2;
	char head_buf[head_size];
	char net_buf[BUF_SIZE];

	int recv_result = recv(g_c_socket, head_buf, head_size, MSG_WAITALL);
	if (recv_result == SOCKET_ERROR) { error_display("recv()"); return; }

	int packet_size = head_buf[0];
	int packet_type = head_buf[1];
	cout << "[패킷테스트]: " << packet_size << ", " << packet_type << endl;
	switch (packet_type)
	{
	case SC_PACKET_LOGIN_OK: {
		sc_packet_login_ok recv_packet;
		recv_result += recv(g_c_socket, reinterpret_cast<char*>(&recv_packet) + head_size, packet_size - head_size, MSG_WAITALL);
		cout << "[SC_PACKET_LOGIN_OK] received" << endl;
		break;
	}
	case SC_PACKET_MOVE_OBJECT: {
		sc_packet_move_object recv_packet;
		recv_result += recv(g_c_socket, reinterpret_cast<char*>(&recv_packet) + head_size, packet_size - head_size, MSG_WAITALL);
		cout << "[SC_PACKET_MOVE_OBJECT] received" << endl;
		break;
	}
	case SC_PACKET_PUT_OBJECT: {
		sc_packet_put_object recv_packet;
		recv_result += recv(g_c_socket, reinterpret_cast<char*>(&recv_packet) + head_size, packet_size - head_size, MSG_WAITALL);
		cout << "[SC_PACKET_PUT_OBJECT] received" << endl;
		break;
	}
	case SC_PACKET_REMOVE_OBJECT: {
		sc_packet_remove_object recv_packet;
		recv_result += recv(g_c_socket, reinterpret_cast<char*>(&recv_packet) + head_size, packet_size - head_size, MSG_WAITALL);
		cout << "[SC_PACKET_REMOVE_OBJECT] received" << endl;
		break;
	}
	default: {
		char garbage[20];
		recv_result += recv(g_c_socket, garbage, packet_size - 2, MSG_WAITALL);
		cout << "[Unknown_Packet] received" << endl;
		break;
	}
	}
}

void SendPacket(LPVOID lp_packet)
{
	const auto send_packet = static_cast<char*>(lp_packet);
	int send_result = send(g_c_socket, send_packet, *send_packet, 0);
	cout << "[Send_Packet] Type: " << *(send_packet + 1) << " , Byte: " << *send_packet << endl;
}

void error_quit(const char* msg)
{
	WCHAR* lp_msg_buf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		nullptr, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		reinterpret_cast<LPTSTR>(&lp_msg_buf), 0, nullptr);
	MessageBox(nullptr, reinterpret_cast<LPCTSTR>(lp_msg_buf), reinterpret_cast<LPCWSTR>(msg), MB_ICONERROR);
	LocalFree(lp_msg_buf);
	exit(1);
}

void error_display(const char* msg)
{
	WCHAR* lp_msg_buf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		nullptr, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		reinterpret_cast<LPTSTR>(&lp_msg_buf), 0, nullptr);
	wcout << "[" << msg << "]" << lp_msg_buf << endl;
	LocalFree(lp_msg_buf);
}
#include "stdafx.h"
#include "resource.h"
#include "framework.h"
#define MAX_LOADSTRING 100

// 콘솔
//#ifdef UNICODE
//#pragma comment(linker, "/entry:wWinMainCRTStartup /subsystem:console")
//#else
//#pragma comment(linker, "/entry:WinMainCRTStartup /subsystem:console")
//#endif

// 전역 변수:
HINSTANCE           hInst;
WCHAR               szTitle[MAX_LOADSTRING];
WCHAR               szWindowClass[MAX_LOADSTRING];

// 이 코드 모듈에 포함된 함수의 선언을 전달합니다:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_		HINSTANCE hInstance,
					  _In_opt_	HINSTANCE hPrevInstance,
					  _In_		LPWSTR    lpCmdLine,
					  _In_		int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// 명령행 인자로부터 서버 IP를 받음
	wstring ip{ lpCmdLine };
	if (!ip.empty())
	{
		g_serverIP.clear();
		ranges::transform(ip, back_inserter(g_serverIP), [](wchar_t c) { return static_cast<char>(c); });
	}

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
			g_gameFramework.GameLoop();
		}
	}
	return (int) msg.wParam;
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
	RECT rect{ 0, 0, static_cast<LONG>(Setting::SCREEN_WIDTH), static_cast<LONG>(Setting::SCREEN_HEIGHT) };
	DWORD dwStyle{ WS_OVERLAPPED | WS_SYSMENU | WS_BORDER };
	//AdjustWindowRect(&rect, dwStyle, FALSE);

	HWND hWnd = CreateWindowW(szWindowClass, szTitle, dwStyle, CW_USEDEFAULT, CW_USEDEFAULT, rect.right - rect.left, rect.bottom - rect.top, nullptr, nullptr, hInstance, nullptr);
	if (!hWnd)
	{
		return FALSE;
	}

	// 프레임워크 초기화
	g_gameFramework.OnInit(hInst, hWnd);

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_ACTIVATE:
		g_gameFramework.SetIsActive(static_cast<BOOL>(wParam));
		break;
	case WM_SIZE:
		g_gameFramework.OnResize(hWnd, message, wParam, lParam);
		break;
	case WM_MOVE:
		g_gameFramework.OnMove(hWnd, message, wParam, lParam);
		break;
	case WM_MOUSEMOVE:
	case WM_MOUSEWHEEL:
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
		g_gameFramework.OnMouseEvent(hWnd, message, wParam, lParam);
		break;
	case WM_CHAR:
	case WM_KEYUP:
	case WM_KEYDOWN:
		g_gameFramework.OnKeyboardEvent(hWnd, message, wParam, lParam);
		break;
	case WM_DESTROY:
		g_gameFramework.OnDestroy();
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

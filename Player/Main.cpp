// 製品 exe（AsteroidPlayer.exe）のエントリポイント
// エディタ版（Editor/Main.cpp）からメニューバーとエディタの UI を除いたもの

#include "Code/Manager/Main.h"
#include "Player/GameApp.h"
#include <exception>


#define APP_NAME "AsteroidPlayer"
#define CLASS_NAME "AsteroidPlayer"


HINSTANCE   g_Instance;
HWND        g_Window;
bool        g_FullWindow;
int         g_WindowWidth;
int         g_WindowHeight;

std::unique_ptr<Player::GameApp> g_GameApp = nullptr;


LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);


int APIENTRY wWinMain(  _In_ HINSTANCE hInstance,
						_In_opt_ HINSTANCE hPrevInstance,
						_In_ LPWSTR    lpCmdLine,
						_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	g_Instance = hInstance;
	g_FullWindow = false;

	int windowWidth = 1280;
	int windowHeight = 720;

	{
		WNDCLASSEX wcex;
		wcex.cbSize = sizeof(WNDCLASSEX);
		wcex.style = 0;
		wcex.lpfnWndProc = WndProc;
		wcex.cbClsExtra = 0;
		wcex.cbWndExtra = 0;
		wcex.hInstance = g_Instance;
		wcex.hIcon = nullptr;
		wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
		wcex.hbrBackground = nullptr;
		wcex.lpszMenuName = nullptr;
		wcex.lpszClassName = CLASS_NAME;
		wcex.hIconSm = nullptr;

		RegisterClassEx(&wcex);

		RECT rc = { 0, 0, (LONG)windowWidth, (LONG)windowHeight };
		AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

		g_WindowWidth = rc.right - rc.left;
		g_WindowHeight = rc.bottom - rc.top;

		g_Window = CreateWindow(CLASS_NAME, APP_NAME, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
			rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, g_Instance, nullptr);
	}

	{
		g_GameApp = std::make_unique<Player::GameApp>(g_Window);

		ShowWindow(g_Window, SW_SHOW);

		//フレームカウント初期化
		DWORD dwExecLastTime;
		DWORD dwCurrentTime;
		timeBeginPeriod(1);
		dwExecLastTime = timeGetTime();
		dwCurrentTime = 0;

		g_GameApp->Initialize();

		while (true)
		{
			MSG msg;

			if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
			{
				if (msg.message == WM_QUIT)
				{
					break;
				}
				else
				{
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
			}
			else
			{
				dwCurrentTime = timeGetTime();

				if ((dwCurrentTime - dwExecLastTime) >= (1000 / 60 / 5))
				{
					g_GameApp->Update();
					try {
						g_GameApp->Draw();
					}
					catch (const std::exception& error) {
						// Do not submit this partially recorded frame or draw with stale constants.
						OutputDebugStringA(error.what());
						MessageBoxA(g_Window, error.what(), "Rendering stopped", MB_OK | MB_ICONERROR);
						break;
					}

					dwExecLastTime = dwCurrentTime;
				}
				else
				{
					Sleep(0);
				}
			}
		}
	}

	return 0;
}


void ChangeFullWindow()
{
	g_FullWindow = !g_FullWindow;

	if (g_FullWindow)
	{
		SetWindowLong(g_Window, GWL_STYLE, WS_POPUP);
		ShowWindow(g_Window, SW_MAXIMIZE);
	}
	else
	{
		SetWindowLong(g_Window, GWL_STYLE, WS_OVERLAPPEDWINDOW);

		SetWindowPos(g_Window, NULL, 0, 0, g_WindowWidth, g_WindowHeight, (SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED));
		ShowWindow(g_Window, SW_NORMAL);
	}
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_SIZE:
		if (g_GameApp && wParam != SIZE_MINIMIZED) {
			g_GameApp->Resize((unsigned int)LOWORD(lParam), (unsigned int)HIWORD(lParam));
		}
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	case WM_KEYDOWN:
		switch (wParam)
		{
		case VK_ESCAPE:
			DestroyWindow(hWnd);
			break;
		case VK_F11:
			ChangeFullWindow();
			break;
		}
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}

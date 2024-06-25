#define UNICODE
#define _UNICODE

#include <stdbool.h>
#include <windows.h>

#include "resource.h"

// TODO: unpin all windows on close

#define TRAY_MSG (WM_USER + 0x100)
#define IDM_EXIT 100
#define IDM_UNPINALL 101

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
bool unpinAllWindow(); // TODO: implement this shit
bool toggleWindowOnTop(HWND hWnd);
bool isWindowTopMost(HWND hWnd);
bool createTrayIcon();
bool initTrayContextMenu();
bool showTrayContextMenu();
void msgBoxErr();

HWND hMainWindow;
HINSTANCE hMainInstance;

HMENU hTrayMenu;
HICON hIcon;
NOTIFYICONDATA nid;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam,
						 LPARAM lParam) {
	switch (message) {
	case WM_CREATE: {
		hMainWindow = hWnd;
		break;
	}
	case WM_CLOSE: {
		PostQuitMessage(0);
		break;
	}
	case WM_DESTROY: {
		UnregisterClass(TEXT("mudhumyai"), hMainInstance);
		LocalFree(hIcon);
		// free everything here later
		break;
	}
	case WM_HOTKEY: {
		HWND active_window = GetForegroundWindow();
		if (active_window == NULL)
			break;
		if (toggleWindowOnTop(active_window)) {
			PlaySound(MAKEINTRESOURCE(IDR_WAVE1), GetModuleHandle(NULL),
					  SND_ASYNC | SND_NODEFAULT | SND_RESOURCE);
		}
		break;
	}
	case TRAY_MSG: {
		switch (LOWORD(lParam)) {
		case NIN_SELECT:
		case NIN_KEYSELECT:
		case WM_CONTEXTMENU: {
			showTrayContextMenu(); // TODO: handle when this shit error
			break;
		}
		}
	}
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
}

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
					 LPSTR pCmdLine, int nCmdShow) {
	LPCWSTR class_name = TEXT("mudhumyai");
	LPCWSTR window_name = TEXT("PinWindow");
	hIcon = (HICON)LoadImage(hInstance, MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON,
							 0, 0, LR_DEFAULTCOLOR);
	if (hIcon == NULL) {
		msgBoxErr(); // Fail to load icon
		PostQuitMessage(0);
	}

	MSG msg = {0};
	WNDCLASS wc = {0};
	wc.lpfnWndProc = WndProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = class_name;
	wc.hIcon = hIcon;
	if (!RegisterClass(&wc)) {
		msgBoxErr(); // Fail to register class
		PostQuitMessage(0);
	}

	hMainWindow = CreateWindowEx(WS_EX_CLIENTEDGE, class_name, window_name,
								 WS_OVERLAPPEDWINDOW, CW_USEDEFAULT,
								 CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
								 NULL, NULL, hInstance, NULL);
	if (hMainWindow == NULL) {
		msgBoxErr(); // Fail to create window
		PostQuitMessage(0);
	}

	hMainInstance = wc.hInstance;

	if (!RegisterHotKey(hMainWindow, 1, MOD_WIN | MOD_CONTROL,
						0x54)) { // WIN+CTRL+t
		msgBoxErr();			 // Fail to register hotkey
		PostQuitMessage(0);
	}

	if (!createTrayIcon()) {
		msgBoxErr(); // Fail to create tray icon
		PostQuitMessage(0);
	}

	if (!initTrayContextMenu()) {
		msgBoxErr(); // Fail to create tray popup menu
		PostQuitMessage(0);
	}

	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}

bool createTrayIcon() {
	nid.cbSize = sizeof(nid);
	nid.uID = 1;
	nid.hWnd = hMainWindow;
	nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;
	nid.uCallbackMessage = TRAY_MSG;
	nid.uVersion = NOTIFYICON_VERSION_4; // NOTIFYICON_VERSION_4
	nid.hIcon = hIcon;
	memcpy(nid.szTip, TEXT("Pin Window Util"), 32);

	if (Shell_NotifyIcon(NIM_ADD, &nid)) {
		Shell_NotifyIcon(NIM_SETVERSION, &nid);
		return true;
	} else {
		return false;
	}
}

bool initTrayContextMenu() {
	hTrayMenu = CreatePopupMenu();
	if (hTrayMenu == NULL) {
		return false;
	}
	AppendMenu(hTrayMenu, MF_POPUP | MF_STRING | MF_DISABLED, IDM_UNPINALL,
			   TEXT("Unpin all windows")); // TODO: enable this shit
	AppendMenu(hTrayMenu, MF_POPUP | MF_SEPARATOR, 999, NULL);
	AppendMenu(hTrayMenu, MF_POPUP | MF_STRING, IDM_EXIT, TEXT("Exit"));
	return true;
}

bool showTrayContextMenu() {
	SetForegroundWindow(
		hMainWindow); // need cuz menu wont hide when unfocus wat

	POINT pt;
	GetCursorPos(&pt);
	int cmd = TrackPopupMenu(hTrayMenu,
							 TPM_RETURNCMD | TPM_LEFTBUTTON | TPM_LEFTALIGN,
							 pt.x, pt.y, 0, hMainWindow, NULL);

	PostMessage(
		hMainWindow, WM_NULL, 0,
		0); // https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-trackpopupmenu
			// REMARKS

	switch (cmd) {
	case IDM_UNPINALL: {
		PlaySound(MAKEINTRESOURCE(IDR_WAVE1), GetModuleHandle(NULL),
				  SND_ASYNC | SND_NODEFAULT | SND_RESOURCE);
		break;
	}
	case IDM_EXIT: {
		Shell_NotifyIcon(NIM_DELETE, &nid);
		DestroyMenu(hTrayMenu);
		PostQuitMessage(0);
		break;
	}
	}
	return true;
}

bool toggleWindowOnTop(HWND hWnd) {
	HWND prop = isWindowTopMost(hWnd) ? HWND_NOTOPMOST : HWND_TOPMOST;
	if (SetWindowPos(hWnd, prop, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE))
		return true;
	else
		return false;
}

bool isWindowTopMost(HWND hWnd) {
	int ex_style = GetWindowLong(hWnd, GWL_EXSTYLE);
	return (ex_style & WS_EX_TOPMOST) == WS_EX_TOPMOST;
}

void msgBoxErr() {
	DWORD dw = GetLastError();
	void *buf;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
				  NULL, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				  (LPTSTR)&buf, 0, NULL);
	MessageBox(NULL, (LPCWSTR)buf, TEXT("Error"),
			   MB_ICONERROR | MB_OK | MB_TOPMOST);
	LocalFree(buf);
}

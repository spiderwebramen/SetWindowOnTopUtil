#define UNICODE
#define _UNICODE

#include <assert.h>
#include <dwmapi.h>
#include <stdbool.h>
#include <windows.h>

#include "resource.h"

// TODO: unpin all windows on close

#define BORDER_THICKNESS 3
#define BORDER_COLOR GetSysColor(COLOR_HIGHLIGHT)
#define TRANSPARENT_COLOR_KEY RGB(69, 69, 69)

#define TRAY_MSG (WM_USER + 0x100)
#define IDM_EXIT 100
#define IDM_UNPINALL 101

#define HKID_TOGGLEONTOP 1
#define HKID_TEST 2
#define HKID_TEST_QUIT 10

#define TID_BORDERUPDATE 1
#define T_BORDERUPDATE_MS 100

#define PROP_ATTACHED_WINDOW TEXT("PROP_ATTACHED_WINDOW")

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK BorderWndProc(HWND hWnd, UINT message, WPARAM wParam,
							   LPARAM lParam);
bool unpinAllWindow(); // TODO: implement this shit
bool drawBorder(HWND hWnd);
bool toggleWindowOnTop(HWND hWnd);
bool isWindowTopMost(HWND hWnd);
bool createTrayIcon();
bool initTrayContextMenu();
bool showTrayContextMenu();
void msgBoxErr();

LPCWSTR MAIN_CLASS_NAME = TEXT("aiyaMainClass");
LPCWSTR BORDER_CLASS_NAME = TEXT("aiyaBorderClass");

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
		DestroyMenu(hTrayMenu);
		UnregisterClass(TEXT("mudhumyai"), hMainInstance);
		UnregisterClass(TEXT("mudhumlek"), hMainInstance);
		UnregisterHotKey(hMainWindow, HKID_TOGGLEONTOP);
		UnregisterHotKey(hMainWindow, HKID_TEST);
		LocalFree(hIcon);
		// free everything here
		break;
	}
	case WM_HOTKEY: {
		switch (wParam) {
		case HKID_TOGGLEONTOP: {
			HWND active_window = GetForegroundWindow();
			if (active_window == NULL)
				break;
			if (toggleWindowOnTop(active_window)) {
				PlaySound(MAKEINTRESOURCE(IDR_WAVE1), GetModuleHandle(NULL),
						  SND_ASYNC | SND_NODEFAULT | SND_RESOURCE);
			}
			break;
		}
		case HKID_TEST: {
			drawBorder(NULL);
			PlaySound(MAKEINTRESOURCE(IDR_WAVE1), GetModuleHandle(NULL),
					  SND_ASYNC | SND_NODEFAULT | SND_RESOURCE);
			break;
		}
		case HKID_TEST_QUIT: {
			PostQuitMessage(0);
			break;
		}
		default:
			assert(0 && "unreachable");
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
		break;
	}
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0; // TODO: properly handle each case return value
}

LRESULT CALLBACK BorderWndProc(HWND hWnd, UINT message, WPARAM wParam,
							   LPARAM lParam) {
	switch (message) {
	case WM_PAINT: {
		PAINTSTRUCT ps = {0};
		HDC hdc = BeginPaint(hWnd, &ps);

		RECT rc = {0};
		GetClientRect(hWnd, &rc);

		HPEN hPen = CreatePen(PS_SOLID, BORDER_THICKNESS, BORDER_COLOR);
		HBRUSH hBrush = CreateSolidBrush(TRANSPARENT_COLOR_KEY);
		HGDIOBJ hOldPen = SelectObject(hdc, hPen);
		HGDIOBJ hOldBrush = SelectObject(hdc, hBrush);

		Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);

		if (hOldPen)
			SelectObject(hdc, hOldPen);
		if (hOldBrush)
			SelectObject(hdc, hOldBrush);
		if (hPen)
			DeleteObject(hPen);
		if (hBrush)
			DeleteObject(hBrush);

		EndPaint(hWnd, &ps);
		break;
	}
	case WM_TIMER: {
		if (wParam == TID_BORDERUPDATE) {
			HWND hAttachedWindow = GetProp(hWnd, PROP_ATTACHED_WINDOW);
			RECT attached_window_rect = {0};
			DwmGetWindowAttribute(hAttachedWindow, DWMWA_EXTENDED_FRAME_BOUNDS,
								  &attached_window_rect,
								  sizeof(attached_window_rect));
			SetWindowPos(hAttachedWindow, hWnd, 0, 0, 0, 0,
						 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
			SetWindowPos(hWnd, 0, attached_window_rect.left,
						 attached_window_rect.top,
						 attached_window_rect.right - attached_window_rect.left,
						 attached_window_rect.bottom - attached_window_rect.top,
						 SWP_SHOWWINDOW | SWP_NOACTIVATE);
			RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE);
			// TODO: free attached_window_rect
		}
		break;
	}
	// case WM_NCHITTEST:
	// 	return HTCAPTION; // to be able to drag the window around
	// 	break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0; // TODO: properly handle each case return value
}

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/,
					 LPSTR /*pCmdLine*/, int /*nCmdShow*/) {
	LPCWSTR window_name = TEXT("SetWindowOnTop");
	hIcon = (HICON)LoadImage(hInstance, MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON,
							 0, 0, LR_DEFAULTCOLOR);
	if (hIcon == NULL) {
		msgBoxErr(); // Fail to load icon
		PostQuitMessage(0);
	}

	MSG msg = {0};
	WNDCLASS main_wc = {0};
	main_wc.lpfnWndProc = WndProc;
	main_wc.hInstance = hInstance;
	main_wc.lpszClassName = MAIN_CLASS_NAME;
	main_wc.hIcon = hIcon;
	if (!RegisterClass(&main_wc)) {
		msgBoxErr(); // Fail to register main class
		PostQuitMessage(0);
	}

	WNDCLASS border_wc = {0};
	border_wc.lpfnWndProc = BorderWndProc;
	border_wc.hInstance = hMainInstance;
	border_wc.lpszClassName = BORDER_CLASS_NAME;
	border_wc.hIcon = hIcon;
	if (!RegisterClass(&border_wc)) {
		msgBoxErr(); // Fail to register border class
		PostQuitMessage(0);
	}

	hMainWindow = CreateWindowEx(WS_EX_CLIENTEDGE, MAIN_CLASS_NAME, window_name,
								 WS_OVERLAPPEDWINDOW, CW_USEDEFAULT,
								 CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
								 NULL, NULL, hInstance, NULL);
	if (hMainWindow == NULL) {
		msgBoxErr(); // Fail to create main window
		PostQuitMessage(0);
	}

	hMainInstance = main_wc.hInstance;

	if (!RegisterHotKey(hMainWindow, HKID_TOGGLEONTOP,
						MOD_WIN | MOD_CONTROL | MOD_NOREPEAT,
						0x54)) { // WIN+CTRL+t
		msgBoxErr();			 // Fail to register hotkey
		PostQuitMessage(0);
	}

	if (!RegisterHotKey(hMainWindow, HKID_TEST,
						MOD_WIN | MOD_CONTROL | MOD_NOREPEAT,
						0x55)) { // WIN+CTRL+u
		msgBoxErr();			 // Fail to register hotkey
		PostQuitMessage(0);
	}

	if (!RegisterHotKey(hMainWindow, HKID_TEST_QUIT,
						MOD_WIN | MOD_CONTROL | MOD_SHIFT | MOD_NOREPEAT,
						0x51)) { // WIN+CTRL+SHIFT+q
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
	nid.uVersion = NOTIFYICON_VERSION_4;
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

bool drawBorder(HWND /*hWnd*/) {
	HWND hBorderWindow = CreateWindowEx(
		WS_EX_LAYERED | WS_EX_TOOLWINDOW, BORDER_CLASS_NAME, TEXT(""),
		WS_VISIBLE | WS_POPUP, 0, 0, 0, 0, NULL, NULL, hMainInstance, NULL);

	if (hBorderWindow == NULL) {
		msgBoxErr(); // Fail to create border window
		return false;
	}

	if (!SetLayeredWindowAttributes(hBorderWindow, TRANSPARENT_COLOR_KEY, 255,
									LWA_COLORKEY)) {
		msgBoxErr(); // Fail to set transparent color key
		return false;
	}

	HWND hActiveWindow = GetForegroundWindow();
	SetProp(hBorderWindow, PROP_ATTACHED_WINDOW, hActiveWindow);

	SetTimer(hBorderWindow, TID_BORDERUPDATE, T_BORDERUPDATE_MS, NULL);
	// TODO: handle return val
	// TODO: UOI_TIMERPROC_EXCEPTION_SUPPRESSION flag set
	// https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-settimer
	// REMARKS

	// TODO: remove prop on border delete

	return true;
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

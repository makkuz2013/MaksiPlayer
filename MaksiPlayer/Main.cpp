#include <Windows.h>
#include <mmsystem.h>
#include <commdlg.h>
#include <vector>
#include <string>
#include <fstream>
#include <locale>
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "winmm.lib")

void EnableModernWindow(HWND hwnd)
{
	BOOL dark = TRUE;
	DwmSetWindowAttribute(hwnd, 20, &dark, sizeof(dark));
}

HFONT hFont = CreateFontW(
	-14, 0, 0, 0,
	FW_NORMAL, FALSE, FALSE, FALSE,
	DEFAULT_CHARSET,
	OUT_DEFAULT_PRECIS,
	CLIP_DEFAULT_PRECIS,
	CLEARTYPE_QUALITY,
	DEFAULT_PITCH | FF_DONTCARE,
	L"Segoe UI"
);

const wchar_t* AppVersion = L"v1.1";

std::vector<std::wstring> g_playlist;
std::wstring g_currentFile;

enum PlayerState { STOPPED, PLAYING, PAUSED };
PlayerState g_playerState = STOPPED;

// Function prototype
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

const wchar_t* PLAYLIST_FILE = L"playlist.txt";

std::wstring GetPlaylistPath()
{
	wchar_t exePath[MAX_PATH];
	GetModuleFileName(NULL, exePath, MAX_PATH);

	std::wstring path = exePath;
	size_t pos = path.find_last_of(L"\\/");
	return path.substr(0, pos + 1) + L"playlist.txt"; // next to exe
}

void SavePlaylist()
{
	std::wofstream file(GetPlaylistPath());
	if (!file.is_open()) return;

	for (int i = 0; i < g_playlist.size(); i++)
	{
		file << g_playlist[i] << L"\n";
	}

	file.close();
}

void LoadPlaylist(HWND hwnd)
{
	std::wifstream file(GetPlaylistPath());
	if (!file.is_open()) return;

	g_playlist.clear();
	HWND hList = GetDlgItem(hwnd, 200);
	SendMessage(hList, LB_RESETCONTENT, 0, 0);

	std::wstring line;
	while (std::getline(file, line))
	{
		if (line.empty()) continue;

		g_playlist.push_back(line);

		size_t pos = line.find_last_of(L"\\/");
		std::wstring fileOnly = line.substr(pos + 1);

		SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)fileOnly.c_str());
	}
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
	const wchar_t CLASS_NAME[] = L"SampleWindowClass";

	// Register window class
	WNDCLASS wc = {};
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = CLASS_NAME;
	wc.hbrBackground = /*(HBRUSH)(COLOR_WINDOW + 1);*/NULL;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);

	RegisterClass(&wc);

	// Create window
	HWND hwnd = CreateWindowEx(
		0,
		CLASS_NAME,
		L"MaksiPlayer",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
		NULL,
		NULL,
		hInstance,
		NULL
	);

	if (hwnd == NULL)
		return 0;

	ShowWindow(hwnd, nCmdShow);
	UpdateWindow(hwnd);

	MSG msg = {};
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}

void PlaySongByIndex(HWND hwnd, int index)
{
	if (index < 0 || index >= (int)g_playlist.size())
		return;

	g_currentFile = g_playlist[index];

	mciSendString(L"close mp3", NULL, 0, NULL);

	std::wstring openCmd = L"open \"" + g_currentFile + L"\" type mpegvideo alias mp3";
	mciSendString(openCmd.c_str(), NULL, 0, NULL);

	mciSendString(L"set mp3 time format milliseconds", NULL, 0, NULL);
	mciSendString(L"play mp3 notify", NULL, 0, hwnd);

	SetTimer(hwnd, 1, 500, NULL);

	// Update "Now Playing" label
	size_t pos = g_currentFile.find_last_of(L"\\/");
	std::wstring fileOnly = g_currentFile.substr(pos + 1);
	SetWindowText(GetDlgItem(hwnd, 101), fileOnly.c_str());

	// Select in ListBox
	SendMessage(GetDlgItem(hwnd, 200), LB_SETCURSEL, index, 0);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CREATE:
	{
		HINSTANCE hInstance = ((LPCREATESTRUCT)lParam)->hInstance;

		CreateWindow(
			L"BUTTON",
			L"Play",
			WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
			20, 100, 120, 30,
			hwnd,
			(HMENU)1,
			hInstance,
			NULL
		);
		CreateWindow(
			L"BUTTON",
			L"Pause",
			WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
			150, 100, 120, 30,
			hwnd,
			(HMENU)2,
			hInstance,
			NULL
		);
		CreateWindow(
			L"BUTTON",
			L"Stop",
			WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
			280, 100, 120, 30,
			hwnd,
			(HMENU)3,
			hInstance,
			NULL
		);
		CreateWindow(
			L"STATIC",
			L"00:00 / 00:00",
			WS_VISIBLE | WS_CHILD,
			20, 50, 200, 25,
			hwnd,
			(HMENU)100,
			hInstance,
			NULL
		);
		CreateWindow(
			L"LISTBOX",
			NULL,
			WS_VISIBLE | WS_CHILD | WS_BORDER | LBS_NOTIFY,
			20, 165, 350, 390,
			hwnd,
			(HMENU)200,
			hInstance,
			NULL
		);
		LoadPlaylist(hwnd);
		CreateWindow(
			L"BUTTON",
			L"Add File",
			WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
			400, 175, 120, 30,
			hwnd,
			(HMENU)201,
			hInstance,
			NULL
		);
		CreateWindow(
			L"STATIC",
			L"Nothing playing",
			WS_VISIBLE | WS_CHILD,
			20, 20, 500, 25,
			hwnd,
			(HMENU)101,
			hInstance,
			NULL
		);
		CreateWindow(
			L"Button",
			L"About",
			WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
			665, 0, 120, 30,
			hwnd,
			(HMENU)202,
			hInstance,
			NULL
		);
		CreateWindow(
			L"STATIC",
			L"Note: Do not add songs with russian letters. Playlist saving will break.",
			WS_VISIBLE | WS_CHILD,
			20, 140, 500, 25,
			hwnd,
			(HMENU)105,
			hInstance,
			NULL
		);
		EnumChildWindows(hwnd, [](HWND child, LPARAM) -> BOOL {
			SendMessage(child, WM_SETFONT, (WPARAM)hFont, TRUE);
			return TRUE;
			}, 0);
		return 0;
	}
	case WM_COMMAND:
	{
		switch (LOWORD(wParam))
		{
		case 1: // Play
		{
			HWND hList = GetDlgItem(hwnd, 200);
			int selected = SendMessage(hList, LB_GETCURSEL, 0, 0);

			if (selected != LB_ERR)
			{
				if (g_playerState == PAUSED)
				{
					// Resume the paused track
					mciSendString(L"resume mp3", NULL, 0, NULL);
					g_playerState = PLAYING;
				}
				else if (g_playerState == STOPPED || g_currentFile != g_playlist[selected])
				{
					// Play a new track
					PlaySongByIndex(hwnd, selected);
					g_playerState = PLAYING;
				}
				// else: already playing the selected track, do nothing
			}
			break;
		}
		case 2: // Pause
			if (g_playerState == PLAYING)
			{
				mciSendString(L"pause mp3", NULL, 0, NULL);
				g_playerState = PAUSED;
			}
			break;
		case 3: // Stop
			KillTimer(hwnd, 1);
			mciSendString(L"stop mp3", NULL, 0, NULL);
			mciSendString(L"close mp3", NULL, 0, NULL);
			g_playerState = STOPPED;
			break;
		case 202: // About button
		{
			std::wstring aboutText = L"MaksiPlayer ";
			aboutText += AppVersion;
			aboutText += L"\nBy MaksiSoft\nSimple Win32 MP3 Player";
			MessageBox(
				hwnd,
				aboutText.c_str(),
				L"About this thing",
				MB_OK | MB_ICONINFORMATION
			);
			break;
		}
		case 201: // Add File button
		{
			OPENFILENAME ofn = {};
			wchar_t fileBuffer[65536] = L"";

			ofn.lStructSize = sizeof(ofn);
			ofn.lpstrFilter = L"Muzikele (*.mp3)\0*.mp3\0All Files (*.*)\0*.*\0";
			ofn.lpstrFile = fileBuffer;
			ofn.nMaxFile = 65536;
			ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_ALLOWMULTISELECT | OFN_EXPLORER;
			ofn.hwndOwner = hwnd;

			if (GetOpenFileName(&ofn))
			{
				wchar_t* p = fileBuffer;
				std::wstring directory = p;
				p += directory.length() + 1;

				if (*p == 0) {
					g_playlist.push_back(directory);
					size_t pos = directory.find_last_of(L"\\/");
					std::wstring fileOnly = directory.substr(pos + 1);
					SendMessage(GetDlgItem(hwnd, 200), LB_ADDSTRING, 0, (LPARAM)fileOnly.c_str());
				}
				else {
					// MULTIPLE files selected
					while (*p) {
						std::wstring fullPath = directory + L"\\" + p; // combine folder + file
						g_playlist.push_back(fullPath);
						SendMessage(GetDlgItem(hwnd, 200), LB_ADDSTRING, 0, (LPARAM)p);
						p += wcslen(p) + 1;
					}
				}
				SavePlaylist();
			}
			
			break;
			}
			
		}
		break;
	}
	case MM_MCINOTIFY:
		if (g_playerState == PLAYING) {
			KillTimer(hwnd, 1);

			HWND hList = GetDlgItem(hwnd, 200);
			int currentIndex = SendMessage(hList, LB_GETCURSEL, 0, 0);

			if (g_playlist.size() > 0)
			{
				int nextIndex = (currentIndex + 1) % g_playlist.size();
				PlaySongByIndex(hwnd, nextIndex);
			}
		}
		break;
	case WM_DESTROY:
		SavePlaylist();
		PostQuitMessage(0);
		return 0;
	/*
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);
		TextOut(hdc, 50, 50, L"Hello, Win32!", 13);
		EndPaint(hwnd, &ps);
		break;
	}
	*/
	case WM_TIMER:
	{
		wchar_t buffer[128];
		wchar_t lengthBuffer[128];

		mciSendString(L"status mp3 position", buffer, 128, NULL);

		mciSendString(L"status mp3 length", lengthBuffer, 128, NULL);

		int currentMs = _wtoi(buffer);
		int totalMs = _wtoi(lengthBuffer);

		int currentSec = currentMs / 1000;
		int totalSec = totalMs / 1000;

		int curMin = currentSec / 60;
		int curSec = currentSec % 60;

		int totMin = totalSec / 60;
		int totSec = totalSec % 60;

		wchar_t timeText[64];
		wsprintf(timeText, L"%02d:%02d / %02d:%02d",
			curMin, curSec,
			totMin, totSec);

		SetWindowText(GetDlgItem(hwnd, 100), timeText);
		
		return 0;
	}
	return 0;
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
#include <iostream>
#include <windows.h>
#include <shellapi.h>
#include <thread>

#include "resource.h"

#define WM_TRAYICON (WM_USER + 1)
#define TRAYICON_ID 1

HHOOK mouseHook;

int selectedDelay = 100; 

struct ButtonOptions {
    bool LBUTTON = true;
    bool RBUTTON = true;
    bool MBUTTON = true;
    bool BUTTON1 = true;
    bool BUTTON2 = true;
};
ButtonOptions buttons;


LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode >= 0)
    {
        if (wParam == WM_LBUTTONDOWN || wParam == WM_RBUTTONDOWN || wParam == WM_MBUTTONDOWN ||
            wParam == WM_XBUTTONDOWN || wParam == WM_XBUTTONUP)
        {
            static LONGLONG lastClickTime = 0;
            LONGLONG currentTime = GetTickCount64();

            if (currentTime - lastClickTime < selectedDelay)
            {
                return 1;
            }

            lastClickTime = currentTime;
            
        }
    }

    return CallNextHookEx(mouseHook, nCode, wParam, lParam);
}

void SetHook()
{
    mouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseProc, NULL, 0);
    if (!mouseHook)
    {
        MessageBoxA(NULL, "Falha ao configurar o hook do mouse.", "Prevent Double Click", MB_OK | MB_ICONERROR);
    }
}

void RemoveHook()
{
    if (mouseHook)
    {
        UnhookWindowsHookEx(mouseHook);
    }
}


bool CheckIfKeyExists()
{
    HKEY hKey;
    bool exists = false;

    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
    {
        DWORD valueType;
        char valueData[MAX_PATH];
        DWORD valueDataSize = sizeof(valueData);

        if (RegQueryValueExW(hKey, L"PreventDoubleClick", NULL, &valueType, (BYTE*)valueData, &valueDataSize) == ERROR_SUCCESS)
        {
            exists = true;
        }

        RegCloseKey(hKey);
    }

    return exists;
}

void AddToStartup()
{
    if (CheckIfKeyExists())
    {
        return;
    }

    HKEY hKey;
    char applicationName[MAX_PATH];
    DWORD pathLength = GetModuleFileNameA(NULL, applicationName, MAX_PATH);

    if (pathLength > 0)
    {
        if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS)
        {
            if (RegSetValueExW(hKey, L"PreventDoubleClick", 0, REG_SZ, (BYTE*)applicationName, pathLength) != ERROR_SUCCESS)
            {
                MessageBoxW(NULL, L"Falha ao adicionar o programa ao registro de inicialização.", L"Prevent Double Click", MB_OK | MB_ICONERROR);
            }

            RegCloseKey(hKey);
        }
    }
}

int GetValue(LPCWSTR key = L"Delay")
{
    int value = 1;
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\PreventDoubleClick", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        DWORD dwSize = sizeof(value);
        RegQueryValueExW(hKey, key, NULL, NULL, (BYTE*)&value, &dwSize);
        RegCloseKey(hKey);         
    }
   
    return value;
}

void SaveValue(int value, LPCWSTR key = L"Delay")
{
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\PreventDoubleClick", 0, KEY_WRITE, &hKey) == ERROR_SUCCESS)
    {
        RegSetValueExW(hKey, key, 0, REG_DWORD, (BYTE*)&value, sizeof(value));
        RegCloseKey(hKey);
    }
    else
    {
        if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\PreventDoubleClick", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS)
        {
            RegSetValueExW(hKey, key, 0, REG_DWORD, (BYTE*)&value, sizeof(value));
            RegCloseKey(hKey);
        }
    }
}

void ThreadMouse() 
{
    SetHook();

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    RemoveHook();
}

//Tray icon
NOTIFYICONDATA notifyIconData;

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
    {
        ZeroMemory(&notifyIconData, sizeof(NOTIFYICONDATA));
        notifyIconData.cbSize = sizeof(NOTIFYICONDATA);
        notifyIconData.hWnd = hwnd;
        notifyIconData.uID = TRAYICON_ID;
        notifyIconData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
        notifyIconData.uCallbackMessage = WM_TRAYICON;
        notifyIconData.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON1));

        wcscpy_s(notifyIconData.szTip, sizeof(notifyIconData.szTip), L"Prevent Mouse Click");
        Shell_NotifyIcon(NIM_ADD, &notifyIconData);

        break;
    }
    case WM_TRAYICON:
    {
        switch (lParam)
        {
        case WM_RBUTTONDOWN:
        {
            POINT pt;
            GetCursorPos(&pt);

            HMENU hMenu = CreatePopupMenu();
            HMENU hSubMenu = CreatePopupMenu();
            HMENU hSubMenu2 = CreatePopupMenu();

            AppendMenu(hSubMenu2, MF_STRING, 9, L"LBUTTON (Mouse 1)");
            AppendMenu(hSubMenu2, MF_STRING, 10, L"RBUTTON (Mouse 2)");
            AppendMenu(hSubMenu2, MF_STRING, 11, L"MBUTTON (Scroll Click)");
            AppendMenu(hSubMenu2, MF_STRING, 12, L"XBUTTON1 (Side down)");
            AppendMenu(hSubMenu2, MF_STRING, 13, L"XBUTTON2 (Side up)");

            AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hSubMenu2, L"Buttons");

            AppendMenu(hSubMenu, MF_STRING, 4, L"10 ms");
            AppendMenu(hSubMenu, MF_STRING, 5, L"50 ms");
            AppendMenu(hSubMenu, MF_STRING, 6, L"100 ms");
            AppendMenu(hSubMenu, MF_STRING, 7, L"150 ms");
            AppendMenu(hSubMenu, MF_STRING, 8, L"200 ms");

            AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hSubMenu, L"Settings");

            AppendMenu(hMenu, MF_STRING, 1, L"Exit");


            CheckMenuItem(hSubMenu, 4, MF_BYCOMMAND | (selectedDelay == 10 ? MF_CHECKED : MF_UNCHECKED));
            CheckMenuItem(hSubMenu, 5, MF_BYCOMMAND | (selectedDelay == 50 ? MF_CHECKED : MF_UNCHECKED));
            CheckMenuItem(hSubMenu, 6, MF_BYCOMMAND | (selectedDelay == 100 ? MF_CHECKED : MF_UNCHECKED));
            CheckMenuItem(hSubMenu, 7, MF_BYCOMMAND | (selectedDelay == 150 ? MF_CHECKED : MF_UNCHECKED));
            CheckMenuItem(hSubMenu, 8, MF_BYCOMMAND | (selectedDelay == 200 ? MF_CHECKED : MF_UNCHECKED));

            CheckMenuItem(hSubMenu2, 9, MF_BYCOMMAND | (buttons.LBUTTON ? MF_CHECKED : MF_UNCHECKED));
            CheckMenuItem(hSubMenu2, 10, MF_BYCOMMAND | (buttons.RBUTTON ? MF_CHECKED : MF_UNCHECKED));
            CheckMenuItem(hSubMenu2, 11, MF_BYCOMMAND | (buttons.MBUTTON ? MF_CHECKED : MF_UNCHECKED));
            CheckMenuItem(hSubMenu2, 12, MF_BYCOMMAND | (buttons.BUTTON1 ? MF_CHECKED : MF_UNCHECKED));
            CheckMenuItem(hSubMenu2, 13, MF_BYCOMMAND | (buttons.BUTTON2 ? MF_CHECKED : MF_UNCHECKED));

            SetForegroundWindow(hwnd);
            TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, NULL);
            PostMessage(hwnd, WM_NULL, 0, 0);

            DestroyMenu(hMenu);

            break;
        }
        }
        break;
    }
    case WM_COMMAND:
    {
        int menuId = LOWORD(wParam);

        if (menuId == 1) // Finalizar
        {
            Shell_NotifyIcon(NIM_DELETE, &notifyIconData);
            PostQuitMessage(0);
            ExitProcess(0);
        }       
        else if (menuId == 4) // 10 ms
        {
            selectedDelay = 10; 
            SaveValue(selectedDelay);
            break;
        }
        else if (menuId == 5) // 50 ms
        {
            selectedDelay = 50; 
            SaveValue(selectedDelay);
            break;
        }
        else if (menuId == 6) // 100 ms
        {
            selectedDelay = 100;
            SaveValue(selectedDelay);
            break;
        }
        else if (menuId == 7) // 150 ms
        {
            selectedDelay = 150;
            SaveValue(selectedDelay);
            break;
        }
        else if (menuId == 8) // 200 ms
        {
            selectedDelay = 200; 
            SaveValue(selectedDelay);
            break;
        }           
        else if (menuId == 9) // LBUTTON
        {
            buttons.LBUTTON = !buttons.LBUTTON;
			SaveValue(buttons.LBUTTON, L"LBUTTON");
			break;
		}
        else if (menuId == 10) // RBUTTON
        {
            buttons.RBUTTON = !buttons.RBUTTON;
            SaveValue(buttons.RBUTTON, L"RBUTTON");
			break;
		}
        else if (menuId == 11) // MBUTTON
        {
            buttons.LBUTTON = !buttons.MBUTTON;
            SaveValue(buttons.MBUTTON, L"MBUTTON");
			break;
		}
        else if (menuId == 12) // XBUTTON1
        {
            buttons.BUTTON1 = !buttons.BUTTON1;
            SaveValue(buttons.BUTTON1, L"XBUTTON1");
			break;
		}
        else if (menuId == 13) // XBUTTON2
        {
            buttons.BUTTON2 = !buttons.BUTTON2;
            SaveValue(buttons.BUTTON2, L"XBUTTON2");
			break;
		}

        break;
    }
    case WM_DESTROY:
    {
        Shell_NotifyIcon(NIM_DELETE, &notifyIconData);
        PostQuitMessage(0);

        break;
    }
    default:
    {
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    }

    return 0;
}

void GetPreviewSettings() 
{
    selectedDelay = GetValue(L"Delay");
    if(selectedDelay < 10 || selectedDelay > 200)
        selectedDelay == 100;

    buttons.LBUTTON = GetValue(L"LBUTTON");
    buttons.RBUTTON = GetValue(L"RBUTTON");
    buttons.MBUTTON = GetValue(L"MBUTTON");
    buttons.BUTTON1 = GetValue(L"XBUTTON1");
    buttons.BUTTON2 = GetValue(L"XBUTTON2");
}


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    GetPreviewSettings();

    AddToStartup();

    std::thread tMouseHook(ThreadMouse);
    tMouseHook.detach();

    WNDCLASS wndClass = { 0 };
    wndClass.hInstance = hInstance;
    wndClass.lpszClassName = L"PreventDoubleClick";
    wndClass.lpfnWndProc = WndProc;
    RegisterClass(&wndClass);

    HWND hwnd = CreateWindow(wndClass.lpszClassName, L"Prevent Double Click", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInstance, NULL);
    ShowWindow(hwnd, SW_HIDE);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}
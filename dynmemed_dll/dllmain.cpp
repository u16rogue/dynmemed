#include <Windows.h>
#include "globals.h"
#include <cstdio>
#include <iostream>
#include "hooks.h"
#include <MinHook.h>
#include "utils.h"
#include <Psapi.h>

template <typename T>
static auto w_aob_scan(T &out, const char *name, MODULEINFO &mi, const char *sig, const char *mask) -> bool
{
    std::cout << "\n\t >> " << name;
    out = utils::aob_scan(mi.lpBaseOfDll, mi.SizeOfImage, sig, mask);
    if (!out)
    {
        std::cout << "not found!";
        return false;
    }

    std::cout << " @ 0x" << reinterpret_cast<void *&>(out);
    return true;
}

static auto init() -> bool
{
    if (!AllocConsole())
        return false;

    FILE *file_ptr;
    _wfreopen_s(&file_ptr, L"CONOUT$", L"w", stdout);
    _wfreopen_s(&file_ptr, L"CONOUT$", L"w", stderr);
    _wfreopen_s(&file_ptr, L"CONIN$", L"r", stdin);

    SetConsoleTitleW(L"" APP_NAME);

    std::cout << "\n" APP_NAME " build date: " __DATE__ " " __TIME__
                 "\n[-] Press [INSERT] to unload."
                 "\n[-] Press [F2] to disable dyned screen blocker."
                 "\n[-] Press [F3] to enable dyned screen blocker."
                 "\n[+] Loading module info... ";

    auto dyned_mod = GetModuleHandleW(NULL);
    MODULEINFO dyned_mi = {  };
    GetModuleInformation(GetCurrentProcess(), dyned_mod, &dyned_mi, sizeof(dyned_mi));

    std::cout << "\n[+] Loading signatures";

    if (!w_aob_scan(dyned::string_formatter, "dyned string formatter", dyned_mi, "\x55\x8B\xEC\x83\xEC\x00\x8D", "xxxxx?x")
    ) {
        return false;
    }

    // study full \xA1\x00\x00\x00\x00\x50\x8B\x0D\x00\x00\x00\x00\x51\x8B\x55\x08 x????xxx????xxxx
    // study summary \x8B\x0D\x00\x00\x00\x00\x51\x8B\x55\x08\x52\xE8\x00\x00\x00\x00\x83\xC4\x00\x83 xx????xxxxxx????xx?x

    std::cout << "\n[+] Installing hooks... ";
    if (!hooks::install())
    {
        std::cout << "\n[!] Installing hooks failed!";
        return false;
    }

    #if defined(NDEBUG)
    {
        std::cout << "\n[?] Enter custom progress (1-100): ";
        std::cin >> globals::custom_progress;
        std::cout << "\n[?] Enter custom score (-12 - 12): ";
        std::cin >> globals::custom_score;
        std::cout << "\n[?] Enter custom time (HH:MM): ";
        std::cin >> globals::custom_time;
        std::cout << "\n[?] Enter custom date (DD-MMM-YYYY): ";
        std::cin >> globals::custom_date;
    }
    #endif

    std::cout << "\n[+] Ready!\n";

    return true;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
    globals::dll_handle = hModule;
    if (HANDLE thread_handle = nullptr; ul_reason_for_call == DLL_PROCESS_ATTACH && (thread_handle = CreateThread(nullptr, NULL, [](LPVOID arg0) -> DWORD
    {
        if (!init())
        {
            MessageBoxW(nullptr, L"Failed to initialize!", L"" APP_NAME, MB_OK);
            FreeLibraryAndExitThread(globals::dll_handle, 0);
        }

        for (;;)
        {
            Sleep(500);

            // too lazy too hook windowproc :/
            if (GetAsyncKeyState(VK_INSERT) & 0x1)
            {
                if (MessageBox(dyned::hwnd_engine, L"Are you sure you want to unload " APP_NAME "?", L"" APP_NAME, MB_YESNO) == IDNO)
                    continue;

                std::cout << "\n[+] Unloading " APP_NAME "...";
                hooks::uninstall();
                FreeConsole();
                FreeLibraryAndExitThread(globals::dll_handle, 0);
                return 0;
            }
            else if (GetAsyncKeyState(VK_F2) & 0x1)
            {
                ShowWindow(dyned::hwnd_bg, SW_HIDE);
            }
            else if (GetAsyncKeyState(VK_F3) & 0x1)
            {
                ShowWindow(dyned::hwnd_bg, SW_SHOW);
                SetForegroundWindow(dyned::hwnd_engine);
            }
        }

        return 0;
    }, nullptr, NULL, nullptr))) { CloseHandle(thread_handle); }

    return TRUE;
}


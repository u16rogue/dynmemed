#define _CRT_SECURE_NO_WARNINGS

#include "hooks.h"
#include <MinHook.h>
#include <iostream>
#include "utils.h"
#include <intrin.h>

#pragma warning (disable: 26812)

decltype(CreateWindowExW) *o_CreateWindowExW = nullptr;
HWND __stdcall hk_CreateWindowExW(DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
{
	HWND *store_result_to = nullptr;
	if (wcscmp(lpClassName, L"dynedEngineWindowClass") == 0)
	{
		printf("\n[+] Created class: dynedEngineWindowClass >> Return address 0x%p\n", _ReturnAddress());
		store_result_to = &dyned::hwnd_engine;

		RECT work_area = {};
		SystemParametersInfoW(SPI_GETWORKAREA, 0, &work_area, 0);
		nHeight = work_area.bottom - 20;

		hWndParent = nullptr;
		dwStyle |= WS_OVERLAPPEDWINDOW;
	}
	else if (wcscmp(lpClassName, L"BackgroundClass") == 0)
	{
		printf("\n[+] Created class: BackgroundClass >> Return address 0x%p\n", _ReturnAddress());
		store_result_to = &dyned::hwnd_bg;

		/*
		X = 5;
		Y = 5;
		nWidth = 30;
		nHeight = 30;

		dwStyle &= ~(WS_MAXIMIZE);
		dwStyle |= WS_OVERLAPPEDWINDOW | WS_MINIMIZE;
		*/

		lpWindowName = L"nice block";
	}

	auto result = o_CreateWindowExW(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);

	if (store_result_to)
		*store_result_to = result;

	return result;
}

decltype(SetForegroundWindow) *o_SetForegroundWindow = nullptr;
BOOL __stdcall hk_SetForegroundWindow(HWND hWnd)
{
	if (hWnd == dyned::hwnd_bg)
		return TRUE;

	return o_SetForegroundWindow(hWnd);
}

decltype(GetForegroundWindow) *o_GetForegroundWindow = nullptr;
HWND __stdcall hk_GetForegroundWindow()
{
	return dyned::hwnd_engine;
}

decltype(GetSystemMetrics) *o_GetSystemMetrics = nullptr;
int __stdcall hk_GetSystemMetrics(int nIndex)
{
	switch (nIndex)
	{
		case SM_CMONITORS: // just stop...
			return 1;
	}

	return o_GetSystemMetrics(nIndex);
}

int(__cdecl*o_dyned_string_formatter)(char *, const char *, ...) = nullptr;
int __cdecl hk_dyned_string_formatter(char *buffer, const char *format)
{
	void **varargs_stack = reinterpret_cast<void **>(_AddressOfReturnAddress()) + 0x3;
	int varargs_count = 0;

	const char *fmt = format;
	while (*fmt != '\0')
	{
		if (*fmt == '%')
		{
			++varargs_count;
			++fmt; // advance by one to skip the specifier (%<this>), this should also prevent recounting %%
		}

		++fmt;
	}

	// Spoofs the lecture progress
	if (strcmp(format, "%d%%") == 0)
	{
		printf("\n[+] Spoofing progress to %d%%...", globals::custom_progress);
		varargs_stack[0] = (void *)globals::custom_progress;
	}
	// spoofs the study details
	else if (strcmp(format, "  ~  %s  ~  %s: %s  ~  %s: %s") == 0)
	{
		printf("\n[+] Spoofing hours to %s and last studied date to %s...", globals::custom_time.c_str(), globals::custom_date.c_str());
		varargs_stack[2] = (void *)globals::custom_time.c_str();
		varargs_stack[4] = (void *)globals::custom_date.c_str();
	}
	// spoofs the study score
	else if (strcmp(format, "%s = %d%s") == 0)
	{
		printf("\n[+] Spoofing score to %d...", globals::custom_score);
		varargs_stack[1] = (void *)globals::custom_score;
	}
	// spoofs no score
	else if (strcmp(format, "%s = <none>%s") == 0)
	{
		printf("\n[+] Spoofing blank study score...");
		format = "%s = %d\r\n";
		varargs_stack[1] = (void *)globals::custom_score;
	}

	for (int i = varargs_count - 1; i != -1; i--)
	{
		void *argval = varargs_stack[i];
		__asm push argval;
	}

	int retval = 0;
	int stack_unwind = varargs_count * sizeof(void *) + sizeof(buffer) + sizeof(format);

	__asm
	{
		push format
		push buffer
		call o_dyned_string_formatter
		add esp, stack_unwind
		mov retval, eax // this is super unecessary since the return value is already in eax, its not the compiler's fault too anyway
	};

	return retval;
}

// ==================================================================================================================================================================================================================

static auto helper_create_hook(const char *name, void *target, void *detour, void **orig) -> bool
{
	printf("\n[+] Hooking %s (0x%p)...", name, target);
	return MH_CreateHook(target, detour, orig) == MH_OK && MH_EnableHook(target) == MH_OK;
}

static auto helper_create_api_hook(const char *mod, const char *name, void *hk, void **orig) -> bool
{
	printf("\n[+] Hooking %s...", name);
	return utils::iat_hook(mod, name, hk, orig);
}

#define M_CREATE_HOOK(fnname, target) helper_create_hook(#fnname, target, hk_##fnname, reinterpret_cast<void **>(&o_##fnname))
#define M_CREATE_API_HOOK_BY_MODULE(mod, api_fn) helper_create_api_hook(mod, #api_fn, hk_ ## api_fn, reinterpret_cast<void **>(&o_##api_fn))
#define M_CREATE_API_HOOK(api_fn) M_CREATE_API_HOOK_BY_MODULE(nullptr, api_fn)

auto hooks::install() -> bool
{
	std::cout << "\n[+] Initializing MinHook";
	if (MH_Initialize() != MH_OK)
		return false;

	// (trying to avoid detour hooking api's that are sandboxed by using an iat hook instead)
	if (!M_CREATE_API_HOOK(CreateWindowExW)
	||  !M_CREATE_API_HOOK(SetForegroundWindow)
	||  !M_CREATE_API_HOOK(GetSystemMetrics)
	||  !M_CREATE_HOOK(GetForegroundWindow, ::GetForegroundWindow)
	||  !M_CREATE_HOOK(dyned_string_formatter, dyned::string_formatter)
	) {
		return false;
	}

	return true;
}

auto hooks::uninstall() -> bool
{
	MH_DisableHook(MH_ALL_HOOKS);
	MH_Uninitialize();
	return true;
}

#pragma warning (default: 26812)